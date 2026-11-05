#pragma once

#include "JsonException.hpp"

class JSON_API AccessException : public JsonException
{
public:
    explicit AccessException(
        const std::string& message
    );
};