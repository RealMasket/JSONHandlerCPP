#include "JsonSerializer.hpp"
#include "pch.h"
#include "JsonHandler.hpp"
#include <sstream>

/**
 * @brief Serializes the JSON value to a string.
 * @param pretty If true, the output will be indented for readability.
 * @param indentLevel The starting indentation level for pretty-printing.
 * @return The serialized JSON string.
 */
std::string JsonSerializer::serialize(const Json& json, bool pretty, int indentLevel) {
    std::string indent(pretty ? indentLevel * 2 : 0, ' ');
    if (json.isNull()) return "null";
    if (json.isBool()) return json.asBool() ? "true" : "false";
    if (json.isNumber()) {
        std::ostringstream oss;
        oss << json.asNumber();
        return oss.str();
    }
    if (json.isBigNumber()) return json.asString();
    if (json.isString()) return "\"" + escapeString(json.asString()) + "\"";
    if (json.isObject()) return serializeObject(json.asObject(), pretty, indentLevel);
    if (json.isArray()) return serializeArray(json.asArray(), pretty, indentLevel);
    return "";
}

/*
* @brief Serializes a JSON object to a string with optional pretty-printing.
* @param obj The JSON object to serialize.
* @param pretty If true, the output will be indented for readability.
* @param indentLevel The current indentation level for pretty-printing.
* @return The serialized JSON object as a string.
*/
std::string JsonSerializer::serializeObject(const Json::JsonObject& obj, bool pretty, int indentLevel) {
    std::string result = (pretty ? ("\n" + std::string(indentLevel * 2, ' ')) : "") + "{";
    std::string indent((indentLevel + 1) * 2, ' ');
    std::string outerIndent(indentLevel * 2, ' ');
    bool first = true;

    for (const auto& [key, value] : obj) {
        if (!first) result += ",";
        result += "\n" + indent;

        if (pretty) {
            result += "\"" + escapeString(key) + "\": ";
        }
        else {
            result += "\"" + escapeString(key) + "\":";
        }

        result += serialize(value, pretty, indentLevel + 1);
        first = false;
    }

    result += "\n" + outerIndent + "}";
    return result;
}

/*
* @brief Serializes a JSON array to a string with optional pretty-printing.
* @param arr The JSON array to serialize.
* @param pretty If true, the output will be indented for readability.
* @param indentLevel The current indentation level for pretty-printing.
* @return The serialized JSON array as a string.
*/
std::string JsonSerializer::serializeArray(const Json::JsonArray& arr, bool pretty, int indentLevel) {
    std::string result = "[";
    std::string newLine = pretty ? "\n" : "";
    std::string indent(pretty ? (indentLevel + 1) * 2 : 0, ' ');
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) result += ","; 
        result += newLine + indent + serialize(arr[i], pretty, indentLevel + 1);
    }
    result += newLine + std::string(pretty ? indentLevel * 2 : 0, ' ') + "]";
    return result;
}

/*
* @brief Escapes special characters in a string for JSON serialization.
* @param input The input string to escape.
* @return The escaped string suitable for JSON output.
*/
std::string JsonSerializer::escapeString(const std::string& input) {
    std::string escaped;
    for (char c : input) {
        switch (c) {
        case '\"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\t': escaped += "\\t"; break;
        case '\r': escaped += "\\r"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '/': escaped += "\\/"; break;
        default: escaped += c;
        }
    }
    return escaped;
}