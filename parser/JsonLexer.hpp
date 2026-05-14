#pragma once

#include <vector>
#include <string>

#include "Token.hpp"
#include "Api.hpp"

class JSON_API JsonLexer
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
    std::string source;

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
    * @brief Checks if the current character matches the expected character and advances if it does.
    * @param expected The character to match against the current character.
    * @return True if the current character matches the expected character, false otherwise.
    */
    bool match(char expected);
    /*
    * @brief Peeks at the next character without advancing the position.
    * @return The next character, or '\0' if at the end of the input.
    */
    char peekNext() const;
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

    /*
    * @brief Parses a unicode escape sequence in a JSON string and returns the corresponding UTF-8 encoded string.
    * @return A UTF-8 encoded string representing the parsed unicode character.
    * @throws LexerException if the unicode escape sequence is invalid.
    */
    std::string parseUnicodeEscape();
};