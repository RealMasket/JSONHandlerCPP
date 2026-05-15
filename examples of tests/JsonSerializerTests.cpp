#include "pch.h"
#include "JsonSerializer.hpp"

TEST(JsonSerializerTests, EscapeSpecialCharacters)
{
    Json json("line1\rline2");

    std::string result =
        JsonSerializer::serialize(json);

    EXPECT_EQ(
        result,
        "\"line1\\rline2\""
    );
}