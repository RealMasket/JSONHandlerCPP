# Changelog

## Unreleased

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
- Add shared/static CMake options and the JsonHandlerCPP::JsonHandlerCPP alias.
- Integrate Google Test and CTest, with a pinned, checksummed dependency.
- Add regression tests for the failure cases above.
- Compile all examples and update them to the actual public API.
- Remove unconditional Windows header inclusion on other platforms.
- Align all legacy Visual Studio configurations with C++17 and common include
  paths; disable inconsistent precompiled-header settings.
- Add a Windows/Linux, shared/static, Debug/Release CI matrix.
- Add the jsonhandler CLI: file/stdin validation, pretty/compact output,
  output files, source diagnostics, and documented exit codes.
- Add CLI integration checks and a bundled example document.
- Replace the interactive benchmark with deterministic dataset generation,
  separate parse/serialize measurements, warm-up, sample medians, and CSV export.
- Add optional Boost.JSON source and Qt6 Core comparison backends, plus CI
  generation and benchmark smoke checks.

### Remaining limits

Decimal/exponent literals still use double. Arbitrary-precision numeric storage,
atomic file replacement and streaming parsing are future work.
