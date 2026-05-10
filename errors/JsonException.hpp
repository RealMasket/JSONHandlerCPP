#pragma once

#include <stdexcept>
#include <string>

class JsonException : public std::runtime_error
{
public:
    explicit JsonException(
        const std::string& message
    )
        : std::runtime_error(message)
    {
    }
};