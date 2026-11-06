#pragma once
#include "JsonHandler.hpp"

class MASKETJSON_API JsonFile
{
public:
    // File failures throw FileException. Invalid contents propagate LexerException
    // or ParseException, preserving their source location.
    static Json read(const std::string& filename);

    // Serialize before opening the file; check write/flush/close failures.
    // Writes are not atomic: an I/O failure can leave a partial output file.
    static void write(const std::string& filename, const Json& json);
};
