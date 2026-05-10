#include "pch.h"
#include "JsonParser.hpp"
#include "JsonLexer.hpp"

/*
* @brief Constructs a JsonParser with the given vector of tokens.
* @param tokens The vector of tokens to parse.
*/
JsonParser::JsonParser(const std::vector<Token>& tokens) : tokens(tokens) { }

/*
* @brief Checks if the parser has reached the end of the token stream.
* @return True if at the end of the token stream, false otherwise.
*/
bool JsonParser::isAtEnd() const
{
    return peek().type == Parser::TokenType::EndOfFile;
}

/*
* @brief Peeks at the current token without advancing the position.
* @return The current token.
*/
const Token& JsonParser::peek() const
{
    return tokens[current];
}

/*
* @brief Retrieves the previous token (the last one that was matched).
* @return The previous token.
*/
const Token& JsonParser::previous() const
{
    return tokens[current - 1];
}

/*
* @brief Checks if the current token matches the expected type and advances if it does.
* @param type The expected token type to match.
* @return True if the current token matches the expected type, false otherwise.
*/
bool JsonParser::match(Parser::TokenType type)
{
    if (peek().type == type)
    {
        ++current;
        return true;
    }

    return false;
}

/*
* @brief Parses a JSON string and returns the corresponding Json value.
* @param source The JSON string to parse.
* @return The parsed Json value.
* @throws std::runtime_error if the input string is not valid JSON.
*/
Json JsonParser::parse(const std::string& source)
{
    JsonLexer lexer(source);

    auto tokens = lexer.tokenize();

    JsonParser parser(tokens);

    return parser.parseValue();
}

/*
* @brief Parses a JSON value based on the current token and returns the corresponding Json value.
* @return The parsed Json value.
* @throws std::runtime_error if the current token does not represent a valid JSON value.
*/
Json JsonParser::parseValue()
{
    if (match(Parser::TokenType::Null))
        return nullptr;

    if (match(Parser::TokenType::True))
        return true;

    if (match(Parser::TokenType::False))
        return false;

    if (match(Parser::TokenType::String))
        return previous().lexeme;

    if (match(Parser::TokenType::Number))
    {
        double number =
            std::stod(previous().lexeme);

        return number;
    }

    if (match(Parser::TokenType::LeftBrace))
    {
        return parseObject();
    }

    if (match(Parser::TokenType::LeftBracket))
    {
        return parseArray();
    }

    throw std::runtime_error("Invalid JSON value");
}

/*
* @brief Parses a JSON object from the token stream and returns the corresponding Json value.
* @return The parsed Json object.
* @throws std::runtime_error if the token stream does not represent a valid JSON object.
*/
Json JsonParser::parseObject()
{
    Json::JsonObject obj;

    if (match(Parser::TokenType::RightBrace))
    {
        return obj;
    }

    while (true)
    {
        if (!match(Parser::TokenType::String))
        {
            throw std::runtime_error(
                "Expected string key");
        }

        std::string key =
            previous().lexeme;

        if (!match(Parser::TokenType::Colon))
        {
            throw std::runtime_error(
                "Expected ':'");
        }

        obj[key] = parseValue();

        if (match(Parser::TokenType::RightBrace))
        {
            break;
        }

        if (!match(Parser::TokenType::Comma))
        {
            throw std::runtime_error(
                "Expected ','");
        }
    }

    return obj;
}

/*
* @brief Parses a JSON array from the token stream and returns the corresponding Json value.
* @return The parsed Json array.
* @throws std::runtime_error if the token stream does not represent a valid JSON array.
*/
Json JsonParser::parseArray()
{
    Json::JsonArray arr;

    if (match(Parser::TokenType::RightBracket))
    {
        return arr;
    }

    while (true)
    {
        arr.push_back(parseValue());

        if (match(Parser::TokenType::RightBracket))
        {
            break;
        }

        if (!match(Parser::TokenType::Comma))
        {
            throw std::runtime_error(
                "Expected ','");
        }
    }

    return arr;
}