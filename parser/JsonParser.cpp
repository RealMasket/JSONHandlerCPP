#include "../utils/pch.h"
#include "JsonParser.hpp"

/**
 * @brief Parses a JSON string into a Json object.
 * @param str The JSON string to parse.
 * @return Parsed Json object.
 * @throws std::runtime_error if the string is not valid JSON.
 */
Json JsonParser::parse(const std::string& str) {
    size_t pos = 0;

    auto throwError = [&](const std::string& message) {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": " + message);
        };

    auto skipWhitespace = [&]() {
        while (pos < str.size() && std::isspace(str[pos])) ++pos;
        };

    auto parseString = [&]() -> std::string {
        ++pos; // Skip opening quote
        std::string result;
        while (pos < str.size() && str[pos] != '"') {
            if (str[pos] == '\\') {
                ++pos;
                if (pos >= str.size()) throwError("Unexpected end of input in string");
                switch (str[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: throwError("Invalid escape character");
                }
            }
            else {
                result += str[pos];
            }
            ++pos;
        }
        if (pos >= str.size() || str[pos] != '"') throwError("Unterminated string");
        ++pos; // Skip closing quote
        return result;
        };

    auto parseNumber = [&]() -> Json {
        size_t start = pos;
        while (pos < str.size() && (isdigit(str[pos]) || str[pos] == '.' || str[pos] == 'e' || str[pos] == 'E' || str[pos] == '+' || str[pos] == '-')) {
            ++pos;
        }
        std::string numberStr = str.substr(start, pos - start);
        char* end;
        double value = strtod(numberStr.c_str(), &end);
        if (*end != '\0') throwError("Invalid number format");
        return Json(value);
        };

    std::function<Json()> parseValue = [&]() -> Json {
        skipWhitespace();
        if (pos >= str.size()) throwError("Unexpected end of input");

        if (str[pos] == '"') return Json(parseString());
        if (str[pos] == 'n') {
            if (str.substr(pos, 4) != "null") throwError("Invalid token");
            pos += 4;
            return { nullptr };
        }
        if (str[pos] == 't') {
            if (str.substr(pos, 4) != "true") throwError("Invalid token");
            pos += 4;
            return { true };
        }
        if (str[pos] == 'f') {
            if (str.substr(pos, 5) != "false") throwError("Invalid token");
            pos += 5;
            return { false };
        }
        if (isdigit(str[pos]) || str[pos] == '-') {
            return parseNumber();
        }
        if (str[pos] == '{') {
            ++pos;
            Json::JsonObject obj;
            skipWhitespace();
            while (pos < str.size() && str[pos] != '}') {
                std::string key = parseValue().asString();
                skipWhitespace();
                if (pos >= str.size() || str[pos] != ':') throwError("Expected ':'");
                ++pos;
                skipWhitespace();
                obj[key] = parseValue();
                skipWhitespace();
                if (str[pos] == ',') ++pos;
                skipWhitespace();
            }
            if (pos >= str.size() || str[pos] != '}') throwError("Expected '}'");
            ++pos;
            return Json(obj);
        }
        if (str[pos] == '[') {
            ++pos;
            Json::JsonArray arr;
            skipWhitespace();
            while (pos < str.size() && str[pos] != ']') {
                arr.push_back(parseValue());
                skipWhitespace();
                if (str[pos] == ',') ++pos;
                skipWhitespace();
            }
            if (pos >= str.size() || str[pos] != ']') throwError("Expected ']'");
            ++pos;
            return Json(arr);
        }

        throwError("Invalid value");
        return {}; // Return default null
        };

    return parseValue();
}