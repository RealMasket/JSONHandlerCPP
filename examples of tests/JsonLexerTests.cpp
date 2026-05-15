#include "pch.h"
#include "JsonLexer.hpp"
#include "LexerException.hpp"

TEST(JsonLexerTests, StringToken)
{
    JsonLexer lexer("\"hello\"");

    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(
        tokens[0].type,
        Parser::TokenType::String
    );

    EXPECT_EQ(
        tokens[0].lexeme,
        "hello"
    );

    EXPECT_EQ(
        tokens[1].type,
        Parser::TokenType::EndOfFile
    );
}

TEST(JsonLexerTests, NumberToken)
{
    JsonLexer lexer("123.45");

    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(
        tokens[0].type,
        Parser::TokenType::Number
    );

    EXPECT_EQ(
        tokens[0].lexeme,
        "123.45"
    );
}

TEST(JsonLexerTests, TrueKeyword)
{
    JsonLexer lexer("true");

    auto tokens = lexer.tokenize();

    EXPECT_EQ(
        tokens[0].type,
        Parser::TokenType::True
    );
}

TEST(JsonLexerTests, InvalidKeywordThrows)
{
    JsonLexer lexer("tru");

    EXPECT_THROW(
        lexer.tokenize(),
        LexerException
    );
}

TEST(JsonLexerTests, InvalidCharacterThrows)
{
    JsonLexer lexer("@");

    EXPECT_THROW(
        lexer.tokenize(),
        LexerException
    );
}

TEST(JsonLexerTests, ExceptionContainsMessage)
{
    JsonLexer lexer("@");

    try
    {
        lexer.tokenize();

        FAIL();
    }
    catch (const LexerException& ex)
    {
        std::string message = ex.what();

        EXPECT_NE(
            message.find("Unexpected character"),
            std::string::npos
        );
    }
}

TEST(JsonLexerTests, EscapedString)
{
    JsonLexer lexer("\"hello\\nworld\"");

    auto tokens = lexer.tokenize();

    EXPECT_EQ(
        tokens[0].lexeme,
        "hello\nworld"
    );
}

TEST(JsonLexerTests, RejectLeadingZero)
{
    JsonLexer lexer("01");

    EXPECT_THROW(
        lexer.tokenize(),
        LexerException
    );
}

TEST(JsonLexerTests, RejectInvalidExponent)
{
    JsonLexer lexer("1e");

    EXPECT_THROW(
        lexer.tokenize(),
        LexerException
    );
}

TEST(JsonLexerTests, RejectDoubleDecimal)
{
    JsonLexer lexer("1.2.3");

    EXPECT_THROW(
        lexer.tokenize(),
        LexerException
    );
}

TEST(JsonLexerTests, EscapedCarriageReturn)
{
    JsonLexer lexer("\"hello\\rworld\"");

    auto tokens = lexer.tokenize();

    EXPECT_EQ(
        tokens[0].lexeme,
        "hello\rworld"
    );
}

TEST(JsonLexerTests, UnicodeEscape)
{
    JsonLexer lexer("\"\\u0041\"");

    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].lexeme, "A");
}

TEST(JsonLexerTests, UnicodeCyrillic)
{
    JsonLexer lexer("\"\\u041F\"");

    auto tokens = lexer.tokenize();

    EXPECT_FALSE(tokens[0].lexeme.empty());
}

TEST(JsonLexerTests, InvalidUnicodeEscape)
{
    JsonLexer lexer("\"\\uZZZZ\"");

    EXPECT_THROW(
        lexer.tokenize(),
        LexerException
    );
}