#include "pch.h"
#include "JsonParser.hpp"
#include "ParseException.hpp"

TEST(JsonParserTests, ParseNumber)
{
    Json result =
        JsonParser::parse("123");

    EXPECT_TRUE(result.isNumber());

    EXPECT_DOUBLE_EQ(
        result.asNumber(),
        123
    );
}

TEST(JsonParserTests, ParseObject)
{
    Json result =
        JsonParser::parse(
            R"({
                "name": "John",
                "age": 25
            })"
        );

    EXPECT_TRUE(result.isObject());

    EXPECT_EQ(
        result["name"].asString(),
        "John"
    );

    EXPECT_DOUBLE_EQ(
        result["age"].asNumber(),
        25
    );
}

TEST(JsonParserTests, ParseArray)
{
    Json result =
        JsonParser::parse("[1,2,3]");

    EXPECT_TRUE(result.isArray());

    EXPECT_EQ(result.size(), 3);

    EXPECT_DOUBLE_EQ(
        result[0].asNumber(),
        1
    );
}

TEST(JsonParserTests, InvalidJsonThrows)
{
    EXPECT_THROW(
        JsonParser::parse("{"),
        ParseException
    );
}

TEST(JsonParserTests, DiagnosticsContainLineInfo)
{
    try
    {
        JsonParser::parse(
            R"({
                "name":
            })"
        );

        FAIL();
    }
    catch (const ParseException& ex)
    {
        std::string message =
            ex.what();

        EXPECT_NE(
            message.find("line"),
            std::string::npos
        );

        EXPECT_NE(
            message.find("^"),
            std::string::npos
        );
    }
}

TEST(JsonParserTests, RejectTrailingTokens)
{
    EXPECT_THROW(
        JsonParser::parse("{} []"),
        ParseException
    );
}

TEST(JsonParserTests, RejectTooDeepNesting)
{
    std::string json;

    for (int i = 0; i < 200; ++i)
    {
        json += "[";
    }

    for (int i = 0; i < 200; ++i)
    {
        json += "]";
    }

    EXPECT_THROW(
        JsonParser::parse(json),
        ParseException
    );
}

TEST(JsonParserTests, AcceptReasonableDepth)
{
    std::string json;

    for (int i = 0; i < 10; ++i)
    {
        json += "[";
    }

    json += "0";

    for (int i = 0; i < 10; ++i)
    {
        json += "]";
    }

    EXPECT_NO_THROW(
        JsonParser::parse(json)
    );
}