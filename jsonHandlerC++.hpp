#ifndef JSON_HANDLERC_HPP
#define JSON_HANDLERC_HPP

#ifdef JSONHANDLERC_EXPORTS
#define JSON_API __declspec(dllexport)
#else
#define JSON_API __declspec(dllimport)
#endif

#include <string>
#include <map>
#include <vector>
#include <variant>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <cstdint>
#include <iostream>

class JSON_API Json {
public:
    using JsonObject = std::map<std::string, Json>;
    using JsonArray = std::vector<Json>;

    using JsonValue = std::variant<
        std::nullptr_t,
        bool,
        int64_t,
        uint64_t,
        double,
        std::string,
        JsonObject,
        JsonArray
    >;

    Json();
    Json(std::nullptr_t);
    Json(bool b);
    Json(int i);
    Json(int64_t i);
    Json(uint64_t u);
    Json(double d);
    Json(const std::string& s);
    Json(const char* s);
    Json(const JsonObject& obj);
    Json(const JsonArray& arr);

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isInt64() const;
    bool isUInt64() const;
    bool isDouble() const;
    bool isString() const;
    bool isObject() const;
    bool isArray() const;

    bool asBool() const;
    int64_t asInt64() const;
    uint64_t asUInt64() const;
    double asDouble() const;
    const std::string& asString() const;
    const JsonObject& asObject() const;
    const JsonArray& asArray() const;

    /**
     * @brief Retrieves the JSON value as a specific type.
     * @tparam T The type to convert to.
     * @return The value as the specified type.
     * @throws std::runtime_error if the value is not of the specified type.
     */
    template<typename T>
    T as() const;

    /**
     * @brief Checks if the JSON value is of a specific type.
     * @tparam T The type to check against.
     * @return True if the value is of the specified type, false otherwise.
     */
    template<typename T>
    bool is() const;

    Json& operator[](const std::string& key);
    Json& operator[](size_t index);
    const Json& operator[](const std::string& key) const;
    const Json& operator[](size_t index) const;

    /**
     * @brief Adds a new element to a JSON array.
     * @param json The JSON value to add to the array.
     */
    void push_back(const Json& json);

    /**
     * @brief Adds multiple elements to a JSON array.
     * @param json The first JSON value to add.
     * @param args Additional JSON values to add.
     */
    template<typename... Args>
    void push_back(const Json& json, Args... args);

    /**
     * @brief Retrieves the size of a JSON array.
     * @return The number of elements in the array.
     * @throws std::runtime_error if the current value is not an array.
     */
    size_t size() const;

    /**
     * @brief Removes an element from a JSON array by index.
     * @param index The index of the element to remove.
     * @throws std::runtime_error if the current value is not an array or the index is out of range.
     */
    void remove(size_t index);

    /**
     * @brief Removes an element from a JSON object by key.
     * @param key The key of the element to remove.
     * @throws std::runtime_error if the current value is not an object.
     */
    void remove(const std::string& key);

    /**
     * @brief Checks if a JSON object contains a specific value.
     * @param string The key to check for.
     * @return True if the key exists in the object, false otherwise.
     * @throws std::runtime_error if the current value is not an object.
     */
    bool contains(const std::string& key) const;

    /**
     * @brief Checks if a JSON array contains a specific value.
     * @param json The JSON value to check for.
     * @return True if the value exists in the array, false otherwise.
     * @throws std::runtime_error if the current value is not an array.
     */
    bool contains(const Json& json) const;

	/**
	 * @brief Checks if the JSON value is empty.
	 * @return True if the JSON value is empty (null, empty object, or empty array), false otherwise.
	 */
	bool isEmpty() const;

	/**
	 * @brief Retrieves a value from a JSON object or array by key or index.
	 * @param key The key to look for in an object, or the index in an array.
	 * @param defaultValue The value to return if the key/index is not found.
	 * @return The JSON value associated with the key/index, or defaultValue if not found.
	 * @throws std::runtime_error if the current value is not an object or array.
	 */
	Json get(const std::string& key, const Json& defaultValue = Json()) const;
	Json get(size_t index, const Json& defaultValue = Json()) const;

    /**
     * @brief Returns a list of all keys in the object.
     * @return vector<string> of keys.
     * @throws std::runtime_error if the current value is not an object.
     */
    std::vector<std::string> keys() const;

    bool operator==(const Json& other) const;

    /**
     * @brief Finds all occurrences of a specific JSON value in the current JSON object or array.
     * @param val The JSON value to search for.
     * @return A vector of strings representing the paths to the found values.
     */
    std::vector<std::string> findValue(const Json& value) const;

    /**
     * @brief Parses a JSON string into a Json object.
     * @param str The JSON string to parse.
     * @return Parsed Json object.
     * @throws std::runtime_error if the string is not valid JSON.
     */
    static Json parse(const std::string& str, bool allowComments = false);

    /**
     * @brief Serializes the JSON value to a string.
     * @param pretty If true, the output will be indented for readability.
     * @param indentLevel The starting indentation level for pretty-printing.
     * @return The serialized JSON string.
     */
    std::string serialize(bool pretty = true, int indentLevel = 0) const;

    /**
     * @brief Merges another JSON object into the current object.
     * (Existing keys are overwritten based on the overwrite parameter.)
     * @param other The JSON object to merge.
     * @param overwrite If true, existing keys will be overwritten.
     * @throws std::runtime_error if the current value is not an object.
     */
    void mergeObject(const JsonObject& other, bool overwrite);

    /**
     * @brief Merges another JSON array into the current array.
     * @param other The JSON array to merge.
     * @throws std::runtime_error if the current value is not an array.
     */
    void mergeArray(const JsonArray& other);

    /**
     * @brief Saves the JSON value to a file.
     * @param filename The name of the file to save to.
     * @param pretty If true, the output will be indented for readability.
     * @throws std::runtime_error if the file cannot be opened.
     */
    void saveToFile(const std::string& filename, bool pretty = true) const;

    /**
     * @brief Loads a JSON value from a file.
     * @param filename The name of the file to load from.
     * @param commentsFlag If true, comments will be removed from the JSON string.
     * @throws std::runtime_error if the file cannot be opened or the content is not valid JSON.
     */
    void loadFromFile(const std::string& filename, bool commentFlag = false);

    /**
     * @brief Saves the JSON value to a file (alias for saveToFile).
     * @param filename The name of the file to save to.
     * @param pretty If true, the output will be indented for readability.
     */
	void toFile(const std::string& filename, bool pretty = true) const;

    /**
     * @brief Loads a JSON value from a file (alias for loadFromFile).
     * @param filename The name of the file to load from.
     * @param commentsFlag If true, comments will be removed from the JSON string.
     */
	void fromFile(const std::string& filename, bool commentFlag = false);

    /**
     * @brief Retrieves a value from a JSON object or array using a dot-separated path.
     * @param path The dot-separated path to the value (e.g., "Marks.MathMark").
     * @return A reference to the JSON value at the specified path.
     * @throws std::runtime_error if the path is invalid or the value is not found.
     */
    Json& atPath(const std::string& path);

    /**
     * @brief Retrieves a value from a JSON object or array using a dot-separated path (const version).
     * @param path The dot-separated path to the value (e.g., "Marks.MathMark").
     * @return A reference to the JSON value at the specified path.
     * @throws std::runtime_error if the path is invalid or the value is not found.
     */
    const Json& getByPath(const std::string& path) const;

    /**
     * @brief Sorts a JSON array of objects by a nested key path (e.g., "Marks.MathMark").
     * @param keyPath The key path to sort by (e.g., "Marks.MathMark").
     * @param ascending If true, sorts in ascending order; otherwise, descending.
     * @throws std::runtime_error if the JSON value is not an array of objects or the key path is invalid.
     */
    void sortByPath(const std::string& keyPath, bool ascending);

    JsonArray::iterator begin();
    JsonArray::iterator end();
    JsonArray::const_iterator begin() const;
    JsonArray::const_iterator end() const;
    JsonArray::const_iterator cbegin() const;
    JsonArray::const_iterator cend() const;
private:
    JsonValue value;

    static std::string escapeString(const std::string& input);
    std::string serializeObject(const JsonObject& obj, bool pretty, int indentLevel) const;
    std::string serializeArray(const JsonArray& arr, bool pretty, int indentLevel) const;
    static std::string removeJsonComments(const std::string& input);

    static std::vector<std::string> splitPath(const std::string& path);
};

JSON_API std::ostream& operator<<(std::ostream& os, const Json& json);

#endif // JSON_HANDLERC_HPP
