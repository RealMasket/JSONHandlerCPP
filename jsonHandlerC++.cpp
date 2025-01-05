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
Json::Json(int i) : value(static_cast<double>(i)) {}

// Type checks

bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
bool Json::isBool() const { return std::holds_alternative<bool>(value); }
bool Json::isNumber() const { return std::holds_alternative<double>(value); }
bool Json::isString() const { return std::holds_alternative<std::string>(value); }
bool Json::isObject() const { return std::holds_alternative<JsonObject>(value); }
bool Json::isArray() const { return std::holds_alternative<JsonArray>(value); }

/**
 * @brief Checks if the JSON value is a string representing a big number.
 * (Cached result improves performance for repeated checks.)
 * @return True if the value is a big number string, false otherwise.
 */
bool Json::isBigNumber() const {
    static thread_local std::optional<size_t> cachedResult;
    if (isString()) {
        const auto& str = asString();
        if (!cachedResult.has_value()) {
            cachedResult = str.find_first_not_of("0123456789");
        }
        return cachedResult.value() == std::string::npos;
    }
    return false;
}

// Accessors

/**
 * @brief Retrieves the JSON value as a boolean.
 * @return Boolean value.
 * @throws std::runtime_error if the value is not a boolean.
 */
bool Json::asBool() const {
    if (!isBool()) throw std::runtime_error("Json value is not a boolean");
    return std::get<bool>(value);
}

/**
 * @brief Retrieves the JSON value as a number (double).
 * @return Double value.
 * @throws std::runtime_error if the value is not a number.
 */
double Json::asNumber() const {
    if (!isNumber()) throw std::runtime_error("Json value is not a number");
    return std::get<double>(value);
}

/**
 * @brief Retrieves the JSON value as a string.
 * @return String value.
 * @throws std::runtime_error if the value is not a string.
 */
const std::string& Json::asString() const {
    if (!isString()) throw std::runtime_error("Json value is not a string");
    return std::get<std::string>(value);
}


/**
 * @brief Retrieves the JSON value as an object.
 * @return The object value.
 * @throws std::runtime_error if the value is not an object.
 */
const Json::JsonObject& Json::asObject() const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    return std::get<JsonObject>(value);
}

/**
 * @brief Retrieves the JSON value as an array.
 * @return The array value.
 * @throws std::runtime_error if the value is not an array.
 */
const Json::JsonArray& Json::asArray() const {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    return std::get<JsonArray>(value);
}

/**
 * @brief Indexing operator for accessing or creating a value in a JSON object.
 * If the JSON value is not an object, it will be set as a new empty object.
 * @param key to access in the JSON object.
 * @return A reference to the corresponding value in the JSON object.
 */
Json& Json::operator[](const std::string& key) {
    if (!isObject()) {
        value = JsonObject{}; // Ensure the value is a JsonObject
    }
    auto& obj = std::get<JsonObject>(value);
    return obj[key]; // Create default Json if key doesn't exist
}

/**
 * @brief Indexing operator for accessing or creating a value in a JSON array.
 * If the JSON value is not an array, it will be set as a new empty array.
 * Expands the array if the given index is out of range.
 * @param index The index to access in the JSON array.
 * @return A reference to the corresponding value in the JSON array.
 */
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

/**
 * @brief Indexing operator for accessing a value in a JSON object (const version).
 * @param key The key to access in the JSON object.
 * @return A reference to the corresponding value in the JSON object.
 * @throws std::runtime_error if the JSON value is not an object or if the key is not found.
 */
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

/**
 * @brief Indexing operator for accessing a value in a JSON array (const version).
 * @param index The index to access in the JSON array.
 * @return A reference to the corresponding value in the JSON array.
 * @throws std::runtime_error if the JSON value is not an array or if the index is out of range.
 */
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

/**
 * @brief Adds a new element to a JSON array.
 * (Only works if the current value is an array.)
 * @param json The JSON value to add to the array.
 * @throws std::runtime_error if the current value is not an array.
 */
void Json::push_back(const Json& json) {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    std::get<JsonArray>(value).push_back(json);
}

template<typename... Args>
void Json::push_back(const Json& json, Args... args) {
    push_back(json);
    push_back(args...);
}

/**
 * @brief Retrieves the size of a JSON array.
 * @return The number of elements in the array.
 * @throws std::runtime_error if the current value is not an array.
 */
size_t Json::size() const {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    return std::get<JsonArray>(value).size();
}

/**
 * @brief Removes an element from a JSON array by index.
 * @param index The index of the element to remove.
 * @throws std::runtime_error if the current value is not an array or the index is out of range.
 */
void Json::remove(size_t index) {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    auto& arr = std::get<JsonArray>(value);
    if (index >= arr.size()) throw std::runtime_error("Index out of range");
    arr.erase(arr.begin() + index);
}

/**
 * @brief Checks if a JSON array contains a specific value.
 * @param json The JSON value to check for.
 * @return True if the value exists in the array, false otherwise.
 * @throws std::runtime_error if the current value is not an array.
 */
bool Json::contains(const Json& json) const {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    const auto& arr = std::get<JsonArray>(value);
    return std::find(arr.begin(), arr.end(), json) != arr.end();
}

/**
 * @brief Compares two JSON values for equality.
 * @param other The JSON value to compare with.
 * @return True if the values are equal, false otherwise.
 */
bool Json::operator==(const Json& other) const {
    if (value.index() != other.value.index()) return false;
    if (isArray()) return asArray() == other.asArray();
    if (isObject()) return asObject() == other.asObject();
    return value == other.value;
}

/**
 * @brief Parses a JSON string into a Json object.
 * @param str The JSON string to parse.
 * @return Parsed Json object.
 * @throws std::runtime_error if the string is not valid JSON.
 */
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
            return parseNumber();
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

/**
 * @brief Serializes the JSON value to a string.
 * @param pretty If true, the output will be indented for readability.
 * @param indentLevel The starting indentation level for pretty-printing.
 * @return The serialized JSON string.
 */
std::string Json::serialize(bool pretty = false, int indentLevel = 0) const {
    std::string indent(pretty ? indentLevel * 2 : 0, ' ');
    if (isNull()) return "null";
    if (isBool()) return asBool() ? "true" : "false";
    if (isNumber()) {
        std::ostringstream oss;
        oss << asNumber();
        return oss.str();
    }
    if (isBigNumber()) return asString();
    if (isString()) return "\"" + escapeString(asString()) + "\"";
    if (isObject()) return serializeObject(asObject(), pretty, indentLevel);
    if (isArray()) return serializeArray(asArray(), pretty, indentLevel);
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
std::string Json::serializeObject(const JsonObject& obj, bool pretty, int indentLevel) const {
    std::string result = (pretty ? ("\n" + std::string(indentLevel * 2, ' ')) : "") + "{";
    std::string indent((indentLevel + 1) * 2, ' ');
    std::string outerIndent(indentLevel * 2, ' ');
    bool first = true;

    for (const auto& [key, value] : obj) {
        if (!first) result += ",";
        result += "\n" + indent;

        if (!pretty) {
            result += "\"" + escapeString(key) + "\": ";
        }
        else {
            result += key + ": ";
        }

        result += value.serialize(pretty, indentLevel + 1);
        first = false;
    }

    result += "\n" + outerIndent + "}";
    return result;
}

// Serialize JSON Array
std::string Json::serializeArray(const JsonArray& arr, bool pretty, int indentLevel) const {
    std::string result = "[";
    std::string newLine = pretty ? "\n" : "";
    std::string indent(pretty ? (indentLevel + 1) * 2 : 0, ' ');
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) result += ",";
        result += newLine + indent + arr[i].serialize(pretty, indentLevel + 1);
    }
    result += newLine + std::string(pretty ? indentLevel * 2 : 0, ' ') + "]";
    return result;
}

/**
 * @brief Merges another JSON object into the current object.
 * (Existing keys are overwritten based on the overwrite parameter.)
 * @param other The JSON object to merge.
 * @param overwrite If true, existing keys will be overwritten.
 * @throws std::runtime_error if the current value is not an object.
 */
void Json::mergeObject(const JsonObject& other, bool overwrite) {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    auto& obj = std::get<JsonObject>(value);
    for (const auto& [key, value] : other) {
        if (overwrite || obj.find(key) == obj.end()) {
            obj[key] = value;
        }
    }
}

/**
 * @brief Saves the JSON value to a file.
 * @param filename The name of the file to save to.
 * @throws std::runtime_error if the file cannot be opened.
 */
void Json::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + filename);
    }
    file << serialize();
    file.close();
}

/**
 * @brief Loads JSON data from a file and replaces the current value.
 * @param filename The name of the file to load from.
 * @throws std::runtime_error if the file cannot be opened or contains invalid JSON.
 */
void Json::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Unable to open file: " + filename);

    std::ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    *this = parse(buffer.str());
}