#include "JsonParser.hpp"
#include "JsonLexer.hpp"
#include "ParseException.hpp"
#include "MemoryTracker.hpp"
#ifdef MASKETJSON_RESERVATION_STATS
#include "ReservationStats.hpp"
#endif
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <locale>
#include <numeric>
#include <stdexcept>

namespace {
using MemoryTracker::Category;
Json parseInput(const std::string& source) {
#ifdef MATRIX_LOW_MEMORY
    return JsonParser::parse(source, ParseOptions{ParsePreset::LowMemory});
#else
    return JsonParser::parse(source);
#endif
}
std::vector<Token> tokenizeInput(const std::string& source) {
#ifdef MATRIX_LOW_MEMORY
    return JsonLexer(source).tokenize(ParseOptions{ParsePreset::LowMemory});
#else
    return JsonLexer(source).tokenize();
#endif
}
struct Inventory {
    size_t nodes=0, members=0, elements=0, capacity=0, strings=0, chars=0, stringCapacity=0;
};
struct Row {
    const char* stage;
    MemoryTracker::Stats memory{}, after{};
    std::array<std::uint64_t, static_cast<size_t>(Category::Count)> categories{};
    Inventory dom{};
    size_t tokens=0, tokenCapacity=0;
#ifdef MASKETJSON_RESERVATION_STATS
    ReservationStats::Counters reservation{};
#endif
};
void inspect(const Json& value, MemoryTracker::LiveSet& blocks, Inventory& data) {
    ++data.nodes;
    if (value.isObject()) {
        data.members += value.asObject().size();
        for (const auto& member : value.asObject()) {
            blocks.containing(&member, Category::MapNode);
            blocks.containing(member.first.data(), Category::String);
            data.chars += member.first.size();
            data.stringCapacity += member.first.capacity();
            inspect(member.second, blocks, data);
        }
    } else if (value.isArray()) {
        const auto& array = value.asArray();
        data.elements += array.size(); data.capacity += array.capacity();
        blocks.containing(array.data(), Category::Array);
        for (const auto& child : array) inspect(child, blocks, data);
    } else if (value.isString()) {
        const auto& string = value.asString();
        ++data.strings; data.chars += string.size(); data.stringCapacity += string.capacity();
        // Containers are tagged first: SSO cannot reclassify its owner.
        blocks.containing(string.data(), Category::String);
    }
}
Row profile(const std::string& source, unsigned stage) {
    Row row{stage==0 ? "whole_parse" : stage==1 ? "lex_only" : "input_copy"};
#ifdef MASKETJSON_RESERVATION_STATS
    ReservationStats::Scope reservationScope(row.reservation);
#endif
    MemoryTracker::Session session;
    if (stage==0) {
        const auto value = parseInput(source);
        MemoryTracker::LiveSet blocks(session);
        inspect(value, blocks, row.dom);
        row.memory=blocks.stats(); row.categories=blocks.categories();
    } else if (stage==1) {
        // Temporary lexer/source copy die here; only the tokens remain.
        const auto tokens=tokenizeInput(source);
        MemoryTracker::LiveSet blocks(session);
        blocks.containing(tokens.data(), Category::Token);
        row.tokens=tokens.size(); row.tokenCapacity=tokens.capacity();
        row.memory=blocks.stats(); row.categories=blocks.categories();
    } else {
        const auto copy=source;
        MemoryTracker::LiveSet blocks(session);
        blocks.containing(copy.data(), Category::Input);
        row.memory=blocks.stats(); row.categories=blocks.categories();
    }
    row.after=session.snapshot();
    if (row.after.liveBytes || row.after.liveBlocks || row.memory.peakBytes<row.memory.liveBytes ||
        std::accumulate(row.categories.begin(),row.categories.end(),std::uint64_t{0})!=row.memory.liveBytes)
        throw std::runtime_error("Memory accounting invariant failed");
    return row;
}
void selfTest() {
    MemoryTracker::selfTest();
    const std::string source="{\"array\":[\"short\",\""+std::string(1024,'x')+"\"],\"flag\":true}";
    { const auto warm=parseInput(source); (void)warm; }
    const auto row=profile(source,0);
    if (row.dom.nodes!=5 || !row.categories[static_cast<size_t>(Category::Array)] ||
        row.categories[static_cast<size_t>(Category::String)]<1024 ||
        !row.categories[static_cast<size_t>(Category::MapNode)])
        throw std::runtime_error("Category accounting self-test failed");
    const std::string invalid="{\"array\":[1,}";
    MemoryTracker::Stats after; bool rejected=false;
    {
        MemoryTracker::Session session;
        try { (void)parseInput(invalid); } catch (const ParseException&) { rejected=true; }
        after=session.snapshot();
    }
    if (!rejected || after.liveBytes || after.liveBlocks) throw std::runtime_error("Unwind self-test failed");
}
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && std::string(argv[1])=="--self-test") {
            selfTest(); std::cout<<"Memory tracker self-test passed\n"; return 0;
        }
        if (argc<2) throw std::invalid_argument("Usage: ParserProfile INPUT... | --self-test (memory only)");
        std::cout.imbue(std::locale::classic());
        std::cout<<"dataset,input_bytes,stage,new_calls,new_bytes,aligned_calls,peak_live_bytes,peak_live_blocks,"
            "retained_bytes,retained_blocks,after_destroy_bytes,after_destroy_blocks,other_bytes,string_bytes,"
            "array_bytes,map_node_bytes,token_bytes,input_copy_bytes,dom_nodes,object_members,array_elements,"
            "array_capacity,string_values,string_chars,string_capacity_chars,token_count,token_capacity,token_size,json_size";
#ifdef MASKETJSON_RESERVATION_STATS
        std::cout<<",initial_token_capacity,token_buffer_allocations,token_growth_events,copied_token_bytes";
#endif
        std::cout<<'\n';
        for(int i=1;i<argc;++i) {
            const std::filesystem::path path(argv[i]);
            std::ifstream file(path,std::ios::binary);
            if(!file) throw std::runtime_error("Unable to open profile input");
            const std::string source((std::istreambuf_iterator<char>(file)),{});
            if(file.bad()) throw std::runtime_error("Unable to read profile input");
            { const auto value=parseInput(source);
              if(!(parseInput(value.serialize(false))==value)) throw std::runtime_error("Round-trip check failed"); }
            const std::array<Row,3> rows{profile(source,0),profile(source,1),profile(source,2)};
            for(const auto& row:rows) {
                std::cout<<'"'; for(char c:path.filename().string()) {std::cout<<c;if(c=='"')std::cout<<'"';}
                const auto& m=row.memory; const auto& d=row.dom;
                std::cout<<"\","<<source.size()<<','<<row.stage<<','<<m.calls<<','<<m.bytes<<','<<m.alignedCalls
                    <<','<<m.peakBytes<<','<<m.peakBlocks<<','<<m.liveBytes<<','<<m.liveBlocks
                    <<','<<row.after.liveBytes<<','<<row.after.liveBlocks;
                for(auto bytes:row.categories) std::cout<<','<<bytes;
                std::cout<<','<<d.nodes<<','<<d.members<<','<<d.elements<<','<<d.capacity<<','<<d.strings<<','<<d.chars
                    <<','<<d.stringCapacity<<','<<rows[1].tokens<<','<<rows[1].tokenCapacity<<','<<sizeof(Token)<<','<<sizeof(Json);
#ifdef MASKETJSON_RESERVATION_STATS
                const auto& r=row.reservation;
                std::cout<<','<<r.initialCapacity<<','<<r.bufferAllocations<<','<<r.growthEvents<<','<<r.copiedTokens*sizeof(Token);
#endif
                std::cout<<'\n';
            }
        }
        return std::cout?0:1;
    }catch(const std::exception& error){std::cerr<<"Memory profile failed: "<<error.what()<<'\n';return 1;}
}
