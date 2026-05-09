#include "JsonFile.hpp"
#include "../utils/pch.h"
#include "../parser/JsonParser.hpp"
#include "../serializer/JsonSerializer.hpp"

/**
 * @brief Loads JSON data from a file and replaces the current value.
 * @param filename The name of the file to load from.
 * @throws std::runtime_error if the file cannot be opened or contains invalid JSON.
 */
Json JsonFile::read(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Unable to open file: " + filename);

    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return JsonParser::parse(buffer.str());
}

/**
 * @brief Saves the JSON value to a file.
 * @param filename The name of the file to save to.
 * @throws std::runtime_error if the file cannot be opened.
 */
void JsonFile::write(const std::string& filename, const Json& json) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }
    file << JsonSerializer::serialize(json);
    file.close();
}