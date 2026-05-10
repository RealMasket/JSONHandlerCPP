#pragma once

#include "JsonException.hpp"

class FileException : public JsonException
{
public:
    explicit FileException(
        const std::string& message
    );
};