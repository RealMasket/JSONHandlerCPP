#pragma once

#include "JsonException.hpp"

class OperationException : public JsonException
{
public:
    explicit OperationException(
        const std::string& message
    );
};