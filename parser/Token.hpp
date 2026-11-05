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
* @brief Token metadata. Strings store the raw content range without quotes.
* All offsets refer to the original source; positions and columns count bytes.
*/
struct Token
{
    Parser::TokenType type;
    // Position information and optional raw range in the original source
    size_t position;
    size_t line;
    size_t column;

    // Range [start, start+length) in the original source for this token
    size_t start = 0;
    size_t length = 0;
};
