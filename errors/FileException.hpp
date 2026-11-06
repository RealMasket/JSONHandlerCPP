#pragma once

#include "JsonException.hpp"

class MASKETJSON_API FileException : public JsonException
{
public:
    explicit FileException(
        const std::string& message
    );
};