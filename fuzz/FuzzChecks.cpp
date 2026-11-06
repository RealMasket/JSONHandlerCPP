#include "FuzzChecks.hpp"
#include "JsonParser.hpp"
#include "LexerException.hpp"
#include "ParseException.hpp"
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
bool sameValue(const Json& left, const Json& right)
{
    if (left.isFloatingPoint() || right.isFloatingPoint()) {
        if (!left.isFloatingPoint() || !right.isFloatingPoint()) return false;
        return left.asNumber() == right.asNumber() &&
            std::signbit(left.asNumber()) == std::signbit(right.asNumber());
    }
    if (left.isArray() && right.isArray()) {
        if (left.size() != right.size()) return false;
        for (size_t i = 0; i < left.size(); ++i)
            if (!sameValue(left.at(i), right.at(i))) return false;
        return true;
    }
    if (left.isObject() && right.isObject()) {
        if (left.size() != right.size()) return false;
        for (const auto& [key, value] : left.asObject()) {
            const auto it = right.asObject().find(key);
            if (it == right.asObject().end() || !sameValue(value, it->second)) return false;
        }
        return true;
    }
    return left == right;
}

template<class Exception>
bool verifyLowMemoryFailure(const std::string& source, const Exception& expected)
{
    try { (void)JsonParser::parse(source, ParseOptions{ParsePreset::LowMemory}); }
    catch (const Exception& actual) {
        if (actual.getLine() != expected.getLine() || actual.getColumn() != expected.getColumn()
            || std::string(actual.what()) != expected.what())
            throw std::runtime_error("LowMemory changed source diagnostics");
        return false;
    }
    throw std::runtime_error("LowMemory accepted input rejected by Balanced");
}
}

bool JsonFuzz::checkInput(const std::uint8_t* data, std::size_t size)
{
    if (size > maxInputSize) throw std::invalid_argument("Fuzz input exceeds 64 KiB limit");
    const std::string source = size == 0 ? std::string{} :
        std::string(reinterpret_cast<const char*>(data), size);
    std::optional<Json> original;
    // Keep this catch scope narrow: parse failures after serialization are bugs.
    try { original.emplace(JsonParser::parse(source)); }
    catch (const LexerException& error) { return verifyLowMemoryFailure(source, error); }
    catch (const ParseException& error) { return verifyLowMemoryFailure(source, error); }

    const ParseOptions low{ParsePreset::LowMemory};
    const auto lowValue = JsonParser::parse(source, low);
    if (!sameValue(*original, lowValue))
        throw std::runtime_error("LowMemory changed value, number kind, or zero sign");

    const auto compact = original->serialize(false);
    const auto reparsed = JsonParser::parse(compact, low);
    if (!sameValue(*original, reparsed))
        throw std::runtime_error("Compact JSON round-trip changed value, number kind, or zero sign");
    if (reparsed.serialize(false) != compact)
        throw std::runtime_error("Compact JSON serialization is not idempotent");
    const auto pretty = original->serialize(true);
    const auto prettyParsed = JsonParser::parse(pretty, low);
    if (!sameValue(*original, prettyParsed) || prettyParsed.serialize(false) != compact)
        throw std::runtime_error("Pretty JSON round-trip changed the value");
    return true;
}
