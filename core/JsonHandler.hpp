#ifndef JSON_HANDLERC_HPP
#define JSON_HANDLERC_HPP

#include <string>
#include <map>
#include <vector>
#include <variant>
#include "Api.hpp"

class JSON_API Json {
public:
    using JsonObject = std::map<std::string, Json>;
    using JsonArray = std::vector<Json>;

    Json();
    Json(std::nullptr_t);
    Json(bool b);
    Json(double d);
    Json(const std::string& s);
    Json(const JsonObject& obj);
    Json(const JsonArray& arr);
    Json(const char* s);
    Json(int i);

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isObject() const;
    bool isArray() const;
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
     * @brief Retrieves the JSON value as a number (double).
     * @return Double value.
     * @throws TypeException if the value is not a number.
     */
    double asNumber() const;
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
     * If the JSON value is not an object, it will be set as a new empty object.
     * @param key to access in the JSON object.
     * @return A reference to the corresponding value in the JSON object.
     */
    Json& operator[](const std::string& key);
    /**
     * @brief Indexing operator for accessing or creating a value in a JSON array (non-const version).
     * If the JSON value is not an array, it will be set as a new empty array.
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
    /**
     * @brief Adds a new element to a JSON array.
     * (Only works if the current value is an array.)
     * @param json The JSON value to add to the array.
     * @throws TypeException if the current value is not an array.
     */
    void push_back(const Json& json);
    template<typename... Args>
    void push_back(const Json& json, Args... args);

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
    using JsonValue = std::variant<std::nullptr_t, bool, double, std::string, JsonObject, JsonArray>;
    JsonValue value;
};

#endif // JSON_HANDLERC_HPP
