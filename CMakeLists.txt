cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(
    JsonHandlerCPP
    VERSION 1.0
    DESCRIPTION "Modern modular JSON library for C++"
    LANGUAGES CXX
)

# =========================
# C++ Standard
# =========================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================
# Library
# =========================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(JSONHANDLER_BUILD_EXAMPLES "Build usage examples" ON)
option(JSONHANDLER_BUILD_CLI "Build the JSON validation/formatting command line tool" ON)
option(JSONHANDLER_BUILD_BENCHMARKS "Build benchmarks (requires external input data)" OFF)
option(JSONHANDLER_ENABLE_SANITIZERS "Enable ASan and UBSan (Linux GCC/Clang)" OFF)
option(JSONHANDLER_BUILD_FUZZERS "Build libFuzzer target (Linux Clang, static, sanitizers)" OFF)
option(JSONHANDLER_BUILD_FUZZ_REPLAY "Build portable corpus replay without unit tests" OFF)

add_library(JsonHandlerCPP)
add_library(JsonHandlerCPP::JsonHandlerCPP ALIAS JsonHandlerCPP)

include(cmake/Sanitizers.cmake)

target_compile_definitions(JsonHandlerCPP
    PRIVATE JSONHANDLERC_EXPORTS
)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(JsonHandlerCPP PUBLIC JSONHANDLER_STATIC)
endif()

target_sources(JsonHandlerCPP
    PRIVATE

    # Core
    core/JsonHandler.cpp

    # Diagnostics
    diagnostics/DiagnosticFormatter.cpp

    # Exceptions
    errors/AccessException.cpp
    errors/FileException.cpp
    errors/LexerException.cpp
    errors/OperationException.cpp
    errors/ParseException.cpp
    errors/TypeException.cpp

    # IO
    io/JsonFile.cpp

    # Parser
    parser/JsonLexer.cpp
    parser/JsonParser.cpp

    # Serializer
    serializer/JsonSerializer.cpp

    # Utils
    utils/JsonAlgorithms.cpp

    # PCH
    pch.cpp
)

# =========================
# Include Directories
# =========================

target_include_directories(JsonHandlerCPP
    PUBLIC

    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics
    ${CMAKE_CURRENT_SOURCE_DIR}/errors
    ${CMAKE_CURRENT_SOURCE_DIR}/io
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/serializer
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

# =========================
# Benchmarks
# =========================

if(JSONHANDLER_BUILD_CLI)
    add_executable(jsonhandler tools/JsonCli.cpp)
    target_link_libraries(jsonhandler PRIVATE JsonHandlerCPP)
endif()

if(JSONHANDLER_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(JSONHANDLER_BUILD_EXAMPLES)
    foreach(example BasicJsonExample ArrayExample ParsingExample)
        add_executable(${example} examples/${example}.cpp)
        target_link_libraries(${example} PRIVATE JsonHandlerCPP)
    endforeach()
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest
            URL https://codeload.github.com/google/googletest/zip/refs/tags/v1.15.2
            URL_HASH SHA256=f179ec217f9b3b3f3c6e8b02d3e7eda997b49e4ce26d6b235c9053bec9c0bf9f
            TIMEOUT 30
        )
        # Keep the test framework static so Windows test discovery can load the
        # executable without extra DLL search paths, even for a shared library.
        set(JSONHANDLER_SHARED_LIBS ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        set(BUILD_GMOCK OFF)
        FetchContent_MakeAvailable(googletest)
        set(BUILD_SHARED_LIBS ${JSONHANDLER_SHARED_LIBS})
    endif()
    add_executable(JsonHandlerTests
        "examples of tests/JsonLexerTests.cpp"
        "examples of tests/JsonParserTests.cpp"
        "examples of tests/JsonRoundTripTests.cpp"
        "examples of tests/JsonSerializerTests.cpp"
        "examples of tests/JsonRegressionTests.cpp"
        "examples of tests/JsonNumberTests.cpp"
    )
    target_link_libraries(JsonHandlerTests PRIVATE JsonHandlerCPP GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(JsonHandlerTests)
    if(JSONHANDLER_BUILD_CLI)
        add_test(NAME JsonCli.Integration COMMAND ${CMAKE_COMMAND}
            "-DCLI=$<TARGET_FILE:jsonhandler>"
            "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-integration"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/CliIntegration.cmake")
    endif()
    if(JSONHANDLER_BUILD_BENCHMARKS)
        add_test(NAME Benchmark.Generate COMMAND ParseBenchmark --generate
            "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke")
        set_tests_properties(Benchmark.Generate PROPERTIES FIXTURES_SETUP benchmark_data)
        add_test(NAME Benchmark.Smoke COMMAND ParseBenchmark --iterations 2 --samples 3
            --csv "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/results.csv"
            "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/tiny.json")
        set_tests_properties(Benchmark.Smoke PROPERTIES FIXTURES_REQUIRED benchmark_data)
    endif()
endif()

# =========================
# Compiler Features
# =========================

target_compile_features(JsonHandlerCPP
    PUBLIC cxx_std_17
)

if(BUILD_TESTING OR JSONHANDLER_BUILD_FUZZ_REPLAY OR JSONHANDLER_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()
