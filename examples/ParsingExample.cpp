#include "JsonParser.hpp"
#include <iostream>

int main()
{
    std::string jsonText =
        R"({
            "name": "Alice",
            "age": 25
        })";

    Json json = JsonParser::parse(jsonText);

    std::cout << json["name"].asString();

    return 0;
}