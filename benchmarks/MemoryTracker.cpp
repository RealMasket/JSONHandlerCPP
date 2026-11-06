#include "MemoryTracker.hpp"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <limits>
#include <new>
#include <stdexcept>
#include <thread>
#include <string>
#include <vector>
#ifdef _WIN32
#include <malloc.h>
#endif

namespace {
std::atomic_flag gate = ATOMIC_FLAG_INIT;
MemoryTracker::Session* active = nullptr;
thread_local MemoryTracker::Session* owner = nullptr;
struct Lock {
    Lock() noexcept { while (gate.test_and_set(std::memory_order_acquire)) {} }
    ~Lock() { gate.clear(std::memory_order_release); }
};
size_t hash(std::uintptr_t address) noexcept {
    std::uint64_t value = address;
    value ^= value >> 33;
    value *= UINT64_C(0xff51afd7ed558ccd);
    value ^= value >> 33;
    return static_cast<size_t>(value);
}
void release(void* pointer, bool aligned) noexcept {
    if (!pointer) return;
    { Lock lock; if (active) active->eraseLocked(pointer); }
#ifdef _WIN32
    if (aligned) { _aligned_free(pointer); return; }
#else
    (void)aligned;
#endif
    std::free(pointer);
}
void* allocate(size_t requested, size_t alignment = 0) {
    for (;;) {
        void* pointer = nullptr;
        const size_t physical = requested ? requested : 1;
        if (!alignment) pointer = std::malloc(physical);
        else {
#ifdef _WIN32
            pointer = _aligned_malloc(physical, alignment);
#else
            if (::posix_memalign(&pointer, (std::max)(alignment, sizeof(void*)), physical) != 0) pointer = nullptr;
#endif
        }
        if (pointer) {
            bool recorded = true;
            { Lock lock; if (owner && active == owner) recorded = owner->insertLocked(pointer, requested, alignment != 0); }
            if (recorded) return pointer;
            release(pointer, alignment != 0);
            throw std::bad_alloc();
        }
        if (const auto handler = std::get_new_handler()) handler();
        else throw std::bad_alloc();
    }
}
void require(bool condition) { if (!condition) throw std::runtime_error("Memory tracker self-test failed"); }
}

namespace MemoryTracker {
Session::Session() {
    if (!rehash(1024)) throw std::bad_alloc();
    bool conflict;
    {
        Lock lock;
        conflict = active != nullptr;
        if (!conflict) { active = this; owner = this; }
    }
    if (conflict) { std::free(table_); throw std::logic_error("Nested/concurrent memory windows are unsupported"); }
}
Session::~Session() {
    { Lock lock; if (active == this) active = nullptr; if (owner == this) owner = nullptr; }
    std::free(table_);
}
Stats Session::snapshot() const noexcept { Lock lock; return stats_; }
bool Session::rehash(size_t capacity) noexcept {
    if (capacity > (std::numeric_limits<size_t>::max)() / sizeof(Record)) return false;
    auto* replacement = static_cast<Record*>(std::malloc(capacity * sizeof(Record)));
    if (!replacement) return false;
    for (size_t i = 0; i < capacity; ++i) ::new (static_cast<void*>(replacement + i)) Record{};
    for (size_t i = 0; i < capacity_; ++i) {
        if (table_[i].address <= 1) continue;
        size_t slot = hash(table_[i].address) & (capacity - 1);
        while (replacement[slot].address) slot = (slot + 1) & (capacity - 1);
        replacement[slot] = table_[i];
    }
    std::free(table_);
    table_ = replacement;
    capacity_ = capacity;
    used_ = static_cast<size_t>(stats_.liveBlocks);
    return true;
}
bool Session::insertLocked(void* pointer, size_t bytes, bool aligned) noexcept {
    if (used_ + 1 >= capacity_ - capacity_ / 3) {
        const bool grow = stats_.liveBlocks >= capacity_ / 3;
        if (grow && capacity_ > (std::numeric_limits<size_t>::max)() / 2) return false;
        if (!rehash(grow ? capacity_ * 2 : capacity_)) return false;
    }
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    size_t slot = hash(address) & (capacity_ - 1), tombstone = capacity_;
    while (table_[slot].address) {
        if (table_[slot].address == 1 && tombstone == capacity_) tombstone = slot;
        slot = (slot + 1) & (capacity_ - 1);
    }
    if (tombstone != capacity_) slot = tombstone;
    else ++used_;
    table_[slot] = {address, bytes};
    ++stats_.calls;
    stats_.bytes += bytes;
    stats_.alignedCalls += aligned ? 1 : 0;
    stats_.liveBytes += bytes;
    ++stats_.liveBlocks;
    stats_.peakBytes = (std::max)(stats_.peakBytes, stats_.liveBytes);
    stats_.peakBlocks = (std::max)(stats_.peakBlocks, stats_.liveBlocks);
    return true;
}
void Session::eraseLocked(void* pointer) noexcept {
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    size_t slot = hash(address) & (capacity_ - 1);
    while (table_[slot].address) {
        if (table_[slot].address == address) {
            stats_.liveBytes -= table_[slot].bytes;
            --stats_.liveBlocks;
            table_[slot] = {1, 0};
            return;
        }
        slot = (slot + 1) & (capacity_ - 1);
    }
}

LiveSet::LiveSet(const Session& session) {
    bool failed = false;
    {
        Lock lock;
        stats_ = session.stats_;
        count_ = static_cast<size_t>(stats_.liveBlocks);
        if (count_) {
            if (count_ > (std::numeric_limits<size_t>::max)() / sizeof(Block)) failed = true;
            else blocks_ = static_cast<Block*>(std::malloc(count_ * sizeof(Block)));
            if (!blocks_) failed = true;
            if (!failed) {
                size_t index = 0;
                for (size_t i = 0; i < session.capacity_; ++i)
                    if (session.table_[i].address > 1)
                        ::new (static_cast<void*>(blocks_ + index++)) Block{session.table_[i], Category::Other};
            }
        }
    }
    if (failed) throw std::bad_alloc();
    if (count_ > 1) std::sort(blocks_, blocks_ + count_, [](const Block& a, const Block& b) {
        return a.record.address < b.record.address;
    });
}
LiveSet::~LiveSet() { std::free(blocks_); }
void LiveSet::exact(const void* pointer, Category category) noexcept {
    if (!pointer || !count_) return;
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    auto* block = std::lower_bound(blocks_, blocks_ + count_, address,
        [](const Block& a, std::uintptr_t b) { return a.record.address < b; });
    if (block != blocks_ + count_ && block->record.address == address && block->category == Category::Other)
        block->category = category;
}
void LiveSet::containing(const void* pointer, Category category) noexcept {
    if (!pointer || !count_) return;
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    auto* block = std::upper_bound(blocks_, blocks_ + count_, address,
        [](std::uintptr_t a, const Block& b) { return a < b.record.address; });
    if (block == blocks_) return;
    --block;
    if (address - block->record.address < block->record.bytes && block->category == Category::Other)
        block->category = category;
}
std::array<std::uint64_t, static_cast<size_t>(Category::Count)> LiveSet::categories() const noexcept {
    std::array<std::uint64_t, static_cast<size_t>(Category::Count)> result{};
    for (size_t i = 0; i < count_; ++i) result[static_cast<size_t>(blocks_[i].category)] += blocks_[i].record.bytes;
    return result;
}

void selfTest() {
    Stats measured;
    {
        Session session;
        void* a = ::operator new(13);
        void* b = ::operator new[](31);
        require(session.snapshot().liveBytes == 44);
        ::operator delete(a);
        void* c = ::operator new(5, std::nothrow);
        require(c != nullptr && session.snapshot().liveBytes == 36);
        ::operator delete[](b);
        ::operator delete(c, std::nothrow);
        measured = session.snapshot();
    }
    require(measured.calls == 3 && measured.bytes == 49 && measured.peakBytes == 44 && measured.liveBlocks == 0);
    {
        Session session;
        void* zero = ::operator new(0);
        void* aligned = ::operator new(97, std::align_val_t{128}, std::nothrow);
        require(aligned && reinterpret_cast<std::uintptr_t>(aligned) % 128 == 0);
        require(session.snapshot().liveBlocks == 2 && session.snapshot().liveBytes == 97);
        ::operator delete(zero);
        ::operator delete(aligned, std::align_val_t{128}, std::nothrow);
        measured = session.snapshot();
    }
    require(measured.alignedCalls == 1 && measured.liveBytes == 0 && measured.liveBlocks == 0);
    void* survivor;
    { Session session; survivor = ::operator new(41); }
    {
        Session session;
        void* current = ::operator new(7);
        ::operator delete(survivor); // Old-window and never-captured pointers must not underflow.
        require(session.snapshot().liveBytes == 7);
        ::operator delete(current);
        measured = session.snapshot();
    }
    require(measured.liveBlocks == 0);
    {
        Session session;
        auto* pointer = ::operator new(41);
        std::thread worker([pointer] { ::operator delete(pointer); });
        worker.join();
        measured = session.snapshot();
    }
    require(measured.liveBytes == 0 && measured.liveBlocks == 0);
    std::array<void*, 5000> pointers{};
    {
        Session session;
        for (size_t i = 0; i < pointers.size(); ++i) pointers[i] = ::operator new(i % 127 + 1);
        require(session.snapshot().liveBlocks == pointers.size());
        for (size_t i = 0; i < pointers.size(); i += 2) ::operator delete(pointers[i]);
        for (size_t i = 0; i < pointers.size(); i += 2) pointers[i] = ::operator new(17);
        for (auto* pointer : pointers) ::operator delete(pointer);
        measured = session.snapshot();
    }
    require(measured.liveBytes == 0 && measured.liveBlocks == 0);
    {
        Session session;
        {
            std::vector<std::string> values(500, "short");
            std::string large(8192, 'x');
            LiveSet blocks(session);
            blocks.containing(values.data(), Category::Array);
            const auto before = blocks.categories();
            for (const auto& value : values) blocks.containing(value.data(), Category::String);
            blocks.containing(large.data(), Category::String);
            const auto after = blocks.categories();
            require(after[static_cast<size_t>(Category::Array)] >= values.capacity() * sizeof(std::string));
            require(after[static_cast<size_t>(Category::Array)] == before[static_cast<size_t>(Category::Array)]);
            require(after[static_cast<size_t>(Category::String)] >= large.size());
        }
        measured = session.snapshot();
    }
    require(measured.liveBytes == 0 && measured.liveBlocks == 0);
}
}

// Only ParserProfile links these replacements. Direct malloc/CRT allocations,
// tracker metadata, allocator overhead, input storage, and stack are excluded.
void* operator new(size_t size) { return allocate(size); }
void* operator new[](size_t size) { return allocate(size); }
void* operator new(size_t size, const std::nothrow_t&) noexcept { try { return allocate(size); } catch (...) { return nullptr; } }
void* operator new[](size_t size, const std::nothrow_t&) noexcept { try { return allocate(size); } catch (...) { return nullptr; } }
void* operator new(size_t size, std::align_val_t alignment) { return allocate(size, static_cast<size_t>(alignment)); }
void* operator new[](size_t size, std::align_val_t alignment) { return allocate(size, static_cast<size_t>(alignment)); }
void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try { return allocate(size, static_cast<size_t>(alignment)); } catch (...) { return nullptr; }
}
void* operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept {
    try { return allocate(size, static_cast<size_t>(alignment)); } catch (...) { return nullptr; }
}
void operator delete(void* pointer) noexcept { release(pointer, false); }
void operator delete[](void* pointer) noexcept { release(pointer, false); }
void operator delete(void* pointer, size_t) noexcept { release(pointer, false); }
void operator delete[](void* pointer, size_t) noexcept { release(pointer, false); }
void operator delete(void* pointer, const std::nothrow_t&) noexcept { release(pointer, false); }
void operator delete[](void* pointer, const std::nothrow_t&) noexcept { release(pointer, false); }
void operator delete(void* pointer, std::align_val_t) noexcept { release(pointer, true); }
void operator delete[](void* pointer, std::align_val_t) noexcept { release(pointer, true); }
void operator delete(void* pointer, size_t, std::align_val_t) noexcept { release(pointer, true); }
void operator delete[](void* pointer, size_t, std::align_val_t) noexcept { release(pointer, true); }
void operator delete(void* pointer, std::align_val_t, const std::nothrow_t&) noexcept { release(pointer, true); }
void operator delete[](void* pointer, std::align_val_t, const std::nothrow_t&) noexcept { release(pointer, true); }
