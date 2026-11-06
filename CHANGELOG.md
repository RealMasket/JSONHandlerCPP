# Changelog

## Unreleased

### Planned for 1.1.0

- Add `ParseOptions` with `Balanced` and opt-in `LowMemory` presets to parser and
  lexer overloads. Bound speculative LowMemory token growth without changing the
  existing default, Json layout, numeric rules, diagnostics or depth limit.
- Select the reserve bound on 47 training fixtures, freeze it before a separate
  12-input evaluation, and verify public-API memory/timing against the frozen
  prototype. Test installed headers/DLL overloads, size-limit arithmetic and
  cross-preset fuzz acceptance, diagnostics and round trips.
- Add an opt-in, isolated four-strategy reservation experiment with 47 input
  shapes, token-relocation counters, rotating timing order, Windows/Linux smoke
  checks, raw measurements and a design proposal for future parser presets.
- Bound the lexer's initial token reservation and adapt subsequent growth to
  observed token density. Reduce unused token storage for long strings and
  whitespace; preserve token ranges, diagnostics and public APIs. Record paired
  memory/timing measurements and add density-transition regressions.
- Add captured heap peak/retained accounting, retained block attribution and DOM
  inventory, dedicated tracker self-tests, scalable fixtures, and a reproducible
  memory baseline runner. Move stage timings into the uninstrumented benchmark.
- Add JsonFile::writeAtomic(filename, json, pretty=true), using an exclusively
  created sibling temporary file, complete writes, flush/close checks, and one
  native same-filesystem replacement with cleanup on failure.
- Use the same mechanism for CLI file output, retaining formatting and final LF,
  and support formatting a file in place.
- Support UTF-8 Windows paths throughout JsonFile and wide CLI arguments.
- Reject non-regular atomic destinations, including symlinks, Windows reparse
  points, and alternate streams. Preserve existing basic POSIX mode bits;
  document metadata, concurrency, network filesystem, and durability limits.
- Test injected partial-write/flush/close/commit errors, native replacement
  failures, Unicode paths, temporary cleanup, hard links, and installed DLL API.
- Advance the development version to 1.1.0; the published 1.0.0 tag is unchanged.

## 1.0.0 — 2026-11-06

### Correctness

- Precompute array capacity hints in one token pass and reserve after the first
  value parses, avoiding repeated vector growth and deep copies. Skip capacity
  metadata when there are no commas, and preserve ordinary diagnostics for
  malformed or over-depth input.
- Store signed and unsigned 64-bit integers exactly through construction, parsing,
  serialization, nested equality, and numeric sorting.
- Compare integers and doubles without converting large integers to rounded double
  values. Add independently generated rational comparison fixtures.
- Reject integer literal overflow rather than silently falling back to double.
- Preserve floating storage for whole-valued doubles and retain negative zero.
- Complete the source-range token migration and record token start locations.
- Keep digit-only strings quoted instead of treating them as big numbers.
- Preserve finite double values during serialization, independent of locale.
- Reject non-finite output and report input range errors as ParseException.
- Correctly escape every control character U+0000 through U+001F.
- Accept only JSON whitespace; reject raw control characters in strings.
- Validate UTF-8 and decode UTF-16 surrogate pairs; reject unpaired surrogates.
- Balance parser nesting depth for empty and nonempty containers with RAII.
- Make compact output compact and remove the initial pretty-object newline.
- Serialize before opening an output file and report I/O failures.
- Fix variadic push_back visibility and array self-merge.
- Export exception classes from the Windows DLL and implement ParseException
  location getters.

### API compatibility

- Rename the project from JsonHandlerCPP to MasketJSON before its first release:
  use find_package(MasketJSON), MasketJSON::MasketJSON, MasketJSON/ includes,
  the MASKETJSON_* build options/macros, and the masket-json CLI. Public C++
  class names and short header names remain unchanged.
- Add integer type checks, checked asInt64()/asUInt64(), and compareNumbers().
  asNumber() remains a potentially rounded double conversion for compatibility.
- Numeric equality compares values across signed/unsigned/double storage. All
  numeric variants still report typeName() == "number".
- The Json variant layout changes; rebuild the library and its C++ consumers.
  Whole-valued doubles now serialize with .0 when no exponent is present.
- Add mutable and const at(key) / at(index).
- Mutable operator[] converts only null to a container. Using it on a conflicting
  type now throws TypeException instead of silently destroying the old value.
- Deprecate isBigNumber(); it remains available for source compatibility.
- Non-finite numbers, invalid UTF-8 strings, and excessively deep value trees
  now throw OperationException during serialization.
- Normalize pretty formatting; applications should not depend on old whitespace.

### Build and verification

- Add the MIT license with RealMasket attribution and include it in installation
  and release packages.
- Define version 1.0.0 once and expose it through the generated Version.hpp,
  CLI --version, CMake package configuration, and archive names.
- Add opt-in CPack ZIP/tar.gz archives and SHA-256 checksums, with an independent
  extracted-archive consumer test and Release CI artifacts.
- Prepare release notes and a repeatable source/binary release procedure.
- Install a relocatable CMake package with version configuration and the imported
  MasketJSON::MasketJSON target, public headers, libraries, CLI, and docs.
- Preserve short installed includes and add the MasketJSON/ header prefix;
  keep implementation-only headers out of the package.
- Add library version metadata, a Debug suffix, and relative CLI runtime lookup
  on Linux/macOS. Export static definitions and sanitizer requirements.
- Test a separate consumer after relocating the install prefix, including all
  public headers, exported APIs, and CLI loading in the existing CI matrix.
- Add optional static-build stage/allocation profiling and an affinity-controlled
  Windows before/after runner, with raw measurements and an optimization report.
- Add wide/nested/duplicate-key array regressions and a static-build profiler
  smoke check; keep performance thresholds out of CI.
- Add Linux ASan/UBSan configuration with fail-fast undefined-behavior checks.
- Add a dedicated coverage-instrumented libFuzzer parser/round-trip target.
- Add portable corpus replay and deterministic byte/dictionary mutations,
  strict replay of known valid regressions, and failure-artifact checks to CTest.
- Preserve corpus bytes with .gitattributes and upload Linux sanitizer/fuzzer
  logs, crash inputs, and generated corpus from bounded CI jobs.
- Add shared/static CMake options and the MasketJSON::MasketJSON alias.
- Integrate Google Test and CTest, with a pinned, checksummed dependency.
- Add regression tests for the failure cases above.
- Compile all examples and update them to the actual public API.
- Remove unconditional Windows header inclusion on other platforms.
- Align all legacy Visual Studio configurations with C++17 and common include
  paths; disable inconsistent precompiled-header settings.
- Add a Windows/Linux, shared/static, Debug/Release CI matrix.
- Add the masket-json CLI: file/stdin validation, pretty/compact output,
  output files, source diagnostics, and documented exit codes.
- Add CLI integration checks and a bundled example document.
- Replace the interactive benchmark with deterministic dataset generation,
  separate parse/serialize measurements, warm-up, sample medians, and CSV export.
- Add optional Boost.JSON source and Qt6 Core comparison backends, plus CI
  generation and benchmark smoke checks.

### Remaining limits

Decimal/exponent literals still use double. Arbitrary-precision numeric storage
and streaming parsing are future work. Atomic replacement is available in the
1.1.0 development branch with documented filesystem and durability limits.
