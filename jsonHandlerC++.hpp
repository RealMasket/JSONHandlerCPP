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

    template<typename T>
    T as() const;

    template<typename T>
    bool is() const;

    Json& operator[](const std::string& key);
    Json& operator[](size_t index);
    const Json& operator[](const std::string& key) const;
    const Json& operator[](size_t index) const;
    void push_back(const Json& json);
    template<typename... Args>
    void push_back(const Json& json, Args... args);

    size_t size() const;
    void remove(size_t index);
    void remove(const std::string& key);
    bool contains(const std::string& key) const;
    bool contains(const Json& json) const;
    std::vector<std::string> keys() const;

    std::vector<std::string> findValue(const Json& value) const;

    bool operator==(const Json& other) const;

    static Json parse(const std::string& str, bool allowComments = false);
    std::string serialize(bool pretty = true, int indentLevel = 0) const;

    void mergeObject(const JsonObject& other, bool overwrite);
    void mergeArray(const JsonArray& other);
    void saveToFile(const std::string& filename, bool pretty = true) const;
    void loadFromFile(const std::string& filename, bool commentFlag = false);
	void toFile(const std::string& filename, bool pretty = true) const;
	void fromFile(const std::string& filename, bool commentFlag = false);
    void sortByPath(const std::string& keyPath, bool ascending);

    Json& atPath(const std::string& path);
    const Json& getByPath(const std::string& path) const;

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
