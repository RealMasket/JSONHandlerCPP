#include "BenchmarkUtils.hpp"

#include <iostream>
#include <chrono>

double BenchmarkUtils::measureExecutionTime(
    const std::function<void()>& function,
    size_t iterations)
{
    using namespace std::chrono;

    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i)
    {
        function();
    }

    auto end = high_resolution_clock::now();

    return duration<double, std::milli>(end - start).count();
}

void BenchmarkUtils::printResult(
    const std::string& name,
    double milliseconds,
    size_t iterations,
    std::string fileName)
{
    std::cout
        << name
        << "\nIterations: " << iterations
        << "\nTotal time: " << milliseconds << " ms"
        << "\nAverage time: "
        << (milliseconds / iterations)
        << " ms"
        << "\nFile name: " << fileName << "\n\n";
}