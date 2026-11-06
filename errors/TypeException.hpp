#pragma once

#include "JsonException.hpp"

class MASKETJSON_API TypeException : public JsonException
{
public:
    explicit TypeException(
        const std::string& message
    );
};