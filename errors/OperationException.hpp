#pragma once

#include "JsonException.hpp"

class MASKETJSON_API OperationException : public JsonException
{
public:
    explicit OperationException(
        const std::string& message
    );
};