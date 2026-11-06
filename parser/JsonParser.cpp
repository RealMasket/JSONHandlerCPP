#include "pch.h"
#include "JsonParser.hpp"
#include "JsonLexer.hpp"
#include "ParseException.hpp"
#include "DiagnosticFormatter.hpp"
#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <limits>
#include <array>

namespace {
std::vector<std::pair<size_t, size_t>> findArrayReservations(const std::vector<Token>& tokens)
{
    using Type = Parser::TokenType;
    struct Frame { Type type; size_t reservation; };
    std::array<Frame, 128> stack{};
    size_t depth = 0;
    std::vector<std::pair<size_t, size_t>> reservations;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto type = tokens[i].type;
        if (type == Type::LeftBracket || type == Type::LeftBrace) {
            // Leave malformed/deep input to the ordinary parser and diagnostics.
            if (depth == stack.size()) return {};
            const size_t entry = reservations.size();
            if (type == Type::LeftBracket) {
                const bool empty = i + 1 < tokens.size() && tokens[i + 1].type == Type::RightBracket;
                reservations.emplace_back(i, empty ? 0 : 1);
            }
            stack[depth++] = {type, entry};
        } else if (type == Type::Comma && depth && stack[depth - 1].type == Type::LeftBracket) {
            ++reservations[stack[depth - 1].reservation].second;
        } else if (type == Type::RightBracket || type == Type::RightBrace) {
            const auto expected = type == Type::RightBracket ? Type::LeftBracket : Type::LeftBrace;
            if (!depth || stack[depth - 1].type != expected) return {};
            --depth;
        }
    }
    if (depth) return {};
    return reservations;
}

class DepthGuard {
public:
    explicit DepthGuard(size_t& depth) : depth(depth) { ++depth; }
    ~DepthGuard() { --depth; }
    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;
private:
    size_t& depth;
};

unsigned readHex(std::string_view text, size_t& pos)
{
    unsigned result = 0;
    for (int n = 0; n < 4; ++n) {
        const char c = text[pos++];
        const unsigned value = c <= '9' ? c - '0' :
            (c <= 'F' ? c - 'A' + 10 : c - 'a' + 10);
        result = (result << 4) | value;
    }
    return result;
}

void appendUtf8(std::string& out, unsigned cp)
{
    if (cp <= 0x7F) out.push_back(static_cast<char>(cp));
    else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

std::string unescapeString(const std::string& source, const Token& token)
{
    const std::string_view raw(source.data() + token.start, token.length);
    std::string out;
    out.reserve(raw.size());
    auto invalidSurrogate = [&]() {
        throw ParseException(DiagnosticFormatter::format(source, token.line,
            token.column, "Invalid UTF-16 surrogate pair"), token.line, token.column);
    };
    // Escape syntax and bounds were checked by the lexer.
    for (size_t i = 0; i < raw.size();) {
        const char c = raw[i++];
        if (c != '\\') { out.push_back(c); continue; }
        switch (raw[i++]) {
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case '/': out.push_back('/'); break;
        case 'b': out.push_back('\b'); break;
        case 'f': out.push_back('\f'); break;
        case 'n': out.push_back('\n'); break;
        case 'r': out.push_back('\r'); break;
        case 't': out.push_back('\t'); break;
        case 'u': {
            unsigned cp = readHex(raw, i);
            if (cp >= 0xD800 && cp <= 0xDBFF) {
                if (raw.size() - i < 6 || raw[i] != '\\' || raw[i + 1] != 'u')
                    invalidSurrogate();
                i += 2;
                const unsigned low = readHex(raw, i);
                if (low < 0xDC00 || low > 0xDFFF) invalidSurrogate();
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
            } else if (cp >= 0xDC00 && cp <= 0xDFFF) invalidSurrogate();
            appendUtf8(out, cp);
            break;
        }
        }
    }
    return out;
}
}

JsonParser::JsonParser(const std::vector<Token>& tokens, const std::string& source)
    : tokens(tokens), source(source)
{
    if (tokens.empty() || tokens.back().type != Parser::TokenType::EndOfFile)
        throw ParseException("Token stream must end with EOF", 1, 1);
    // Every multi-element array needs a literal comma in the source. A raw
    // string search is cheaper than scanning strided Token records for deeply
    // nested singleton containers. Commas inside strings only cause extra work.
    if (source.find(',') != std::string::npos)
        arrayReservations = findArrayReservations(tokens);
}

bool JsonParser::isAtEnd() const { return peek().type == Parser::TokenType::EndOfFile; }
const Token& JsonParser::peek() const { return tokens.at(current); }

bool JsonParser::match(Parser::TokenType type)
{
    if (peek().type != type) return false;
    ++current;
    return true;
}

void JsonParser::fail(const std::string& message) const
{
    const auto& token = peek();
    throw ParseException(DiagnosticFormatter::format(source, token.line,
        token.column, message), token.line, token.column);
}

Json JsonParser::parse(const std::string& source)
{
    JsonLexer lexer(source);
    const auto tokens = lexer.tokenize();
    JsonParser parser(tokens, source);
    Json result = parser.parseValue();
    if (!parser.isAtEnd()) parser.fail("Unexpected token after JSON value");
    return result;
}

Json JsonParser::parse(const std::string& source, const ParseOptions& options)
{
    JsonLexer lexer(source);
    const auto tokens = lexer.tokenize(options);
    JsonParser parser(tokens, source);
    Json result = parser.parseValue();
    if (!parser.isAtEnd()) parser.fail("Unexpected token after JSON value");
    return result;
}

Json JsonParser::parseValue()
{
    const Token& token = peek();
    if (match(Parser::TokenType::Null)) return nullptr;
    if (match(Parser::TokenType::True)) return true;
    if (match(Parser::TokenType::False)) return false;
    if (match(Parser::TokenType::String)) return unescapeString(source, token);
    if (token.type == Parser::TokenType::Number) {
        const char* begin = source.data() + token.start;
        const char* end = begin + token.length;
        const std::string_view spelling(begin, token.length);
        if (spelling.find_first_of(".eE") == std::string_view::npos) {
            // Preserve the sign of JSON's negative zero, as for floating -0.0.
            if (spelling == "-0") { ++current; return Json(-0.0); }
            if (*begin == '-') {
                std::int64_t integer;
                const auto result = std::from_chars(begin, end, integer);
                if (result.ec != std::errc{} || result.ptr != end)
                    fail("Integer is outside the supported int64_t range");
                ++current;
                return integer;
            }
            std::uint64_t integer;
            const auto result = std::from_chars(begin, end, integer);
            if (result.ec != std::errc{} || result.ptr != end)
                fail("Integer is outside the supported uint64_t range");
            ++current;
            if (integer <= static_cast<std::uint64_t>((std::numeric_limits<std::int64_t>::max)()))
                return Json(static_cast<std::int64_t>(integer));
            return Json(integer);
        }
        double number;
        const auto result = std::from_chars(begin, end, number, std::chars_format::general);
        if (result.ec != std::errc{} || result.ptr != end)
            fail("Number is outside the supported double range");
        ++current;
        return number;
    }
    if (match(Parser::TokenType::LeftBrace)) return parseObject();
    if (match(Parser::TokenType::LeftBracket)) return parseArray();
    fail("Invalid JSON value");
}

Json JsonParser::parseObject()
{
    if (currentDepth >= MAX_DEPTH) fail("Maximum nesting depth exceeded");
    DepthGuard depth(currentDepth);
    Json::JsonObject object;
    if (match(Parser::TokenType::RightBrace)) return object;
    while (true) {
        if (peek().type != Parser::TokenType::String) fail("Expected string key");
        std::string key = unescapeString(source, peek());
        ++current;
        if (!match(Parser::TokenType::Colon)) fail("Expected ':' after key");
        // Preserve the established first-key-wins behavior for duplicate names.
        object.emplace(std::move(key), parseValue());
        if (match(Parser::TokenType::RightBrace)) return object;
        if (!match(Parser::TokenType::Comma)) fail("Expected ',' between key-value pairs");
    }
}

Json JsonParser::parseArray()
{
    if (currentDepth >= MAX_DEPTH) fail("Maximum nesting depth exceeded");
    DepthGuard depth(currentDepth);
    Json::JsonArray array;
    size_t capacity = 0;
    if (nextArrayReservation < arrayReservations.size() &&
        arrayReservations[nextArrayReservation].first == current - 1)
        capacity = arrayReservations[nextArrayReservation++].second;
    if (match(Parser::TokenType::RightBracket)) return array;
    while (true) {
        Json element = parseValue();
        // Don't allocate the hinted DOM capacity before the first value parses.
        if (array.empty() && capacity) array.reserve(capacity);
        array.emplace_back(std::move(element));
        if (match(Parser::TokenType::RightBracket)) return array;
        if (!match(Parser::TokenType::Comma)) fail("Expected ',' between array elements");
    }
}
