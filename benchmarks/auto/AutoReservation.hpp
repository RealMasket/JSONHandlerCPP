#pragma once
#include "Token.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <vector>

// Experimental only: no production preset or runtime hooks are introduced.
namespace AutoReserve {
enum class Goal { Speed, Memory };
enum class Reason { Initial, Warming, Stable, DensityDrop, DensityRise, Unstable, CopyPressure, EndOfInput, Finished, Aborted, AllocationFailed };
inline const char* name(Reason reason) {
    switch (reason) {
    case Reason::Initial: return "initial";
    case Reason::Warming: return "warming";
    case Reason::Stable: return "stable";
    case Reason::DensityDrop: return "density_drop";
    case Reason::DensityRise: return "density_rise";
    case Reason::Unstable: return "unstable";
    case Reason::CopyPressure: return "copy_pressure";
    case Reason::EndOfInput: return "end_of_input";
    case Reason::Finished: return "finished";
    case Reason::Aborted: return "aborted";
    default: return "allocation_failed";
    }
}
inline size_t add(size_t a, size_t b, size_t limit) noexcept {
    return a + (std::min)(b, limit-a);
}
inline size_t multiply(size_t n, size_t factor, size_t limit) noexcept {
    return n > limit/factor ? limit : n*factor;
}
struct Bounds { size_t lower, upper; };
inline Bounds bounds(Goal goal, size_t capacity, size_t limit, bool mature) noexcept {
    const size_t minimum = (std::max)(capacity/(goal == Goal::Speed ? 2 : 4), size_t{1});
    // Speed: at most 16*n+258 requested slots (4*n+258 while warming).
    // Memory: at most 2*n+258. Fixed slack includes small buffers/pending token/EOF.
    const size_t factor = goal == Goal::Memory ? 1 : mature ? 15 : 3;
    const size_t spare = add(multiply(capacity, factor, limit), (std::min)(size_t{258}, limit), limit);
    return {add(capacity, minimum, limit), add(capacity, spare, limit)};
}
inline size_t clampPrediction(double prediction, Bounds range) noexcept {
    if (prediction >= static_cast<double>(range.upper)) return range.upper;
    // prediction is finite/nonnegative; vector's max_size is below SIZE_MAX.
    return (std::min)(range.upper, (std::max)(range.lower, static_cast<size_t>(prediction)));
}

#ifdef MASKETJSON_AUTO_TRACE
struct Decision {
    Reason reason{};
    size_t position=0, tokens=0, oldCapacity=0, request=0, actualCapacity=0;
    size_t lower=0, upper=0, moved=0, streak=0;
    double globalDensity=0, recentDensity=0, prediction=0;
    bool clamped=false, floored=false;
};
struct Journal {
    std::array<Decision,512> entries{};
    size_t size=0, dropped=0;
    void push(const Decision& decision) noexcept {
        if (size < entries.size()) entries[size++] = decision;
        else ++dropped;
    }
};
inline thread_local Journal* activeJournal=nullptr;
struct JournalScope {
    Journal* previous;
    explicit JournalScope(Journal& journal) : previous(activeJournal) { activeJournal=&journal; }
    ~JournalScope() { activeJournal=previous; }
    JournalScope(const JournalScope&)=delete;
    JournalScope& operator=(const JournalScope&)=delete;
};
#endif

template<Goal goal>
class Controller {
    const size_t inputBytes;
    const size_t& position;
    std::vector<Token>& tokens;
    size_t previousPosition=0, previousTokens=0, stableStreak=0, relocated=0;
    double previousDensity=0;
    bool finished=false;

    void record(Reason reason, size_t oldCapacity, size_t requested, Bounds range,
                double global=0, double recent=0, double prediction=0) const noexcept {
#ifdef MASKETJSON_AUTO_TRACE
        if (activeJournal) activeJournal->push({reason,position,tokens.size(),oldCapacity,requested,tokens.capacity(),
            range.lower,range.upper,relocated,stableStreak,global,recent,prediction,
            prediction>static_cast<double>(range.upper),prediction<static_cast<double>(range.lower)});
#else
        (void)reason; (void)oldCapacity; (void)requested; (void)range;
        (void)global; (void)recent; (void)prediction;
#endif
    }
public:
    Controller(size_t bytes, const size_t& cursor, std::vector<Token>& buffer)
        : inputBytes(bytes), position(cursor), tokens(buffer) {
        record(Reason::Initial,0,tokens.capacity(),{0,tokens.capacity()});
    }
    ~Controller() { if (!finished) record(Reason::Aborted,tokens.capacity(),tokens.capacity(),{0,tokens.capacity()}); }
    Controller(const Controller&)=delete;
    Controller& operator=(const Controller&)=delete;
    size_t movedTokens() const noexcept { return relocated; }

    void grow(bool eof) {
        const size_t n=tokens.size(), capacity=tokens.capacity(), limit=tokens.max_size();
        const size_t pending=add(n,1,limit);
        const size_t deltaTokens=pending-previousTokens;
        const size_t deltaBytes=position-previousPosition;
        const double global=static_cast<double>(pending)/static_cast<double>((std::max)(position,size_t{1}));
        const double recent=deltaBytes ? static_cast<double>(deltaTokens)/static_cast<double>(deltaBytes) : global;
        const double ratio=previousDensity>0 ? recent/previousDensity : 1;
        const bool drop=previousDensity>0 && ratio<0.25;
        const bool rise=previousDensity>0 && ratio>4;
        const bool stable=previousDensity>0 && ratio>=0.5 && ratio<=2;
        stableStreak=stable ? add(stableStreak,1,(std::numeric_limits<size_t>::max)()) : 0;
        const bool cold=n<1024 || position<4096 || previousDensity==0;
        const bool copyPressure=n && static_cast<double>(relocated)>(goal==Goal::Speed ? 2.0 : 3.0)*static_cast<double>(n);
        const bool mature=!cold && (stableStreak>=2 || (copyPressure && stable));
        Reason reason=drop ? Reason::DensityDrop : rise ? Reason::DensityRise
            : cold ? Reason::Warming : copyPressure ? Reason::CopyPressure : stable ? Reason::Stable : Reason::Unstable;
        auto range=bounds(goal,capacity,limit,mature);
        if constexpr (goal==Goal::Memory) {
            if (copyPressure) range.lower=(std::max)(range.lower,add(capacity,(std::max)(capacity/2,size_t{1}),limit));
        }
        const double density = goal == Goal::Memory || drop ? (std::min)(global,recent)
            : rise ? (std::max)(global,recent) : global;
        double prediction=static_cast<double>(pending)+density*static_cast<double>(inputBytes-position)+1;
        if constexpr (goal == Goal::Speed) prediction*=1.0625; // small, bounded forecast margin
        size_t request=clampPrediction(prediction,range);
        if (position==inputBytes) {
            // Remaining count is known now; do not force a geometric step at EOF.
            reason=Reason::EndOfInput;
            request=add(pending,eof ? 0 : 1,limit);
            prediction=static_cast<double>(request);
        }
#ifdef MASKETJSON_AUTO_TRACE
        try { tokens.reserve(request); }
        catch (...) { record(Reason::AllocationFailed,capacity,request,range,global,recent,prediction); throw; }
#else
        tokens.reserve(request);
#endif
        relocated=add(relocated,n,(std::numeric_limits<size_t>::max)());
        record(reason,capacity,request,range,global,recent,prediction);
        previousPosition=position; previousTokens=pending; previousDensity=recent;
    }
    void finish() noexcept {
        finished=true;
        record(Reason::Finished,tokens.capacity(),tokens.capacity(),{0,tokens.capacity()});
    }
};
}
