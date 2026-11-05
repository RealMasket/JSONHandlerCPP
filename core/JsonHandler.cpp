#include "pch.h"
#include "JsonHandler.hpp"
#include "JsonAlgorithms.hpp"
#include "TypeException.hpp"
#include "AccessException.hpp"
#include "JsonSerializer.hpp"
#include "OperationException.hpp"
#include <cmath>
#include <limits>

namespace {
template<class Number>
int compare(Number a, Number b) { return (a > b) - (a < b); }

// Check exact power-of-two bounds BEFORE floating-to-integer conversion.
// Unlike casting both arguments to double (or MSVC's long double), these
// comparisons distinguish adjacent integers above 2^53 without overflow/UB.
int compareIntegerDouble(std::int64_t integer, double floating)
{
    if (floating < -0x1p63) return 1;
    if (floating >= 0x1p63) return -1;
    const auto truncated = static_cast<std::int64_t>(floating);
    const int order = compare(integer, truncated);
    if (order != 0 || floating == std::trunc(floating)) return order;
    return floating > 0 ? -1 : 1;
}

int compareIntegerDouble(std::uint64_t integer, double floating)
{
    if (floating < 0) return 1;
    if (floating >= 0x1p64) return -1;
    const auto truncated = static_cast<std::uint64_t>(floating);
    const int order = compare(integer, truncated);
    if (order != 0 || floating == std::trunc(floating)) return order;
    return -1;
}
}

Json::Json(std::string&& s)
    : value(std::move(s)) {}

Json::Json(JsonObject&& obj)
    : value(std::move(obj)) {}

Json::Json(JsonArray&& arr)
    : value(std::move(arr)) {}

// Constructors

Json::Json() : value(nullptr) {}
Json::Json(std::nullptr_t) : value(nullptr) {}
Json::Json(bool b) : value(b) {}
Json::Json(double d) : value(d) {}
Json::Json(const std::string& s) : value(s) {}
Json::Json(const JsonObject& obj) : value(obj) {}
Json::Json(const JsonArray& arr) : value(arr) {}
Json::Json(const char* s) : value(std::string(s)) {}
Json::Json(int i) : value(static_cast<std::int64_t>(i)) {}
Json::Json(std::int64_t i) : value(i) {}
Json::Json(std::uint64_t i) : value(i) {}

// Type checks

bool Json::isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
bool Json::isBool() const { return std::holds_alternative<bool>(value); }
bool Json::isNumber() const { return isInteger() || isFloatingPoint(); }
bool Json::isInteger() const { return isSignedInteger() || isUnsignedInteger(); }
bool Json::isSignedInteger() const { return std::holds_alternative<std::int64_t>(value); }
bool Json::isUnsignedInteger() const { return std::holds_alternative<std::uint64_t>(value); }
bool Json::isFloatingPoint() const { return std::holds_alternative<double>(value); }
bool Json::isString() const { return std::holds_alternative<std::string>(value); }
bool Json::isObject() const { return std::holds_alternative<JsonObject>(value); }
bool Json::isArray() const { return std::holds_alternative<JsonArray>(value); }
bool Json::isBigNumber() const {
    if (!isString())
        return false;

    const auto& str = asString();

    return !str.empty() &&
        str.find_first_not_of("0123456789") == std::string::npos;
}
bool Json::isEmpty() const {
    if (isObject()) {
        return std::get<JsonObject>(value).empty();
    }
    if (isArray()) {
        return std::get<JsonArray>(value).empty();
    }
    return false;
}

/*
* @brief Clears the contents of the JSON value. If it's an object or array, it will be emptied. For other types, it will be set to null.
* @return A reference to the current JSON value after clearing.
*/
Json& Json::clear() {
    if (isObject()) {
        std::get<JsonObject>(value).clear();
    }
    else if (isArray()) {
        std::get<JsonArray>(value).clear();
    }
    else {
        value = nullptr;
    }
    return *this;
}

/*
* @brief Returns a string representation of the JSON value's type (e.g., "null", "bool", "number", "string", "object", "array").
* @return A string representing the type of the JSON value.
*/
std::string Json::typeName() const
{
    if (isNull()) return "null";
    if (isBool()) return "bool";
    if (isNumber()) return "number";
    if (isString()) return "string";
    if (isObject()) return "object";
    if (isArray()) return "array";

    return "unknown";
}

// Accessors

/**
 * @brief Retrieves the JSON value as a boolean.
 * @return Boolean value.
 * @throws TypeException if the value is not a boolean.
 */
bool Json::asBool() const {
    if (!isBool()) throw TypeException(
        "Expected boolean but got " + typeName()
    );
    return std::get<bool>(value);
}

/**
 * @brief Converts any numeric value to double. Large integers may round.
 * @return Double value.
 * @throws TypeException if the value is not a number.
 */
double Json::asNumber() const {
    if (isSignedInteger()) return static_cast<double>(std::get<std::int64_t>(value));
    if (isUnsignedInteger()) return static_cast<double>(std::get<std::uint64_t>(value));
    if (!isNumber()) throw TypeException(
        "Expected number but got " + typeName()
    );
    return std::get<double>(value);
}

std::int64_t Json::asInt64() const {
    if (isSignedInteger()) return std::get<std::int64_t>(value);
    if (isUnsignedInteger() && std::get<std::uint64_t>(value) <=
        static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)()))
        return static_cast<std::int64_t>(std::get<std::uint64_t>(value));
    throw TypeException("Expected a stored integer fitting int64_t");
}

std::uint64_t Json::asUInt64() const {
    if (isUnsignedInteger()) return std::get<std::uint64_t>(value);
    if (isSignedInteger() && std::get<std::int64_t>(value) >= 0)
        return static_cast<std::uint64_t>(std::get<std::int64_t>(value));
    throw TypeException("Expected a stored integer fitting uint64_t");
}

int Json::compareNumbers(const Json& other) const {
    if (!isNumber() || !other.isNumber())
        throw TypeException("Numeric comparison requires two numbers");
    if ((isFloatingPoint() && std::isnan(asNumber())) ||
        (other.isFloatingPoint() && std::isnan(other.asNumber())))
        throw OperationException("NaN cannot be ordered");
    if (isFloatingPoint()) {
        if (other.isFloatingPoint()) return compare(asNumber(), other.asNumber());
        return -other.compareNumbers(*this);
    }
    if (other.isFloatingPoint()) {
        if (isSignedInteger()) return compareIntegerDouble(asInt64(), other.asNumber());
        return compareIntegerDouble(asUInt64(), other.asNumber());
    }
    if (isSignedInteger() && other.isSignedInteger())
        return compare(asInt64(), other.asInt64());
    if (isUnsignedInteger() && other.isUnsignedInteger())
        return compare(asUInt64(), other.asUInt64());
    if (isSignedInteger()) {
        if (asInt64() < 0) return -1;
        return compare(static_cast<std::uint64_t>(asInt64()), other.asUInt64());
    }
    return -other.compareNumbers(*this);
}

/**
 * @brief Retrieves the JSON value as a string (const version).
 * @return String value.
 * @throws TypeException if the value is not a string.
 */
const std::string& Json::asString() const {
    if (!isString()) throw TypeException(
        "Expected string but got " + typeName()
    );
    return std::get<std::string>(value);
}

/**
 * @brief Retrieves the JSON value as an object (const version).
 * @return The object value.
 * @throws TypeException if the value is not an object.
 */
const Json::JsonObject& Json::asObject() const {
    if (!isObject()) throw TypeException(
        "Expected JSON object but got " + typeName()
    );
    return std::get<JsonObject>(value);
}

/**
 * @brief Retrieves the JSON value as an array (const version).
 * @return The array value.
 * @throws TypeException if the value is not an array.
 */
const Json::JsonArray& Json::asArray() const {
    if (!isArray()) throw TypeException(
        "Expected JSON array but got " + typeName()
    );
    return std::get<JsonArray>(value);
}

/**
 * @brief Retrieves the JSON value as a string (non-const version).
 * @return String value.
 * @throws TypeException if the value is not a string.
 */
std::string& Json::asString() {
    if (!isString()) throw TypeException(
        "Expected string but got " + typeName()
    );
    return std::get<std::string>(value);
}

/**
 * @brief Retrieves the JSON value as an object (non-const version).
 * @return The object value.
 * @throws TypeException if the value is not an object.
 */
Json::JsonObject& Json::asObject() {
    if (!isObject()) throw TypeException(
        "Expected JSON object but got " + typeName()
    );
    return std::get<JsonObject>(value);
}

/**
 * @brief Retrieves the JSON value as an array (non-const version).
 * @return The array value.
 * @throws TypeException if the value is not an array.
 */
Json::JsonArray& Json::asArray() {
    if (!isArray()) throw TypeException(
        "Expected JSON array but got " + typeName()
    );
    return std::get<JsonArray>(value);
}

/**
 * @brief Indexing operator for accessing or creating a value in a JSON object (non-const version).
 * A null value becomes an object; other non-object types throw TypeException.
 * @param key to access in the JSON object.
 * @return A reference to the corresponding value in the JSON object.
 */
Json& Json::operator[](const std::string& key) {
    if (isNull()) {
        value = JsonObject{}; // Ensure the value is a JsonObject
    }
    if (!isObject()) throw TypeException("Expected JSON object but got " + typeName());
    auto& obj = std::get<JsonObject>(value);
    return obj[key]; // Create default Json if key doesn't exist
}

/**
 * @brief Indexing operator for accessing or creating a value in a JSON array (non-const version).
 * A null value becomes an array; other non-array types throw TypeException.
 * Expands the array if the given index is out of range.
 * @param index The index to access in the JSON array.
 * @return A reference to the corresponding value in the JSON array.
 */
Json& Json::operator[](size_t index) {
    if (isNull()) {
        value = JsonArray{}; // Ensure the value is a JsonArray
    }
    if (!isArray()) throw TypeException("Expected JSON array but got " + typeName());
    auto& arr = std::get<JsonArray>(value);
    if (index >= arr.max_size()) throw AccessException("Array index is too large");
    if (index >= arr.size()) {
        arr.resize(index + 1); // Expand array size if index is out of range
    }
    return arr[index];
}

/**
 * @brief Indexing operator for accessing a value in a JSON object (const version).
 * @param key The key to access in the JSON object.
 * @return A reference to the corresponding value in the JSON object.
 * @throws TypeException if the JSON value is not an object.
 * @throws AccessException if the key is not found in the JSON object.
 */
const Json& Json::operator[](const std::string& key) const {
    if (!isObject()) {
        throw TypeException("Expected JSON object but got " + typeName());
    }
    const auto& obj = std::get<JsonObject>(value);
    auto it = obj.find(key);
    if (it == obj.end()) {
        throw AccessException("Key not found in object: " + key);
    }
    return it->second;
}

/**
 * @brief Indexing operator for accessing a value in a JSON array (const version).
 * @param index The index to access in the JSON array.
 * @return A reference to the corresponding value in the JSON array.
 * @throws TypeException if the JSON value is not an array.
 * @throws AccessException if the index is out of range.
 */
const Json& Json::operator[](size_t index) const {
    if (!isArray()) {
        throw TypeException("Expected JSON array but got " + typeName());
    }
    const auto& arr = std::get<JsonArray>(value);
    if (index >= arr.size()) {
        throw AccessException(
            "Index " +
            std::to_string(index) +
            " is out of range (size = " +
            std::to_string(arr.size()) +
            ")"
        );
    }
    return arr[index];
}

/**
 * @brief Adds a new element to a JSON array.
 * (Only works if the current value is an array.)
 * @param json The JSON value to add to the array.
 * @throws TypeException if the current value is not an array.
 */
void Json::push_back(const Json& json) {
    if (!isArray()) throw TypeException("Expected JSON array but got " + typeName());
    std::get<JsonArray>(value).push_back(json);
}

Json& Json::at(const std::string& key) {
    return const_cast<Json&>(std::as_const(*this)[key]);
}
const Json& Json::at(const std::string& key) const { return (*this)[key]; }
Json& Json::at(size_t index) {
    return const_cast<Json&>(std::as_const(*this)[index]);
}
const Json& Json::at(size_t index) const { return (*this)[index]; }

/**
 * @brief Retrieves the size of a JSON array.
 * @return The number of elements in the array.
 * @throws TypeException if the current value is not an array.
 */
size_t Json::size() const {
    if (isObject()) {
        return std::get<JsonObject>(value).size();
    }
    if (isArray()) {
        return std::get<JsonArray>(value).size();
    }
    throw TypeException("Expected JSON object or array but got " + typeName());
}

/**
 * @brief Removes an element from a JSON array by index.
 * @param index The index of the element to remove.
 * @throws TypeException if the current value is not an array.
 * @throws AccessException if the index is out of range.
 */
void Json::remove(size_t index) {
    if (!isArray()) throw TypeException("Expected JSON array but got " + typeName());
    auto& arr = std::get<JsonArray>(value);
    if (index >= arr.size()) throw AccessException(
        "Index " +
        std::to_string(index) +
        " is out of range (size = " +
        std::to_string(arr.size()) +
        ")"
    );
    arr.erase(arr.begin() + index);
}

/**
 * @brief Removes an element from a JSON object by key.
 * @param key The key of the element to remove.
 * @throws TypeException if the current value is not an object.
 */
void Json::remove(const std::string& key) {
    if (!isObject()) throw TypeException("Expected JSON object but got " + typeName());
    auto& obj = std::get<JsonObject>(value);
    obj.erase(key);
}

/**
 * @brief Checks if a JSON object contains a specific value.
 * @param string The key to check for.
 * @return True if the key exists in the object, false otherwise.
 * @throws TypeException if the current value is not an object.
 */
bool Json::contains(const std::string& key) const {
    if (!isObject()) throw TypeException("Expected JSON object but got " + typeName());
    const auto& obj = std::get<JsonObject>(value);
    return obj.find(key) != obj.end();
}

/**
 * @brief Checks if a JSON array contains a specific value.
 * @param json The JSON value to check for.
 * @return True if the value exists in the array, false otherwise.
 * @throws TypeException if the current value is not an array.
 */
bool Json::contains(const Json& json) const {
    if (!isArray()) throw TypeException("Expected JSON array but got " + typeName());
    const auto& arr = std::get<JsonArray>(value);
    return std::find(arr.begin(), arr.end(), json) != arr.end();
}

/**
 * @brief Returns a list of all keys in the object.
 * @return vector<string> of keys.
 * @throws TypeException if the current value is not an object.
 */
std::vector<std::string> Json::keys() const {
    if (!isObject()) throw TypeException("Expected JSON object but got " + typeName());
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
    if (isNumber() && other.isNumber()) {
        if ((isFloatingPoint() && std::isnan(asNumber())) ||
            (other.isFloatingPoint() && std::isnan(other.asNumber()))) return false;
        return compareNumbers(other) == 0;
    }
    if (value.index() != other.value.index()) return false;
    if (isArray()) return asArray() == other.asArray();
    if (isObject()) return asObject() == other.asObject();
    return value == other.value;
}

/**
 * @brief Wrapper method to merge another JSON object into the current object.
 * (Existing keys are overwritten based on the overwrite parameter.)
 * @param target The target JSON object to merge into.
 * @param other The JSON object to merge.
 * @param overwrite If true, existing keys will be overwritten.
 * @throws TypeException if the current value is not an object.
 */
void Json::mergeObject(const Json& other, bool overwrite) {
    JsonAlgorithms::mergeObject(*this, other, overwrite);
}

/**
 * @brief Wrapper method to merge another JSON array into the current array.
 * @param target The target JSON array to merge into.
 * @param other The JSON array to merge.
 * @throws TypeException if the current value is not an array.
 */
void Json::mergeArray(const Json& other) {
    JsonAlgorithms::mergeArray(*this, other);
}

/**
 * @brief Wrapper method to sort a JSON array of objects by a nested key path (e.g., "Marks.MathMark").
 * @param target The target JSON array to sort.
 * @param keyPath The key path to sort by (e.g., "Marks.MathMark").
 * @param ascending If true, sorts in ascending order; otherwise, descending.
 * @throws TypeException if the JSON value is not an array of objects.
 * @throws AccessException if the key path does not exist in the objects.
 */
void Json::sortByPath(const std::string& keyPath, bool ascending) {
    JsonAlgorithms::sortByPath(*this, keyPath, ascending);
}

/**
 * @brief Wrapper method to serializes the JSON value to a string.
 * @param pretty If true, the output will be indented for readability.
 * @param indentLevel The starting indentation level for pretty-printing.
 * @return The serialized JSON string.
 */
std::string Json::serialize(bool pretty, int indentLevel) const {
    return JsonSerializer::serialize(*this, pretty, indentLevel);
}
