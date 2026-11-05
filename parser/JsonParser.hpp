#pragma once
#include "JsonHandler.hpp"
#include "Token.hpp"

class JSON_API JsonParser
{
public:
    explicit JsonParser(const std::vector<Token>& tokens, const std::string& source);
    // Accepts UTF-8 JSON with a maximum container nesting depth of 128.
    // Numbers are represented as double; out-of-range values throw ParseException.
    static Json parse(const std::string& source);

private:
    const std::vector<Token>& tokens;
    const std::string& source;
    size_t current = 0;
    size_t currentDepth = 0;
    static constexpr size_t MAX_DEPTH = 128;

    Json parseValue();
    Json parseObject();
    Json parseArray();
    bool match(Parser::TokenType type);
    const Token& peek() const;
    bool isAtEnd() const;
    [[noreturn]] void fail(const std::string& message) const;
};
