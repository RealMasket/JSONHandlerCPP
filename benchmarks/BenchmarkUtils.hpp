#pragma once
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

struct BenchmarkMeasurement {
    double medianUs;
    double minUs;
    double maxUs;
};

class BenchmarkUtils {
public:
    static void consume(std::uint64_t value);
    // Defined out of line so the copy-stage buffer escapes the optimizer.
    static void escape(const void* pointer);
    static BenchmarkMeasurement summarize(std::vector<double> samples);

    // Each operation creates and destroys its result before returning a checksum.
    // Warm-up, clock reads, and the final volatile write are outside each loop.
    template<class Operation>
    static BenchmarkMeasurement measure(Operation operation, size_t iterations, size_t samples)
    {
        using Clock = std::chrono::steady_clock;
        for (int i = 0; i < 5; ++i) consume(operation());
        std::vector<double> times;
        times.reserve(samples);
        for (size_t sample = 0; sample < samples; ++sample) {
            std::uint64_t checksum = 0;
            const auto start = Clock::now();
            for (size_t i = 0; i < iterations; ++i) checksum += operation();
            const auto end = Clock::now();
            consume(checksum);
            times.push_back(std::chrono::duration<double, std::micro>(end - start).count()
                / static_cast<double>(iterations));
        }
        return summarize(std::move(times));
    }
};
