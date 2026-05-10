#include "pch.h"
#include "AccessException.hpp"

AccessException::AccessException(
    const std::string& message
)
    : JsonException(
        "Access error: " + message
    )
{
}