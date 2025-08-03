#include "pch.h"
#include "jsonHandlerC++.hpp"

// Constructors

Json::Json() : value(nullptr) {}
Json::Json(std::nullptr_t) : value(nullptr) {}
Json::Json(bool b) : value(b) {}
Json::Json(int i) : value(static_cast<int64_t>(i)) {}
Json::Json(int64_t i) : value(i) {}
Json::Json(uint64_t u) : value(u) {}
Json::Json(double d) : value(d) {}
Json::Json(const std::string& s) : value(s) {}
Json::Json(const char* s) : value(std::string(s)) {}
Json::Json(const JsonObject& obj) : value(obj) {}
Json::Json(const JsonArray& arr) : value(arr) {}

// Type checks

bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
bool Json::isBool() const { return std::holds_alternative<bool>(value); }
bool Json::isInt64() const { return std::holds_alternative<int64_t>(value); }
bool Json::isUInt64() const { return std::holds_alternative<uint64_t>(value); }
bool Json::isDouble() const { return std::holds_alternative<double>(value); }
bool Json::isNumber() const { return isInt64() || isUInt64() || isDouble(); }
bool Json::isString() const { return std::holds_alternative<std::string>(value); }
bool Json::isObject() const { return std::holds_alternative<JsonObject>(value); }
bool Json::isArray() const { return std::holds_alternative<JsonArray>(value); }

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
 * @brief Retrieves the JSON value as double.
 * @return Double value.
 * @throws std::runtime_error if the value is not a number.
 */
double Json::asDouble() const {
    if (isInt64()) return static_cast<double>(asInt64());
    if (isUInt64()) return static_cast<double>(asUInt64());
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
 * @brief Retrieves the JSON value as an int64_t.
 * @return The int64_t value.
 * @throws std::runtime_error if the value is not an int64_t.
 */
int64_t Json::asInt64() const {
    if (!isInt64()) throw std::runtime_error("Json value is not an int64_t");
    return std::get<int64_t>(value);
}

/**
 * @brief Retrieves the JSON value as a uint64_t.
 * @return The uint64_t value.
 * @throws std::runtime_error if the value is not a uint64_t.
 */
uint64_t Json::asUInt64() const {
    if (!isUInt64()) throw std::runtime_error("Json value is not a uint64_t");
    return std::get<uint64_t>(value);
}

/**
 * @brief Retrieves the JSON value as a specific type.
 * @tparam T The type to convert to.
 * @return The value as the specified type.
 * @throws std::runtime_error if the value is not of the specified type.
 */
template<typename T>
T Json::as() const { return std::get<T>(value); }

/**
 * @brief Checks if the JSON value is of a specific type.
 * @tparam T The type to check against.
 * @return True if the value is of the specified type, false otherwise.
 */
template<typename T>
bool Json::is() const { return std::holds_alternative<T>(value); }

/**
 * @brief Indexing operator for accessing or creating a value in a JSON object.
 * If the JSON value is not an object, it will be set as a new empty object.
 * @param key to access in the JSON object.
 * @return A reference to the corresponding value in the JSON object.
 */
Json& Json::operator[](const std::string& key) {
    if (!isObject()) value = JsonObject{}; // Ensure the value is a JsonObject
    return std::get<JsonObject>(value)[key]; // Create default Json if key doesn't exist
}

/**
 * @brief Indexing operator for accessing or creating a value in a JSON array.
 * If the JSON value is not an array, it will be set as a new empty array.
 * Expands the array if the given index is out of range.
 * @param index The index to access in the JSON array.
 * @return A reference to the corresponding value in the JSON array.
 */
Json& Json::operator[](size_t index) {
    if (!isArray()) value = JsonArray{}; // Ensure the value is a JsonArray
    auto& arr = std::get<JsonArray>(value);
    if (index >= arr.size()) arr.resize(index + 1); // Expand array size if index is out of range
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
 * @param json The JSON value to add to the array.
 */
void Json::push_back(const Json& json) {
    if (!isArray()) value = JsonArray{};
    std::get<JsonArray>(value).push_back(json);
}

/**
 * @brief Adds multiple elements to a JSON array.
 * @param json The first JSON value to add.
 * @param args Additional JSON values to add.
 */
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
    if (isObject()) return asObject().size();
    if (isArray()) return asArray().size();
    throw std::runtime_error("Json value is not an object or array so size is applicable");
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
 * @brief Removes an element from a JSON object by key.
 * @param key The key of the element to remove.
 * @throws std::runtime_error if the current value is not an object.
 */
void Json::remove(const std::string& key) {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    std::get<JsonObject>(value).erase(key);
}

/**
 * @brief Checks if a JSON object contains a specific value.
 * @param string The key to check for.
 * @return True if the key exists in the object, false otherwise.
 * @throws std::runtime_error if the current value is not an object.
 */
bool Json::contains(const std::string& key) const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    return asObject().count(key);
}

/**
 * @brief Checks if a JSON array contains a specific value.
 * @param json The JSON value to check for.
 * @return True if the value exists in the array, false otherwise.
 * @throws std::runtime_error if the current value is not an array.
 */
bool Json::contains(const Json& j) const {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    return std::find(asArray().begin(), asArray().end(), j) != asArray().end();
}

/**
 * @brief Checks if the JSON value is empty.
 * @return True if the JSON value is empty (null, empty object, or empty array), false otherwise.
 */
bool Json::isEmpty() const {
    if (isNull()) return true;
    if (isObject()) return asObject().empty();
    if (isArray()) return asArray().empty();
    return false; // Non-empty primitive values are not considered empty
}

/**
 * @brief Retrieves a value from a JSON object or array by key or index.
 * @param key The key to look for in an object, or the index in an array.
 * @param defaultValue The value to return if the key/index is not found.
 * @return The JSON value associated with the key/index, or defaultValue if not found.
 * @throws std::runtime_error if the current value is not an object or array.
 */
Json Json::get(const std::string& key, const Json& defaultValue) const {
    if (isObject()) {
        auto it = asObject().find(key);
        if (it != asObject().end()) return it->second;
    }
    else if (isArray()) {
        try {
            size_t index = std::stoul(key);
            if (index < asArray().size()) return asArray()[index];
        }
        catch (...) {
            // Ignore conversion errors
        }
    }
    return defaultValue; // Return default value if key not found
}
Json Json::get(size_t index, const Json& defaultValue) const {
	if (isArray()) {
		if (index < asArray().size()) return asArray()[index];
	}
	else if (isObject()) {
		auto it = asObject().find(std::to_string(index));
		if (it != asObject().end()) return it->second;
	}
	return defaultValue; // Return default value if index not found
}
/**
 * @brief Returns a list of all keys in the object.
 * @return vector<string> of keys.
 * @throws std::runtime_error if the current value is not an object.
 */
std::vector<std::string> Json::keys() const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    std::vector<std::string> out;
    for (auto& [k, _] : asObject()) out.push_back(k);
    return out;
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
 * @brief Finds all occurrences of a specific JSON value in the current JSON object or array.
 * @param val The JSON value to search for.
 * @return A vector of strings representing the paths to the found values.
 */
std::vector<std::string> Json::findValue(const Json& val) const {
    std::vector<std::string> result;
    std::function<void(const Json&, std::string)> search = [&](const Json& node, std::string path) {
        if (node == val) result.push_back(path);
        if (node.isObject()) {
            for (auto& [k, v] : node.asObject()) search(v, path.empty() ? k : path + "." + k);
        }
        else if (node.isArray()) {
            for (size_t i = 0; i < node.asArray().size(); ++i) search(node[i], path + "[" + std::to_string(i) + "]");
        }
        };
    search(*this, "");
    return result;
}

/**
 * @brief Parses a JSON string into a Json object.
 * @param str The JSON string to parse.
 * @return Parsed Json object.
 * @throws std::runtime_error if the string is not valid JSON.
 */
Json Json::parse(const std::string& str, bool commentFlag) {
    std::string input = commentFlag ? removeJsonComments(str) : str;
    size_t pos = 0;

    auto throwError = [&](const std::string& msg) {
        throw std::runtime_error("Parse error at position " + std::to_string(pos) + ": " + msg);
    };

    auto skipWhitespace = [&]() {
        while (pos < str.size() && std::isspace(static_cast<unsigned char>(str[pos]))) ++pos;
    };

    auto parseString = [&]() -> std::string {
        ++pos;
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
            } else result += str[pos];
            ++pos;
        }
        if (pos >= str.size() || str[pos] != '"') throwError("Unterminated string");
        ++pos;
        return result;
    };

    auto parseNumber = [&]() -> Json {
        size_t start = pos;
        bool hasDot = false, hasExp = false;
        while (pos < str.size() && (std::isdigit(str[pos]) || str[pos] == '.' || str[pos] == 'e' || str[pos] == 'E' || str[pos] == '+' || str[pos] == '-')) {
            if (str[pos] == '.') hasDot = true;
            if (str[pos] == 'e' || str[pos] == 'E') hasExp = true;
            ++pos;
        }
        std::string numStr = str.substr(start, pos - start);

        try {
            if (hasDot || hasExp) return Json(std::stod(numStr));
            int64_t val = std::stoll(numStr);
            return Json(val);
        } catch (...) {
            try {
                uint64_t val = std::stoull(numStr);
                return Json(val);
            } catch (...) {
                throwError("Invalid number format");
            }
        }
        return Json();
    };

    std::function<Json()> parseValue = [&]() -> Json {
        skipWhitespace();
        if (pos >= str.size()) throwError("Unexpected end of input");

        if (str[pos] == '"') return Json(parseString());
        if (str[pos] == 'n' && str.substr(pos, 4) == "null") { pos += 4; return Json(nullptr); }
        if (str[pos] == 't' && str.substr(pos, 4) == "true") { pos += 4; return Json(true); }
        if (str[pos] == 'f' && str.substr(pos, 5) == "false") { pos += 5; return Json(false); }
        if (std::isdigit(str[pos]) || str[pos] == '-') return parseNumber();
        if (str[pos] == '{') {
            ++pos; skipWhitespace();
            JsonObject obj;
            while (pos < str.size() && str[pos] != '}') {
                Json key = parseValue();
                if (!key.isString()) throwError("Expected string key");
                skipWhitespace();
                if (str[pos] != ':') throwError("Expected ':'");
                ++pos; skipWhitespace();
                obj[key.asString()] = parseValue();
                skipWhitespace();
                if (str[pos] == ',') { ++pos; skipWhitespace(); }
            }
            if (str[pos] != '}') throwError("Expected '}'");
            ++pos;
            return Json(obj);
        }
        if (str[pos] == '[') {
            ++pos; skipWhitespace();
            JsonArray arr;
            while (pos < str.size() && str[pos] != ']') {
                arr.push_back(parseValue());
                skipWhitespace();
                if (str[pos] == ',') { ++pos; skipWhitespace(); }
            }
            if (str[pos] != ']') throwError("Expected ']");
            ++pos;
            return Json(arr);
        }

        throwError("Invalid value");
        return Json();
    };

    return parseValue();
}

/**
 * @brief Serializes the JSON value to a string.
 * @param pretty If true, the output will be indented for readability.
 * @param indentLevel The starting indentation level for pretty-printing.
 * @return The serialized JSON string.
 */
std::string Json::serialize(bool pretty, int indentLevel) const {
    std::string indent(pretty ? indentLevel * 2 : 0, ' ');
    if (isNull()) return "null";
    if (isBool()) return asBool() ? "true" : "false";
    if (isInt64()) {
        std::ostringstream oss;
        oss << asInt64();
        return oss.str();
    }
    if (isUInt64()) {
        std::ostringstream oss;
        oss << asUInt64();
        return oss.str();
    }
    if (isDouble()) {
        std::ostringstream oss;
        oss << asDouble();
        return oss.str();
    }
    if (isString()) return "\"" + escapeString(asString()) + "\"";
    if (isObject()) return serializeObject(asObject(), pretty, indentLevel);
    if (isArray()) return serializeArray(asArray(), pretty, indentLevel);
    return "";
}

/**
 * @brief Escapes special characters in a JSON string.
 * @param input The string to escape.
 * @return The escaped string.
 */
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

/**
 * @brief Serializes a JSON object to a string.
 * @param obj The JSON object to serialize.
 * @param pretty If true, the output will be indented for readability.
 * @param indentLevel The starting indentation level for pretty-printing.
 * @return The serialized JSON string.
 */
std::string Json::serializeObject(const JsonObject& obj, bool pretty, int indentLevel) const {
    std::string result = "{";
    std::string indent((indentLevel + 1) * 2, ' ');
    std::string outerIndent(indentLevel * 2, ' ');
    bool first = true;

    for (const auto& [key, value] : obj) {
        if (!first) result += ",";
        if (pretty) result += "\n" + indent;
        result += "\"" + escapeString(key) + "\": ";
        result += value.serialize(pretty, indentLevel + 1);
        first = false;
    }

    if (pretty && !obj.empty()) result += "\n" + outerIndent;
	result += "}";
    return result;
}

/**
 * @brief Serializes the JSON array to a string.
 * @param pretty If true, the output will be indented for readability.
 * @param indentLevel The starting indentation level for pretty-printing.
 * @return The serialized JSON string.
 */
std::string Json::serializeArray(const JsonArray& arr, bool pretty, int indentLevel) const {
    std::string result = "[";
    std::string newLine = pretty ? "\n" : "";
    std::string indent(pretty ? (indentLevel + 1) * 2 : 0, ' ');
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) result += ",";
        result += newLine + indent + arr[i].serialize(pretty, indentLevel + 1);
    }
    if (pretty && !arr.empty())
        result += "\n" + std::string(indentLevel * 2, ' ');
    result += "]";
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
 * @brief Merges another JSON array into the current array.
 * @param other The JSON array to merge.
 * @throws std::runtime_error if the current value is not an array.
 */
void Json::mergeArray(const JsonArray& other) {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    auto& arr = std::get<JsonArray>(value);
    arr.insert(arr.end(), other.begin(), other.end());
}

/**
 * @brief Saves the JSON value to a file.
 * @param filename The name of the file to save to.
 * @param pretty If true, the output will be indented for readability.
 * @throws std::runtime_error if the file cannot be opened.
 */
void Json::saveToFile(const std::string& filename, bool pretty) const {
    std::ofstream out(filename);
    if (!out.is_open()) throw std::runtime_error("Unable to open file: " + filename);
    out << serialize(pretty);
}

/**
 * @brief Loads a JSON value from a file.
 * @param filename The name of the file to load from.
 * @param commentsFlag If true, comments will be removed from the JSON string.
 * @throws std::runtime_error if the file cannot be opened or the content is not valid JSON.
 */
void Json::loadFromFile(const std::string& filename, bool commentsFlag) {
    std::ifstream in(filename);
    if (!in.is_open()) throw std::runtime_error("Unable to open file: " + filename);
    std::ostringstream ss;
    ss << in.rdbuf();
    *this = parse(ss.str(), commentsFlag);
}

/**
 * @brief Saves the JSON value to a file (alias for saveToFile).
 * @param filename The name of the file to save to.
 * @param pretty If true, the output will be indented for readability.
 */
void Json::toFile(const std::string& filename, bool pretty) const {
    saveToFile(filename, pretty);
}

/**
 * @brief Loads a JSON value from a file (alias for loadFromFile).
 * @param filename The name of the file to load from.
 * @param commentsFlag If true, comments will be removed from the JSON string.
 */
void Json::fromFile(const std::string& filename, bool commentsFlag) {
    loadFromFile(filename, commentsFlag);
}

Json::JsonArray::iterator Json::begin() { return std::get<JsonArray>(value).begin(); }
Json::JsonArray::iterator Json::end() { return std::get<JsonArray>(value).end(); }
Json::JsonArray::const_iterator Json::begin() const { return std::get<JsonArray>(value).begin(); }
Json::JsonArray::const_iterator Json::end() const { return std::get<JsonArray>(value).end(); }
Json::JsonArray::const_iterator Json::cbegin() const { return std::get<JsonArray>(value).cbegin(); }
Json::JsonArray::const_iterator Json::cend() const { return std::get<JsonArray>(value).cend(); }

/**
 * @brief Outputs the JSON value to an output stream.
 * @param os The output stream to write to.
 * @param json The JSON value to output.
 * @return The output stream.
 */
std::ostream& operator<<(std::ostream& os, const Json& json) {
    // Simplified for this stage
    if (json.isNull()) os << "null";
    else if (json.isBool()) os << (json.asBool() ? "true" : "false");
    else if (json.isInt64()) os << json.asInt64();
    else if (json.isUInt64()) os << json.asUInt64();
    else if (json.isDouble()) os << json.asDouble();
    else if (json.isString()) os << '"' << json.asString() << '"';
    else if (json.isArray()) {
        os << '[';
        bool first = true;
        for (const auto& el : json.asArray()) {
            if (!first) os << ',';
            os << el;
            first = false;
        }
        os << ']';
    }
    else if (json.isObject()) {
        os << '{';
        bool first = true;
        for (const auto& [k, v] : json.asObject()) {
            if (!first) os << ',';
            os << '"' << k << "\": " << v;
                first = false;
        }
        os << '}';
    }
    return os;
}

/**
 * @brief Splits a dot-separated path into individual keys.
 * @param path The dot-separated path (e.g., "Marks.MathMark").
 * @return A vector of keys.
 */
std::vector<std::string> Json::splitPath(const std::string& path) {
    std::vector<std::string> keys;
    std::istringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '.')) keys.push_back(part);
    return keys;
}

/**
 * @brief Retrieves a value from a JSON object or array using a dot-separated path.
 * @param path The dot-separated path to the value (e.g., "Marks.MathMark").
 * @return A reference to the JSON value at the specified path.
 * @throws std::runtime_error if the path is invalid or the value is not found.
 */
Json& Json::atPath(const std::string& path) {
    auto keys = splitPath(path);
    Json* curr = this;
    for (const auto& key : keys) curr = &((*curr)[key]);
    return *curr;
}

/**
 * @brief Retrieves a value from a JSON object or array using a dot-separated path (const version).
 * @param path The dot-separated path to the value (e.g., "Marks.MathMark").
 * @return A reference to the JSON value at the specified path.
 * @throws std::runtime_error if the path is invalid or the value is not found.
 */
const Json& Json::getByPath(const std::string& path) const {
    auto keys = splitPath(path);
    const Json* curr = this;
    for (const auto& key : keys) curr = &((*curr)[key]);
    return *curr;
}

/**
 * @brief Sorts a JSON array of objects by a nested key path (e.g., "Marks.MathMark").
 * @param keyPath The key path to sort by (e.g., "Marks.MathMark").
 * @param ascending If true, sorts in ascending order; otherwise, descending.
 * @throws std::runtime_error if the JSON value is not an array of objects or the key path is invalid.
 */
void Json::sortByPath(const std::string& keyPath, bool ascending) {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    auto& arr = std::get<JsonArray>(value);

    auto keys = splitPath(keyPath);

    auto getValueByPath = [&](const Json& json) -> const Json& {
        const Json* current = &json;
        for (const auto& key : keys) {
            if (!current->isObject()) throw std::runtime_error("Invalid key path at: " + key);
            auto& obj = current->asObject();
            auto it = obj.find(key);
            if (it == obj.end()) throw std::runtime_error("Key not found: " + key);
            current = &it->second;
        }
        return *current;
    };

    std::sort(arr.begin(), arr.end(), [&](const Json& a, const Json& b) {
        const Json& valA = getValueByPath(a);
        const Json& valB = getValueByPath(b);

        if (valA.isString() && valB.isString()) return ascending ? valA.asString() < valB.asString() : valA.asString() > valB.asString();
        if (valA.isInt64() && valB.isInt64()) return ascending ? valA.asInt64() < valB.asInt64() : valA.asInt64() > valB.asInt64();
        if (valA.isUInt64() && valB.isUInt64()) return ascending ? valA.asUInt64() < valB.asUInt64() : valA.asUInt64() > valB.asUInt64();
        if (valA.isDouble() && valB.isDouble()) return ascending ? valA.asDouble() < valB.asDouble() : valA.asDouble() > valB.asDouble();

        throw std::runtime_error("Cannot compare values of incompatible or unsupported types");
    });
}

/**
 * @brief Removes comments from a JSON string.
 * This function removes both single-line and multi-line comments,
 * while preserving comments inside string literals.
 * @param input The JSON string with potential comments.
 * @return The JSON string without comments.
*/
std::string Json::removeJsonComments(const std::string& input) {
    std::string output;
    bool inString = false;
    bool inSingleLineComment = false;
    bool inMultiLineComment = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        char next = i + 1 < input.size() ? input[i + 1] : '\0';

        if (inSingleLineComment) {
            if (c == '\n') {
                inSingleLineComment = false;
                output += c;
            }
            continue;
        }

        if (inMultiLineComment) {
            if (c == '*' && next == '/') {
                inMultiLineComment = false;
                ++i;
            }
            continue;
        }

        if (!inString && c == '/' && next == '/') {
            inSingleLineComment = true;
            ++i;
            continue;
        }

        if (!inString && c == '/' && next == '*') {
            inMultiLineComment = true;
            ++i;
            continue;
        }

        if (c == '"') {
            output += c;
            if (!(i > 0 && input[i - 1] == '\\')) inString = !inString;
        }
        else {
            output += c;
        }
    }
    return output;
}