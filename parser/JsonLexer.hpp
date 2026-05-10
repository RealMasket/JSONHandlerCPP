#pragma once

#include <vector>
#include <string>

#include "Token.hpp"

class JsonLexer
{
public:
    /*
    * @brief Constructs a JsonLexer with the given source string.
    * @param source The JSON string to tokenize.
    */
    explicit JsonLexer(const std::string& source);

    /*
    * @brief Tokenizes the input JSON string into a vector of tokens.
    * @return A vector of Token objects representing the tokenized input.
    * @throws LexerException if an unexpected character is encountered.
    */
    std::vector<Token> tokenize();

private:
    const std::string& source;

    size_t position = 0;
    size_t line = 1;
    size_t column = 1;

private:
    /*
    * @brief Checks if the lexer has reached the end of the input.
    * @return True if at the end of the input, false otherwise.
    */
    bool isAtEnd() const;

    /*
    * @brief Peeks at the current character without advancing the position.
    * @return The current character, or '\0' if at the end of the input.
    */
    char peek() const;
    /*
    * @brief Advances the position and returns the current character.
    * @return The current character, or '\0' if at the end of the input.
    */
    char advance();
    
    /*
    * @brief Skips over any whitespace characters in the input.
    */
    void skipWhitespace();

    /*
    * @brief Parses a string token from the input, handling escape sequences.
    * @return A Token object representing the parsed string.
    * @throws LexerException if the string is unterminated or contains invalid escape sequences.
    */
    Token string();
    /*
    * @brief Parses a number token from the input, handling integers, decimals, and scientific notation.
    * @return A Token object representing the parsed number.
    * @throws LexerException if the number format is invalid.
    */
    Token number();
    /*
    * @brief Parses a keyword token from the input, handling true, false, and null.
    * @return A Token object representing the parsed keyword.
    */
    Token keyword();

    /*
    * @brief Creates a token with the given type and lexeme, including position information.
    * @param type The type of the token.
    * @param lexeme The string representation of the token.
    * @return A Token object with the specified type, lexeme, and position information.
    */
    Token makeToken(
        Parser::TokenType type,
        const std::string& lexeme
    );
};