#pragma once
#include "JsonHandler.hpp"
#include "Token.hpp"
#include "ParseOptions.hpp"

class MASKETJSON_API JsonParser
{
public:
    explicit JsonParser(const std::vector<Token>& tokens, const std::string& source);
    // Accepts UTF-8 JSON with a maximum container nesting depth of 128.
    // Integers use int64_t/uint64_t; fractions/exponents use double.
    // Out-of-range literals throw ParseException, never silently fall back to double.
    static Json parse(const std::string& source);
    // Options affect temporary token reservation, not JSON semantics/depth.
    // Unknown preset values throw std::invalid_argument.
    static Json parse(const std::string& source, const ParseOptions& options);

private:
    const std::vector<Token>& tokens;
    const std::string& source;
    size_t current = 0;
    size_t currentDepth = 0;
    // Array capacities in opening-token order. Precompute once to avoid repeated
    // subtree scans and deep copies when vector growth cannot use noexcept moves.
    std::vector<std::pair<size_t, size_t>> arrayReservations;
    size_t nextArrayReservation = 0;
    static constexpr size_t MAX_DEPTH = 128;

    Json parseValue();
    Json parseObject();
    Json parseArray();
    bool match(Parser::TokenType type);
    const Token& peek() const;
    bool isAtEnd() const;
    [[noreturn]] void fail(const std::string& message) const;
};
