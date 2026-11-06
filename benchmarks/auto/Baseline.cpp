#include "Compare.hpp"
#include "JsonLexer.hpp"
#include "JsonParser.hpp"
#include "LexerException.hpp"
#include "ParseException.hpp"

AutoCheck::Outcome AutoCheck::baseline(const std::string& source, bool lowMemory) {
    Outcome result;
    const ParseOptions options{lowMemory ? ParsePreset::LowMemory : ParsePreset::Balanced};
    try {
        result.tokens=JsonLexer(source).tokenize(options);
        result.canonical=JsonParser::parse(source,options).serialize(false);
        result.accepted=true;
    } catch (const LexerException& ex) {
        result.errorKind=1; result.error=ex.what(); result.line=ex.getLine(); result.column=ex.getColumn();
    } catch (const ParseException& ex) {
        result.errorKind=2; result.error=ex.what(); result.line=ex.getLine(); result.column=ex.getColumn();
    }
    return result;
}
