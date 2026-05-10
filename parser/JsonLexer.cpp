#include "pch.h"
#include "JsonLexer.hpp"

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
* @throws std::runtime_error if the string is unterminated or contains invalid escape sequences.
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
                throw std::runtime_error("Unterminated escape sequence");
            }

            char escaped = advance();

            switch (escaped)
            {
            case 'n': value += '\n'; break;
            case 't': value += '\t'; break;
            case '"': value += '"'; break;
            case '\\': value += '\\'; break;

            default:
                throw std::runtime_error("Invalid escape sequence");
            }
        }
        else
        {
            value += c;
        }
    }

    if (isAtEnd())
    {
        throw std::runtime_error("Unterminated string");
    }

    advance(); // closing quote

    return makeToken(Parser::TokenType::String, value);
}

/*
* @brief Parses a number token from the input, handling integers, decimals, and scientific notation.
* @return A Token object representing the parsed number.
* @throws std::runtime_error if the number format is invalid.
*/
Token JsonLexer::number()
{
    size_t start = position;

    while (!isAtEnd())
    {
        char c = peek();

        if (isdigit(c)
            || c == '.'
            || c == '-'
            || c == '+'
            || c == 'e'
            || c == 'E')
        {
            advance();
        }
        else
        {
            break;
        }
    }

    std::string value =
        source.substr(start, position - start);

    return makeToken(Parser::TokenType::Number, value);
}

/*
* @brief Parses a keyword token from the input, handling true, false, and null.
* @return A Token object representing the parsed keyword.
* @throws std::runtime_error if the keyword is unknown.
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

    throw std::runtime_error("Unknown keyword: " + value);
}

/*
* @brief Tokenizes the input JSON string into a vector of tokens.
* @return A vector of Token objects representing the tokenized input.
* @throws std::runtime_error if an unexpected character is encountered.
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
                throw std::runtime_error(
                    "Unexpected character");
            }
        }
    }

    tokens.push_back(
        makeToken(Parser::TokenType::EndOfFile, ""));

    return tokens;
}