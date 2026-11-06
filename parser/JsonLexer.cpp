#include "pch.h"
#include "JsonLexer.hpp"
#include "LexerException.hpp"
#include "DiagnosticFormatter.hpp"
#include "Utf8.hpp"
#include "TokenReservation.hpp"
#include <string_view>

namespace {
    bool isDigit(char c) { return c >= '0' && c <= '9'; }
    bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    bool isHex(char c) { return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
}

/*
* @brief Constructs a JsonLexer with the given source string.
* @param source The JSON string to tokenize.
*/
JsonLexer::JsonLexer(std::string source) : source(std::move(source)) { }

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
    while (!isAtEnd() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r'))
    {
        advance();
    }
}

/*
* @brief Creates a token with its source range and starting position.
* @param type The type of the token.
* @param start Start of the raw token content in the source.
* @param length Length of the raw token content.
* @return A Token object with the specified type, source range, and starting position.
*/
Token JsonLexer::makeToken(
    Parser::TokenType type,
    size_t start,
    size_t length)
{
    Token t = {
        type,
        tokenPosition,
        tokenLine,
        tokenColumn,
        start,
        length
    };

    return t;
}

/*
* @brief Parses a string token from the input, handling escape sequences.
* @return A Token object representing the parsed string.
* @throws LexerException if the string is unterminated or contains invalid escape sequences.
*/
Token JsonLexer::string()
{
    advance(); // opening quote
    const size_t contentStart = position;

    while (!isAtEnd() && peek() != '"')
    {
        const size_t characterLine = line;
        const size_t characterColumn = column;
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
            case 'n': break;
            case 't': break;
            case '"': break;
            case '\\': break;
            case 'r': break;
            case 'b': break;
            case 'f': break;
            case '/': break;
            case 'u':
            {
                parseUnicodeEscape();
                break;
            }

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
            if (static_cast<unsigned char>(c) < 0x20)
            {
                throw LexerException(
                    DiagnosticFormatter::format(source, characterLine, characterColumn,
                        "Unescaped control character in string"), characterLine, characterColumn);
            }
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

    size_t contentLen = (position - 1) - contentStart; // exclude closing quote
    if (!JsonDetail::isValidUtf8(std::string_view(source).substr(contentStart, contentLen)))
        throw LexerException("Invalid UTF-8 in string", tokenLine, tokenColumn);
    return makeToken(Parser::TokenType::String, contentStart, contentLen);
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
        if (isDigit(peek()))
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
    else if (isDigit(peek()))
    {
        while (isDigit(peek()))
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

        if (!isDigit(peek()))
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

        while (isDigit(peek()))
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

        if (!isDigit(peek()))
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

        while (isDigit(peek()))
        {
            advance();
        }
    }

    // For numbers we avoid allocating the lexeme by storing range
    return makeToken(Parser::TokenType::Number, start, position - start);
}

/*
* @brief Parses a keyword token from the input, handling true, false, and null.
* @return A Token object representing the parsed keyword.
* @throws LexerException if the keyword is unknown.
*/
Token JsonLexer::keyword()
{
    size_t start = position;

    while (!isAtEnd() && isAlpha(peek()))
    {
        advance();
    }

    size_t len = position - start;
    std::string_view sv(source.c_str() + start, len);

    if (sv == "true") return makeToken(Parser::TokenType::True, start, len);
    if (sv == "false") return makeToken(Parser::TokenType::False, start, len);
    if (sv == "null") return makeToken(Parser::TokenType::Null, start, len);

    throw LexerException(
        DiagnosticFormatter::format(
            source,
            line,
            column,
            std::string("Unexpected keyword: ") + std::string(sv)
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
    return tokenizeImpl<false>();
}

std::vector<Token> JsonLexer::tokenize(const ParseOptions& options)
{
    switch (options.preset)
    {
    case ParsePreset::Balanced: return tokenizeImpl<false>();
    case ParsePreset::LowMemory: return tokenizeImpl<true>();
    default: throw std::invalid_argument("Unknown JSON parse preset");
    }
}

template<bool lowMemory>
std::vector<Token> JsonLexer::tokenizeImpl()
{
    std::vector<Token> tokens;
    // Input bytes alone are a poor token-count estimate for long strings or
    // whitespace. Start small and learn the density from consumed input.
    tokens.reserve((std::min)(source.size() / 4, size_t{256}));

    auto append = [&](Token token) {
        if (tokens.size() == tokens.capacity() && tokens.size() < tokens.max_size())
        {
            const size_t capacity = tokens.capacity();
            const size_t limit = tokens.max_size();
            const size_t increment = (std::max)(capacity / 2, size_t{1});
            const size_t lower = capacity + (std::min)(increment, limit - capacity);
            // Bound extrapolation when a dense prefix precedes a long string.
            // A generous upper step avoids repeatedly copying large token arrays.
            const size_t upper = capacity == 0 ? size_t{1}
                : capacity > limit / 64 ? limit : capacity * 64;
            // Include the pending token and EOF; this is only an allocation hint.
            const double estimate = static_cast<double>(source.size())
                * static_cast<double>(tokens.size() + 1)
                / static_cast<double>((std::max)(position, size_t{1})) + 1;
            const size_t desired = estimate >= static_cast<double>(upper)
                ? upper : (std::min)(upper, (std::max)(lower, static_cast<size_t>(estimate)));
            if constexpr (lowMemory)
                tokens.reserve(MasketJSONDetail::lowMemoryCapacity(capacity, limit, estimate));
            else
                tokens.reserve(desired);
        }
        tokens.push_back(token);
    };

    while (!isAtEnd())
    {
        skipWhitespace();

        if (isAtEnd())
            break;

        tokenPosition = position;
        tokenLine = line;
        tokenColumn = column;
        char c = peek();

        switch (c)
        {
        case '{':
            advance();
            append(
                makeToken(Parser::TokenType::LeftBrace, position - 1, 1));
            break;

        case '}':
            advance();
            append(
                makeToken(Parser::TokenType::RightBrace, position - 1, 1));
            break;

        case '[':
            advance();
            append(
                makeToken(Parser::TokenType::LeftBracket, position - 1, 1));
            break;

        case ']':
            advance();
            append(
                makeToken(Parser::TokenType::RightBracket, position - 1, 1));
            break;

        case ':':
            advance();
            append(
                makeToken(Parser::TokenType::Colon, position - 1, 1));
            break;

        case ',':
            advance();
            append(
                makeToken(Parser::TokenType::Comma, position - 1, 1));
            break;

        case '"':
            append(string());
            break;

        default:
            if (isDigit(c) || c == '-')
            {
                append(number());
            }
            else if (isAlpha(c))
            {
                append(keyword());
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

    tokenPosition = position;
    tokenLine = line;
    tokenColumn = column;
    append(
        makeToken(Parser::TokenType::EndOfFile, position, 0));

    return tokens;
}

/*
* @brief Validates and consumes four hexadecimal digits of a Unicode escape.
* @throws LexerException if the unicode escape sequence is invalid.
*/
void JsonLexer::parseUnicodeEscape()
{
    for (int i = 0; i < 4; ++i)
    {
        if (isAtEnd() || !isHex(peek()))
        {
            throw LexerException(
                DiagnosticFormatter::format(
                    source,
                    line,
                    column,
                    "Invalid unicode escape"
                ),
                line,
                column
            );
        }

        advance(); // consume hex digit
    }
}
