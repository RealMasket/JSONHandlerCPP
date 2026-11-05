#include "pch.h"
#include "JsonSerializer.hpp"
#include "OperationException.hpp"
#include "Utf8.hpp"
#include <charconv>
#include <cmath>

std::string JsonSerializer::serialize(const Json& json, bool pretty, int indentLevel)
{
    if (indentLevel < 0 || indentLevel > 128)
        throw OperationException("Indentation level must be between 0 and 128");
    if (json.isNull()) return "null";
    if (json.isBool()) return json.asBool() ? "true" : "false";
    if (json.isNumber()) {
        char buffer[128];
        std::to_chars_result result;
        if (json.isSignedInteger())
            result = std::to_chars(buffer, buffer + sizeof(buffer), json.asInt64());
        else if (json.isUnsignedInteger())
            result = std::to_chars(buffer, buffer + sizeof(buffer), json.asUInt64());
        else {
            const double value = json.asNumber();
            if (!std::isfinite(value))
                throw OperationException("Cannot serialize NaN or infinity as JSON");
            result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);
        }
        if (result.ec != std::errc{})
            throw OperationException("Unable to serialize number");
        std::string text(buffer, result.ptr);
        if (json.isFloatingPoint() && text.find_first_of(".eE") == std::string::npos)
            text += ".0";
        return text;
    }
    if (json.isString()) return "\"" + escapeString(json.asString()) + "\"";
    if (indentLevel >= 128)
        throw OperationException("Maximum serialization nesting depth exceeded");
    if (json.isObject()) return serializeObject(json.asObject(), pretty, indentLevel);
    if (json.isArray()) return serializeArray(json.asArray(), pretty, indentLevel);
    throw OperationException("Unknown JSON value type");
}

std::string JsonSerializer::serializeObject(const Json::JsonObject& object,
    bool pretty, int indentLevel)
{
    if (object.empty()) return "{}";
    std::string result = "{";
    const std::string indent(pretty ? (indentLevel + 1) * 2 : 0, ' ');
    bool first = true;
    for (const auto& [key, value] : object) {
        if (!first) result += ',';
        if (pretty) result += "\n" + indent;
        result += "\"" + escapeString(key) + (pretty ? "\": " : "\":");
        result += serialize(value, pretty, indentLevel + 1);
        first = false;
    }
    if (pretty) result += "\n" + std::string(indentLevel * 2, ' ');
    result += '}';
    return result;
}

std::string JsonSerializer::serializeArray(const Json::JsonArray& array,
    bool pretty, int indentLevel)
{
    if (array.empty()) return "[]";
    std::string result = "[";
    const std::string indent(pretty ? (indentLevel + 1) * 2 : 0, ' ');
    bool first = true;
    for (const auto& value : array) {
        if (!first) result += ',';
        if (pretty) result += "\n" + indent;
        result += serialize(value, pretty, indentLevel + 1);
        first = false;
    }
    if (pretty) result += "\n" + std::string(indentLevel * 2, ' ');
    result += ']';
    return result;
}

std::string JsonSerializer::escapeString(const std::string& input)
{
    if (!JsonDetail::isValidUtf8(input))
        throw OperationException("Cannot serialize invalid UTF-8");
    constexpr char hex[] = "0123456789abcdef";
    std::string escaped;
    escaped.reserve(input.size());
    for (unsigned char c : input) {
        switch (c) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\t': escaped += "\\t"; break;
        case '\r': escaped += "\\r"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        default:
            if (c < 0x20) {
                escaped += "\\u00";
                escaped += hex[c >> 4];
                escaped += hex[c & 0x0F];
            } else escaped += static_cast<char>(c);
        }
    }
    return escaped;
}
