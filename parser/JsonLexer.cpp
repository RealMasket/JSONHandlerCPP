#include "pch.h"
#include "JsonLexer.hpp"
#include "LexerException.hpp"
#include "DiagnosticFormatter.hpp"

/*
* @brief Constructs a JsonLexer with the given source string.
* @param source The JSON string to tokenize.
*/
JsonLexer::JsonLexer(const std::string& source) : source(source) { }

/*
* @brief Checks if the lexer has reached the end of the input.
* @return True if at the end of the input, false otherwise.
*/
bool JsonLexer::isAtEnd() const
{
    return position >= source.size();
}

/*
* @brief Peeks at the current character without advancing the position.
* @return The current character, or '\0' if at the end of the input.
*/
char JsonLexer::peek() const
{
    if (isAtEnd()) return '\0';
    return source[position];
}

/*
* @brief Checks if the current character matches the expected character and advances if it does.
* @param expected The character to match against the current character.
* @return True if the current character matches the expected character, false otherwise.
*/
bool JsonLexer::match(char expected)
{
    if (isAtEnd())
        return false;

    if (source[position] != expected)
        return false;

    advance();

    return true;
}

/*
* @brief Peeks at the next character without advancing the position.
* @return The next character, or '\0' if at the end of the input.
*/
char JsonLexer::peekNext() const
{
    if (position + 1 >= source.size())
        return '\0';

    return source[position + 1];
}

/*
* @brief Advances the position and returns the current character.
* @return The current character, or '\0' if at the end of the input.
*/
char JsonLexer::advance()
{
    char c = source[position++];

    ++column;

    if (c == '\n')
    {
        ++line;
        column = 1;
    }

    return c;
}

/*
* @brief Skips over any whitespace characters in the input.
*/
void JsonLexer::skipWhitespace()
{
    while (!isAtEnd() && std::isspace(peek()))
    {
        advance();
    }
}

/*
* @brief Creates a token with the given type and lexeme, including position information.
* @param type The type of the token.
* @param lexeme The string representation of the token.
* @return A Token object with the specified type, lexeme, and position information.
*/
Token JsonLexer::makeToken(
    Parser::TokenType type,
    const std::string& lexeme)
{
    return {
        type,
        lexeme,
        position,
        line,
        column
    };
}

/*
* @brief Parses a string token from the input, handling escape sequences.
* @return A Token object representing the parsed string.
* @throws LexerException if the string is unterminated or contains invalid escape sequences.
*/
Token JsonLexer::string()
{
    advance(); // opening quote

    std::string value;

    while (!isAtEnd() && peek() != '"')
    {
        char c = advance();

        if (c == '\\')
        {
            if (isAtEnd())
            {
                throw LexerException(
                    DiagnosticFormatter::format(
                        source,
                        line,
                        column,
                        "Unterminated escape sequence"
                    ),
                    line,
                    column
                );
            }

            char escaped = advance();

            switch (escaped)
            {
            case 'n': value += '\n'; break;
            case 't': value += '\t'; break;
            case '"': value += '"'; break;
            case '\\': value += '\\'; break;

            default:
                throw LexerException(
                    DiagnosticFormatter::format(
                        source,
                        line,
                        column,
                        "Invalid escape sequence: \\" + std::string(1, escaped)
                    ),
                    line,
                    column
                );
            }
        }
        else
        {
            value += c;
        }
    }

    if (isAtEnd())
    {
        throw LexerException(
            DiagnosticFormatter::format(
                source,
                line,
                column,
                "Unterminated string"
            ),
            line,
            column
        );
    }

    advance(); // closing quote

    return makeToken(Parser::TokenType::String, value);
}

/*
* @brief Parses a number token from the input, handling integers, decimals, and scientific notation.
* @return A Token object representing the parsed number.
* @throws LexerException if the number format is invalid.
*/
Token JsonLexer::number()
{
    size_t start = position;

    // optional minus
    if (peek() == '-')
    {
        advance();
    }

    // integer part
    if (peek() == '0')
    {
        advance();

        // leading zero check
        if (isdigit(peek()))
        {
            throw LexerException(
                DiagnosticFormatter::format(
                    source,
                    line,
                    column,
                    "Leading zeros are not allowed"
                ),
                line,
                column
            );
        }
    }
    else if (isdigit(peek()))
    {
        while (isdigit(peek()))
        {
            advance();
        }
    }
    else
    {
        throw LexerException(
            DiagnosticFormatter::format(
                source,
                line,
                column,
                "Invalid number"
            ),
            line,
            column
        );
    }

    // fraction
    if (peek() == '.')
    {
        advance();

        if (!isdigit(peek()))
        {
            throw LexerException(
                DiagnosticFormatter::format(
                    source,
                    line,
                    column,
                    "Expected digit after decimal point"
                ),
                line,
                column
            );
        }

        while (isdigit(peek()))
        {
            advance();
        }
    }

    // exponent
    if (peek() == 'e' || peek() == 'E')
    {
        advance();

        if (peek() == '+' || peek() == '-')
        {
            advance();
        }

        if (!isdigit(peek()))
        {
            throw LexerException(
                DiagnosticFormatter::format(
                    source,
                    line,
                    column,
                    "Invalid exponent"
                ),
                line,
                column
            );
        }

        while (isdigit(peek()))
        {
            advance();
        }
    }

    std::string value =
        source.substr(start, position - start);

    return makeToken(
        Parser::TokenType::Number,
        value
    );
}

/*
* @brief Parses a keyword token from the input, handling true, false, and null.
* @return A Token object representing the parsed keyword.
* @throws LexerException if the keyword is unknown.
*/
Token JsonLexer::keyword()
{
    size_t start = position;

    while (!isAtEnd() && std::isalpha(peek()))
    {
        advance();
    }

    std::string value =
        source.substr(start, position - start);

    if (value == "true")
        return makeToken(Parser::TokenType::True, value);

    if (value == "false")
        return makeToken(Parser::TokenType::False, value);

    if (value == "null")
        return makeToken(Parser::TokenType::Null, value);

    throw LexerException(
        DiagnosticFormatter::format(
            source,
            line,
            column,
            "Unexpected keyword: " + value
        ),
        line,
        column
    );
}

/*
* @brief Tokenizes the input JSON string into a vector of tokens.
* @return A vector of Token objects representing the tokenized input.
* @throws LexerException if an unexpected character is encountered.
*/
std::vector<Token> JsonLexer::tokenize()
{
    std::vector<Token> tokens;

    while (!isAtEnd())
    {
        skipWhitespace();

        if (isAtEnd())
            break;

        char c = peek();

        switch (c)
        {
        case '{':
            advance();
            tokens.push_back(
                makeToken(Parser::TokenType::LeftBrace, "{"));
            break;

        case '}':
            advance();
            tokens.push_back(
                makeToken(Parser::TokenType::RightBrace, "}"));
            break;

        case '[':
            advance();
            tokens.push_back(
                makeToken(Parser::TokenType::LeftBracket, "["));
            break;

        case ']':
            advance();
            tokens.push_back(
                makeToken(Parser::TokenType::RightBracket, "]"));
            break;

        case ':':
            advance();
            tokens.push_back(
                makeToken(Parser::TokenType::Colon, ":"));
            break;

        case ',':
            advance();
            tokens.push_back(
                makeToken(Parser::TokenType::Comma, ","));
            break;

        case '"':
            tokens.push_back(string());
            break;

        default:
            if (isdigit(c) || c == '-')
            {
                tokens.push_back(number());
            }
            else if (std::isalpha(c))
            {
                tokens.push_back(keyword());
            }
            else
            {
                throw LexerException(
                    DiagnosticFormatter::format(
                        source,
                        line,
                        column,
                        "Unexpected character: " + std::string(1, c)
                    ),
                    line,
                    column
                );
            }
        }
    }

    tokens.push_back(
        makeToken(Parser::TokenType::EndOfFile, ""));

    return tokens;
}