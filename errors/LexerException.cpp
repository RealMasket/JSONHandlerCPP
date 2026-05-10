#include "LexerException.hpp"

LexerException::LexerException(
    const std::string& message,
    size_t line,
    size_t column
)
    : JsonException(
        "Lexer error at line " +
        std::to_string(line) +
        ", column " +
        std::to_string(column) +
        ": " + message
    ),
    line(line),
    column(column)
{ 
}

size_t LexerException::getLine() const
{
    return line;
}

size_t LexerException::getColumn() const
{
    return column;
}