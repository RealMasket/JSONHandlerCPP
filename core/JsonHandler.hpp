#ifndef JSON_HANDLERC_HPP
#define JSON_HANDLERC_HPP

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <utility>
#include <cstdint>
#include <type_traits>
#include "Api.hpp"

class MASKETJSON_API Json {
public:
    using JsonObject = std::map<std::string, Json>;
    using JsonArray = std::vector<Json>;

    Json(std::string&& s);
    Json(JsonObject&& obj);
    Json(JsonArray&& arr);

    Json();
    Json(std::nullptr_t);
    Json(bool b);
    Json(double d);
    Json(const std::string& s);
    Json(const JsonObject& obj);
    Json(const JsonArray& arr);
    Json(const char* s);
    Json(int i);
    Json(std::int64_t i);
    Json(std::uint64_t i);

    // Cover all standard integral types without ambiguous signed/unsigned/double
    // overloads (int64_t aliases differ between Windows and Linux).
    template<class Integer, std::enable_if_t<std::is_integral_v<Integer> &&
        !std::is_same_v<Integer, bool>, int> = 0>
    Json(Integer i)
        : Json(static_cast<std::conditional_t<std::is_signed_v<Integer>,
            std::int64_t, std::uint64_t>>(i))
    {
        static_assert(sizeof(Integer) <= sizeof(std::uint64_t),
            "Integer types wider than 64 bits are not supported");
    }

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isInteger() const;
    bool isSignedInteger() const;
    bool isUnsignedInteger() const;
    bool isFloatingPoint() const;
    bool isString() const;
    bool isObject() const;
    bool isArray() const;
    [[deprecated("Digit-only strings are strings, not numeric values")]]
    bool isBigNumber() const;
    bool isEmpty() const;

    /*
    * @brief Clears the contents of the JSON value. If it's an object or array, it will be emptied. For other types, it will be set to null.
    * @return A reference to the current JSON value after clearing.
    */
    Json& clear();

    /*
    * @brief Returns a string representation of the JSON value's type (e.g., "null", "bool", "number", "string", "object", "array").
    * @return A string representing the type of the JSON value.
    */
	std::string typeName() const;
    /**
     * @brief Retrieves the JSON value as a boolean.
     * @return Boolean value.
     * @throws TypeException if the value is not a boolean.
     */
    bool asBool() const;
    /**
     * @brief Converts any numeric value to double. Large integers may round.
     * @return Double value.
     * @throws TypeException if the value is not a number.
     */
    double asNumber() const;
    // Checked access to stored integers, accepting either signedness when the
    // value fits. Floating-point values and out-of-range values throw TypeException.
    std::int64_t asInt64() const;
    std::uint64_t asUInt64() const;

    // Exact numeric ordering, including integer/double pairs: returns -1, 0, 1.
    // Non-numbers throw TypeException; NaN throws OperationException.
    int compareNumbers(const Json& other) const;
    /**
     * @brief Retrieves the JSON value as a string (const version).
     * @return String value.
     * @throws TypeException if the value is not a string.
     */
    const std::string& asString() const;
    /**
     * @brief Retrieves the JSON value as an object (const version).
     * @return The object value.
     * @throws TypeException if the value is not an object.
     */
    const JsonObject& asObject() const;
    /**
     * @brief Retrieves the JSON value as an array (const version).
     * @return The array value.
     * @throws TypeException if the value is not an array.
     */
    const JsonArray& asArray() const;

    /**
     * @brief Retrieves the JSON value as a string (non-const version).
     * @return String value.
     * @throws TypeException if the value is not a string.
     */
    std::string& asString();
    /**
     * @brief Retrieves the JSON value as an object (non-const version).
     * @return The object value.
     * @throws TypeException if the value is not an object.
     */
    JsonObject& asObject();
    /**
     * @brief Retrieves the JSON value as an array (non-const version).
     * @return The array value.
     * @throws TypeException if the value is not an array.
     */
    JsonArray& asArray();

    /**
     * @brief Indexing operator for accessing or creating a value in a JSON object (non-const version).
     * A null value becomes an object; other non-object types throw TypeException.
     * @param key to access in the JSON object.
     * @return A reference to the corresponding value in the JSON object.
     */
    Json& operator[](const std::string& key);
    /**
     * @brief Indexing operator for accessing or creating a value in a JSON array (non-const version).
     * A null value becomes an array; other non-array types throw TypeException.
     * Expands the array if the given index is out of range.
     * @param index The index to access in the JSON array.
     * @return A reference to the corresponding value in the JSON array.
     */
    Json& operator[](size_t index);
    /**
     * @brief Indexing operator for accessing a value in a JSON object (const version).
     * @param key The key to access in the JSON object.
     * @return A reference to the corresponding value in the JSON object.
     * @throws TypeException if the JSON value is not an object.
     * @throws AccessException if the key is not found in the JSON object.
     */
    const Json& operator[](const std::string& key) const;
    /**
     * @brief Indexing operator for accessing a value in a JSON array (const version).
     * @param index The index to access in the JSON array.
     * @return A reference to the corresponding value in the JSON array.
     * @throws TypeException if the JSON value is not an array.
     * @throws AccessException if the index is out of range.
     */
    const Json& operator[](size_t index) const;

    // Strict access: never inserts, resizes, or changes the stored type.
    Json& at(const std::string& key);
    const Json& at(const std::string& key) const;
    Json& at(size_t index);
    const Json& at(size_t index) const;
    /**
     * @brief Adds a new element to a JSON array.
     * (Only works if the current value is an array.)
     * @param json The JSON value to add to the array.
     * @throws TypeException if the current value is not an array.
     */
    void push_back(const Json& json);
    template<typename... Args>
    void push_back(const Json& json, const Args&... args)
    {
        push_back(json);
        (push_back(args), ...);
    }

    /**
     * @brief Retrieves the size of a JSON array.
     * @return The number of elements in the array.
     * @throws TypeException if the current value is not an array.
     */
    size_t size() const;
    /**
     * @brief Removes an element from a JSON array by index.
     * @param index The index of the element to remove.
     * @throws TypeException if the current value is not an array.
     * @throws AccessException if the index is out of range.
     */
    void remove(size_t index);
    /**
     * @brief Removes an element from a JSON object by key.
     * @param key The key of the element to remove.
     * @throws TypeException if the current value is not an object.
     * @throws AccessException if the key is not found in the JSON object.
     */
    void remove(const std::string& key);
    /**
     * @brief Checks if a JSON object contains a specific value.
     * @param string The key to check for.
     * @return True if the key exists in the object, false otherwise.
     * @throws TypeException if the current value is not an object.
     */
    bool contains(const std::string& key) const;
    /**
     * @brief Checks if a JSON array contains a specific value.
     * @param json The JSON value to check for.
     * @return True if the value exists in the array, false otherwise.
     * @throws TypeException if the current value is not an array.
     */
    bool contains(const Json& json) const;
    /**
     * @brief Returns a list of all keys in the object.
     * @return vector<string> of keys.
     * @throws TypeException if the current value is not an object.
     */
    std::vector<std::string> keys() const;

    /**
     * @brief Compares two JSON values for equality.
     * @param other The JSON value to compare with.
     * @return True if the values are equal, false otherwise.
     */
    bool operator==(const Json& other) const;
    
    /**
     * @brief Merges another JSON object into this one.
     * @param other The JSON object to merge.
     * @param overwrite Whether to overwrite existing keys.
     */
    void mergeObject(const Json& other, bool overwrite = true);

    /**
     * @brief Merges another JSON array into this one.
     * @param other The JSON array to merge.
     */
    void mergeArray(const Json& other);

    /**
     * @brief Sorts a JSON array by a specified key path.
     * @param keyPath The key path to sort by.
     * @param ascending Whether to sort in ascending order.
     */
    void sortByPath(const std::string& keyPath, bool ascending = true);

    /**
     * @brief Serializes the JSON value to a string.
     * @param pretty If true, the output will be indented for readability.
     * @param indentLevel The starting indentation level for pretty-printing.
     * @return The serialized JSON string.
     */
    std::string serialize(bool pretty = true, int indentLevel = 0) const;

private:
    using JsonValue = std::variant<std::nullptr_t, bool, std::int64_t, std::uint64_t,
        double, std::string, JsonObject, JsonArray>;
    JsonValue value;
};

#endif // JSON_HANDLERC_HPP
