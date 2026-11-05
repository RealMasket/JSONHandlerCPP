#include "pch.h"
#include "FileException.hpp"

FileException::FileException(
    const std::string& message
)
    : JsonException(
        "File error: " + message
    )
{
}