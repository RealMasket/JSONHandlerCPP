#pragma once

#include "JsonException.hpp"

class JSON_API FileException : public JsonException
{
public:
    explicit FileException(
        const std::string& message
    );
};