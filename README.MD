# MasketJSON

A C++17 JSON library.

Version **1.1.0** (development). Latest published release:
[1.0.0](https://github.com/RealMasket/MasketJSON/releases/tag/v1.0.0).
Licensed under [MIT](LICENSE).

A modular C++17 JSON library originally developed for college practice. It
demonstrates tokenization, recursive descent parsing, a value tree based on
`std::variant`, serialization, and contextual diagnostics.
Previously named JsonHandlerCPP.

## Features

- Objects, arrays, strings, exact signed/unsigned 64-bit integers, floating-point
  numbers, booleans, and null.
- Tokens store source ranges instead of allocating a string per token.
- UTF-8 validation, JSON escapes, and UTF-16 surrogate-pair decoding.
- Source line, column, and contextual error messages.
- Compact and pretty output with locale-independent number conversion.
- Checked access, object/array helpers, and file I/O.
- Optional atomic file replacement with `JsonFile::writeAtomic()`.
- A command-line validator and formatter with stdin/stdout support.
- Google Test tests integrated with CTest.
- Corpus replay, deterministic mutations, and optional ASan/UBSan + libFuzzer.
- An installable CMake package with shared and static library support.

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
| `MASKETJSON_BUILD_EXAMPLES` | `ON` | Build usage examples |
| `MASKETJSON_BUILD_CLI` | `ON` | Build the validator/formatter |
| `MASKETJSON_BUILD_BENCHMARKS` | `OFF` | Build the parser benchmark |
| `MASKETJSON_ENABLE_SANITIZERS` | `OFF` | ASan/UBSan on Linux GCC/Clang |
| `MASKETJSON_BUILD_FUZZERS` | `OFF` | Linux Clang libFuzzer; static + sanitizers required |
| `MASKETJSON_BUILD_FUZZ_REPLAY` | `OFF` | Build corpus replay without unit tests |
| `MASKETJSON_ENABLE_PACKAGING` | `OFF` | Generate CPack release archives and SHA-256 checksums |

With a multi-configuration generator such as Visual Studio, select the same
configuration for build and CTest. With a single-configuration generator, set
`CMAKE_BUILD_TYPE` during configuration.

The existing `.sln` / `.vcxproj` files build the DLL directly with the v145
toolset. CMake is the common entry point for the library, examples, and tests.

## Use from another CMake project

Build and install a Release package into a prefix of your choice:

```sh
cmake -S . -B build-package -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DMASKETJSON_BUILD_EXAMPLES=OFF
cmake --build build-package --config Release
cmake --install build-package --config Release --prefix /path/to/masket-json-install
```

Add `-DBUILD_SHARED_LIBS=OFF` when configuring to install a static library.
Use separate installation prefixes for shared and static builds. Debug libraries
have a `d` suffix, so Debug and Release of the same library type can share a
prefix. Build the required configuration before installing it.

In the consuming project, define your executable and link the imported target:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyApp LANGUAGES CXX)
find_package(MasketJSON 1.0 CONFIG REQUIRED)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE MasketJSON::MasketJSON)
```

Configure that project with
`-DCMAKE_PREFIX_PATH=/path/to/masket-json-install`. Installed headers support
`#include <MasketJSON/JsonParser.hpp>` as well as the existing short form
`#include "JsonParser.hpp"`. CMake propagates include directories, C++17, and
`MASKETJSON_STATIC` for static builds; consumers do not need Google Test,
Boost, or Qt. The version file accepts compatible requests within major version 1.

The prefix contains public headers under `include/MasketJSON`, libraries
under `lib`, package configuration under `lib/cmake/MasketJSON`, and the CLI
under `bin` when enabled. GNUInstallDirs options can customize these locations.
Keep them relative to the prefix to retain relocatability. The whole installed
prefix can then be moved without rebuilding; configure consumers with its new
location. On Linux/macOS the installed CLI locates its library relative to itself.
On Windows, put the installed DLL beside your application or add the package's
`bin` directory to `PATH` when running it. See
[the independent consumer](tests/installed-consumer/CMakeLists.txt) for a DLL
copy command. Sanitized developer builds also export their sanitizer requirements;
use an ordinary Release build for distribution.

Alternatively, include the source tree directly and use short header includes:

```cmake
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(MASKETJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MASKETJSON_BUILD_CLI OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/MasketJSON)
target_link_libraries(my_app PRIVATE MasketJSON::MasketJSON)
```

When linking a static build manually on Windows, define `MASKETJSON_STATIC`
for the consumer.

The Windows DLL exposes standard-library types in its C++ interface. Build DLL
consumers with compatible MSVC toolset and runtime settings; a stable C ABI
across unrelated compiler versions is not provided. The legacy Visual Studio
build can report C4251/C4275 warnings for this interface.

The generated `<MasketJSON/Version.hpp>` provides `MASKETJSON_VERSION_STRING`
and `MASKETJSON_VERSION_MAJOR`, `_MINOR`, and `_PATCH` macros.

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
masket-json check examples/data/student.json
masket-json format examples/data/student.json
masket-json format --compact -o compact.json examples/data/student.json
masket-json format --compact -
masket-json --version
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

File output is prepared in a sibling temporary file and then replaces the
destination with a same-filesystem rename. Input and output may be the same file.
The CLI retains its final LF; stdout behavior is unchanged. Directories, symlinks,
Windows reparse points, and alternate data streams are rejected as destinations.
See the guarantees and limits below.

## Atomic file output (1.1+)

```cpp
JsonFile::writeAtomic("settings.json", value);        // pretty JSON
JsonFile::writeAtomic("settings.json", value, false); // compact JSON
```

Request `find_package(MasketJSON 1.1 CONFIG REQUIRED)` when using this API.
The method adds no trailing newline and serializes before creating any file.
The existing `JsonFile::write()` keeps its direct-write behavior.

- The temporary file is created exclusively in the destination directory. Its
  data is fully written, flushed (`fsync` / `FlushFileBuffers`), and closed before
  a single `rename` / `MoveFileExW` replacement. There is no delete-first or
  copy/delete fallback. Failures before replacement leave the destination intact.
- This requires a filesystem supporting atomic same-directory replacement.
  Guarantees apply to ordinary local files with a stable parent directory.
  Remote/network filesystems can have ambiguous outcomes on a reported rename
  failure. The method does not provide a transaction across files or compare-and-swap:
  concurrent successful replacements use the last writer's contents.
- Cleanup on exceptions removes only the temporary file owned by this call.
  Cleanup is best effort if permissions change or the file becomes locked.
  Process termination may leave a temporary file. Crash/power-loss durability
  of the directory entry is not promised; no directory-sync protocol is provided.
- Existing destinations must be regular files. Symlinks (including dangling
  ones), directories, devices/FIFOs, Windows reparse points, and alternate data
  streams are rejected. Hard-linked names and existing open handles retain the
  old file; only the destination name is replaced. Windows handles that deny
  delete sharing can prevent replacement, which is reported as `FileException`.
- On POSIX, new files have mode `0600`; existing basic `rwx` mode bits are copied
  from the initial destination. Ownership, ACLs, extended attributes, timestamps,
  and special mode bits are not preserved. Windows files inherit the destination
  directory's default security; existing file metadata is not copied. Parent
  directory write/rename permissions are required. POSIX may allow replacement
  of a read-only file; a Windows read-only destination blocks replacement.
- Windows filename strings are UTF-8 for all `JsonFile` methods. Use
  `std::filesystem::path::u8string()` rather than the locale-dependent `string()`.
  The Windows CLI decodes wide command-line arguments to support Unicode paths.
  POSIX paths retain their native byte representation. NUL-containing paths fail.

The platform behavior is based on [Linux rename](https://man7.org/linux/man-pages/man2/rename.2.html),
[mkstemp](https://man7.org/linux/man-pages/man3/mkstemp.3.html), and
[Windows MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw).

## Performance comparison

The optional benchmark generates reproducible datasets, times parsing and
serialization separately, and exports CSV. Boost.JSON and Qt6 Core comparisons
are opt-in and use your local installations. See
[benchmarks/README.md](benchmarks/README.md) for commands and methodology.

## Robustness testing

Ordinary CTest runs include byte-preserving corpus replay and deterministic
mutations. Additional Linux CI jobs run ASan/UBSan and a bounded libFuzzer
campaign, preserving logs and failing inputs. See [fuzz/README.md](fuzz/README.md)
for local commands and the regression workflow.

`Package.InstalledConsumer` installs into a temporary prefix, moves it to a path
containing spaces, and builds/runs a separate `find_package` consumer. It checks
each public header independently, both include styles, exported compile settings,
exception and file APIs, and the relocated CLI when enabled. This check runs in
the Windows/Linux, Debug/Release, shared/static CI matrix and with sanitizers.
When `MASKETJSON_ENABLE_PACKAGING=ON`, `Package.ArchiveConsumer` additionally
checks the generated archive and checksum, then builds/runs from its extracted
contents. See [RELEASING.md](RELEASING.md) for packaging commands and
[RELEASE_NOTES.md](RELEASE_NOTES.md) for the next release's development notes.

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

## Numbers

```cpp
const Json id = JsonParser::parse("9007199254740993");
std::int64_t exactId = id.asInt64();
const Json largest = JsonParser::parse("18446744073709551615");
std::uint64_t exactUnsigned = largest.asUInt64();
```

All numeric variants satisfy `isNumber()` and have `typeName() == "number"`.
Use `isInteger()`, `isSignedInteger()`, `isUnsignedInteger()`, and
`isFloatingPoint()` to inspect storage. Standard C++ integral constructors keep
their signedness; `bool` remains a separate JSON type.

`asInt64()` and `asUInt64()` accept either stored integer variant if the value
fits the requested range. They throw `TypeException` for negative-to-unsigned,
oversized unsigned-to-signed, or floating-point conversions, including `1.0`.
The existing `asNumber()` converts any numeric variant to `double` and **may
round large integers**; use the exact accessors for IDs and counters.

`operator==` and `compareNumbers()` compare numeric values across storage types
without rounding integers to double. For example, integer `1` equals `1.0`,
but integer `9007199254740993` does not equal double `9007199254740992.0`.
`compareNumbers()` returns -1, 0, or 1; it rejects non-numbers and unordered NaN.
Infinity is ordered by comparison but cannot be serialized. `sortByPath()` uses
the same numeric ordering. Booleans and strings are never equal to numbers.

## Behavior and limits

- Integer literals without a decimal point or exponent are stored exactly:
  negative values must fit `int64_t`, and nonnegative values must fit `uint64_t`.
  Nonnegative parsed values up to `INT64_MAX` use signed storage. Larger values
  use unsigned storage. Out-of-range integer literals throw `ParseException`
  instead of falling back to double. Arbitrary-precision integers are not supported.
- Decimal/exponent literals use `double` and may round high-precision decimal
  input. Finite stored doubles round-trip; whole-valued doubles include `.0`
  when needed to retain floating storage. Negative zero (`-0`, `-0.0`) keeps its
  sign and serializes as `-0.0`. Integer signedness is not encoded by JSON, so
  a small unsigned integer may parse back into signed storage with the same value.
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
- `JsonFile::write()` serializes first and checks I/O errors, but writes directly
  and can leave a partial file on failure. `writeAtomic()` and CLI file output
  prepare a sibling file before replacement; see the guarantees above.
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
| `tools` / `tests` | CLI, integration checks, and installed-package consumer |
| `fuzz` / `cmake` | Corpus replay, libFuzzer, sanitizers, and package configuration |

See [CHANGELOG.md](CHANGELOG.md) for the fixes and compatibility changes.
GitHub Actions is configured to build and test shared and static variants on
Windows and Linux, in Debug and Release.

## Future work

Longer fuzz campaigns, streaming/SAX parsing,
arbitrary-precision numbers, and sampled CPU profiling. Captured heap baselines
and stage timing tools are described in [benchmarks/README.md](benchmarks/README.md).

## License

MIT License. Copyright (c) 2026 RealMasket. See [LICENSE](LICENSE).
