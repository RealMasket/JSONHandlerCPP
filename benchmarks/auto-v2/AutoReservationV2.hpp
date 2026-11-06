#pragma once
#include "AutoReservation.hpp"

#ifndef AUTO_V2_SPEED_BYTES
#define AUTO_V2_SPEED_BYTES 4194304
#endif
#ifndef AUTO_V2_MEMORY_BYTES
#define AUTO_V2_MEMORY_BYTES 524288
#endif

namespace AutoReserveV2 {
using AutoReserve::Goal;
using AutoReserve::Reason;
using AutoReserve::add;
using AutoReserve::multiply;
inline constexpr size_t speedBudget=AUTO_V2_SPEED_BYTES, memoryBudget=AUTO_V2_MEMORY_BYTES;
enum class Kind { Initial, Forecast, ByteBudget, FloorOverride, Tail, End, Finished, Aborted, AllocationFailed };
inline const char* name(Kind kind,Reason state) {
    switch(kind) {
    case Kind::Initial:return "initial";
    case Kind::ByteBudget:return "byte_budget";
    case Kind::FloorOverride:return "budget_floor_override";
    case Kind::Tail:return "remaining_byte_bound";
    case Kind::End:return "end_of_input";
    case Kind::Finished:return "finished";
    case Kind::Aborted:return "aborted";
    case Kind::AllocationFailed:return "allocation_failed";
    default:return AutoReserve::name(state);
    }
}
struct Tail { bool fits; size_t count; };
inline Tail remainingBound(size_t pending,size_t remaining,bool eof,size_t limit) noexcept {
    const size_t extra=eof ? 0 : 1;
    if (pending>limit || extra>limit-pending) return {false,0};
    const size_t base=pending+extra;
    if (remaining>limit-base) return {false,0};
    return {true,base+remaining};
}
struct Hint {
    size_t request=0, lower=0, relativeUpper=0, byteUpper=0, effectiveUpper=0, budgetBytes=0, tailBound=0;
    bool floorOverride=false, byteLimited=false, tailApplied=false;
};
inline Hint choose(Goal goal,size_t capacity,size_t limit,bool mature,bool pressure,
                   size_t remaining,bool eof,double prediction,size_t budgetBytes) noexcept {
    Hint h;
    const size_t pending=add(capacity,1,limit);
    const auto relative=AutoReserve::bounds(goal,capacity,limit,mature);
    h.lower=relative.lower;
    if (goal==Goal::Memory && pressure)
        h.lower=(std::max)(h.lower,add(capacity,(std::max)(capacity/2,size_t{1}),limit));
    h.relativeUpper=relative.upper;
    h.budgetBytes=budgetBytes;
    // Budget counts speculative slots beyond the scanned pending token.
    h.byteUpper=add(pending,budgetBytes/sizeof(Token),limit);
    h.effectiveUpper=(std::max)(h.lower,(std::min)(h.relativeUpper,h.byteUpper));
    const size_t beforeBudget=AutoReserve::clampPrediction(prediction,{h.lower,h.relativeUpper});
    h.request=AutoReserve::clampPrediction(prediction,{h.lower,h.effectiveUpper});
    h.byteLimited=h.request<beforeBudget;
    const auto tail=remainingBound(pending,remaining,eof,limit);
    if (tail.fits) {
        h.tailBound=tail.count;
        if (tail.count<=h.request) { h.request=tail.count; h.tailApplied=true; }
    }
    h.floorOverride=h.request>h.byteUpper;
    return h;
}

#ifdef MASKETJSON_AUTO_V2_TRACE
struct Decision {
    Kind kind{}; Reason state{};
    size_t position=0, tokens=0, oldCapacity=0, actualCapacity=0, moved=0, streak=0;
    Hint hint{};
    double globalDensity=0,recentDensity=0,prediction=0;
};
struct Journal {
    std::array<Decision,512> entries{};
    size_t size=0,dropped=0;
    void push(const Decision& d) noexcept { if(size<entries.size()) entries[size++]=d; else ++dropped; }
};
inline thread_local Journal* activeJournal=nullptr;
struct JournalScope {
    Journal* previous;
    explicit JournalScope(Journal& j):previous(activeJournal){activeJournal=&j;}
    ~JournalScope(){activeJournal=previous;}
    JournalScope(const JournalScope&)=delete;
    JournalScope& operator=(const JournalScope&)=delete;
};
#endif

template<Goal goal>
class Controller {
    const size_t inputBytes;
    const size_t& position;
    std::vector<Token>& tokens;
    size_t previousPosition=0,previousTokens=0,stableStreak=0,relocated=0;
    double previousDensity=0;
    bool finished=false;
    void record(Kind kind,Reason state,size_t old,const Hint& hint,double global=0,double recent=0,double prediction=0) const noexcept {
#ifdef MASKETJSON_AUTO_V2_TRACE
        if(activeJournal)activeJournal->push({kind,state,position,tokens.size(),old,tokens.capacity(),relocated,stableStreak,hint,global,recent,prediction});
#else
        (void)kind;(void)state;(void)old;(void)hint;(void)global;(void)recent;(void)prediction;
#endif
    }
public:
    Controller(size_t bytes,const size_t& cursor,std::vector<Token>& buffer):inputBytes(bytes),position(cursor),tokens(buffer) {
        Hint h;h.request=tokens.capacity();h.budgetBytes=goal==Goal::Speed ? speedBudget : memoryBudget;record(Kind::Initial,Reason::Initial,0,h);
    }
    ~Controller(){if(!finished){Hint h;h.request=tokens.capacity();h.budgetBytes=goal==Goal::Speed ? speedBudget : memoryBudget;record(Kind::Aborted,Reason::Aborted,tokens.capacity(),h);}}
    Controller(const Controller&)=delete;
    Controller& operator=(const Controller&)=delete;
    void grow(bool eof) {
        const size_t n=tokens.size(),capacity=tokens.capacity(),limit=tokens.max_size();
        const size_t pending=add(n,1,limit),deltaBytes=position-previousPosition;
        const double global=static_cast<double>(pending)/static_cast<double>((std::max)(position,size_t{1}));
        const double recent=deltaBytes ? static_cast<double>(pending-previousTokens)/static_cast<double>(deltaBytes) : global;
        const double ratio=previousDensity>0 ? recent/previousDensity : 1;
        const bool drop=previousDensity>0 && ratio<0.25,rise=previousDensity>0 && ratio>4;
        const bool stable=previousDensity>0 && ratio>=0.5 && ratio<=2;
        stableStreak=stable ? add(stableStreak,1,(std::numeric_limits<size_t>::max)()) : 0;
        const bool cold=n<1024 || position<4096 || previousDensity==0;
        // V1's speed threshold 2*n was unreachable under its 1.5x growth floor.
        const bool pressure=n && static_cast<double>(relocated)>(goal==Goal::Speed ? 1.5 : 3.0)*static_cast<double>(n);
        const bool mature=!cold && (stableStreak>=2 || (pressure && stable));
        const Reason state=drop ? Reason::DensityDrop : rise ? Reason::DensityRise : cold ? Reason::Warming
            : pressure ? Reason::CopyPressure : stable ? Reason::Stable : Reason::Unstable;
        const double density=goal==Goal::Memory || drop ? (std::min)(global,recent) : rise ? (std::max)(global,recent) : global;
        double prediction=static_cast<double>(pending)+density*static_cast<double>(inputBytes-position)+1;
        if constexpr(goal==Goal::Speed)prediction*=1.0625;
        const auto hint=choose(goal,capacity,limit,mature,pressure,inputBytes-position,eof,prediction,
            goal==Goal::Speed ? speedBudget : memoryBudget);
        const Kind kind=hint.tailApplied ? (position==inputBytes ? Kind::End : Kind::Tail)
            : hint.floorOverride ? Kind::FloorOverride : hint.byteLimited ? Kind::ByteBudget : Kind::Forecast;
#ifdef MASKETJSON_AUTO_V2_TRACE
        try{tokens.reserve(hint.request);}catch(...){record(Kind::AllocationFailed,state,capacity,hint,global,recent,prediction);throw;}
#else
        tokens.reserve(hint.request);
#endif
        relocated=add(relocated,n,(std::numeric_limits<size_t>::max)());
        record(kind,state,capacity,hint,global,recent,prediction);
        previousPosition=position;previousTokens=pending;previousDensity=recent;
    }
    void finish() noexcept {finished=true;Hint h;h.request=tokens.capacity();h.budgetBytes=goal==Goal::Speed ? speedBudget : memoryBudget;record(Kind::Finished,Reason::Finished,tokens.capacity(),h);}
};
}
