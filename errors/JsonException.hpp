#pragma once

#include <stdexcept>
#include <string>
#include "Api.hpp"

class JSON_API JsonException : public std::runtime_error
{
public:
    explicit JsonException(
        const std::string& message
    )
        : std::runtime_error(message)
    {
    }
};