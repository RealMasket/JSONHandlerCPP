#include "pch.h"
#include "JsonParser.hpp"
#include "JsonSerializer.hpp"

TEST(JsonRoundTripTests, SimpleObject)
{
    Json::JsonObject obj;

    obj["name"] = "Egor";
    obj["age"] = 18;
    obj["active"] = true;

    Json original(obj);

    std::string serialized =
        JsonSerializer::serialize(original);

    Json parsed =
        JsonParser::parse(serialized);

    EXPECT_EQ(original, parsed);
}

TEST(JsonRoundTripTests, NestedArray)
{
    Json::JsonArray arr;

    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);

    Json::JsonObject obj;
    obj["numbers"] = arr;

    Json original(obj);

    std::string serialized =
        JsonSerializer::serialize(original);

    Json parsed =
        JsonParser::parse(serialized);

    EXPECT_EQ(original, parsed);
}

TEST(JsonRoundTripTests, EscapedStrings)
{
    Json original(
        "Hello\nWorld\tJSON\rTest"
    );

    std::string serialized =
        JsonSerializer::serialize(original);

    Json parsed =
        JsonParser::parse(serialized);

    EXPECT_EQ(original, parsed);
}

TEST(JsonRoundTripTests, DeeplyNestedObjects)
{
    Json::JsonObject level3;
    level3["value"] = 42;

    Json::JsonObject level2;
    level2["nested"] = level3;

    Json::JsonObject level1;
    level1["nested"] = level2;

    Json original(level1);

    std::string serialized =
        JsonSerializer::serialize(original);

    Json parsed =
        JsonParser::parse(serialized);

    EXPECT_EQ(original, parsed);
}

TEST(JsonRoundTripTests, PrettySerialization)
{
    Json::JsonObject obj;
    obj["hello"] = "world";

    Json original(obj);

    std::string serialized =
        JsonSerializer::serialize(
            original,
            true
        );

    Json parsed =
        JsonParser::parse(serialized);

    EXPECT_EQ(original, parsed);
}