#include <gtest/gtest.h>
#include "JsonLexer.hpp"
#include "JsonParser.hpp"
#include "LexerException.hpp"

TEST(JsonLexerTests, RawStringRange)
{
    const std::string source = "\"hello\\nworld\"";
    auto tokens = JsonLexer(source).tokenize();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, Parser::TokenType::String);
    EXPECT_EQ(source.substr(tokens[0].start, tokens[0].length), "hello\\nworld");
    EXPECT_EQ(tokens[0].position, 0u);
    EXPECT_EQ(tokens[0].line, 1u);
    EXPECT_EQ(tokens[0].column, 1u);
    EXPECT_EQ(tokens[1].type, Parser::TokenType::EndOfFile);
}

TEST(JsonLexerTests, NumberRangeAndStartLocation)
{
    const std::string source = "\n  123.45";
    const auto tokens = JsonLexer(source).tokenize();
    EXPECT_EQ(tokens[0].type, Parser::TokenType::Number);
    EXPECT_EQ(source.substr(tokens[0].start, tokens[0].length), "123.45");
    EXPECT_EQ(tokens[0].position, 3u);
    EXPECT_EQ(tokens[0].line, 2u);
    EXPECT_EQ(tokens[0].column, 3u);
    EXPECT_EQ(tokens.back().column, 9u);
}

TEST(JsonLexerTests, PunctuationAndKeywordLocations)
{
    const auto tokens = JsonLexer("[true,\n false]").tokenize();
    EXPECT_EQ(tokens[0].column, 1u);
    EXPECT_EQ(tokens[1].type, Parser::TokenType::True);
    EXPECT_EQ(tokens[1].column, 2u);
    EXPECT_EQ(tokens[2].column, 6u);
    EXPECT_EQ(tokens[3].type, Parser::TokenType::False);
    EXPECT_EQ(tokens[3].line, 2u);
    EXPECT_EQ(tokens[3].column, 2u);
}

class InvalidLexemeTests : public testing::TestWithParam<const char*> {};
TEST_P(InvalidLexemeTests, RejectsInvalidInput)
{
    EXPECT_THROW(JsonLexer(GetParam()).tokenize(), LexerException);
}
INSTANTIATE_TEST_SUITE_P(StrictJson, InvalidLexemeTests, testing::Values(
    "tru", "@", "01", "1e", "1.2.3", "-", "1.", "+1", "1e+",
    "\"\\uZZZZ\"", "\"\\u123\"", "\"\\q\"", "\"unterminated",
    "\"a\nb\"", "\"a\tb\"", "\vnull", "\fnull"
));

TEST(JsonLexerTests, ExceptionContainsLocationAndContext)
{
    try {
        JsonLexer("\n@").tokenize();
        FAIL();
    } catch (const LexerException& ex) {
        EXPECT_EQ(ex.getLine(), 2u);
        EXPECT_EQ(ex.getColumn(), 1u);
        EXPECT_NE(std::string(ex.what()).find("Unexpected character"), std::string::npos);
        EXPECT_NE(std::string(ex.what()).find("^"), std::string::npos);
    }
}
