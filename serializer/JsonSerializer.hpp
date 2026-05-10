#pragma once
#include "JsonHandler.hpp"

class JSON_API JsonSerializer
{
public:
    /**
     * @brief Serializes the JSON value to a string.
     * @param pretty If true, the output will be indented for readability.
     * @param indentLevel The starting indentation level for pretty-printing.
     * @return The serialized JSON string.
     */
    static std::string serialize(const Json& json, bool pretty = true, int indentLevel = 0);

private:
    /*
    * @brief Serializes a JSON object to a string with optional pretty-printing.
    * @param obj The JSON object to serialize.
    * @param pretty If true, the output will be indented for readability.
    * @param indentLevel The current indentation level for pretty-printing.
    * @return The serialized JSON object as a string.
    */
    static std::string escapeString(const std::string& input);
    /*
    * @brief Serializes a JSON array to a string with optional pretty-printing.
    * @param arr The JSON array to serialize.
    * @param pretty If true, the output will be indented for readability.
    * @param indentLevel The current indentation level for pretty-printing.
    * @return The serialized JSON array as a string.
    */
    static std::string serializeObject(const Json::JsonObject& obj, bool pretty, int indentLevel);
    /*
    * @brief Escapes special characters in a string for JSON serialization.
    * @param input The input string to escape.
    * @return The escaped string suitable for JSON output.
    */
    static std::string serializeArray(const Json::JsonArray& arr, bool pretty, int indentLevel);
};