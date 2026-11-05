# Fuzzing and sanitizer checks

The fuzz target exercises JSON parsing and, for accepted input, compact and
pretty serialization followed by reparsing. It checks value equality, integer
versus floating storage, the sign of zero, and stable compact serialization.

Only a source `LexerException` or `ParseException` is an expected rejection.
The catch scope ends before serialization. Unexpected exceptions, round-trip
errors, sanitizer findings, hangs, and excessive memory use fail a fuzz run.
The maximum input size is 64 KiB; the library itself has no new size limit.

## Ordinary tests and portable replay

When `BUILD_TESTING=ON`, CTest includes corpus replay, 2,000 deterministic
mutations, strict replay of known valid regressions, and a test of failure
artifact preservation. No Clang or Python installation is needed.

To build the replay tool without Google Test:

```sh
cmake -S . -B build-replay -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DJSONHANDLER_BUILD_FUZZ_REPLAY=ON
cmake --build build-replay --config Release --target JsonFuzzReplay
```

Windows / Visual Studio:

```powershell
.\build-replay\fuzz\Release\JsonFuzzReplay.exe --mutations 100000 --seed 20261105 --artifacts build-replay/artifacts fuzz/corpus fuzz/regressions
```

On Linux with a single-configuration generator, use
`./build-replay/fuzz/JsonFuzzReplay` and configure `CMAKE_BUILD_TYPE=Release`.

The replay tool accepts individual files and recursively reads directories.
A fixed seed makes a run repeatable with the same corpus, binary, and options.
It reports accepted/rejected counts; invalid JSON is a normal result. Mutations
are bounded byte flips, insertions, erasures, duplication, dictionary splicing,
and wrapping. This runner is **not coverage-guided**; libFuzzer is.

## ASan and UBSan on Linux

Use GCC or Clang and a separate build directory:

```sh
CC=clang CXX=clang++ cmake -S . -B build-sanitized -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DJSONHANDLER_ENABLE_SANITIZERS=ON -DJSONHANDLER_BUILD_BENCHMARKS=ON
cmake --build build-sanitized --parallel 2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build-sanitized --output-on-failure
```

The sanitizer option instruments the library and its own consumers and links
runtimes through the compiler driver. Undefined-behavior recovery is disabled.
It currently requires Linux; unsupported compiler/platform combinations fail at
configuration instead of silently omitting checks.

## Coverage-guided libFuzzer

Requirements: Linux Clang with compiler-rt/libFuzzer, static build, sanitizers
enabled. A dedicated copy of the library sources receives coverage
instrumentation, so ordinary executables do not acquire a libFuzzer `main()`
or coverage-runtime dependency.

```sh
CC=clang CXX=clang++ cmake -S . -B build-fuzz -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF -DJSONHANDLER_BUILD_CLI=OFF -DJSONHANDLER_BUILD_EXAMPLES=OFF -DJSONHANDLER_ENABLE_SANITIZERS=ON -DJSONHANDLER_BUILD_FUZZERS=ON
cmake --build build-fuzz --target JsonParserFuzz --parallel 2
mkdir -p build-fuzz/corpus build-fuzz/artifacts
cp fuzz/corpus/* fuzz/regressions/* build-fuzz/corpus/
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ./build-fuzz/fuzz/JsonParserFuzz build-fuzz/corpus -dict=fuzz/json.dict -seed=20261105 -max_total_time=60 -max_len=65536 -timeout=5 -rss_limit_mb=2048 -print_final_stats=1 -artifact_prefix=build-fuzz/artifacts/
```

Use a copy of the corpus: libFuzzer writes newly interesting inputs into its
first corpus directory. Run longer locally by increasing `-max_total_time`;
the CI smoke run is deliberately bounded. A fixed libFuzzer seed does not
guarantee identical execution counts between machines or compiler versions.

## Corpora and failures

- `corpus/` contains valid and malformed inputs, including raw control bytes,
  invalid UTF-8, Unicode escapes, overflow, duplicate keys, and depth boundaries.
- `regressions/` contains valid reproducers for bugs already fixed during this
  project's earlier review. Every file must remain accepted and round-trip.
- Nested `.gitattributes` preserves corpus bytes across Windows/Linux checkouts.
- Normal C++ failures in replay write `reproducer.bin` and `failure.txt` into the
  chosen artifact directory before returning nonzero. The file is overwritten
  on the next failure. Hard process crashes are handled by the libFuzzer driver
  rather than by replay's C++ exception handler.
- libFuzzer writes crash, timeout, and OOM reproducers under `-artifact_prefix`.
  CI uploads the working corpus, crash inputs, and log, even when a job fails.

Reproduce one libFuzzer finding without starting a mutation campaign:

```sh
./build-fuzz/fuzz/JsonParserFuzz build-fuzz/artifacts/crash-HASH
```

After fixing a finding, retain its exact minimized input in the repository.
Put valid inputs in `regressions/`; malformed inputs belong in `corpus/` and
should also receive a unit test asserting their expected rejection. Never
discard a failing input merely to make CI pass.

## CI

The normal Windows/Linux matrix replays the corpus. Additional Linux jobs run
the full CTest suite under ASan/UBSan and a 60-second libFuzzer campaign.
Sanitizer logs and fuzz artifacts are retained for 14 days. No benchmark
performance conclusions are drawn from instrumented builds.

These are bounded checks, not proof that every input is safe or conformant.

## References

- [LLVM libFuzzer](https://llvm.org/docs/LibFuzzer.html)
- [Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [Clang UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [MSVC AddressSanitizer](https://learn.microsoft.com/en-us/cpp/sanitizers/asan)
