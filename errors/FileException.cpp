#include "FileException.hpp"

FileException::FileException(
    const std::string& message
)
    : JsonException(
        "File error: " + message
    )
{
}