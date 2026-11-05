#include "JsonParser.hpp"
#include "JsonLexer.hpp"
#include "BenchmarkUtils.hpp"
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#ifdef _WIN32
#include <malloc.h>
#endif

namespace {
struct Allocations { size_t calls = 0; size_t bytes = 0; };
thread_local Allocations* activeAllocations = nullptr;

void recordAllocation(size_t bytes)
{
    if (activeAllocations) {
        ++activeAllocations->calls;
        activeAllocations->bytes += bytes;
    }
}

void* allocate(size_t bytes)
{
    while (true) {
        if (void* pointer = std::malloc(bytes ? bytes : 1)) {
            recordAllocation(bytes);
            return pointer;
        }
        if (auto handler = std::get_new_handler()) handler();
        else throw std::bad_alloc();
    }
}

template<class Operation>
void profile(const char* stage, size_t iterations, Operation operation)
{
    BenchmarkUtils::consume(operation());
    Allocations allocations;
    struct Capture {
        explicit Capture(Allocations& stats) { activeAllocations = &stats; }
        ~Capture() { activeAllocations = nullptr; }
    };
    {
        Capture capture(allocations);
        BenchmarkUtils::consume(operation());
    }
    const auto timing = BenchmarkUtils::measure(operation, iterations, 9);
    std::cout << stage << ',' << std::fixed << std::setprecision(3) << timing.medianUs
        << ',' << timing.minUs << ',' << timing.maxUs << ',' << allocations.calls
        << ',' << allocations.bytes << '\n';
}
}

// Isolated executable-level instrumentation. Production targets never link
// these replacement operators. Counts cover ordinary C++ new/new[], not malloc
// inside the CRT or aligned allocations (the JSON value/token types use neither).
void* operator new(size_t size) { return allocate(size); }
void* operator new[](size_t size) { return allocate(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, size_t) noexcept { std::free(pointer); }

int main(int argc, char** argv)
{
    try {
        if (argc < 2 || argc > 3)
            throw std::invalid_argument("Usage: ParserProfile INPUT [ITERATIONS]");
        size_t iterations = 100;
        if (argc == 3) {
            const std::string_view arg(argv[2]);
            const auto result = std::from_chars(arg.data(), arg.data() + arg.size(), iterations);
            if (result.ec != std::errc{} || result.ptr != arg.data() + arg.size() ||
                iterations == 0 || iterations > 1000000)
                throw std::invalid_argument("Invalid iteration count");
        }
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("Unable to open profile input");
        const std::string source((std::istreambuf_iterator<char>(file)), {});
        if (file.bad()) throw std::runtime_error("Unable to read profile input");
        {
            const auto tokens = JsonLexer(source).tokenize();
            std::cerr << "bytes=" << source.size() << " tokens=" << tokens.size()
                << " token_size=" << sizeof(Token) << " token_capacity=" << tokens.capacity()
                << " json_nothrow_move=" << std::is_nothrow_move_constructible_v<Json> << '\n';
        }
        std::cout << "stage,median_us,min_us,max_us,new_calls,new_bytes\n";
        profile("whole_parse", iterations, [&] {
            const auto value = JsonParser::parse(source);
            return value.isObject() || value.isArray() ? value.size() : size_t{1};
        });
        profile("lex_only", iterations, [&] {
            const auto tokens = JsonLexer(source).tokenize();
            return tokens.size();
        });
        profile("input_copy", iterations, [&] {
            const auto copy = source;
            return copy.size() + (copy.empty() ? 0 : static_cast<unsigned char>(copy.front()));
        });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
