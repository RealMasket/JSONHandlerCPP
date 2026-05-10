#include "DiagnosticFormatter.hpp"

#include <sstream>

std::string DiagnosticFormatter::format(
    const std::string& source,
    size_t line,
    size_t column,
    const std::string& message
)
{
    std::ostringstream oss;

    std::string lineText = getLine(source, line);

    oss << "Error at line "
        << line
        << ", column "
        << column
        << "\n\n";

    oss << lineText << "\n";

    for (size_t i = 1; i < column; ++i)
    {
        oss << ' ';
    }

    oss << "^\n";

    oss << message;

    return oss.str();
}

std::string DiagnosticFormatter::getLine(
    const std::string& source,
    size_t targetLine
)
{
    std::istringstream stream(source);

    std::string line;

    size_t currentLine = 1;

    while (std::getline(stream, line))
    {
        if (currentLine == targetLine)
        {
            return line;
        }

        ++currentLine;
    }

    return "";
}