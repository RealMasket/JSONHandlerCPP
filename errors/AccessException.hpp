#pragma once

#include "JsonException.hpp"

class AccessException : public JsonException
{
public:
    explicit AccessException(
        const std::string& message
    );
};