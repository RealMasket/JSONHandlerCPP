# JsonHandlerC++

Modern modular JSON parsing and serialization library for C++17.

JsonHandlerC++ is a custom JSON processing library developed as an educational and research-oriented software engineering project.  
The library implements a complete JSON pipeline including lexical analysis, parsing, serialization, diagnostics, exception handling, and automated testing.

The project focuses on:
- modular architecture,
- RFC-oriented validation,
- parser reliability,
- detailed diagnostics,
- extensibility,
- and modern C++ development practices.

---

# Features

## Core JSON Support
- JSON object support
- JSON array support
- String, number, boolean, and null types
- Dynamic JSON value system using `std::variant`

## Lexer & Parser
- Recursive descent parser architecture
- Token-based lexer system
- Detailed token metadata:
  - line
  - column
  - source position
- Strict JSON validation
- EOF validation
- Safe token access
- Malformed JSON detection

## Unicode & Escaping
- Escape sequence support:
  - `\n`
  - `\t`
  - `\\`
  - `\"`
- Unicode escape support:
  - `\uXXXX`
- UTF-8 conversion support

## Serialization
- Compact JSON serialization
- Pretty-print JSON serialization
- Recursive object/array serialization
- String escaping system

## Diagnostics & Error Handling
- Detailed parser diagnostics
- Line and column error reporting
- Specialized exception hierarchy:
  - `LexerException`
  - `ParseException`
  - `TypeException`
  - `FileException`
  - `OperationException`
  - `AccessException`

Example diagnostic:

```text
Parse error at line 4, column 15:
Expected ',' between array elements
````

---

## Robustness & Safety

* Maximum nesting depth protection
* Invalid number rejection
* Invalid escape sequence detection
* Defensive parser validation
* Safe recursive parsing

---

## Testing

The project uses Google Test for automated testing.

Current test coverage includes:

* Lexer validation
* Parser validation
* Serialization tests
* Round-trip tests
* Unicode handling
* Invalid JSON rejection
* Escape sequence validation
* Deep nesting protection

---

# Architecture

The library is divided into independent modules.

## Core Layer

Responsible for storing and manipulating JSON values.

Modules:

* `JsonHandler`

## Lexer Layer

Converts raw JSON text into token stream.

Module:

* `JsonLexer`
* `Token`

Responsibilities:

* token generation
* number validation
* keyword parsing
* escape sequence handling
* unicode parsing

## Parser Layer

Builds JSON structures from tokens.

Module:

* `JsonParser`

Responsibilities:

* recursive descent parsing
* object parsing
* array parsing
* syntax validation
* nesting depth protection

## Serializer Layer

Converts JSON structures back into text in JSON form.

Module:

* `JsonSerializer`

Responsibilities:

* compact serialization
* pretty serialization
* string escaping

## Diagnostics Layer

Provides contextual error formatting and debugging information.

Module:

* `DiagnosticFormatter`

## Exception Layer

Provides specialized exception hierarchy.

Modules:

* `LexerException`
* `ParseException`
* `TypeException`
* `AccessException`
* `FileException`
* `OperationException`

---

# Project Structure

```text
JsonHandlerC++/
│
├── core/
│   └── JsonHandler
│
├── diagnostics/
│   └── DiagnosticFormatter
│
├── errors/
│   ├── AccessException
│   ├── FileException
│   ├── LexerException
│   ├── OperationException
│   ├── ParseException
│   ├── TypeException
│   └── JsonException (base class)
│
├── examples/
│   ├── ArrayExample
│   ├── BasicJsonExample
│   └── ParsingExample
│
├── examples of tests/
│   ├── JsonLexerTests
│   ├── JsonParserTests
│   ├── JsonRoundTripTests
│   └── JsonSerializerTests
│
├── io/
│   └── JsonFile
│
├── parser/
│   ├── JsonParser
│   ├── JsonLexer
│   └── Token
│
├── serializer/
│   └── JsonSerializer
│
└── utils/
    ├── Api
    ├── dllmain
    ├── framework
    └── JsonAlgorithms
```

---

# Usage

## Basic Example

```cpp
#include "JsonHandler.hpp"
#include <iostream>

int main()
{
    Json obj(Json::JsonObject{});

    obj["name"] = "Adam";
    obj["age"] = 21.0;
    obj["student"] = true;

    std::cout << obj.serialize(true, 0);

    return 0;
}
```

## Parsing JSON

```cpp
#include "JsonHandler.hpp"

int main()
{
    std::string jsonText =
        R"({
            "name": "Alice",
            "age": 25
        })";

    Json json = Json::parse(jsonText);

    std::cout << json["name"].asString();

    return 0;
}
```

## Arrays

```cpp
Json::JsonArray arr =
{
    Json(1.0),
    Json(2.0),
    Json("hello"),
    Json(true)
};

Json json = arr;

json.push_back("new value");

std::cout << json.serialize(true, 0);
```

---

# Example of Diagnostics

Input:

```json
{
    "name": "John"
    "age": 25
}
```

Diagnostic:

```text
Parse error at line 3, column 5:
Expected ',' between key-value pairs
```

---

# Build Requirements

## Requirements

* CMake 3.20 or newer (if using CMake)
* C++17 or newer
* Google Test (if you want to run tests. But it's better to do in a separate project with compiled and linked Google Test libraries and this library)

---

## Supported Compilers

* MSVC
* GCC
* Clang

(Basically any C++ compiler modern enough to support C++17)

---

# Building

## Visual Studio

1. Open the solution file.
2. Build the project.
3. Build test project separately if needed.

---

## CMake

1.Clone the Repository

```bash
git clone https://github.com/RealMasket/JSONHandlerCPP.git
cd JSONHandlerCPP
````

2. Configure Project

```bash
cmake -S . -B build
```

3. Build Project

```bash
cmake --build build
```

---

# Future Improvements

Planned future improvements include:

* Streaming JSON parser
* SAX-style parser API
* Full UTF-16 surrogate pair support
* Continuous Integration (CI)
* Fuzz testing
* Custom memory allocators
* Improved performance optimizations
* Iterator support
* JSON schema validation

---

# Engineering Goals

This project was created to explore:

* parser architecture,
* compiler-inspired design,
* defensive programming,
* diagnostics systems,
* modular C++ development,
* and automated software testing.

The project is intended as both:

* a practical JSON library,
* and a software engineering research/educational project.

---

# License

This project is provided for educational and research purposes.