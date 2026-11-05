cmake_minimum_required(VERSION 3.20)
project(JsonHandlerInstalledConsumer LANGUAGES CXX)

# The imported target must raise this to C++17 without producer flags/includes.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(JsonHandlerCPP 1.0 CONFIG REQUIRED)
if(NOT TARGET JsonHandlerCPP::JsonHandlerCPP)
    message(FATAL_ERROR "Installed package did not export its public target")
endif()

if(DEFINED EXPECT_PREFIX)
    cmake_path(ABSOLUTE_PATH EXPECT_PREFIX NORMALIZE OUTPUT_VARIABLE expected_root)
    cmake_path(IS_PREFIX expected_root "${JsonHandlerCPP_DIR}" NORMALIZE correct_package)
    if(NOT correct_package)
        message(FATAL_ERROR "find_package used a different prefix: ${JsonHandlerCPP_DIR}")
    endif()
    get_target_property(imported_includes JsonHandlerCPP::JsonHandlerCPP INTERFACE_INCLUDE_DIRECTORIES)
    foreach(directory IN LISTS imported_includes)
        cmake_path(IS_PREFIX expected_root "${directory}" NORMALIZE correct_include)
        if(NOT correct_include)
            message(FATAL_ERROR "Package include directory escapes relocated prefix: ${directory}")
        endif()
    endforeach()
endif()
get_target_property(library_type JsonHandlerCPP::JsonHandlerCPP TYPE)
if(DEFINED EXPECT_SHARED)
    if(EXPECT_SHARED AND NOT library_type STREQUAL "SHARED_LIBRARY")
        message(FATAL_ERROR "Expected a shared installed target, got ${library_type}")
    elseif(NOT EXPECT_SHARED AND NOT library_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR "Expected a static installed target, got ${library_type}")
    endif()
endif()

add_executable(installed_consumer main.cpp legacy_headers.cpp)
target_link_libraries(installed_consumer PRIVATE JsonHandlerCPP::JsonHandlerCPP)
target_compile_definitions(installed_consumer PRIVATE
    CONSUMER_EXPECT_VERSION="${JsonHandlerCPP_VERSION}"
    CONSUMER_EXPECT_SHARED=$<STREQUAL:${library_type},SHARED_LIBRARY>)
if(WIN32 AND library_type STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET installed_consumer POST_BUILD COMMAND "${CMAKE_COMMAND}" -E
        copy_if_different "$<TARGET_FILE:JsonHandlerCPP::JsonHandlerCPP>"
        "$<TARGET_FILE_DIR:installed_consumer>")
endif()

# Compile each installed header on its own; do not inherit a producer PCH.
set(public_headers Api JsonHandler JsonLexer JsonParser Token JsonSerializer JsonFile
    JsonException AccessException FileException LexerException OperationException
    ParseException TypeException JsonAlgorithms Version)
foreach(header IN LISTS public_headers)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/header-${header}.cpp")
    file(WRITE "${source}" "#include <JsonHandlerCPP/${header}.hpp>\n")
    list(APPEND header_sources "${source}")
endforeach()
add_library(installed_headers OBJECT ${header_sources})
target_link_libraries(installed_headers PRIVATE JsonHandlerCPP::JsonHandlerCPP)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME Consumer.Run COMMAND installed_consumer)
endif()
