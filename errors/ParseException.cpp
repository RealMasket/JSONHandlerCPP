#include "ParseException.hpp"

ParseException::ParseException(
    const std::string& message,
    size_t line,
    size_t column
)
    : JsonException(
        "Parse error at line " +
        std::to_string(line) +
        ", column " +
        std::to_string(column) +
        ": " + message
    ),
    line(line),
    column(column)
{
}