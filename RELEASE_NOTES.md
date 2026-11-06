# MasketJSON 1.1.0 (development draft)

Adds atomic JSON file replacement and safer CLI file output. This version is
under development; the latest published release remains 1.0.0.

## Highlights

- `JsonFile::writeAtomic(filename, json, pretty=true)` prepares a sibling temporary
  file, checks complete writes/flush/close, then performs one native replacement.
- CLI `format -o` uses the same mechanism, keeps its final LF, and supports
  reading and replacing the same file.
- Unicode Windows paths use UTF-8 in JsonFile and wide command-line arguments
  in the CLI. POSIX paths retain native byte semantics.
- Failure coverage includes partial writes, flush/close/commit errors, locked or
  unwritable destinations, cleanup, Unicode paths, and exported DLL consumers.
- Existing basic POSIX mode bits are retained; new POSIX files use mode 0600.

## Getting started

Build from source with CMake 3.20+ and a C++17 standard library implementing
floating-point `std::from_chars` / `std::to_chars`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --config Release
cmake --install build --config Release --prefix /path/to/masket-json
```

Request `find_package(MasketJSON 1.1 CONFIG REQUIRED)` for the new API:

```cpp
JsonFile::writeAtomic("settings.json", value);        // pretty, no final LF
JsonFile::writeAtomic("settings.json", value, false); // compact, no final LF
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

This is an additive API; the Json value layout and numeric behavior are unchanged
from 1.0.0. `JsonFile::write()` still performs a direct write. Windows filename
strings now explicitly use UTF-8: use `path.u8string()` for filesystem paths.
Atomic destinations must be regular files, not symlinks, directories, devices,
Windows reparse points, or alternate streams. Parent-directory rename permission
is required. File identity changes; hard links and existing handles retain the
old file. Metadata beyond basic POSIX mode bits is not preserved.

Failures before replacement preserve the destination, with best-effort temporary
cleanup. Local-filesystem atomic rename support is required; remote filesystem
errors can have ambiguous outcomes. Crash/power-loss durability, automatic cleanup
after process termination, and coordination of concurrent writers are not promised.

Decimal/exponent numbers use `double`; arbitrary-precision numbers and streaming
parsing are not implemented. Objects sort keys lexicographically, and the first
duplicate key wins. Nesting is limited to 128 containers. Input/output strings
must be valid UTF-8, and non-finite output is rejected. Direct writes remain
non-atomic. See [README](https://github.com/RealMasket/MasketJSON/blob/v1.1.0/README.MD)
and [CHANGELOG](https://github.com/RealMasket/MasketJSON/blob/v1.1.0/CHANGELOG.md) for details.

## Verification

CI covers Windows/Linux, shared/static, Debug/Release, independent consumers
after installation and relocation, and consumers built from generated release
archives. Linux also runs ASan/UBSan and a bounded libFuzzer campaign. Performance
measurements and their limitations are recorded in
[the optimization report](https://github.com/RealMasket/MasketJSON/blob/v1.0.0/benchmarks/results/parse-optimization.md).
