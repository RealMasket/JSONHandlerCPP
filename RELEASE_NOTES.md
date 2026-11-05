# JsonHandlerCPP 1.0.0

First packaged release of the C++17 JSON library originally developed for
college practice, with a validator/formatter CLI and an installable CMake package.

## Highlights

- Exact signed/unsigned 64-bit integers, checked access, and mixed numeric
  comparisons without rounding integers through `double`.
- UTF-8 validation, JSON escapes, surrogate-pair decoding, and line/column diagnostics.
- Compact/pretty serialization, JSON algorithms, and file I/O.
- `jsonhandler check`, `format`, and `--version`, including stdin/stdout support.
- Relocatable shared/static packages, `find_package(JsonHandlerCPP 1.0 CONFIG REQUIRED)`,
  and the `JsonHandlerCPP::JsonHandlerCPP` target.
- MIT license, Copyright (c) 2026 RealMasket.

## Getting started

Build from source with CMake 3.20+ and a C++17 standard library implementing
floating-point `std::from_chars` / `std::to_chars`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/jsonhandler
```

Alternatively, extract a binary archive and configure your application with
`-DCMAKE_PREFIX_PATH=/path/to/extracted/package`. Archive names identify the
OS, architecture, compiler version, and shared/static linkage. They contain a
Release library, public headers, CLI, license, and documentation; matching
`.sha256` files accompany the archives. Use a compatible compiler/runtime for
C++ consumers. Windows DLL users must also place the DLL beside their executable
or add the package's `bin` directory to `PATH`. The MSVC runtime must be installed.
Linux binaries require a compatible system runtime and standard library; build
from source when the provided compiler/platform combination does not match.

## Compatibility and limits

Rebuild applications that used the original educational library: the `Json`
variant layout changed. `at()` is strict; mutable `operator[]` converts only null
into a container. Digit-only strings remain quoted, whole-valued floating numbers
retain `.0`, and negative zero retains its sign.

Decimal/exponent numbers use `double`; arbitrary-precision numbers and streaming
parsing are not implemented. Objects sort keys lexicographically, and the first
duplicate key wins. Nesting is limited to 128 containers. Input/output strings
must be valid UTF-8, and non-finite output is rejected. File writes report errors
but are not atomic. See [README](README.MD) and [CHANGELOG](CHANGELOG.md) for details.

## Verification

CI covers Windows/Linux, shared/static, Debug/Release, independent consumers
after installation and relocation, and consumers built from generated release
archives. Linux also runs ASan/UBSan and a bounded libFuzzer campaign. Performance
measurements and their limitations are recorded in
[the optimization report](benchmarks/results/parse-optimization.md).
