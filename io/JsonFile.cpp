#include "pch.h"
#include "JsonFile.hpp"
#include "JsonParser.hpp"
#include "JsonSerializer.hpp"
#include "FileException.hpp"
#include "AtomicFile.hpp"
#include <fstream>

Json JsonFile::read(const std::string& filename)
{
    std::ifstream file(MasketJSONDetail::filePath(filename), std::ios::binary);
    if (!file) throw FileException("Unable to open file: " + filename);
    std::string text;
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) text.append(buffer, sizeof(buffer));
    text.append(buffer, static_cast<size_t>(file.gcount()));
    if (file.bad() || !file.eof()) throw FileException("Unable to read file: " + filename);
    return JsonParser::parse(text);
}

void JsonFile::write(const std::string& filename, const Json& json)
{
    // Serialization failure must not truncate an existing file.
    const std::string text = JsonSerializer::serialize(json);
    std::ofstream file(MasketJSONDetail::filePath(filename), std::ios::binary | std::ios::trunc);
    if (!file) throw FileException("Unable to open file: " + filename);
    file << text;
    file.flush();
    if (!file) throw FileException("Unable to write file: " + filename);
    file.close();
    if (file.fail()) throw FileException("Unable to close file: " + filename);
}

void JsonFile::writeAtomic(const std::string& filename, const Json& json, bool pretty)
{
    const std::string text = JsonSerializer::serialize(json, pretty);
    MasketJSONDetail::writeTextAtomic(filename, text);
}
