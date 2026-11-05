#pragma once

#include "JsonException.hpp"

class JSON_API OperationException : public JsonException
{
public:
    explicit OperationException(
        const std::string& message
    );
};