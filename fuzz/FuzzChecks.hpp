#pragma once
#include <cstddef>
#include <cstdint>

namespace JsonFuzz {
inline constexpr std::size_t maxInputSize = 65536;
// Returns false only for an expected source LexerException/ParseException.
// All round-trip failures and other exceptions remain visible to the driver.
bool checkInput(const std::uint8_t* data, std::size_t size);
}
