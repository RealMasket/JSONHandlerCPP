#include "JsonParser.hpp"
#include "JsonFile.hpp"
#include "JsonException.hpp"
#include "LexerException.hpp"
#include "ParseException.hpp"
#include "FileException.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#endif

namespace {
constexpr const char* usage =
    "Usage:\n"
    "  jsonhandler check INPUT\n"
    "  jsonhandler format [--compact] [--output FILE] INPUT\n"
    "  jsonhandler --help\n"
    "\nINPUT is a file path or '-' for standard input.\n"
    "Format writes to standard output unless --output (or -o) is given.\n"
    "Use '--' before a file name beginning with '-'.\n"
    "Exit codes: 0 success, 1 invalid JSON, 2 usage error, 3 I/O error, 4 internal error.\n";

struct Options {
    std::string command;
    std::string input;
    std::string output;
    bool compact = false;
};

Options parseOptions(int argc, char** argv)
{
    if (argc < 2) throw std::invalid_argument("Missing command");
    Options options;
    options.command = argv[1];
    if (options.command != "check" && options.command != "format")
        throw std::invalid_argument("Unknown command: " + options.command);
    bool positionalOnly = false;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (!positionalOnly && arg == "--") { positionalOnly = true; continue; }
        if (!positionalOnly && arg == "--compact") {
            if (options.command != "format") throw std::invalid_argument("--compact requires format");
            options.compact = true;
        } else if (!positionalOnly && (arg == "--output" || arg == "-o")) {
            if (options.command != "format" || !options.output.empty() || i + 1 >= argc)
                throw std::invalid_argument("format accepts one --output FILE");
            options.output = argv[++i];
            if (options.output.empty()) throw std::invalid_argument("Empty output path");
        } else if (!positionalOnly && arg.size() > 1 && arg.front() == '-') {
            throw std::invalid_argument("Unknown option: " + arg);
        } else {
            if (!options.input.empty()) throw std::invalid_argument("Expected exactly one input");
            options.input = arg;
        }
    }
    if (options.input.empty()) throw std::invalid_argument("Missing input file or '-'");
    return options;
}

Json readInput(const std::string& input)
{
    if (input != "-") return JsonFile::read(input);
#ifdef _WIN32
    // Pipes must preserve raw bytes, including Ctrl-Z, for strict validation.
    if (_setmode(_fileno(stdin), _O_BINARY) == -1)
        throw FileException("Unable to set standard input to binary mode");
#endif
    std::string text;
    char buffer[4096];
    while (std::cin.read(buffer, sizeof(buffer))) text.append(buffer, sizeof(buffer));
    text.append(buffer, static_cast<size_t>(std::cin.gcount()));
    if (std::cin.bad() || !std::cin.eof()) throw FileException("Unable to read standard input");
    return JsonParser::parse(text);
}

void writeOutput(const std::string& path, const std::string& text)
{
    if (path.empty()) {
        std::cout << text << '\n';
        std::cout.flush();
        if (!std::cout) throw FileException("Unable to write standard output");
        return;
    }
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) throw FileException("Unable to open output file: " + path);
    file << text << '\n';
    file.flush();
    if (!file) throw FileException("Unable to write output file: " + path);
    file.close();
    if (file.fail()) throw FileException("Unable to close output file: " + path);
}
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--help") {
        std::cout << usage;
        return std::cout ? 0 : 3;
    }
    try {
        const auto options = parseOptions(argc, argv);
        const Json value = readInput(options.input);
        if (options.command == "check") writeOutput({}, "Valid JSON");
        else writeOutput(options.output, value.serialize(!options.compact));
        return 0;
    } catch (const std::invalid_argument& error) {
        std::cerr << "jsonhandler: " << error.what() << '\n' << usage;
        return 2;
    } catch (const LexerException& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (const ParseException& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (const FileException& error) {
        std::cerr << error.what() << '\n';
        return 3;
    } catch (const std::exception& error) {
        std::cerr << "jsonhandler: " << error.what() << '\n';
        return 4;
    }
}
