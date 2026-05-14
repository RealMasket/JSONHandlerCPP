cmake_minimum_required(VERSION 4.2)
project(JSONHandlerCPP_master)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)
include_directories(core)
include_directories(diagnostics)
include_directories(errors)
include_directories(io)
include_directories(parser)
include_directories(serializer)
include_directories(utils)

add_library(JSONHandlerCPP_master STATIC
        core/JsonHandler.cpp
        core/JsonHandler.hpp
        diagnostics/DiagnosticFormatter.cpp
        diagnostics/DiagnosticFormatter.hpp
        errors/AccessException.cpp
        errors/AccessException.hpp
        errors/FileException.cpp
        errors/FileException.hpp
        errors/JsonException.hpp
        errors/LexerException.cpp
        errors/LexerException.hpp
        errors/OperationException.cpp
        errors/OperationException.hpp
        errors/ParseException.cpp
        errors/ParseException.hpp
        errors/TypeException.cpp
        errors/TypeException.hpp
        io/JsonFile.cpp
        io/JsonFile.hpp
        parser/JsonLexer.cpp
        parser/JsonLexer.hpp
        parser/JsonParser.cpp
        parser/JsonParser.hpp
        parser/Token.hpp
        serializer/JsonSerializer.cpp
        jsonHandlerC++.hpp
        utils/Api.hpp
        utils/JsonAlgorithms.cpp
        utils/JsonAlgorithms.hpp
        pch.cpp
        pch.h)

if (WIN32)
    target_sources(JSONHandlerCPP_master PRIVATE utils/dllmain.cpp)
endif()
