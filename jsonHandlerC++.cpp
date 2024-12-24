#include "pch.h"
#include "jsonHandlerC++.hpp"

// Constructors
Json::Json() : value(nullptr) {}
Json::Json(std::nullptr_t) : value(nullptr) {}
Json::Json(bool b) : value(b) {}
Json::Json(double d) : value(d) {}
Json::Json(const std::string& s) : value(s) {}
Json::Json(const JsonObject& obj) : value(obj) {}
Json::Json(const JsonArray& arr) : value(arr) {}
Json::Json(const char* s) : value(std::string(s)) {}

// Type checks
bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
bool Json::isBool() const { return std::holds_alternative<bool>(value); }
bool Json::isNumber() const { return std::holds_alternative<double>(value); }
bool Json::isString() const { return std::holds_alternative<std::string>(value); }
bool Json::isObject() const { return std::holds_alternative<JsonObject>(value); }
bool Json::isArray() const { return std::holds_alternative<JsonArray>(value); }
bool Json::isBigNumber() const { return isString() && asString().find_first_not_of("0123456789") == std::string::npos; }

// Accessors
bool Json::asBool() const {
    if (!isBool()) throw std::runtime_error("Json value is not a boolean");
    return std::get<bool>(value);
}

double Json::asNumber() const {
    if (!isNumber()) throw std::runtime_error("Json value is not a number");
    return std::get<double>(value);
}

const std::string& Json::asString() const {
    if (!isString()) throw std::runtime_error("Json value is not a string");
    return std::get<std::string>(value);
}

const Json::JsonObject& Json::asObject() const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    return std::get<JsonObject>(value);
}

const Json::JsonArray& Json::asArray() const {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    return std::get<JsonArray>(value);
}

// Indexing operator for object and array
Json& Json::operator[](const std::string& key) {
    if (!isObject()) {
        value = JsonObject{}; // Ensure the value is a JsonObject
    }
    auto& obj = std::get<JsonObject>(value);
    return obj[key]; // Create default Json if key doesn't exist
}

Json& Json::operator[](size_t index) {
    if (!isArray()) {
        value = JsonArray{}; // Ensure the value is a JsonArray
    }
    auto& arr = std::get<JsonArray>(value);
    if (index >= arr.size()) {
        arr.resize(index + 1); // Expand array size if index is out of range
    }
    return arr[index];
}

//Same as above but for const
const Json& Json::operator[](const std::string& key) const {
    if (!isObject()) {
        throw std::runtime_error("Json value is not an object");
    }
    const auto& obj = std::get<JsonObject>(value);
    auto it = obj.find(key);
    if (it == obj.end()) {
        throw std::runtime_error("Key not found: " + key);
    }
    return it->second;
}

const Json& Json::operator[](size_t index) const {
    if (!isArray()) {
        throw std::runtime_error("Json value is not an array");
    }
    const auto& arr = std::get<JsonArray>(value);
    if (index >= arr.size()) {
        throw std::runtime_error("Index out of range");
    }
    return arr[index];
}

// Push back element into array
void Json::push_back(const Json& json) {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    std::get<JsonArray>(value).push_back(json);
}

// Parsing
Json Json::parse(const std::string& str) {
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
                case '\"': result += '\"'; break;
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

    std::function<Json()> parseValue = [&]() -> Json {
        skipWhitespace();
        if (pos >= str.size()) throwError("Unexpected end of input");

        if (str[pos] == '"') return Json(parseString());
        if (str[pos] == 'n') {
            if (str.substr(pos, 4) != "null") throwError("Invalid token");
            pos += 4;
            return Json(nullptr);
        }
        if (str[pos] == 't') {
            if (str.substr(pos, 4) != "true") throwError("Invalid token");
            pos += 4;
            return Json(true);
        }
        if (str[pos] == 'f') {
            if (str.substr(pos, 5) != "false") throwError("Invalid token");
            pos += 5;
            return Json(false);
        }
        if (isdigit(str[pos]) || str[pos] == '-') {
            size_t start = pos;
            bool isBig = false;
            while (pos < str.size() &&
                (isdigit(str[pos]) || str[pos] == '.' || str[pos] == 'e' || str[pos] == 'E' || str[pos] == '+' || str[pos] == '-')) {
                if (str[pos] == '.' || str[pos] == 'e' || str[pos] == 'E') isBig = true;
                ++pos;
            }
            std::string numberStr = str.substr(start, pos - start);
            if (isBig || numberStr.size() > 18) {  // For numbers with excessive digits
                return Json(numberStr);
            }
            try {
                return Json(std::stod(numberStr));
            }
            catch (...) {
                throwError("Invalid number format");
            }
        }
        if (str[pos] == '{') {
            ++pos;
            JsonObject obj;
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
            JsonArray arr;
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
        return Json(); // Return default null
        };

    return parseValue();
}

// Serialization
std::string Json::serialize() const {
    if (isNull()) return "null";
    if (isBool()) return asBool() ? "true" : "false";
    if (isNumber()) {
        std::ostringstream oss;
        oss << asNumber();
        return oss.str();
    }
    if (isBigNumber()) return asString();
    if (isString()) return "\"" + escapeString(asString()) + "\"";
    if (isObject()) return serializeObject(asObject());
    if (isArray()) return serializeArray(asArray());
    return "";
}

// Helper for string escaping
std::string Json::escapeString(const std::string& input) {
    std::string escaped;
    for (char c : input) {
        switch (c) {
        case '\"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\n': escaped += "\\n"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped += c;
        }
    }
    return escaped;
}

// Serialize JSON Object
std::string Json::serializeObject(const JsonObject& obj) {
    std::string result = "{";
    bool first = true;
    for (const auto& [key, value] : obj) {
        if (!first) result += ",";
        result += "\"" + key + "\":" + value.serialize();
        first = false;
    }
    result += "}";
    return result;
}

// Serialize JSON Array
std::string Json::serializeArray(const JsonArray& arr) {
    std::string result = "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) result += ",";
        result += arr[i].serialize();
    }
    result += "]";
    return result;
}
