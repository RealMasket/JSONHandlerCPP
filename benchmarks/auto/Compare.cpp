#include "Compare.hpp"
#include "AutoReservation.hpp"
#include "JsonLexer.hpp"
#include "JsonParser.hpp"
#include "LexerException.hpp"
#include "ParseException.hpp"
#include "FuzzChecks.hpp"
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void same(const AutoCheck::Outcome& a,const AutoCheck::Outcome& b) {
    require(a.accepted==b.accepted,"Auto changed input acceptance");
    require(a.errorKind==b.errorKind && a.error==b.error && a.line==b.line && a.column==b.column,"Auto changed diagnostics");
    require(a.canonical==b.canonical,"Auto changed canonical value/number storage");
    require(a.tokens.size()==b.tokens.size(),"Auto changed token count");
    for (size_t i=0;i<a.tokens.size();++i) {
        const auto& x=a.tokens[i]; const auto& y=b.tokens[i];
        require(x.type==y.type && x.position==y.position && x.line==y.line && x.column==y.column
            && x.start==y.start && x.length==y.length,"Auto changed token metadata");
    }
}
}
bool AutoCheck::compare(const std::string& source) {
    Outcome observed;
    try {
        observed.tokens=JsonLexer(source).tokenize();
        const auto value=JsonParser::parse(source);
        observed.canonical=value.serialize(false);
        require(JsonParser::parse(observed.canonical).serialize(false)==observed.canonical,"Auto compact round-trip changed storage");
        require(JsonParser::parse(value.serialize(true)).serialize(false)==observed.canonical,"Auto pretty round-trip changed storage");
        observed.accepted=true;
    } catch (const LexerException& ex) {
        observed.errorKind=1; observed.error=ex.what(); observed.line=ex.getLine(); observed.column=ex.getColumn();
    } catch (const ParseException& ex) {
        observed.errorKind=2; observed.error=ex.what(); observed.line=ex.getLine(); observed.column=ex.getColumn();
    }
    same(observed,baseline(source,false));
    same(observed,baseline(source,true));
    return observed.accepted;
}
void AutoCheck::selfTest() {
    using namespace AutoReserve;
    const size_t maximum=(std::numeric_limits<size_t>::max)();
    for (const auto goal : {Goal::Speed,Goal::Memory})
        for (size_t limit : {size_t{0},size_t{1},size_t{1000},maximum/sizeof(Token),maximum})
            for (size_t capacity : {size_t{0},limit/2,limit ? limit-1 : size_t{0},limit})
                for (bool mature : {false,true}) {
                    const auto range=bounds(goal,capacity,limit,mature);
                    require(capacity<=range.lower && range.lower<=range.upper && range.upper<=limit,"Auto bound overflow");
                    if (capacity<limit) require(range.lower>capacity,"Auto cannot progress");
                    for (double prediction : {0.0,10.0,static_cast<double>(limit),(std::numeric_limits<double>::infinity)()}) {
                        const auto hint=clampPrediction(prediction,range);
                        require(hint>=range.lower && hint<=range.upper,"Auto prediction out of bounds");
                    }
                }
    const std::vector<std::string> inputs{"", " \n\t", "[1,]", "01", "1e+", "\"\\uD800\"", "\"\xFF\"",
        "[-9223372036854775808,18446744073709551615,-0,-0.0,1.0]",
        std::string(128,'[')+"0"+std::string(128,']'),std::string(129,'[')+"0"+std::string(129,']')};
    for (const auto& input:inputs) (void)compare(input);
}
bool JsonFuzz::checkInput(const std::uint8_t* bytes, std::size_t size) {
    if (size>maxInputSize) throw std::invalid_argument("Auto fuzz input exceeds limit");
    return AutoCheck::compare(size ? std::string(reinterpret_cast<const char*>(bytes),size) : std::string{});
}
