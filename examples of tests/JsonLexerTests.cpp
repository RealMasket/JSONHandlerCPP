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

TEST(JsonLexerTests, LongStringDoesNotReserveTokensForEveryInputByte)
{
    const std::string source = "\"" + std::string(2 * 1024 * 1024, 'x') + "\"";
    const auto tokens = JsonLexer(source).tokenize();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens.front().type, Parser::TokenType::String);
    EXPECT_EQ(tokens.front().start, 1u);
    EXPECT_EQ(tokens.front().length, source.size() - 2);
    EXPECT_EQ(tokens.back().position, source.size());
    // Guard against input-sized over-reservation, without fixing a growth factor.
    EXPECT_LT(tokens.capacity() * sizeof(Token), source.size());
}

TEST(JsonLexerTests, WhitespaceOnlyInputHasOneCorrectlyLocatedEof)
{
    const std::string source = std::string(2 * 1024 * 1024, ' ') + "\n\t";
    const auto tokens = JsonLexer(source).tokenize();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens.front().type, Parser::TokenType::EndOfFile);
    EXPECT_EQ(tokens.front().position, source.size());
    EXPECT_EQ(tokens.front().start, source.size());
    EXPECT_EQ(tokens.front().length, 0u);
    EXPECT_EQ(tokens.front().line, 2u);
    EXPECT_EQ(tokens.front().column, 2u);
    EXPECT_LT(tokens.capacity() * sizeof(Token), source.size());
}

TEST(JsonLexerTests, EmptyInputHasEof)
{
    const auto tokens = JsonLexer("").tokenize();
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens.front().type, Parser::TokenType::EndOfFile);
    EXPECT_EQ(tokens.front().position, 0u);
    EXPECT_EQ(tokens.front().line, 1u);
    EXPECT_EQ(tokens.front().column, 1u);
}

TEST(JsonLexerTests, DensityChangesPreserveEveryTokenRange)
{
    const size_t count = 512;
    const std::string longString = "\"" + std::string(2 * 1024 * 1024, 's') + "\\n\"";
    for (bool stringFirst : {false, true})
    {
        SCOPED_TRACE(stringFirst);
        std::string source = "[";
        if (stringFirst) source += longString + ",";
        for (size_t i = 0; i < count; ++i)
            source += i + 1 == count && stringFirst ? "0" : "0,";
        if (!stringFirst) source += longString;
        source += "]";
        const auto tokens = JsonLexer(source).tokenize();
        ASSERT_EQ(tokens.size(), 2 * count + 4);
        EXPECT_LT(tokens.capacity() * sizeof(Token), source.size());
        EXPECT_EQ(tokens.front().type, Parser::TokenType::LeftBracket);
        EXPECT_EQ(tokens[tokens.size() - 2].type, Parser::TokenType::RightBracket);
        EXPECT_EQ(tokens.back().type, Parser::TokenType::EndOfFile);
        size_t numbers = 0, strings = 0;
        for (const auto& token : tokens)
        {
            EXPECT_EQ(token.line, 1u);
            EXPECT_EQ(token.column, token.position + 1);
            if (token.type == Parser::TokenType::Number)
            {
                ++numbers;
                EXPECT_EQ(source.substr(token.start, token.length), "0");
            }
            if (token.type == Parser::TokenType::String)
            {
                ++strings;
                EXPECT_EQ(source.substr(token.start, token.length), longString.substr(1, longString.size() - 2));
            }
        }
        EXPECT_EQ(numbers, count);
        EXPECT_EQ(strings, 1u);
        EXPECT_EQ(tokens.back().position, source.size());
    }
}

TEST(JsonLexerTests, ErrorLocationSurvivesTokenBufferGrowth)
{
    std::string source = "[";
    for (size_t i = 0; i < 2048; ++i) source += "0,";
    source += "\n false,@]";
    try {
        JsonLexer(source).tokenize();
        FAIL();
    } catch (const LexerException& ex) {
        EXPECT_EQ(ex.getLine(), 2u);
        EXPECT_EQ(ex.getColumn(), 8u);
        EXPECT_NE(std::string(ex.what()).find("Unexpected character: @"), std::string::npos);
    }
}
