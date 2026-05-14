#include "OperationException.hpp"
#include "pch.h"

OperationException::OperationException(
    const std::string& message
)
    : JsonException(
        "Operation error: " + message
    )
{
}