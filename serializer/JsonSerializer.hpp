#pragma once
#include "../core/JsonHandler.hpp"

class JSON_API JsonSerializer
{
public:
    static std::string serialize(const Json& json, bool pretty = true, int indentLevel = 0);

private:
    static std::string escapeString(const std::string& input);
    static std::string serializeObject(const Json::JsonObject& obj, bool pretty, int indentLevel);
    static std::string serializeArray(const Json::JsonArray& arr, bool pretty, int indentLevel);
};