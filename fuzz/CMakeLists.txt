add_library(JsonFuzzChecks STATIC FuzzChecks.cpp)
target_include_directories(JsonFuzzChecks PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(JsonFuzzChecks PUBLIC MasketJSON)

add_executable(JsonFuzzReplay FuzzReplay.cpp)
target_link_libraries(JsonFuzzReplay PRIVATE JsonFuzzChecks)
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET JsonFuzzReplay POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "$<TARGET_FILE:MasketJSON>" "$<TARGET_FILE_DIR:JsonFuzzReplay>")
endif()

if(BUILD_TESTING)
    add_test(NAME Fuzz.CorpusAndMutations COMMAND JsonFuzzReplay
        --mutations 2000 --seed 20261105
        --artifacts "${CMAKE_CURRENT_BINARY_DIR}/replay-artifacts"
        "${CMAKE_CURRENT_SOURCE_DIR}/corpus")
    add_test(NAME Fuzz.RegressionCorpus COMMAND JsonFuzzReplay --require-valid
        --artifacts "${CMAKE_CURRENT_BINARY_DIR}/regression-artifacts"
        "${CMAKE_CURRENT_SOURCE_DIR}/regressions")
    add_test(NAME Fuzz.FailureArtifacts COMMAND "${CMAKE_COMMAND}"
        "-DREPLAY=$<TARGET_FILE:JsonFuzzReplay>"
        "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/artifact-test"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/TestFailureArtifacts.cmake")
    set_tests_properties(Fuzz.CorpusAndMutations Fuzz.RegressionCorpus
        Fuzz.FailureArtifacts PROPERTIES TIMEOUT 30)
endif()

if(MASKETJSON_BUILD_FUZZERS)
    # Use the same source list as the normal library. Coverage instrumentation
    # belongs to a separate static target, so ordinary executables don't acquire
    # libFuzzer runtime dependencies or a second main().
    get_target_property(fuzz_sources MasketJSON SOURCES)
    list(TRANSFORM fuzz_sources PREPEND "${PROJECT_SOURCE_DIR}/")
    add_library(JsonFuzzInstrumented STATIC ${fuzz_sources} FuzzChecks.cpp)
    target_include_directories(JsonFuzzInstrumented PUBLIC
        "$<TARGET_PROPERTY:MasketJSON,INTERFACE_INCLUDE_DIRECTORIES>"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(JsonFuzzInstrumented PUBLIC MASKETJSON_STATIC)
    target_compile_options(JsonFuzzInstrumented PRIVATE -fsanitize=fuzzer-no-link)
    target_link_libraries(JsonFuzzInstrumented PUBLIC MasketJSONSanitizers)
    add_executable(JsonParserFuzz ParserFuzz.cpp)
    target_compile_options(JsonParserFuzz PRIVATE -fsanitize=fuzzer-no-link)
    target_link_options(JsonParserFuzz PRIVATE -fsanitize=fuzzer)
    target_link_libraries(JsonParserFuzz PRIVATE JsonFuzzInstrumented)
endif()
