#pragma once
#include "JsonHandler.hpp"

class MASKETJSON_API JsonFile
{
public:
    // Windows filenames use UTF-8 (std::filesystem::path::u8string()).
    // File failures throw FileException. Invalid contents propagate LexerException
    // or ParseException, preserving their source location.
    static Json read(const std::string& filename);

    // Serialize before opening the file; check write/flush/close failures.
    // Writes are not atomic: an I/O failure can leave a partial output file.
    static void write(const std::string& filename, const Json& json);

    // Serialize fully, then replace via a closed sibling temporary file.
    // No trailing newline; pretty=false selects compact JSON. Windows paths
    // are UTF-8. See README for permissions, symlinks, and durability limits.
    static void writeAtomic(const std::string& filename, const Json& json, bool pretty = true);
};
