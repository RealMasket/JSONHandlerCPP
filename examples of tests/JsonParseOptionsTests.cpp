#include <gtest/gtest.h>
#include "ParseOptions.hpp"
#include "JsonParser.hpp"
#include "JsonLexer.hpp"
#include "LexerException.hpp"
#include "ParseException.hpp"
#include "parser/TokenReservation.hpp"
#include <cmath>
#include <limits>

namespace {
const ParseOptions low{ParsePreset::LowMemory};
std::string zeros(size_t count) {
    std::string source = "[";
    for (size_t i = 0; i < count; ++i) source += i ? ",0" : "0";
    return source + "]";
}
void sameTokens(const std::vector<Token>& expected, const std::vector<Token>& actual) {
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(expected[i].type, actual[i].type);
        EXPECT_EQ(expected[i].position, actual[i].position);
        EXPECT_EQ(expected[i].line, actual[i].line);
        EXPECT_EQ(expected[i].column, actual[i].column);
        EXPECT_EQ(expected[i].start, actual[i].start);
        EXPECT_EQ(expected[i].length, actual[i].length);
    }
}
std::string diagnostic(const std::string& source, const ParseOptions& options) {
    try { (void)JsonParser::parse(source, options); return "accepted"; }
    catch (const LexerException& ex) {
        return "lexer:" + std::to_string(ex.getLine()) + ":" + std::to_string(ex.getColumn()) + ":" + ex.what();
    } catch (const ParseException& ex) {
        return "parser:" + std::to_string(ex.getLine()) + ":" + std::to_string(ex.getColumn()) + ":" + ex.what();
    }
}
}

TEST(JsonParseOptionsTests, DefaultOptionsMatchExistingEntryPoints)
{
    EXPECT_EQ(ParseOptions{}.preset, ParsePreset::Balanced);
    const std::string source = zeros(4099);
    const auto implicit = JsonLexer(source).tokenize();
    const auto explicitDefault = JsonLexer(source).tokenize(ParseOptions{});
    sameTokens(implicit, explicitDefault);
    EXPECT_EQ(implicit.capacity(), explicitDefault.capacity());
    EXPECT_EQ(JsonParser::parse(source), JsonParser::parse(source, ParseOptions{}));
}

TEST(JsonParseOptionsTests, LowMemoryPreservesTokenRangesAcrossDensityChanges)
{
    const std::string sparse = "\"" + std::string(131071, 's') + "\\n\"";
    const auto dense = zeros(4099);
    const std::vector<std::string> sources{"", " \n\t", sparse,
        dense.substr(0, dense.size()-1) + "," + sparse + "]",
        "[" + sparse + "," + dense.substr(1), dense + std::string(1024*1024, ' '),
        std::string(131071, ' ') + dense};
    for (const auto& source : sources) {
        sameTokens(JsonLexer(source).tokenize(), JsonLexer(source).tokenize(low));
    }
}

TEST(JsonParseOptionsTests, TrailingWhitespaceDoesNotReserveForItsBytes)
{
    const std::string source = zeros(16384) + std::string(1024*1024, ' ');
    const auto tokens = JsonLexer(source).tokenize(low);
    ASSERT_EQ(tokens.size(), 32770u);
    // Broad regression bound, independent of the exact chosen growth fraction.
    EXPECT_LT(tokens.capacity(), tokens.size() * 3);
    EXPECT_EQ(tokens.back().position, source.size());
    EXPECT_EQ(JsonParser::parse(source, low).size(), 16384u);
}

TEST(JsonParseOptionsTests, LongStringKeepsSmallTokenBuffer)
{
    const std::string source = "\"" + std::string(2*1024*1024, 'x') + "\"";
    const auto tokens = JsonLexer(source).tokenize(low);
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_LT(tokens.capacity() * sizeof(Token), source.size()/8);
    EXPECT_EQ(JsonParser::parse(source, low).asString().size(), source.size()-2);
}

TEST(JsonParseOptionsTests, ValuesAndNumericStorageArePreserved)
{
    const std::vector<std::string> sources{"null", "true", "[]", "{}", "\"\\u041F\\uD83D\\uDE00\"",
        R"({"a":1,"a":2,"b":[-9223372036854775808,18446744073709551615,1.0,-0,-0.0,1e3]})"};
    for (const auto& source : sources) {
        const auto expected = JsonParser::parse(source);
        const auto actual = JsonParser::parse(source, low);
        EXPECT_EQ(actual, expected);
        EXPECT_EQ(actual.serialize(false), expected.serialize(false));
    }
    const auto value = JsonParser::parse("[-9223372036854775808,18446744073709551615,1.0,-0]", low);
    EXPECT_EQ(value.at(0).asInt64(), (std::numeric_limits<std::int64_t>::min)());
    EXPECT_EQ(value.at(1).asUInt64(), (std::numeric_limits<std::uint64_t>::max)());
    EXPECT_TRUE(value.at(2).isFloatingPoint());
    EXPECT_TRUE(std::signbit(value.at(3).asNumber()));
}

TEST(JsonParseOptionsTests, ErrorsAndDepthLimitMatchBalanced)
{
    const std::vector<std::string> sources{"", " \t", "[1,]", "{\"x\":}", "01", "1e+",
        "18446744073709551616", "\"\\uD800\"", "\"\xFF\"",
        std::string(129,'[') + "0" + std::string(129,']'),
        zeros(4099).substr(0,8198) + ",\n false,@]"};
    for (const auto& source : sources) {
        const auto expected = diagnostic(source, ParseOptions{});
        EXPECT_NE(expected, "accepted");
        EXPECT_EQ(diagnostic(source, low), expected);
    }
    const auto boundary = std::string(128,'[') + "0" + std::string(128,']');
    EXPECT_NO_THROW(JsonParser::parse(boundary, low));
}

TEST(JsonParseOptionsTests, InvalidPresetDoesNotConsumeLexerInput)
{
    const ParseOptions invalid{static_cast<ParsePreset>(-1)};
    EXPECT_THROW(JsonParser::parse("[1]", invalid), std::invalid_argument);
    JsonLexer lexer("[1]");
    EXPECT_THROW(lexer.tokenize(invalid), std::invalid_argument);
    EXPECT_EQ(lexer.tokenize(low).front().type, Parser::TokenType::LeftBracket);
}

TEST(JsonParseOptionsTests, CapacityHintStaysInRangeNearSizeLimit)
{
    const auto maximum = (std::numeric_limits<size_t>::max)();
    for (size_t limit : {size_t{0}, size_t{1}, size_t{1000}, maximum / sizeof(Token), maximum}) {
        for (size_t capacity : {size_t{0}, limit/2, limit ? limit-1 : size_t{0}, limit}) {
            for (double estimate : {0.0, 10.0, static_cast<double>(limit), (std::numeric_limits<double>::infinity)()}) {
                const auto hint = MasketJSONDetail::lowMemoryCapacity(capacity, limit, estimate);
                EXPECT_LE(hint, limit);
                EXPECT_GE(hint, capacity);
                EXPECT_LE(hint-capacity, (std::max)(capacity, size_t{256}));
                if (capacity < limit) EXPECT_GT(hint, capacity);
            }
        }
    }
}
