#include "JsonHandler.hpp"
#include "JsonParser.hpp"
#include "BenchmarkUtils.hpp"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef MASKETJSON_HAS_BOOST
#include <boost/json.hpp>
#include <boost/version.hpp>
#endif
#ifdef MASKETJSON_HAS_QT
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QtGlobal>
#endif
#ifndef MASKETJSON_BENCHMARK_BUILD_TYPE
#define MASKETJSON_BENCHMARK_BUILD_TYPE "unspecified"
#endif
#ifndef MASKETJSON_BENCHMARK_VERSION
#define MASKETJSON_BENCHMARK_VERSION "unversioned"
#endif

namespace {
constexpr const char* usage =
    "Usage:\n"
    "  ParseBenchmark --generate DIRECTORY\n"
    "  ParseBenchmark [--iterations N] [--samples N] [--csv FILE] INPUT...\n"
    "\nDefault: 100 iterations per sample, 7 samples, 5 warm-up calls per operation.\n"
    "Use Release builds. File reading and correctness checks are not timed.\n";

struct Options {
    size_t iterations = 100;
    size_t samples = 7;
    std::string csv;
    std::vector<std::filesystem::path> files;
};
struct Row {
    std::string dataset;
    std::string library;
    std::string version;
    std::string operation;
    size_t bytes;
    BenchmarkMeasurement time;
};

size_t positiveCount(std::string_view value, size_t maximum)
{
    size_t count = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), count);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
        count == 0 || count > maximum)
        throw std::invalid_argument("Invalid positive iteration/sample count");
    return count;
}

Options parseOptions(int argc, char** argv)
{
    Options options;
    bool positionalOnly = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (!positionalOnly && arg == "--") { positionalOnly = true; continue; }
        if (!positionalOnly && (arg == "--iterations" || arg == "--samples" || arg == "--csv")) {
            if (++i >= argc) throw std::invalid_argument("Missing value after " + arg);
            if (arg == "--iterations") options.iterations = positiveCount(argv[i], 10000000);
            else if (arg == "--samples") options.samples = positiveCount(argv[i], 101);
            else options.csv = argv[i];
        } else if (!positionalOnly && !arg.empty() && arg.front() == '-') {
            throw std::invalid_argument("Unknown option: " + arg);
        } else options.files.emplace_back(arg);
    }
    if (options.files.empty()) throw std::invalid_argument("At least one input file is required");
    return options;
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Unable to open input: " + path.string());
    std::string text;
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) text.append(buffer, sizeof(buffer));
    text.append(buffer, static_cast<size_t>(file.gcount()));
    if (file.bad() || !file.eof()) throw std::runtime_error("Unable to read input: " + path.string());
    return text;
}

void writeFile(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Unable to open output: " + path.string());
    file << text;
    file.flush();
    if (!file) throw std::runtime_error("Unable to write output: " + path.string());
    file.close();
    if (file.fail()) throw std::runtime_error("Unable to close output: " + path.string());
}

std::string records(size_t count)
{
    std::string text = "[";
    for (size_t i = 0; i < count; ++i) {
        if (i) text += ',';
        text += "{\"id\":" + std::to_string(i) +
            ",\"code\":\"001\",\"name\":\"student-" + std::to_string(i) +
            "\",\"score\":1.25,\"active\":true,\"tags\":[\"C++\",\"JSON\"],"
            "\"message\":\"line\\ntext\\t\\u041F\\uD83D\\uDE00\"}";
    }
    text += ']';
    return text;
}

void generate(const std::filesystem::path& directory)
{
    std::filesystem::create_directories(directory);
    const auto writeDataset = [&](const char* name, const std::string& text) {
        writeFile(directory / name, text);
        std::cout << name << ": " << text.size() << " bytes\n";
    };
    writeDataset("tiny.json", R"({"id":"001","name":"Ada","active":true,"scores":[5,4.5,null]})");
    writeDataset("small.json", records(64));
    writeDataset("medium.json", records(1024));
    writeDataset("large.json", records(8192));
    std::string nested = "0";
    for (int i = 0; i < 64; ++i) nested = "{\"nested\":[" + nested + "]}";
    writeDataset("nested.json", nested);
}

std::string compilerName()
{
#if defined(__clang__)
    return std::string("Clang ") + __clang_version__;
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
#elif defined(__GNUC__)
    return std::string("GCC ") + __VERSION__;
#else
    return "unknown";
#endif
}

std::string csvQuote(const std::string& value)
{
    std::string quoted = "\"";
    for (char c : value) { quoted += c; if (c == '"') quoted += '"'; }
    return quoted + '"';
}

size_t rootSize(const Json& value)
{
    return value.isArray() || value.isObject() ? value.size() : 1;
}

template<class Operation>
void measure(std::vector<Row>& rows, const Options& options, const std::string& dataset,
    size_t bytes, const char* library, const char* version, const char* operation, Operation action)
{
    const auto result = BenchmarkUtils::measure(action, options.iterations, options.samples);
    rows.push_back({dataset, library, version, operation, bytes, result});
    const double throughput = result.medianUs > 0
        ? static_cast<double>(bytes) / (1024 * 1024) * 1e6 / result.medianUs : 0;
    std::cout << std::left << std::setw(15) << dataset << std::setw(16) << library
        << std::setw(11) << operation << std::right << std::setw(12) << bytes
        << std::setw(14) << std::fixed << std::setprecision(3) << result.medianUs
        << std::setw(14) << throughput << '\n';
}

void benchmarkFile(const std::filesystem::path& path, const Options& options, std::vector<Row>& rows)
{
    const std::string text = readFile(path);
    const std::string name = path.filename().string();
    const Json original = JsonParser::parse(text);
    if (!(JsonParser::parse(original.serialize(false)) == original))
        throw std::runtime_error("JsonHandler round-trip failed for " + path.string());

    measure(rows, options, name, text.size(), "MasketJSON", MASKETJSON_BENCHMARK_VERSION, "parse", [&] {
        const auto value = JsonParser::parse(text);
        return rootSize(value);
    });
    measure(rows, options, name, text.size(), "MasketJSON", MASKETJSON_BENCHMARK_VERSION, "serialize", [&] {
        return original.serialize(false).size();
    });

#ifdef MASKETJSON_HAS_BOOST
    boost::json::parse_options boostOptions;
    boostOptions.max_depth = 128;
    const auto boostValue = boost::json::parse(text, {}, boostOptions);
    if (!(JsonParser::parse(boost::json::serialize(boostValue)) == original))
        throw std::runtime_error("Boost.JSON semantic check failed for " + path.string());
    measure(rows, options, name, text.size(), "Boost.JSON", BOOST_LIB_VERSION, "parse", [&] {
        const auto value = boost::json::parse(text, {}, boostOptions);
        return value.is_array() ? value.as_array().size() :
            (value.is_object() ? value.as_object().size() : size_t{1});
    });
    measure(rows, options, name, text.size(), "Boost.JSON", BOOST_LIB_VERSION, "serialize", [&] {
        return boost::json::serialize(boostValue).size();
    });
#endif

#ifdef MASKETJSON_HAS_QT
    // QJsonDocument represents object/array roots; don't time rejected scalars.
    if (!original.isArray() && !original.isObject()) {
        std::cerr << name << ": Qt QJsonDocument skipped (scalar root)\n";
        return;
    }
    const QByteArray qtText = QByteArray::fromRawData(text.data(), static_cast<qsizetype>(text.size()));
    QJsonParseError error;
    const auto qtValue = QJsonDocument::fromJson(qtText, &error);
    if (error.error != QJsonParseError::NoError)
        throw std::runtime_error("Qt rejected " + path.string() + ": " + error.errorString().toStdString());
    if (!(JsonParser::parse(qtValue.toJson(QJsonDocument::Compact).toStdString()) == original))
        throw std::runtime_error("Qt semantic check failed for " + path.string());
    measure(rows, options, name, text.size(), "Qt", QT_VERSION_STR, "parse", [&] {
        QJsonParseError parseError;
        const auto value = QJsonDocument::fromJson(qtText, &parseError);
        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error("Qt parse failed during measurement");
        return static_cast<size_t>(value.isArray() ? value.array().size() : value.object().size());
    });
    measure(rows, options, name, text.size(), "Qt", QT_VERSION_STR, "serialize", [&] {
        return static_cast<size_t>(qtValue.toJson(QJsonDocument::Compact).size());
    });
#endif
}

void writeCsv(const std::string& path, const Options& options, const std::vector<Row>& rows)
{
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "dataset,library,version,operation,bytes,iterations,samples,median_us,min_us,max_us,"
        "mib_per_second,compiler,configuration\n";
    output << std::setprecision(12);
    for (const auto& row : rows) {
        const double throughput = row.time.medianUs > 0
            ? static_cast<double>(row.bytes) / (1024 * 1024) * 1e6 / row.time.medianUs : 0;
        output << csvQuote(row.dataset) << ',' << csvQuote(row.library) << ',' << csvQuote(row.version)
            << ',' << csvQuote(row.operation) << ',' << row.bytes << ',' << options.iterations << ','
            << options.samples << ',' << row.time.medianUs << ',' << row.time.minUs << ','
            << row.time.maxUs << ',' << throughput << ',' << csvQuote(compilerName()) << ','
            << csvQuote(MASKETJSON_BENCHMARK_BUILD_TYPE) << '\n';
    }
    writeFile(path, output.str());
}
}

int main(int argc, char** argv)
{
    try {
        std::cout.imbue(std::locale::classic());
        if (argc == 2 && std::string(argv[1]) == "--help") { std::cout << usage; return 0; }
        if (argc == 3 && std::string(argv[1]) == "--generate") { generate(argv[2]); return 0; }
        const auto options = parseOptions(argc, argv);
#ifndef NDEBUG
        std::cerr << "Warning: use a Release build for performance results.\n";
#endif
        std::cout << compilerName() << "; " << MASKETJSON_BENCHMARK_BUILD_TYPE
            << "; iterations=" << options.iterations << "; samples=" << options.samples << '\n';
        std::cout << std::left << std::setw(15) << "Dataset" << std::setw(16) << "Library"
            << std::setw(11) << "Operation" << std::right << std::setw(12) << "Bytes"
            << std::setw(14) << "Median us" << std::setw(14) << "MiB/s" << '\n';
        std::vector<Row> rows;
        for (const auto& file : options.files) benchmarkFile(file, options, rows);
        if (!options.csv.empty()) writeCsv(options.csv, options, rows);
        return std::cout ? 0 : 1;
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n' << usage;
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
