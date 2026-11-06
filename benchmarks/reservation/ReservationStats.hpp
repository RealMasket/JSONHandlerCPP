#pragma once
#include <cstddef>

// Private to experimental profiling executables. No production library hooks.
namespace ReservationStats {
struct Counters {
    size_t initialCapacity = 0, bufferAllocations = 0, growthEvents = 0, copiedTokens = 0;
};
inline thread_local Counters* active = nullptr;
struct Scope {
    Counters* previous;
    explicit Scope(Counters& counters) : previous(active) { active = &counters; }
    ~Scope() { active = previous; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};
}
