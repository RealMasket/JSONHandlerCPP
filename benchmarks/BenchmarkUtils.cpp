#include "BenchmarkUtils.hpp"
#include <stdexcept>

namespace { volatile std::uint64_t benchmarkSink = 0; }
namespace { const void* volatile pointerSink = nullptr; }

void BenchmarkUtils::consume(std::uint64_t value) { benchmarkSink = value; }
void BenchmarkUtils::escape(const void* pointer) { pointerSink = pointer; }

BenchmarkMeasurement BenchmarkUtils::summarize(std::vector<double> samples)
{
    if (samples.empty()) throw std::invalid_argument("No benchmark samples");
    std::sort(samples.begin(), samples.end());
    const auto middle = samples.size() / 2;
    const double median = samples.size() % 2 ? samples[middle]
        : (samples[middle - 1] + samples[middle]) / 2.0;
    return {median, samples.front(), samples.back()};
}
