#include "JsonHandler.hpp"
#include "JsonSerializer.hpp"
#include "JsonFile.hpp"
#include <iostream>

int main()
{
    Json obj(Json::JsonObject{});

    obj["name"] = "Adam";
    obj["age"] = 21.0;
    obj["student"] = true;

    std::cout << JsonSerializer::serialize(obj, false, 0);

	obj["age"] = 22.0;
    obj["marks"] = Json::JsonObject{
        {"MathMark", 95.0},
        {"EnglishMark", 88.0}
	};

	std::cout << "\n\n" << obj.serialize(true, 0);

	obj["marks"]["ScienceMark"] = 92.0;
	obj.save("student.json");
	obj.clear();

	obj.load("student.json");
	std::cout << "\n\n" << obj.serialize(true, 0);

	obj["marks"].remove("EnglishMark");
	
	Json obj2(Json::JsonObject{{"name", "Eve"}});
	obj.mergeObject(obj2, false);
	std::cout << "\n\n" << obj.serialize(true, 0);

    return 0;
}