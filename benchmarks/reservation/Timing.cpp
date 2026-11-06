#include "Data.hpp"
#include "JsonParser.hpp"
#include "JsonLexer.hpp"
#include "../BenchmarkUtils.hpp"
#include <charconv>
#include <iostream>
#include <locale>

namespace {
size_t count(const char* arg) {
    size_t n = 0;
    const std::string_view text(arg);
    const auto r = std::from_chars(text.data(), text.data() + text.size(), n);
    if (r.ec != std::errc{} || r.ptr != text.data() + text.size() || !n || n > 1000000)
        throw std::invalid_argument("Invalid matrix sample/iteration count");
    return n;
}
}
int main(int argc, char** argv) {
    try {
        if (argc < 4) throw std::invalid_argument("Usage: ReservationTime ITERATIONS SAMPLES INPUT...");
        const size_t iterations = count(argv[1]), samples = count(argv[2]);
        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(12)
            << "dataset,bytes,operation,iterations,samples,median_us,min_us,max_us,configuration\n";
        for (int i = 3; i < argc; ++i) {
            const std::filesystem::path path(argv[i]);
            const auto source = MatrixData::read(path);
            const auto value = MatrixData::parse(source);
            if (!(MatrixData::parse(value.serialize(false)) == value)) throw std::runtime_error("Round-trip mismatch");
            for (bool lex : {false, true}) {
                const auto time = lex ? BenchmarkUtils::measure([&] {
                    const auto tokens = MatrixData::tokenize(source);
                    return static_cast<std::uint64_t>(tokens.size());
                }, iterations, samples) : BenchmarkUtils::measure([&] {
                    const auto parsed = MatrixData::parse(source);
                    // Escape the entire result through an out-of-line consumer.
                    BenchmarkUtils::escape(&parsed);
                    return static_cast<std::uint64_t>(parsed.isArray() ? parsed.asArray().size()
                        : parsed.isObject() ? parsed.asObject().size() : 1);
                }, iterations, samples);
                std::cout << MatrixData::csv(path.filename().string()) << ',' << source.size() << ','
                    << (lex ? "lex_only" : "parse") << ',' << iterations << ',' << samples << ','
                    << time.medianUs << ',' << time.minUs << ',' << time.maxUs << ',' << MATRIX_CONFIGURATION << '\n';
            }
        }
    } catch (const std::exception& ex) { std::cerr << ex.what() << '\n'; return 1; }
}
