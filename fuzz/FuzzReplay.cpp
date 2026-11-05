#include "FuzzChecks.hpp"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using Bytes = std::vector<std::uint8_t>;
struct Options {
    std::uint64_t seed = 20261105;
    std::uint64_t mutations = 0;
    std::filesystem::path artifacts = "fuzz-artifacts";
    std::vector<std::filesystem::path> paths;
    bool requireValid = false;
};
constexpr const char* usage =
    "Usage: JsonFuzzReplay [--mutations N] [--seed N] [--artifacts DIR]\n"
    "                      [--require-valid] FILE_OR_DIRECTORY...\n"
    "Replay each file (directories are recursive) and optionally mutate seeds.\n"
    "Maximum input size: 65536 bytes. Default seed: 20261105.\n"
    "--require-valid applies to original corpus files, not random mutations.\n"
    "Exit: 0 successful checks (invalid JSON allowed), 1 failure, 2 usage error.\n";

std::uint64_t count(std::string_view text, std::uint64_t maximum)
{
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value > maximum)
        throw std::invalid_argument("Invalid numeric option");
    return value;
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    bool positionalOnly = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (!positionalOnly && arg == "--") { positionalOnly = true; continue; }
        if (!positionalOnly && arg == "--require-valid") options.requireValid = true;
        else if (!positionalOnly && (arg == "--mutations" || arg == "--seed" || arg == "--artifacts")) {
            if (++i >= argc) throw std::invalid_argument("Missing option value");
            if (arg == "--mutations") options.mutations = count(argv[i], 10000000);
            else if (arg == "--seed") options.seed = count(argv[i], (std::numeric_limits<std::uint64_t>::max)());
            else options.artifacts = argv[i];
        } else if (!positionalOnly && !arg.empty() && arg.front() == '-')
            throw std::invalid_argument("Unknown option: " + arg);
        else options.paths.emplace_back(arg);
    }
    if (options.paths.empty()) throw std::invalid_argument("Provide a corpus file or directory");
    return options;
}

std::vector<std::filesystem::path> corpusFiles(const Options& options)
{
    std::vector<std::filesystem::path> files;
    for (const auto& path : options.paths) {
        if (std::filesystem::is_regular_file(path)) files.push_back(path);
        else if (std::filesystem::is_directory(path)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
                if (entry.is_regular_file()) files.push_back(entry.path());
        } else throw std::runtime_error("Corpus path not found: " + path.string());
    }
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    if (files.empty()) throw std::runtime_error("Corpus contains no files");
    return files;
}

Bytes readInput(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open: " + path.string());
    Bytes bytes(JsonFuzz::maxInputSize + 1);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (file.bad()) throw std::runtime_error("Cannot read: " + path.string());
    bytes.resize(static_cast<size_t>(file.gcount()));
    if (bytes.size() > JsonFuzz::maxInputSize)
        throw std::runtime_error("Input exceeds 64 KiB: " + path.string());
    return bytes;
}

void saveFailure(const Options& options, const Bytes& input, const std::string& stage,
    const std::string& message)
{
    std::filesystem::create_directories(options.artifacts);
    const auto path = options.artifacts / "reproducer.bin";
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot save reproducer");
    if (!input.empty())
        file.write(reinterpret_cast<const char*>(input.data()), static_cast<std::streamsize>(input.size()));
    file.close();
    if (!file) throw std::runtime_error("Failed to save reproducer");
    std::ofstream details(options.artifacts / "failure.txt");
    details << "seed=" << options.seed << "\nstage=" << stage << "\nbytes=" << input.size()
        << "\nerror=" << message << '\n';
    details.close();
    if (!details) throw std::runtime_error("Failed to save reproducer metadata");
    std::cerr << "Saved reproducer: " << path.string() << '\n';
}

bool execute(const Options& options, const Bytes& input, const std::string& stage, bool requireValid)
{
    try {
        const bool accepted = JsonFuzz::checkInput(input.data(), input.size());
        if (requireValid && !accepted)
            throw std::runtime_error("Regression input was unexpectedly rejected");
        return accepted;
    } catch (const std::exception& error) {
        saveFailure(options, input, stage, error.what());
        throw;
    } catch (...) {
        saveFailure(options, input, stage, "Non-standard exception");
        throw;
    }
}

Bytes mutate(Bytes bytes, std::mt19937_64& random)
{
    static constexpr const char* dictionary[] = {
        "null", "true", "false", "0", "-0.0", "1.23456789", "1e999",
        "18446744073709551615", "-9223372036854775808", "18446744073709551616",
        "\"text\"", "[]", "{}", "[{},[],null]", "{\"v\":1}", "\"\\uD83D\\uDE00\"",
        "\\uD800", "\\uDC00", "\\u0000", "\"", "\\", ":", ",", "\n"
    };
    switch (random() % 7) {
    case 0:
        if (bytes.empty()) bytes.push_back(static_cast<std::uint8_t>(random()));
        else bytes[random() % bytes.size()] ^= static_cast<std::uint8_t>(1u << (random() % 8));
        break;
    case 1:
        if (bytes.size() < JsonFuzz::maxInputSize)
            bytes.insert(bytes.begin() + random() % (bytes.size() + 1), static_cast<std::uint8_t>(random()));
        break;
    case 2:
        if (!bytes.empty()) {
            const size_t begin = random() % bytes.size();
            const size_t length = 1 + random() % (bytes.size() - begin);
            bytes.erase(bytes.begin() + begin, bytes.begin() + begin + length);
        }
        break;
    case 3: {
        const std::string_view token = dictionary[random() % (sizeof(dictionary) / sizeof(*dictionary))];
        if (bytes.size() + token.size() <= JsonFuzz::maxInputSize)
            bytes.insert(bytes.begin() + random() % (bytes.size() + 1), token.begin(), token.end());
        break;
    }
    case 4:
        if (!bytes.empty()) {
            const size_t begin = random() % bytes.size();
            const size_t length = std::min<size_t>(32, bytes.size() - begin);
            if (bytes.size() + length <= JsonFuzz::maxInputSize) {
                const Bytes copy(bytes.begin() + begin, bytes.begin() + begin + length);
                bytes.insert(bytes.begin() + random() % (bytes.size() + 1), copy.begin(), copy.end());
            }
        }
        break;
    case 5:
        if (bytes.size() + 2 <= JsonFuzz::maxInputSize) {
            bytes.insert(bytes.begin(), '[');
            bytes.push_back(']');
        }
        break;
    default: {
        const std::string_view token = dictionary[random() % (sizeof(dictionary) / sizeof(*dictionary))];
        bytes.assign(token.begin(), token.end());
        break;
    }
    }
    return bytes;
}
}

int main(int argc, char** argv)
{
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") { std::cout << usage; return 0; }
        const auto options = parseOptions(argc, argv);
        const auto files = corpusFiles(options);
        std::vector<Bytes> pool;
        size_t accepted = 0, rejected = 0;
        for (const auto& file : files) {
            auto bytes = readInput(file);
            if (execute(options, bytes, "corpus:" + file.string(), options.requireValid)) ++accepted;
            else ++rejected;
            pool.push_back(std::move(bytes));
        }
        const size_t originalCount = pool.size();
        std::mt19937_64 random(options.seed);
        for (std::uint64_t i = 0; i < options.mutations; ++i) {
            auto bytes = mutate(pool[random() % pool.size()], random);
            if (execute(options, bytes, "mutation:" + std::to_string(i), false)) ++accepted;
            else ++rejected;
            if (i % 32 == 0) {
                if (pool.size() < originalCount + 256) pool.push_back(std::move(bytes));
                else pool[originalCount + random() % 256] = std::move(bytes);
            }
        }
        std::cout << "seed=" << options.seed << " corpus=" << files.size()
            << " mutations=" << options.mutations << " accepted=" << accepted
            << " rejected=" << rejected << '\n';
        return 0;
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n' << usage;
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "Fuzz check failed: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Fuzz check failed: non-standard exception\n";
        return 1;
    }
}
