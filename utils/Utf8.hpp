#pragma once
#include <cstdint>
#include <string_view>

namespace JsonDetail {
// Reject overlong encodings, surrogate code points, and values above U+10FFFF.
inline bool isValidUtf8(std::string_view text)
{
    for (size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i++]);
        if (first < 0x80) continue;
        unsigned count;
        std::uint32_t codepoint;
        std::uint32_t minimum;
        if (first >= 0xC2 && first <= 0xDF) {
            count = 1; codepoint = first & 0x1F; minimum = 0x80;
        } else if (first >= 0xE0 && first <= 0xEF) {
            count = 2; codepoint = first & 0x0F; minimum = 0x800;
        } else if (first >= 0xF0 && first <= 0xF4) {
            count = 3; codepoint = first & 0x07; minimum = 0x10000;
        } else return false;
        if (text.size() - i < count) return false;
        while (count--) {
            const auto next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xC0) != 0x80) return false;
            codepoint = (codepoint << 6) | (next & 0x3F);
        }
        if (codepoint < minimum || codepoint > 0x10FFFF ||
            (codepoint >= 0xD800 && codepoint <= 0xDFFF)) return false;
    }
    return true;
}
}
