# JsonHandlerC++

A modular C++17 JSON library originally developed for college practice. It
demonstrates tokenization, recursive descent parsing, a value tree based on
`std::variant`, serialization, and contextual diagnostics.

## Features

- Objects, arrays, strings, numbers, booleans, and null.
- Tokens store source ranges instead of allocating a string per token.
- UTF-8 validation, JSON escapes, and UTF-16 surrogate-pair decoding.
- Source line, column, and contextual error messages.
- Compact and pretty output with locale-independent number conversion.
- Checked access, object/array helpers, and file I/O.
- A command-line validator and formatter with stdin/stdout support.
- Google Test tests integrated with CTest.

## Build and test

Requirements: CMake 3.20+, and a C++17 compiler/standard library implementing
floating-point `std::from_chars` and `std::to_chars`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

The library, command-line tool, and three examples are built by default. Tests use an installed
Google Test package when available, otherwise CMake downloads Google Test
1.15.2 and verifies its SHA-256 checksum. The first download needs internet.

For an offline library-only build:

```sh
cmake -S . -B build -DBUILD_TESTING=OFF
cmake --build build --config Release
```

For offline tests, provide an installed `GTest_DIR` or a local Google Test source
directory with `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest`.

| Option | Default | Purpose |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `ON` | Use `OFF` for a static library |
| `BUILD_TESTING` | `ON` | Build and discover Google Test tests |
| `JSONHANDLER_BUILD_EXAMPLES` | `ON` | Build usage examples |
| `JSONHANDLER_BUILD_CLI` | `ON` | Build the validator/formatter |
| `JSONHANDLER_BUILD_BENCHMARKS` | `OFF` | Build the parser benchmark |

With a multi-configuration generator such as Visual Studio, select the same
configuration for build and CTest. With a single-configuration generator, set
`CMAKE_BUILD_TYPE` during configuration.

The existing `.sln` / `.vcxproj` files build the DLL directly with the v145
toolset. CMake is the common entry point for the library, examples, and tests.

## Use from another CMake project

```cmake
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(JSONHANDLER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/JsonHandlerCPP)
target_link_libraries(my_app PRIVATE JsonHandlerCPP::JsonHandlerCPP)
```

CMake propagates include directories, C++17 requirements, and the static-library
definition. When linking a static build manually on Windows, define
`JSONHANDLER_STATIC` for the consumer.

The Windows DLL exposes standard-library types in its C++ interface. Build DLL
consumers with compatible MSVC toolset and runtime settings; a stable C ABI
across unrelated compiler versions is not provided. The legacy Visual Studio
build can report C4251/C4275 warnings for this interface.

## Example

```cpp
#include "JsonHandler.hpp"
#include "JsonParser.hpp"
#include "JsonFile.hpp"
#include <iostream>

int main()
{
    Json value = JsonParser::parse(R"({"id":"001","scores":[5,4,5]})");
    std::cout << value.at("id").asString() << '\n';
    value.at("scores").push_back(3);
    std::cout << value.serialize(false) << '\n';
    JsonFile::write("result.json", value);
    Json restored = JsonFile::read("result.json");
    return restored == value ? 0 : 1;
}
```

## Command-line tool

With Visual Studio, the executable is in `build/Debug/` or `build/Release/`.
For single-configuration generators it is in `build/`.

```sh
jsonhandler check examples/data/student.json
jsonhandler format examples/data/student.json
jsonhandler format --compact -o compact.json examples/data/student.json
jsonhandler format --compact -
```

The last command reads stdin, so it also accepts input from a pipe or redirected
file. `check` prints `Valid JSON` on success. `format` prints JSON followed by a
newline, or writes it to the specified output file. Diagnostics go to stderr.
Use `--` before filenames beginning with `-`.

| Exit code | Meaning |
| --- | --- |
| 0 | Success |
| 1 | Invalid JSON |
| 2 | Invalid command-line arguments |
| 3 | File or stream I/O failure |
| 4 | Other processing failure |

Input is parsed and formatted before opening an output file. An output write
failure can still leave a partial file; the CLI does not promise atomic writes.

## Performance comparison

The optional benchmark generates reproducible datasets, times parsing and
serialization separately, and exports CSV. Boost.JSON and Qt6 Core comparisons
are opt-in and use your local installations. See
[benchmarks/README.md](benchmarks/README.md) for commands and methodology.

## Access and errors

- `at(key)` / `at(index)` are strict: no insertion, resizing, or type conversion.
  Missing keys and out-of-range indices throw `AccessException`.
- Mutable `operator[]` creates a container from `null`, inserts missing object
  keys, and expands arrays. Access through a conflicting type throws
  `TypeException` and preserves the value.
- Const `operator[]` behaves like `at()`.
- `LexerException` reports malformed tokens and invalid UTF-8.
- `ParseException` reports structural errors, unpaired surrogates, excessive
  nesting, and numbers outside the supported range.
- `OperationException` reports unsupported serialization operations, including
  non-finite numbers and invalid UTF-8.
- `FileException` reports file open/read/write/flush/close failures. File parsing
  preserves the original `LexerException` or `ParseException`.

`JsonParser::parse()` parses text; file operations belong to `JsonFile`.

## Behavior and limits

- All numbers are stored as `double`. Serialization preserves the stored finite
  value, but parsing large integers or high-precision decimals may round them.
  Preserve exact identifiers as strings. Arbitrary-precision numbers are not
  implemented.
- Strings containing digits remain strings, including `"001"`. The legacy
  `isBigNumber()` helper is deprecated and does not affect serialization.
- NaN and infinity cannot be serialized. Out-of-range input numbers are rejected
  rather than silently converted to infinity or zero.
- Input and output strings must be valid UTF-8. Unpaired UTF-16 surrogates are
  rejected. This library does not normalize Unicode.
- Parsing and serialization allow up to 128 nested containers. A nonzero initial
  serialization indentation level counts toward that limit.
- Objects use `std::map`, so keys are ordered lexicographically. For duplicate
  keys during parsing, the first value wins.
- Pretty output uses two spaces. Compact output adds no formatting whitespace.
- File writing serializes before opening the destination and checks I/O errors.
  It is **not atomic**: an I/O failure during writing can leave a partial file.
- Benchmark datasets can be generated with `ParseBenchmark --generate DIR`.

## Architecture

| Module | Responsibility |
| --- | --- |
| `core` | JSON value storage and public access API |
| `parser` | Source-range tokens, lexer, recursive descent parser |
| `serializer` | Escaping, number conversion, compact/pretty formatting |
| `diagnostics` | Source snippets and error pointers |
| `errors` | Public exception hierarchy |
| `io` | File reading and writing |
| `utils` | JSON algorithms, UTF-8 validation, export definitions |
| `examples of tests` | Unit and regression tests |
| `tools` / `tests` | Command-line application and its integration checks |

See [CHANGELOG.md](CHANGELOG.md) for the fixes and compatibility changes.
GitHub Actions is configured to build and test shared and static variants on
Windows and Linux, in Debug and Release.

## Future work

Exact integer types, atomic file replacement, fuzz testing, streaming/SAX
parsing, and memory profiling.

## License

The original project describes educational and research use. No standard
open-source license has been selected yet.
