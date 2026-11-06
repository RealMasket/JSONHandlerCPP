#pragma once

#include "JsonException.hpp"

class MASKETJSON_API AccessException : public JsonException
{
public:
    explicit AccessException(
        const std::string& message
    );
};