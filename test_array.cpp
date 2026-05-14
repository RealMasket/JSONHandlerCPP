#include <iostream>
#include "core/JsonHandler.hpp"
#include "serializer/JsonSerializer.hpp"
#include "parser/JsonParser.hpp"

int main() {
    Json::JsonArray jsonArray = { Json(1.0), Json(2.0), Json(3.5), Json("hello"), Json(true) };
    Json json = jsonArray;
    std::cout << "First element: " << json[0].asNumber() << std::endl;
    std::cout << "Second element: " << json[1].asNumber() << std::endl;
    std::cout << "Third element: " << json[2].asNumber() << std::endl;
    std::cout << "Fourth element: " << json[3].asString() << std::endl;
    std::cout << "Fifth element: " << std::boolalpha << json[4].asBool() << std::endl;
    json[1] = Json(42.5);
    json.push_back(Json("new element"));
    json.push_back(Json(false));
    std::cout << "Serialized JSON array: " << JsonSerializer::serialize(json, true, 0) << std::endl;
    std::string jsonString = "[10, 20, 30, \"world\", false]";
    Json parsedJson = JsonParser::parse(jsonString);
    std::cout << "Parsed JSON array: " << JsonSerializer::serialize(parsedJson, true, 0) << std::endl;
    std::cout << "Iterating through the JSON array:" << std::endl;
    for (size_t i = 0; i < parsedJson.asArray().size(); ++i) {
        const Json& element = parsedJson[i];
        if (element.isNumber()) {
            std::cout << "Number: " << element.asNumber() << std::endl;
        } else if (element.isString()) {
            std::cout << "String: " << element.asString() << std::endl;
        } else if (element.isBool()) {
            std::cout << "Boolean: " << element.asBool() << std::endl;
        }
    }
    return 0;
}
