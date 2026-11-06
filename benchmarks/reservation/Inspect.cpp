#include "Data.hpp"
#include "JsonParser.hpp"
#include "JsonLexer.hpp"
#include <algorithm>
#include <iostream>
#include <locale>

namespace {
void inventory(const Json& value, size_t depth, size_t& maxDepth, size_t& arrayWidth, size_t& objectWidth) {
    if (value.isArray()) {
        maxDepth = (std::max)(maxDepth, depth + 1);
        arrayWidth = (std::max)(arrayWidth, value.asArray().size());
        for (const auto& child : value.asArray()) inventory(child, depth + 1, maxDepth, arrayWidth, objectWidth);
    } else if (value.isObject()) {
        maxDepth = (std::max)(maxDepth, depth + 1);
        objectWidth = (std::max)(objectWidth, value.asObject().size());
        for (const auto& member : value.asObject()) inventory(member.second, depth + 1, maxDepth, arrayWidth, objectWidth);
    }
}
void errors() {
    std::string prefix = "[";
    for (int i = 0; i < 2048; ++i) prefix += "0,";
    const std::vector<std::string> inputs{"", " ", "[1,]", "{\"a\":}", "01", "1e+", "\"\\uZZZZ\"", "\"\\uD800\"",
        "\"\xFF\"", std::string(129, '[') + "0" + std::string(129, ']'), prefix + "\n false,@]"};
    for (size_t i = 0; i < inputs.size(); ++i) for (bool lex : {false, true}) {
        std::string result = "accepted";
        try { if (lex) (void)MatrixData::tokenize(inputs[i]); else (void)MatrixData::parse(inputs[i]); }
        catch (const std::exception& ex) { result = ex.what(); }
        MatrixData::Fingerprint fingerprint; fingerprint.text(result);
        std::cout << i << ',' << lex << ',' << fingerprint.hex() << '\n';
    }
}
}
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--errors") { errors(); return 0; }
        if (argc < 2) throw std::invalid_argument("Usage: ReservationInspect INPUT... | --errors");
        std::cout.imbue(std::locale::classic());
        std::cout << "dataset,input_bytes,token_count,whitespace_bytes,string_tokens,raw_string_bytes,max_raw_string_bytes,escape_sequences,number_tokens,max_depth,max_array_width,max_object_width,first_4k_tokens,last_4k_tokens,token_fingerprint,dom_fingerprint\n";
        for (int i = 1; i < argc; ++i) {
            const std::filesystem::path path(argv[i]);
            const auto source = MatrixData::read(path);
            const auto tokens = MatrixData::tokenize(source);
            const auto value = MatrixData::parse(source);
            const auto canonical = value.serialize(false);
            if (!(MatrixData::parse(canonical) == value)) throw std::runtime_error("Inspection round-trip mismatch");
            size_t whitespace = 0, strings = 0, stringBytes = 0, maxString = 0, escapes = 0, numbers = 0;
            size_t lastEnd = 0, first = 0, last = 0, depth = 0, arrayWidth = 0, objectWidth = 0;
            MatrixData::Fingerprint tokenHash, domHash;
            for (const auto& token : tokens) {
                if (token.position < lastEnd || token.position > source.size()) throw std::runtime_error("Invalid token range");
                whitespace += token.position - lastEnd;
                const bool string = token.type == Parser::TokenType::String;
                lastEnd = string ? token.position + token.length + 2 : token.start + token.length;
                if (lastEnd > source.size()) throw std::runtime_error("Token exceeds source");
                tokenHash.integer(static_cast<unsigned>(token.type)); tokenHash.integer(token.position);
                tokenHash.integer(token.line); tokenHash.integer(token.column);
                tokenHash.integer(token.start); tokenHash.integer(token.length);
                if (string) {
                    ++strings; stringBytes += token.length; maxString = (std::max)(maxString, token.length);
                    for (size_t j = token.start; j < token.start + token.length; ++j)
                        if (source[j] == '\\') { ++escapes; ++j; }
                }
                numbers += token.type == Parser::TokenType::Number;
                if (token.type != Parser::TokenType::EndOfFile) {
                    first += token.position < 4096;
                    last += token.position >= (source.size() > 4096 ? source.size() - 4096 : 0);
                }
            }
            inventory(value, 0, depth, arrayWidth, objectWidth); domHash.text(canonical);
            std::cout << MatrixData::csv(path.filename().string()) << ',' << source.size() << ',' << tokens.size() << ','
                << whitespace << ',' << strings << ',' << stringBytes << ',' << maxString << ',' << escapes << ',' << numbers << ','
                << depth << ',' << arrayWidth << ',' << objectWidth << ',' << first << ',' << last << ','
                << tokenHash.hex() << ',' << domHash.hex() << '\n';
        }
    } catch (const std::exception& ex) { std::cerr << ex.what() << '\n'; return 1; }
}
