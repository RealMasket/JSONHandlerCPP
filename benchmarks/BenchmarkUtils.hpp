#pragma once

#include <functional>
#include <string>

class BenchmarkUtils
{
public:
    static double measureExecutionTime(
        const std::function<void()>& function,
        size_t iterations = 1
    );

    static void printResult(
        const std::string& name,
        double milliseconds,
        size_t iterations,
		std::string fileName = ""
    );
};