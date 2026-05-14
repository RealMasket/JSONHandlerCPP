#include <iostream>
#include "core/JsonHandler.hpp"
#include "serializer/JsonSerializer.hpp"

int main() {
    Json json;
    json["name"] = "Adam";
    json["age"] = 21.0;
    std::cout << JsonSerializer::serialize(json) << std::endl;
    return 0;
}
