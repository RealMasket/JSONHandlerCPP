option(MASKETJSON_BENCHMARK_BOOST "Compare with Boost.JSON built from local sources" OFF)
option(MASKETJSON_BENCHMARK_QT "Compare with the installed Qt6 Core JSON implementation" OFF)
set(MASKETJSON_BOOST_ROOT "" CACHE PATH "Boost source tree containing boost/json/src.hpp")
option(MASKETJSON_BUILD_PROFILER "Build standalone parser allocation/stage profiler (static, unsanitized)" OFF)

add_executable(ParseBenchmark ParseBenchmark.cpp BenchmarkUtils.cpp)
target_link_libraries(ParseBenchmark PRIVATE MasketJSON)
target_compile_definitions(ParseBenchmark PRIVATE
    MASKETJSON_BENCHMARK_BUILD_TYPE="$<CONFIG>"
    MASKETJSON_BENCHMARK_VERSION="${PROJECT_VERSION}")
if(MASKETJSON_BUILD_PROFILER)
    if(BUILD_SHARED_LIBS OR MASKETJSON_ENABLE_SANITIZERS)
        message(FATAL_ERROR "ParserProfile requires a static build without sanitizers")
    endif()
    add_executable(ParserProfile ParserProfile.cpp BenchmarkUtils.cpp)
    target_link_libraries(ParserProfile PRIVATE MasketJSON)
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET ParseBenchmark POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "$<TARGET_FILE:MasketJSON>" "$<TARGET_FILE_DIR:ParseBenchmark>")
endif()

if(MASKETJSON_BENCHMARK_BOOST)
    find_path(MASKETJSON_BOOST_INCLUDE_DIR boost/json/src.hpp
        HINTS "${MASKETJSON_BOOST_ROOT}" "${BOOST_ROOT}" REQUIRED)
    add_library(MasketJSONBenchmarkBoost STATIC BoostJson.cpp)
    target_include_directories(MasketJSONBenchmarkBoost PUBLIC "${MASKETJSON_BOOST_INCLUDE_DIR}")
    target_compile_definitions(MasketJSONBenchmarkBoost PUBLIC BOOST_ALL_NO_LIB BOOST_JSON_NO_LIB)
    target_link_libraries(ParseBenchmark PRIVATE MasketJSONBenchmarkBoost)
    target_compile_definitions(ParseBenchmark PRIVATE MASKETJSON_HAS_BOOST)
endif()

if(MASKETJSON_BENCHMARK_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core)
    target_link_libraries(ParseBenchmark PRIVATE Qt6::Core)
    target_compile_definitions(ParseBenchmark PRIVATE MASKETJSON_HAS_QT)
    if(WIN32)
        add_custom_command(TARGET ParseBenchmark POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
            copy_if_different "$<TARGET_FILE:Qt6::Core>" "$<TARGET_FILE_DIR:ParseBenchmark>")
    endif()
endif()
