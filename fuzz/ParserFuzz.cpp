#include "FuzzChecks.hpp"

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    JsonFuzz::checkInput(data, size);
    // Invalid source still exercises lexer/diagnostics and belongs in the corpus.
    return 0;
}
