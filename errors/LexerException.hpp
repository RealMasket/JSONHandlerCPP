#pragma once

#include "JsonException.hpp"

class LexerException : public JsonException
{
public:
    LexerException(
        const std::string& message,
        size_t line,
        size_t column
    );

    size_t getLine() const;
    size_t getColumn() const;

private:
    size_t line;
    size_t column;
};