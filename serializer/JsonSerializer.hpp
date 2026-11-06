#pragma once
#include "JsonHandler.hpp"

class MASKETJSON_API JsonSerializer
{
public:
    /**
     * @brief Serializes the JSON value to a string.
     * @param pretty If true, the output will be indented for readability.
     * @param indentLevel The starting indentation level for pretty-printing.
     * @return The serialized JSON string.
     */
    // Throws OperationException for invalid UTF-8, non-finite numbers, invalid
    // indentation, or a value tree exceeding the 128-level nesting limit.
    static std::string serialize(const Json& json, bool pretty = true, int indentLevel = 0);

private:
    /*
    * @brief Escapes a UTF-8 string, including every ASCII control character.
    */
    static std::string escapeString(const std::string& input);
    /*
    * @brief Serializes a JSON object to a string with optional pretty-printing.
    * @param obj The JSON object to serialize.
    * @param pretty If true, the output will be indented for readability.
    * @param indentLevel The current indentation level for pretty-printing.
    * @return The serialized JSON object as a string.
    */
    static std::string serializeObject(const Json::JsonObject& obj, bool pretty, int indentLevel);
    /*
    * @brief Serializes a JSON array to a string with optional pretty-printing.
    */
    static std::string serializeArray(const Json::JsonArray& arr, bool pretty, int indentLevel);
};
