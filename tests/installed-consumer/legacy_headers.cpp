#include "JsonHandler.hpp"
#include "JsonParser.hpp"
#include "ParseOptions.hpp"
#include "JsonSerializer.hpp"
#include "TypeException.hpp"

bool legacyHeadersWork()
{
    const auto value = JsonParser::parse("[1,2,3]");
    return value.at(1).asInt64() == 2 && JsonSerializer::serialize(value, false) == "[1,2,3]"
        && JsonParser::parse("[1,2,3]", ParseOptions{ParsePreset::LowMemory}) == value;
}
