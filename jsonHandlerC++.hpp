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

    bool isNull() const;
    bool isBool() const;
    bool isNumber() const;
    bool isString() const;
    bool isObject() const;
    bool isArray() const;
    bool isBigNumber() const;

    bool asBool() const;
    double asNumber() const;
    const std::string& asString() const;
    const JsonObject& asObject() const;
    const JsonArray& asArray() const;

    Json& operator[](const std::string& key);
    Json& operator[](size_t index);
    const Json& operator[](const std::string& key) const;
    const Json& operator[](size_t index) const;
    void push_back(const Json& json);

    std::string serialize() const;
    static Json parse(const std::string& str);

private:
    using JsonValue = std::variant<std::nullptr_t, bool, double, std::string, JsonObject, JsonArray>;
    JsonValue value;

    static std::string escapeString(const std::string& input);
    static std::string serializeObject(const JsonObject& obj);
    static std::string serializeArray(const JsonArray& arr);
};

#endif // JSON_HANDLERC_HPP
