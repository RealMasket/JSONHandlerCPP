#pragma once

#include "JsonException.hpp"

class JSON_API TypeException : public JsonException
{
public:
    explicit TypeException(
        const std::string& message
    );
};