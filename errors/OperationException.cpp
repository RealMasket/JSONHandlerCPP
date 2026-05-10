#include "OperationException.hpp"

OperationException::OperationException(
    const std::string& message
)
    : JsonException(
        "Operation error: " + message
    )
{
}