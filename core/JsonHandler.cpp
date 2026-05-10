#include "pch.h"
#include "JsonHandler.hpp"
#include "JsonAlgorithms.hpp"

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
 * @brief Retrieves the JSON value as a string (const version).
 * @return String value.
 * @throws std::runtime_error if the value is not a string.
 */
const std::string& Json::asString() const {
    if (!isString()) throw std::runtime_error("Json value is not a string");
    return std::get<std::string>(value);
}

/**
 * @brief Retrieves the JSON value as an object (const version).
 * @return The object value.
 * @throws std::runtime_error if the value is not an object.
 */
const Json::JsonObject& Json::asObject() const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    return std::get<JsonObject>(value);
}

/**
 * @brief Retrieves the JSON value as an array (const version).
 * @return The array value.
 * @throws std::runtime_error if the value is not an array.
 */
const Json::JsonArray& Json::asArray() const {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    return std::get<JsonArray>(value);
}

/**
 * @brief Retrieves the JSON value as a string (non-const version).
 * @return String value.
 * @throws std::runtime_error if the value is not a string.
 */
std::string& Json::asString() {
    if (!isString()) throw std::runtime_error("Json value is not a string");
    return std::get<std::string>(value);
}

/**
 * @brief Retrieves the JSON value as an object (non-const version).
 * @return The object value.
 * @throws std::runtime_error if the value is not an object.
 */
Json::JsonObject& Json::asObject() {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    return std::get<JsonObject>(value);
}

/**
 * @brief Retrieves the JSON value as an array (non-const version).
 * @return The array value.
 * @throws std::runtime_error if the value is not an array.
 */
Json::JsonArray& Json::asArray() {
    if (!isArray()) throw std::runtime_error("Json value is not an array");
    return std::get<JsonArray>(value);
}

/**
 * @brief Indexing operator for accessing or creating a value in a JSON object (non-const version).
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
 * @brief Indexing operator for accessing or creating a value in a JSON array (non-const version).
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
    if (isObject()) {
        return std::get<JsonObject>(value).size();
    }
    if (isArray()) {
        return std::get<JsonArray>(value).size();
    }
    throw std::runtime_error("Json value is not an object or array");
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
    auto& obj = std::get<JsonObject>(value);
    obj.erase(key);
}

/**
 * @brief Checks if a JSON object contains a specific value.
 * @param string The key to check for.
 * @return True if the key exists in the object, false otherwise.
 * @throws std::runtime_error if the current value is not an object.
 */
bool Json::contains(const std::string& key) const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    const auto& obj = std::get<JsonObject>(value);
    return obj.find(key) != obj.end();
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
 * @brief Returns a list of all keys in the object.
 * @return vector<string> of keys.
 * @throws std::runtime_error if the current value is not an object.
 */
std::vector<std::string> Json::keys() const {
    if (!isObject()) throw std::runtime_error("Json value is not an object");
    const auto& obj = std::get<JsonObject>(value);
    std::vector<std::string> keys;
    for (const auto& pair : obj) {
        keys.push_back(pair.first);
    }
    return keys;
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
 * @brief Merges another JSON object into this one.
 * @param other The JSON object to merge.
 * @param overwrite Whether to overwrite existing keys.
 */
void Json::mergeObject(const Json& other, bool overwrite) {
    JsonAlgorithms::mergeObject(*this, other, overwrite);
}

/**
 * @brief Merges another JSON array into this one.
 * @param other The JSON array to merge.
 */
void Json::mergeArray(const Json& other) {
    JsonAlgorithms::mergeArray(*this, other);
}

/**
 * @brief Sorts a JSON array by a specified key path.
 * @param keyPath The key path to sort by.
 * @param ascending Whether to sort in ascending order.
 */
void Json::sortByPath(const std::string& keyPath, bool ascending) {
    JsonAlgorithms::sortByPath(*this, keyPath, ascending);
}