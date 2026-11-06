"""Generate benchmark-only allocation variants; preserve the JSON grammar verbatim."""
import hashlib
import sys
from pathlib import Path

source, destination = map(Path, sys.argv[1:])
text = source.read_text(encoding="utf-8-sig")
begin = text.index("    std::vector<Token> tokens;", text.index("JsonLexer::tokenize()"))
end = text.index("    while (!isAtEnd())", begin)
original = text[begin:end]
# Fail closed when the production allocation algorithm changes: reassess variants.
assert "capacity > limit / 64 ? limit : capacity * 64" in original
assert original.count("tokens.push_back(token);") == 1
simple = '''    std::vector<Token> tokens;
    tokens.reserve(INITIAL);
    auto append = [&](Token token) {
        tokens.push_back(token);
    };

'''
policies = {
    "byte-quarter": simple.replace("INITIAL", "source.size() / 4"),
    "geometric": simple.replace("INITIAL", "(std::min)(source.size() / 4, size_t{256})"),
    "adaptive4": original.replace("limit / 64 ? limit : capacity * 64", "limit / 4 ? limit : capacity * 4"),
    "adaptive64": original,
}
destination.mkdir(parents=True, exist_ok=True)
for name, block in policies.items():
    # Counter code exists only in instrumented profiles, never ordinary timings.
    block = block.replace("    auto append = [&](Token token) {", '''#ifdef MASKETJSON_RESERVATION_STATS
    if (ReservationStats::active) {
        ReservationStats::active->initialCapacity = tokens.capacity();
        ReservationStats::active->bufferAllocations = tokens.capacity() != 0;
    }
#endif
    auto append = [&](Token token) {
#ifdef MASKETJSON_RESERVATION_STATS
        const size_t observedCapacity = tokens.capacity(), observedSize = tokens.size();
#endif''')
    block = block.replace("        tokens.push_back(token);", '''        tokens.push_back(token);
#ifdef MASKETJSON_RESERVATION_STATS
        if (tokens.capacity() != observedCapacity && ReservationStats::active) {
            ++ReservationStats::active->growthEvents;
            ++ReservationStats::active->bufferAllocations;
            ReservationStats::active->copiedTokens += observedSize;
        }
#endif''')
    generated = text[:begin] + block + text[end:]
    generated = '#ifdef MASKETJSON_RESERVATION_STATS\n#include "ReservationStats.hpp"\n#endif\n' + generated
    (destination / f"{name}.cpp").write_text(generated, encoding="utf-8")
(destination / "source.sha256").write_text(hashlib.sha256(text.encode()).hexdigest() + "\n", encoding="ascii")
