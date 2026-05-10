#pragma once

#include <string>

class DiagnosticFormatter
{
public:
    static std::string format(
        const std::string& source,
        size_t line,
        size_t column,
        const std::string& message
    );

private:
    static std::string getLine(
        const std::string& source,
        size_t line
    );
};