#pragma once
#include <string>

namespace Parser
{
	/*
    * @brief Enumeration of token types for JSON parsing.
	*/
    enum class TokenType
    {
        LeftBrace,
        RightBrace,

        LeftBracket,
        RightBracket,

        Colon,
        Comma,

        String,
        Number,

        True,
        False,
        Null,

        EndOfFile
    };
}

/*
* @brief Represents a token in the JSON input, including its type, lexeme, and position information.
*/
struct Token
{
    Parser::TokenType type;
    std::string lexeme;

    size_t position;
    size_t line;
    size_t column;
};