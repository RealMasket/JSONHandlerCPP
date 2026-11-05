#include <gtest/gtest.h>
#include "JsonHandler.hpp"
#include "JsonParser.hpp"
#include "JsonSerializer.hpp"
#include "JsonLexer.hpp"
#include "JsonFile.hpp"
#include "AccessException.hpp"
#include "TypeException.hpp"
#include "ParseException.hpp"
#include "LexerException.hpp"
#include "OperationException.hpp"
#include "FileException.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <locale>
#include <random>
#include <string>

class NumericStringTests : public testing::TestWithParam<const char*> {};
TEST_P(NumericStringTests, KeepsStringTypeAndContents)
{
    const Json value(GetParam());
    EXPECT_EQ(JsonParser::parse(value.serialize(false)), value);
    EXPECT_EQ(value.serialize(false), "\"" + std::string(GetParam()) + "\"");
}
INSTANTIATE_TEST_SUITE_P(RoundTrip, NumericStringTests,
    testing::Values("123", "001", "0", "9007199254740993", "-12", "1e3", ""));

TEST(JsonRegressionTests, ExactDoubleRoundTrips)
{
    EXPECT_EQ(Json(0.1).serialize(false), "0.1");
    for (double number : {1.23456789, 0.1, -0.0, 1e-200, 1e200,
        std::numeric_limits<double>::max(), std::numeric_limits<double>::min(),
        std::numeric_limits<double>::denorm_min()}) {
        const double parsed = JsonParser::parse(Json(number).serialize(false)).asNumber();
        EXPECT_EQ(parsed, number);
        EXPECT_EQ(std::signbit(parsed), std::signbit(number));
    }
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> values(-1e100, 1e100);
    for (int i = 0; i < 1000; ++i) {
        const double value = values(rng);
        EXPECT_EQ(JsonParser::parse(Json(value).serialize(false)).asNumber(), value);
    }
}

struct CommaPunct : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};
TEST(JsonRegressionTests, NumberConversionIgnoresStreamLocale)
{
    struct RestoreLocale {
        std::locale previous = std::locale();
        ~RestoreLocale() { std::locale::global(previous); }
    } restore;
    std::locale::global(std::locale(std::locale::classic(), new CommaPunct));
    EXPECT_EQ(Json(1.5).serialize(false), "1.5");
    EXPECT_EQ(JsonParser::parse("1.5").asNumber(), 1.5);
}

TEST(JsonRegressionTests, RejectsNonFiniteNumbers)
{
    EXPECT_THROW(Json(std::numeric_limits<double>::infinity()).serialize(), OperationException);
    EXPECT_THROW(Json(-std::numeric_limits<double>::infinity()).serialize(), OperationException);
    EXPECT_THROW(Json(std::numeric_limits<double>::quiet_NaN()).serialize(), OperationException);
}
TEST(JsonRegressionTests, NumberRangeErrorsUseParseException)
{
    EXPECT_THROW(JsonParser::parse("1e999"), ParseException);
    EXPECT_THROW(JsonParser::parse("-1e999"), ParseException);
    EXPECT_THROW(JsonParser::parse("1e-999"), ParseException);
}

TEST(JsonRegressionTests, EveryControlCharacterIsEscaped)
{
    for (char c = 0; c < 0x20; ++c) {
        const Json value(std::string(1, c));
        const auto encoded = value.serialize(false);
        EXPECT_EQ(JsonParser::parse(encoded), value);
        for (unsigned char byte : encoded) EXPECT_GE(byte, 0x20);
        EXPECT_THROW(JsonParser::parse("\"" + std::string(1, c) + "\""), LexerException);
    }
}
TEST(JsonRegressionTests, DecodesAllEscapes)
{
    EXPECT_EQ(JsonParser::parse(R"("\"\\\/\b\f\n\r\t\u0041\u041F")").asString(),
        std::string("\"\\/\b\f\n\r\tA") + "\xD0\x9F");
    EXPECT_EQ(JsonParser::parse(R"("\u0000")").asString(), std::string(1, '\0'));
}
TEST(JsonRegressionTests, SurrogatePairsAndUnicodeKeys)
{
    const std::string emoji = "\xF0\x9F\x98\x80";
    EXPECT_EQ(JsonParser::parse(R"("\uD83D\uDE00")").asString(), emoji);
    const auto object = JsonParser::parse(R"({"\uD83D\uDE00":"\uDBFF\uDFFF"})");
    EXPECT_EQ(object.at(emoji).asString(), "\xF4\x8F\xBF\xBF");
    EXPECT_EQ(JsonParser::parse(object.serialize(false)), object);
}
class InvalidSurrogateTests : public testing::TestWithParam<const char*> {};
TEST_P(InvalidSurrogateTests, RejectsUnpairedSurrogates)
{
    EXPECT_THROW(JsonParser::parse(GetParam()), ParseException);
}
INSTANTIATE_TEST_SUITE_P(Unicode, InvalidSurrogateTests, testing::Values(
    R"("\uD800")", R"("\uDC00")", R"("\uD800\u0041")",
    R"("\uD800x")", R"("\uD800\nabcd")", R"("\uD800\uD800")"));

class InvalidStructureTests : public testing::TestWithParam<const char*> {};
TEST_P(InvalidStructureTests, RejectsMalformedStructure)
{
    EXPECT_THROW(JsonParser::parse(GetParam()), ParseException);
}
INSTANTIATE_TEST_SUITE_P(Parser, InvalidStructureTests, testing::Values(
    "", " ", "[", "{", "[1,]", "{\"a\":1,}", "[1 2]",
    "{\"a\" 1}", "{1:2}", "{\"a\":}", "true false", "[]{}", "[}", "{]"));

TEST(JsonRegressionTests, DuplicateKeysKeepFirstValue)
{
    EXPECT_EQ(JsonParser::parse(R"({"a":1,"a":2})").at("a").asNumber(), 1);
}

TEST(JsonRegressionTests, NestedArraysObjectsAndDuplicateKeysKeepTheirContents)
{
    const auto value = JsonParser::parse(
        R"({"x":[1,2],"x":[3,[4,5]],"y":[[],{"comma,]":{"a":1,"b":[6,7]}},[8]]})");
    EXPECT_EQ(value.at("x"), Json(Json::JsonArray{1, 2}));
    EXPECT_EQ(value.at("y").size(), 3u);
    EXPECT_TRUE(value.at("y").at(0).asArray().empty());
    EXPECT_EQ(value.at("y").at(1).at("comma,]").at("b"), Json(Json::JsonArray{6, 7}));
    EXPECT_EQ(value.at("y").at(2), Json(Json::JsonArray{8}));
    EXPECT_EQ(JsonParser::parse(value.serialize(false)), value);
}

TEST(JsonRegressionTests, WideArrayOfNestedObjectsRoundTrips)
{
    Json::JsonArray expected;
    for (int i = 0; i < 1000; ++i)
        expected.emplace_back(Json::JsonObject{{"id", i},
            {"children", Json::JsonArray{Json::JsonArray{}, i, Json::JsonObject{{"v", ",[]{}"}}}}});
    const Json original(std::move(expected));
    EXPECT_EQ(JsonParser::parse(original.serialize(false)), original);
    EXPECT_EQ(JsonParser::parse(original.serialize(true)), original);
}

TEST(JsonRegressionTests, RejectsInvalidUtf8OnInputAndOutput)
{
    const std::string invalid[] = {
        "\x80", "\xC0\xAF", "\xE0\x80\x80", "\xED\xA0\x80",
        "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\xE2\x82", "\xC2\x20"
    };
    for (const auto& value : invalid) {
        EXPECT_THROW(JsonParser::parse("\"" + value + "\""), LexerException);
        EXPECT_THROW(Json(value).serialize(), OperationException);
    }
    const Json valid("\xC2\x80\xDF\xBF\xE0\xA0\x80\xF0\x90\x80\x80");
    EXPECT_EQ(JsonParser::parse(valid.serialize(false)), valid);
}

TEST(JsonRegressionTests, SiblingEmptyContainersDoNotConsumeDepth)
{
    std::string text = "[";
    for (int i = 0; i < 500; ++i) {
        if (i) text += ',';
        text += i % 2 ? "{}" : "[]";
    }
    text += ']';
    EXPECT_EQ(JsonParser::parse(text).size(), 500u);
    text = "{";
    for (int i = 0; i < 200; ++i) {
        if (i) text += ',';
        text += "\"" + std::to_string(i) + "\":{}";
    }
    text += '}';
    EXPECT_EQ(JsonParser::parse(text).size(), 200u);
}
TEST(JsonRegressionTests, NestingBoundary)
{
    EXPECT_NO_THROW(JsonParser::parse(std::string(128, '[') + "0" + std::string(128, ']')));
    EXPECT_NO_THROW(JsonParser::parse(std::string(128, '[') + std::string(128, ']')));
    EXPECT_THROW(JsonParser::parse(std::string(129, '[') + "0" + std::string(129, ']')), ParseException);
}
TEST(JsonRegressionTests, DiagnosticsPointAtUnexpectedToken)
{
    try {
        JsonParser::parse("{\n \"a\": 1\n \"b\": 2}");
        FAIL();
    } catch (const ParseException& ex) {
        EXPECT_EQ(ex.getLine(), 3u);
        EXPECT_EQ(ex.getColumn(), 2u);
    }
    EXPECT_THROW(JsonParser::parse(""), ParseException);
    try {
        JsonLexer("\"a\nb\"").tokenize();
        FAIL();
    } catch (const LexerException& ex) {
        EXPECT_EQ(ex.getLine(), 1u);
        EXPECT_EQ(ex.getColumn(), 3u);
    }
}
TEST(JsonRegressionTests, CompactAndPrettyOutput)
{
    const Json value(Json::JsonObject{{"a", Json::JsonArray{1, Json::JsonObject{{"b", "123"}}}}});
    EXPECT_EQ(value.serialize(false), R"({"a":[1,{"b":"123"}]})");
    const auto pretty = value.serialize(true);
    EXPECT_EQ(pretty.front(), '{');
    EXPECT_EQ(pretty.back(), '}');
    EXPECT_EQ(JsonParser::parse(pretty), value);
    EXPECT_EQ(Json(Json::JsonArray{}).serialize(), "[]");
    EXPECT_EQ(Json(Json::JsonObject{}).serialize(), "{}");
    EXPECT_THROW(value.serialize(true, -1), OperationException);
}
TEST(JsonRegressionTests, SerializationDepthLimit)
{
    Json value = 0;
    for (int i = 0; i < 128; ++i) value = Json::JsonArray{value};
    EXPECT_NO_THROW(JsonParser::parse(value.serialize(false)));
    value = Json::JsonArray{value};
    EXPECT_THROW(value.serialize(false), OperationException);
}
TEST(JsonRegressionTests, StrictAccessDoesNotMutate)
{
    Json object(Json::JsonObject{{"x", 1}});
    EXPECT_THROW(object.at("missing"), AccessException);
    EXPECT_EQ(object.size(), 1u);
    object.at("x") = 2;
    const Json& readOnly = object;
    EXPECT_EQ(readOnly.at("x").asNumber(), 2);
    Json array(Json::JsonArray{1});
    EXPECT_THROW(array.at(1), AccessException);
    EXPECT_EQ(array.size(), 1u);
    array.at(0) = 3;
    EXPECT_EQ(std::as_const(array).at(0).asNumber(), 3);
    Json number(42);
    EXPECT_THROW(number["x"], TypeException);
    EXPECT_THROW(number[0], TypeException);
    EXPECT_EQ(number.asNumber(), 42);
    Json null;
    EXPECT_THROW(null.at("x"), TypeException);
    EXPECT_TRUE(null.isNull());
    null["x"] = 1;
    EXPECT_EQ(null.at("x").asNumber(), 1);
    Json growing;
    growing[2] = 4;
    EXPECT_EQ(growing.size(), 3u);
    EXPECT_THROW(growing[std::numeric_limits<size_t>::max()], AccessException);
}
TEST(JsonRegressionTests, VariadicPushBackAndSelfMerge)
{
    Json array(Json::JsonArray{});
    array.push_back(1, "two", true);
    EXPECT_EQ(array, Json(Json::JsonArray{1, "two", true}));
    array.mergeArray(array);
    EXPECT_EQ(array, Json(Json::JsonArray{1, "two", true, 1, "two", true}));
}
class FileTests : public testing::Test {
protected:
    std::filesystem::path dir;
    void SetUp() override {
        dir = std::filesystem::temp_directory_path() /
            ("jsonhandler-test-" + std::to_string(std::random_device{}()));
        ASSERT_TRUE(std::filesystem::create_directory(dir));
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(dir, error);
    }
};
TEST_F(FileTests, FileRoundTrip)
{
    const auto path = (dir / "value.json").string();
    const Json value(Json::JsonObject{{"id", "001"}, {"value", 1.23456789}});
    JsonFile::write(path, value);
    EXPECT_EQ(JsonFile::read(path), value);
}
TEST_F(FileTests, SerializationFailurePreservesExistingFile)
{
    const auto path = (dir / "value.json").string();
    JsonFile::write(path, "original");
    EXPECT_THROW(JsonFile::write(path, std::numeric_limits<double>::infinity()), OperationException);
    EXPECT_EQ(JsonFile::read(path), Json("original"));
}
TEST_F(FileTests, ReportsFileAndParseFailures)
{
    EXPECT_THROW(JsonFile::read((dir / "missing").string()), FileException);
    EXPECT_THROW(JsonFile::read(dir.string()), FileException);
    EXPECT_THROW(JsonFile::write((dir / "missing" / "out.json").string(), 1), FileException);
    const auto path = dir / "invalid.json";
    { std::ofstream file(path); file << "{"; }
    EXPECT_THROW(JsonFile::read(path.string()), ParseException);
}
