#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace MemoryTracker {
enum class Category : size_t { Other, String, Array, MapNode, Token, Input, Count };
struct Stats {
    std::uint64_t calls = 0, bytes = 0, alignedCalls = 0;
    std::uint64_t liveBytes = 0, liveBlocks = 0, peakBytes = 0, peakBlocks = 0;
};
struct Record { std::uintptr_t address = 0; size_t bytes = 0; };
class LiveSet;

// One process-wide window; only its owning thread's allocations are captured.
// Frees from any thread are accounted. Table storage uses untracked malloc.
class Session {
public:
    Session();
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Stats snapshot() const noexcept;
    bool insertLocked(void* pointer, size_t bytes, bool aligned) noexcept;
    void eraseLocked(void* pointer) noexcept;
private:
    friend class LiveSet;
    bool rehash(size_t capacity) noexcept;
    Record* table_ = nullptr;
    size_t capacity_ = 0, used_ = 0;
    Stats stats_;
};

// Detached, sorted snapshot: no pointer to a Session or live allocation metadata.
// Labeling result buffers cannot change the measured heap counters.
class LiveSet {
public:
    explicit LiveSet(const Session& session);
    ~LiveSet();
    LiveSet(const LiveSet&) = delete;
    LiveSet& operator=(const LiveSet&) = delete;
    void exact(const void* pointer, Category category) noexcept;
    void containing(const void* pointer, Category category) noexcept;
    std::array<std::uint64_t, static_cast<size_t>(Category::Count)> categories() const noexcept;
    const Stats& stats() const noexcept { return stats_; }
private:
    struct Block { Record record; Category category = Category::Other; };
    Block* blocks_ = nullptr;
    size_t count_ = 0;
    Stats stats_;
};
void selfTest();
}
