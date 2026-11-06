#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <string_view>
#include "JsonParser.hpp"
#include "JsonLexer.hpp"

namespace MatrixData {
inline Json parse(const std::string& source) {
#ifdef MATRIX_LOW_MEMORY
    return JsonParser::parse(source, ParseOptions{ParsePreset::LowMemory});
#else
    return JsonParser::parse(source);
#endif
}
inline std::vector<Token> tokenize(const std::string& source) {
#ifdef MATRIX_LOW_MEMORY
    return JsonLexer(source).tokenize(ParseOptions{ParsePreset::LowMemory});
#else
    return JsonLexer(source).tokenize();
#endif
}
inline std::string read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Unable to open matrix input");
    std::string source((std::istreambuf_iterator<char>(file)), {});
    if (file.bad()) throw std::runtime_error("Unable to read matrix input");
    return source;
}
inline std::string csv(std::string_view value) {
    std::string result = "\"";
    for (char c : value) { result += c; if (c == '"') result += c; }
    return result + '"';
}
struct Fingerprint {
    std::uint64_t value = 14695981039346656037ull;
    void byte(unsigned char c) { value = (value ^ c) * 1099511628211ull; }
    void integer(std::uint64_t n) { for (unsigned i = 0; i < 8; ++i) { byte(static_cast<unsigned char>(n)); n >>= 8; } }
    void text(std::string_view s) { for (unsigned char c : s) byte(c); }
    std::string hex() const { std::ostringstream out; out << std::hex << std::setw(16) << std::setfill('0') << value; return out.str(); }
};
}
