cmake_minimum_required(VERSION 3.20)

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

add_library(JsonHandlerCPP SHARED)

target_compile_definitions(JsonHandlerCPP
    PRIVATE JSONHANDLERC_EXPORTS
)

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

add_subdirectory(benchmarks)

# =========================
# Compiler Features
# =========================

target_compile_features(JsonHandlerCPP
    PUBLIC cxx_std_17
)