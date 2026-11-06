cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(
    MasketJSON
    VERSION 1.1.0
    DESCRIPTION "A C++17 JSON library"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
option(MASKETJSON_BUILD_EXAMPLES "Build usage examples" ON)
option(MASKETJSON_BUILD_CLI "Build the JSON validation/formatting command line tool" ON)
option(MASKETJSON_BUILD_BENCHMARKS "Build benchmarks (requires external input data)" OFF)
option(MASKETJSON_ENABLE_SANITIZERS "Enable ASan and UBSan (Linux GCC/Clang)" OFF)
option(MASKETJSON_BUILD_FUZZERS "Build libFuzzer target (Linux Clang, static, sanitizers)" OFF)
option(MASKETJSON_BUILD_FUZZ_REPLAY "Build portable corpus replay without unit tests" OFF)
option(MASKETJSON_ENABLE_PACKAGING "Generate release archives with CPack" OFF)

configure_file(utils/Version.hpp.in
    "${CMAKE_CURRENT_BINARY_DIR}/include/MasketJSON/Version.hpp" @ONLY)

add_library(MasketJSON)
add_library(MasketJSON::MasketJSON ALIAS MasketJSON)
set_target_properties(MasketJSON PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    DEBUG_POSTFIX d
)

include(cmake/Sanitizers.cmake)

target_compile_definitions(MasketJSON
    PRIVATE MASKETJSON_EXPORTS
)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(MasketJSON PUBLIC MASKETJSON_STATIC)
endif()

target_sources(MasketJSON
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
    io/AtomicFile.cpp
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

target_include_directories(MasketJSON
    PUBLIC

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/diagnostics>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/errors>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/io>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/parser>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/serializer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/MasketJSON>
)

# =========================
# Benchmarks
# =========================

if(MASKETJSON_BUILD_CLI)
    add_executable(masket-json tools/JsonCli.cpp io/AtomicFile.cpp)
    if(MINGW)
        target_link_options(masket-json PRIVATE -municode)
    endif()
    target_link_libraries(masket-json PRIVATE MasketJSON)
    file(RELATIVE_PATH masketjson_bin_to_lib "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")
    if(APPLE)
        set_target_properties(masket-json PROPERTIES INSTALL_RPATH "@loader_path/${masketjson_bin_to_lib}")
    elseif(UNIX)
        set_target_properties(masket-json PROPERTIES INSTALL_RPATH "\$ORIGIN/${masketjson_bin_to_lib}")
    endif()
    install(TARGETS masket-json RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(MASKETJSON_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(MASKETJSON_BUILD_EXAMPLES)
    foreach(example BasicJsonExample ArrayExample ParsingExample)
        add_executable(${example} examples/${example}.cpp)
        target_link_libraries(${example} PRIVATE MasketJSON)
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
        set(MASKETJSON_SHARED_LIBS ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        set(BUILD_GMOCK OFF)
        set(INSTALL_GTEST OFF)
        FetchContent_MakeAvailable(googletest)
        set(BUILD_SHARED_LIBS ${MASKETJSON_SHARED_LIBS})
    endif()
    add_executable(MasketJSONTests
        io/AtomicFile.cpp
        "examples of tests/JsonAtomicFileTests.cpp"
        "examples of tests/JsonLexerTests.cpp"
        "examples of tests/JsonParserTests.cpp"
        "examples of tests/JsonRoundTripTests.cpp"
        "examples of tests/JsonSerializerTests.cpp"
        "examples of tests/JsonRegressionTests.cpp"
        "examples of tests/JsonNumberTests.cpp"
    )
    target_link_libraries(MasketJSONTests PRIVATE MasketJSON GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(MasketJSONTests)
    set(masketjson_consumer_test_args
        "-DPRODUCER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
        "-DPRODUCER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/installed-consumer"
        "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/package-tests"
        "-DCONFIG=$<CONFIG>"
        "-DGENERATOR=${CMAKE_GENERATOR}"
        "-DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
        "-DEXPECT_SHARED=${BUILD_SHARED_LIBS}"
        "-DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
        "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
        "-DINSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}"
        "-DINSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}"
        "-DCLI_ENABLED=${MASKETJSON_BUILD_CLI}"
        "-DEXECUTABLE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
        "-DCTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
        "-DEXPECTED_VERSION=${PROJECT_VERSION}")
    add_test(NAME Package.InstalledConsumer COMMAND "${CMAKE_COMMAND}"
        ${masketjson_consumer_test_args}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/InstalledPackage.cmake")
    set_tests_properties(Package.InstalledConsumer PROPERTIES TIMEOUT 180)
    if(MASKETJSON_BUILD_CLI)
        add_test(NAME JsonCli.Integration COMMAND ${CMAKE_COMMAND}
            "-DCLI=$<TARGET_FILE:masket-json>"
            "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/cli-integration"
            "-DEXPECTED_VERSION=${PROJECT_VERSION}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/CliIntegration.cmake")
    endif()
    if(MASKETJSON_BUILD_BENCHMARKS)
        add_test(NAME Benchmark.Generate COMMAND ParseBenchmark --generate
            "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke")
        set_tests_properties(Benchmark.Generate PROPERTIES FIXTURES_SETUP benchmark_data)
        add_test(NAME Benchmark.Smoke COMMAND ParseBenchmark --iterations 2 --samples 3
            --csv "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/results.csv"
            "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/tiny.json")
        set_tests_properties(Benchmark.Smoke PROPERTIES FIXTURES_REQUIRED benchmark_data)
        if(TARGET ParserProfile)
            add_test(NAME Benchmark.ParserProfileSmoke COMMAND ParserProfile
                "${CMAKE_CURRENT_BINARY_DIR}/benchmark-smoke/tiny.json" 2)
            set_tests_properties(Benchmark.ParserProfileSmoke PROPERTIES FIXTURES_REQUIRED benchmark_data)
        endif()
    endif()
endif()

# =========================
# Compiler Features
# =========================

target_compile_features(MasketJSON
    PUBLIC cxx_std_17
)

if(BUILD_TESTING OR MASKETJSON_BUILD_FUZZ_REPLAY OR MASKETJSON_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()

# Public package. Implementation-only diagnostics/UTF-8/PCH headers stay private.
install(TARGETS MasketJSON EXPORT MasketJSONTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if(TARGET MasketJSONSanitizers)
    # Sanitized developer packages must retain their runtime link requirements.
    install(TARGETS MasketJSONSanitizers EXPORT MasketJSONTargets)
endif()
install(FILES
    core/JsonHandler.hpp
    parser/JsonLexer.hpp parser/JsonParser.hpp parser/Token.hpp
    serializer/JsonSerializer.hpp io/JsonFile.hpp
    errors/JsonException.hpp errors/AccessException.hpp errors/FileException.hpp
    errors/LexerException.hpp errors/OperationException.hpp errors/ParseException.hpp
    errors/TypeException.hpp
    utils/Api.hpp utils/JsonAlgorithms.hpp
    "${CMAKE_CURRENT_BINARY_DIR}/include/MasketJSON/Version.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/MasketJSON
)
set(masketjson_package_dir ${CMAKE_INSTALL_LIBDIR}/cmake/MasketJSON)
configure_package_config_file(cmake/MasketJSONConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/MasketJSONConfig.cmake"
    INSTALL_DESTINATION ${masketjson_package_dir}
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MasketJSONConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion
)
install(EXPORT MasketJSONTargets
    FILE MasketJSONTargets.cmake NAMESPACE MasketJSON::
    DESTINATION ${masketjson_package_dir}
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MasketJSONConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MasketJSONConfigVersion.cmake"
    DESTINATION ${masketjson_package_dir}
)
install(FILES README.MD CHANGELOG.md LICENSE RELEASE_NOTES.md RELEASING.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR})

if(MASKETJSON_ENABLE_PACKAGING)
    include(cmake/Packaging.cmake)
endif()
