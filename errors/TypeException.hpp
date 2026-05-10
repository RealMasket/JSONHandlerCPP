#pragma once

#include "JsonException.hpp"

class TypeException : public JsonException
{
public:
    explicit TypeException(
        const std::string& message
    );
};