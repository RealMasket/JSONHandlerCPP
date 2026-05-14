#pragma once
#include "JsonHandler.hpp"
#include "Api.hpp"
#include "Token.hpp"

class JSON_API JsonParser
{
public:
    /*
    * @brief Constructs a JsonParser with the given vector of tokens.
    * @param tokens The vector of tokens to parse.
    * @param source The original JSON source string (used for error reporting).
    */
    explicit JsonParser(
        const std::vector<Token>& tokens,
        const std::string& source
    );

    /*
    * @brief Parses a JSON string and returns the corresponding Json value.
    * @param source The JSON string to parse.
    * @return The parsed Json value.
    * @throws ParseException if the input string is not valid JSON.
    */
    static Json parse(const std::string& source);

private:
    const std::vector<Token>& tokens;
    const std::string& source;

    size_t current = 0;
    size_t currentDepth = 0;

    static constexpr size_t MAX_DEPTH = 128;

private:
    /*
    * @brief Parses a JSON value based on the current token and returns the corresponding Json value.
    * @return The parsed Json value.
    * @throws ParseException if the current token does not represent a valid JSON value.
    */
    Json parseValue();

    /*
    * @brief Parses a JSON object from the token stream and returns the corresponding Json value.
    * @return The parsed Json object.
    * @throws ParseException if the token stream does not represent a valid JSON object.
    */
    Json parseObject();
    /*
    * @brief Parses a JSON array from the token stream and returns the corresponding Json value.
    * @return The parsed Json array.
    * @throws ParseException if the token stream does not represent a valid JSON array.
    */
    Json parseArray();

    /*
    * @brief Checks if the current token matches the expected type and advances if it does.
    * @param type The expected token type to match.
    * @return True if the current token matches the expected type, false otherwise.
    */
    bool match(Parser::TokenType type);

    /*
    * @brief Peeks at the current token without advancing the position.
    * @return The current token.
    */
    const Token& peek() const;
    /*
    * @brief Retrieves the previous token (the last one that was matched).
    * @return The previous token.
    */
    const Token& previous() const;

    /*
    * @brief Enters a new depth level in the JSON structure and checks for maximum nesting depth.
    * @throws ParseException if the maximum nesting depth is exceeded.
    */
    void enterDepth();
    /*
    * @brief Leaves the current depth level in the JSON structure, ensuring it does not go below zero.
    */
    void leaveDepth();

    /*
    * @brief Checks if the parser has reached the end of the token stream.
    * @return True if at the end of the token stream, false otherwise.
    */
    bool isAtEnd() const;
};

