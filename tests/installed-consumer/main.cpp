#include <JsonHandlerCPP/JsonHandler.hpp>
#include <JsonHandlerCPP/JsonParser.hpp>
#include <JsonHandlerCPP/JsonLexer.hpp>
#include <JsonHandlerCPP/JsonSerializer.hpp>
#include <JsonHandlerCPP/JsonFile.hpp>
#include <JsonHandlerCPP/JsonAlgorithms.hpp>
#include <JsonHandlerCPP/AccessException.hpp>
#include <JsonHandlerCPP/FileException.hpp>
#include <JsonHandlerCPP/LexerException.hpp>
#include <JsonHandlerCPP/OperationException.hpp>
#include <JsonHandlerCPP/ParseException.hpp>
#include <JsonHandlerCPP/TypeException.hpp>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

#if CONSUMER_EXPECT_SHARED
#ifdef JSONHANDLER_STATIC
#error "Shared package incorrectly propagated JSONHANDLER_STATIC"
#endif
#else
#ifndef JSONHANDLER_STATIC
#error "Static package did not propagate JSONHANDLER_STATIC"
#endif
#endif

bool legacyHeadersWork();
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try {
        require(legacyHeadersWork(), "Legacy include paths failed");
        const auto value = JsonParser::parse(R"({"id":18446744073709551615,"text":"001","emoji":"\uD83D\uDE00"})");
        require(value.at("id").asUInt64() == (std::numeric_limits<std::uint64_t>::max)(),
            "Exact integer API failed");
        require(JsonParser::parse(JsonSerializer::serialize(value, false)) == value,
            "Installed serializer/parser round-trip failed");
        Json array(Json::JsonArray{});
        array.push_back(1, "two", true);
        require(array.size() == 3, "Header-defined variadic push_back failed");
        Json rows(Json::JsonArray{Json::JsonObject{{"id", 2}}, Json::JsonObject{{"id", 1}}});
        JsonAlgorithms::sortByPath(rows, "id", true);
        require(rows.at(0).at("id").asInt64() == 1, "Installed algorithm failed");
        const auto tokens = JsonLexer("[true]").tokenize();
        require(tokens.size() == 4 && tokens.at(1).type == Parser::TokenType::True,
            "Installed lexer failed");
        try {
            value.at("missing");
            throw std::runtime_error("Missing-key exception did not cross library boundary");
        } catch (const AccessException& error) {
            require(std::string(error.what()).find("missing") != std::string::npos,
                "Missing-key diagnostic was lost");
        }
        try {
            JsonParser::parse("[1,]");
            throw std::runtime_error("Parse exception did not cross library boundary");
        } catch (const ParseException& error) {
            require(error.getLine() == 1 && error.getColumn() == 4, "Location getters failed");
        }
        // Exercise exported constructors/getters from every public exception type.
        const LexerException lexerError("probe", 2, 3);
        require(lexerError.getLine() == 2 && lexerError.getColumn() == 3, "Lexer getters failed");
        const JsonException* errors[] = {&lexerError};
        require(std::string(errors[0]->what()).find("probe") != std::string::npos, "Base exception failed");
        require(std::string(TypeException("type").what()).find("type") != std::string::npos, "Type exception failed");
        require(std::string(FileException("file").what()).find("file") != std::string::npos, "File exception failed");
        require(std::string(OperationException("operation").what()).find("operation") != std::string::npos,
            "Operation exception failed");
        const auto file = std::filesystem::path("installed-consumer-round-trip.json");
        JsonFile::write(file.string(), value);
        require(JsonFile::read(file.string()) == value, "Installed file I/O failed");
        require(std::filesystem::remove(file), "Failed to remove consumer output");
        std::cout << "Installed package works\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
