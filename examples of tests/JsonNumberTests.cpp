#include <gtest/gtest.h>
#include "JsonHandler.hpp"
#include "JsonParser.hpp"
#include "ParseException.hpp"
#include "TypeException.hpp"
#include "OperationException.hpp"
#include "tests/data/NumberComparisonCases.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>

namespace {
constexpr auto intMin = (std::numeric_limits<std::int64_t>::min)();
constexpr auto intMax = (std::numeric_limits<std::int64_t>::max)();
constexpr auto uintMax = (std::numeric_limits<std::uint64_t>::max)();
}

TEST(JsonNumberTests, PreservesIntegerLimits)
{
    const auto minimum = JsonParser::parse("-9223372036854775808");
    const auto maximum = JsonParser::parse("9223372036854775807");
    const auto unsignedMax = JsonParser::parse("18446744073709551615");
    EXPECT_TRUE(minimum.isSignedInteger());
    EXPECT_TRUE(maximum.isSignedInteger());
    EXPECT_TRUE(unsignedMax.isUnsignedInteger());
    EXPECT_EQ(minimum.asInt64(), intMin);
    EXPECT_EQ(maximum.asInt64(), intMax);
    EXPECT_EQ(unsignedMax.asUInt64(), uintMax);
    EXPECT_EQ(minimum.serialize(false), "-9223372036854775808");
    EXPECT_EQ(maximum.serialize(false), "9223372036854775807");
    EXPECT_EQ(unsignedMax.serialize(false), "18446744073709551615");
    const auto nextSigned = JsonParser::parse("9223372036854775808");
    EXPECT_TRUE(nextSigned.isUnsignedInteger());
    EXPECT_EQ(nextSigned.asUInt64(), std::uint64_t{1} << 63);
}

class ExactIntegerTextTests : public testing::TestWithParam<const char*> {};
TEST_P(ExactIntegerTextTests, SerializesWithoutRounding)
{
    const auto value = JsonParser::parse(GetParam());
    EXPECT_TRUE(value.isInteger());
    EXPECT_TRUE(value.isNumber());
    EXPECT_FALSE(value.isFloatingPoint());
    EXPECT_EQ(value.typeName(), "number");
    EXPECT_EQ(value.serialize(false), GetParam());
    EXPECT_EQ(JsonParser::parse(value.serialize(false)), value);
}
INSTANTIATE_TEST_SUITE_P(IntegerBoundaries, ExactIntegerTextTests, testing::Values(
    "0", "1", "-1", "9007199254740991", "9007199254740992", "9007199254740993",
    "-9007199254740993", "9223372036854775807", "-9223372036854775808",
    "9223372036854775808", "18446744073709551615"));

class IntegerOverflowTests : public testing::TestWithParam<const char*> {};
TEST_P(IntegerOverflowTests, NeverFallsBackToDouble)
{
    EXPECT_THROW(JsonParser::parse(GetParam()), ParseException);
}
INSTANTIATE_TEST_SUITE_P(StrictRange, IntegerOverflowTests, testing::Values(
    "-9223372036854775809", "18446744073709551616", "100000000000000000000",
    "-9999999999999999999999999999999999999999999999999999999999999"));

TEST(JsonNumberTests, IntegerOverflowReportsTokenStart)
{
    try {
        JsonParser::parse("[\n  18446744073709551616]");
        FAIL();
    } catch (const ParseException& error) {
        EXPECT_EQ(error.getLine(), 2u);
        EXPECT_EQ(error.getColumn(), 3u);
    }
}

TEST(JsonNumberTests, EveryStandardIntegralConstructorKeepsSignedness)
{
    const auto check = [](auto value) {
        using Integer = decltype(value);
        const Json json(value);
        EXPECT_TRUE(json.isInteger());
        EXPECT_EQ(json.isSignedInteger(), std::is_signed_v<Integer>);
        EXPECT_EQ(json.isUnsignedInteger(), std::is_unsigned_v<Integer>);
        if constexpr (std::is_signed_v<Integer>)
            EXPECT_EQ(json.asInt64(), static_cast<std::int64_t>(value));
        else EXPECT_EQ(json.asUInt64(), static_cast<std::uint64_t>(value));
    };
    check(static_cast<signed char>(-7));
    check(static_cast<unsigned char>(255));
    check(static_cast<short>(-123));
    check(static_cast<unsigned short>(65535));
    check(-123);
    check((std::numeric_limits<unsigned int>::max)());
    check((std::numeric_limits<long>::min)());
    check((std::numeric_limits<unsigned long>::max)());
    check((std::numeric_limits<long long>::min)());
    check((std::numeric_limits<unsigned long long>::max)());
    check(size_t{42});
    check('a');
    check(L'a');
    check(u'a');
    check(U'a');
    EXPECT_TRUE(Json(true).isBool());
    EXPECT_FALSE(Json(true).isInteger());
    EXPECT_TRUE(Json(1.25f).isFloatingPoint());
    Json assigned;
    assigned = uintMax;
    EXPECT_EQ(assigned.asUInt64(), uintMax);
}

TEST(JsonNumberTests, CheckedIntegerAccess)
{
    EXPECT_EQ(Json(std::uint64_t{42}).asInt64(), 42);
    EXPECT_EQ(Json(std::int64_t{42}).asUInt64(), 42u);
    EXPECT_EQ(Json(std::uint64_t{0}).asInt64(), 0);
    EXPECT_THROW(Json(-1).asUInt64(), TypeException);
    EXPECT_THROW(Json(uintMax).asInt64(), TypeException);
    EXPECT_THROW(Json(1.0).asInt64(), TypeException);
    EXPECT_THROW(Json(1.0).asUInt64(), TypeException);
    EXPECT_THROW(Json("42").asInt64(), TypeException);
    EXPECT_THROW(Json(true).asInt64(), TypeException);
    EXPECT_THROW(Json().asUInt64(), TypeException);
    EXPECT_THROW(Json("42").asNumber(), TypeException);
    // The old API remains an explicit, potentially rounded double conversion.
    EXPECT_EQ(Json(uintMax).asNumber(), static_cast<double>(uintMax));
}

TEST(JsonNumberTests, DecimalAndExponentLiteralsStayFloatingPoint)
{
    for (const char* text : {"1.0", "1e0", "9007199254740993.0", "1e20"}) {
        EXPECT_TRUE(JsonParser::parse(text).isFloatingPoint()) << text;
        EXPECT_FALSE(JsonParser::parse(text).isInteger()) << text;
    }
    EXPECT_EQ(Json(1.0).serialize(false), "1.0");
    EXPECT_TRUE(JsonParser::parse(Json(1.0).serialize(false)).isFloatingPoint());
    for (const char* text : {"-0", "-0.0", "-0e0"}) {
        const auto zero = JsonParser::parse(text);
        EXPECT_TRUE(zero.isFloatingPoint());
        EXPECT_TRUE(std::signbit(zero.asNumber()));
        EXPECT_EQ(zero.serialize(false), "-0.0");
    }
    EXPECT_TRUE(JsonParser::parse("0").isInteger());
}

TEST(JsonNumberTests, NumericEqualityIsExactAcrossStorageTypes)
{
    EXPECT_EQ(Json(1), Json(std::uint64_t{1}));
    EXPECT_EQ(Json(1), Json(1.0));
    EXPECT_EQ(Json(0), Json(-0.0));
    EXPECT_FALSE(Json(true) == Json(1));
    EXPECT_FALSE(Json("1") == Json(1));
    EXPECT_FALSE(Json(std::int64_t{-1}) == Json(uintMax));
    const Json rounded(9007199254740992.0);
    const Json adjacent(std::uint64_t{9007199254740993ULL});
    EXPECT_FALSE(adjacent == rounded);
    EXPECT_FALSE(rounded == adjacent);
    EXPECT_FALSE(Json(intMax) == Json(0x1p63));
    EXPECT_FALSE(Json(uintMax) == Json(0x1p64));
    EXPECT_EQ(Json(intMin), Json(-0x1p63));
    EXPECT_EQ(Json(Json::JsonObject{{"v", Json::JsonArray{1, 2}}}),
        Json(Json::JsonObject{{"v", Json::JsonArray{1.0, std::uint64_t{2}}}}));
    const Json array(Json::JsonArray{adjacent});
    EXPECT_FALSE(array.contains(rounded));
    EXPECT_TRUE(array.contains(adjacent));
}

TEST(JsonNumberTests, ComparisonMatchesIndependentRationalOracle)
{
    for (const auto& test : numberComparisonCases) {
        SCOPED_TRACE(std::string(test.left) + " vs " + test.right);
        const auto left = JsonParser::parse(test.left);
        const auto right = JsonParser::parse(test.right);
        EXPECT_EQ(left.compareNumbers(right), test.order);
        EXPECT_EQ(right.compareNumbers(left), -test.order);
        EXPECT_EQ(left == right, test.order == 0);
        EXPECT_EQ(right == left, test.order == 0);
    }
}

TEST(JsonNumberTests, NonFiniteComparisons)
{
    const Json infinity(std::numeric_limits<double>::infinity());
    const Json minusInfinity(-std::numeric_limits<double>::infinity());
    const Json nan(std::numeric_limits<double>::quiet_NaN());
    EXPECT_EQ(infinity.compareNumbers(Json(uintMax)), 1);
    EXPECT_EQ(minusInfinity.compareNumbers(Json(intMin)), -1);
    EXPECT_EQ(infinity, infinity);
    EXPECT_FALSE(nan == nan);
    EXPECT_FALSE(nan == Json(0));
    EXPECT_THROW(nan.compareNumbers(Json(0)), OperationException);
    EXPECT_THROW(Json(0).compareNumbers(nan), OperationException);
    EXPECT_THROW(Json(0).compareNumbers(Json("0")), TypeException);
    EXPECT_THROW(Json(true).compareNumbers(Json(1)), TypeException);
}

TEST(JsonNumberTests, SortsMixedNumbersWithoutRounding)
{
    const Json::JsonArray ordered = {
        Json(intMin), Json(-1.5), Json(-1), Json(-0.5), Json(0), Json(0.5),
        Json(9007199254740992.0), Json(std::uint64_t{9007199254740993ULL}),
        Json(intMax), Json(0x1p63), Json(uintMax), Json(0x1p64)};
    Json rows(Json::JsonArray{});
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it)
        rows.push_back(Json::JsonObject{{"value", *it}});
    rows.sortByPath("value");
    for (size_t i = 0; i < ordered.size(); ++i) EXPECT_EQ(rows.at(i).at("value"), ordered[i]);
    rows.sortByPath("value", false);
    for (size_t i = 0; i < ordered.size(); ++i)
        EXPECT_EQ(rows.at(i).at("value"), ordered[ordered.size() - i - 1]);
}

TEST(JsonNumberTests, RandomIntegerRoundTrips)
{
    std::mt19937_64 random(12345);
    for (int i = 0; i < 1000; ++i) {
        const auto positive = static_cast<std::uint64_t>(random());
        EXPECT_EQ(JsonParser::parse(Json(positive).serialize(false)).asUInt64(), positive);
        const auto magnitude = static_cast<std::int64_t>(random() >> 1);
        const auto signedValue = i % 2 ? -magnitude : magnitude;
        EXPECT_EQ(JsonParser::parse(Json(signedValue).serialize(false)).asInt64(), signedValue);
    }
}
