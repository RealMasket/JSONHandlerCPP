#include "FileException.hpp"
#include "pch.h"

FileException::FileException(
    const std::string& message
)
    : JsonException(
        "File error: " + message
    )
{
}