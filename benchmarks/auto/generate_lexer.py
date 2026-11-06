"""Generate private AutoLexer variants; JSON grammar remains production source."""
import sys
from pathlib import Path

source,destination=map(Path,sys.argv[1:])
text=source.read_text(encoding="utf-8-sig")
start=text.index("    std::vector<Token> tokens;",text.index("JsonLexer::tokenize()"))
end=text.index("    while (!isAtEnd())",start)
original=text[start:end]
assert original.count("tokens.push_back(token);")==1
assert "capacity > limit / 64 ? limit : capacity * 64" in original
auto='''    std::vector<Token> tokens;
    tokens.reserve((std::min)(source.size() / 4, size_t{256}));
    AutoReserve::Controller<AutoReserve::Goal::GOAL> controller(source.size(),position,tokens);
    auto append = [&](Token token) {
        if (tokens.size()==tokens.capacity() && tokens.size()<tokens.max_size())
            controller.grow(token.type==Parser::TokenType::EndOfFile);
        tokens.push_back(token);
    };

'''
destination.mkdir(parents=True,exist_ok=True)
for name,block in (("speed",auto.replace("GOAL","Speed")),("memory",auto.replace("GOAL","Memory")),("fixed",original)):
    block=block.replace("    auto append = [&](Token token) {",'''#ifdef MASKETJSON_RESERVATION_STATS
    if (ReservationStats::active) {
        ReservationStats::active->initialCapacity=tokens.capacity();
        ReservationStats::active->bufferAllocations=tokens.capacity()!=0;
    }
#endif
    auto append = [&](Token token) {
#ifdef MASKETJSON_RESERVATION_STATS
        const size_t observedCapacity=tokens.capacity(), observedSize=tokens.size();
#endif''')
    block=block.replace("        tokens.push_back(token);",'''        tokens.push_back(token);
#ifdef MASKETJSON_RESERVATION_STATS
        if (tokens.capacity()!=observedCapacity && ReservationStats::active) {
            ++ReservationStats::active->growthEvents;
            ++ReservationStats::active->bufferAllocations;
            ReservationStats::active->copiedTokens+=observedSize;
        }
#endif''')
    generated=text[:start]+block+text[end:]
    if name!="fixed":
        assert generated.count("    return tokens;")==1
        generated=generated.replace("    return tokens;","    controller.finish();\n    return tokens;")
    generated='#include "AutoReservation.hpp"\n#ifdef MASKETJSON_RESERVATION_STATS\n#include "ReservationStats.hpp"\n#endif\n'+generated
    (destination/(name+".cpp")).write_bytes(generated.encode())
