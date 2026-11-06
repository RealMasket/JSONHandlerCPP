# JSON performance comparison

Historical files under `results/` retain the JsonHandlerCPP name and original
version labels from the measured commits. New measurements identify the library
as MasketJSON and use the configured project version.

The benchmark measures this library and optional local installations of
Boost.JSON and Qt6 Core. It is a comparison harness, not a claim of universal
performance. It does not download Boost or Qt. The ordinary comparison measures
time; the separate ParserProfile executable measures MasketJSON's captured C++ heap.

## Build

A library-only benchmark needs no extra JSON dependency:

```sh
cmake -S . -B build-bench -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DMASKETJSON_BUILD_BENCHMARKS=ON
cmake --build build-bench --config Release --target ParseBenchmark
```

To compare all three libraries with the installations on this Windows machine:

```powershell
cmake -S . -B build-bench -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=OFF -DMASKETJSON_BUILD_BENCHMARKS=ON -DMASKETJSON_BENCHMARK_BOOST=ON -DMASKETJSON_BOOST_ROOT=E:/C++Libs/boost_1_88_0 -DMASKETJSON_BENCHMARK_QT=ON -DCMAKE_PREFIX_PATH=C:/Qt/Qt6.11.0/6.11.0/msvc2022_64
cmake --build build-bench --config Release --target ParseBenchmark
```

These paths are examples, not embedded project settings. Use a Qt kit compatible
with your compiler and architecture. Boost.JSON is compiled from
`boost/json/src.hpp` using the benchmark toolchain; precompiled Boost binaries
are not used. MSVC auto-linking is disabled. Qt is linked as an installed
library; its build settings can differ from ours.

With a single-configuration generator such as Unix Makefiles, add
`-DCMAKE_BUILD_TYPE=Release`. On Windows, CMake copies the required JSONHandler
and Qt Core DLLs next to the benchmark when applicable.

## Generate and run

Windows / Visual Studio:

```powershell
.\build-bench\benchmarks\Release\ParseBenchmark.exe --generate benchmark-data
.\build-bench\benchmarks\Release\ParseBenchmark.exe --iterations 100 --samples 9 --csv results.csv benchmark-data/tiny.json benchmark-data/small.json benchmark-data/medium.json benchmark-data/large.json benchmark-data/nested.json
```

With a single-configuration generator, use
`./build-bench/benchmarks/ParseBenchmark` instead. Arbitrary input paths are also
accepted. Missing files, invalid JSON, and semantic mismatches fail the run;
CSV is written only after all requested datasets finish successfully. Existing
CSV files are overwritten. `--help` describes the options.

The generator writes five deterministic datasets:

| File | Bytes | Content |
| --- | ---: | --- |
| tiny.json | 61 | Small object with mixed values |
| small.json | 8,621 | 64 mixed records |
| medium.json | 140,117 | 1,024 mixed records |
| large.json | 1,136,469 | 8,192 mixed records |
| nested.json | 833 | Alternating objects/arrays, 128 container levels |

Records exercise numbers, booleans, arrays, strings containing digits, escaped
control characters, Cyrillic, and a surrogate pair. Numbers are chosen within
the common representable range. The generator overwrites these five named files
in the destination directory.

## Method

- Load each input once, outside measurement.
- Check that serialization from each library reparses to the same JSONHandler
  value as the input. These checks are outside timing.
- Warm up each operation five times.
- Measure several samples with `std::chrono::steady_clock`. Each sample performs
  the requested number of iterations.
- Report the median time per operation; CSV also contains minimum and maximum
  sample times, input size, iterations, samples, versions, compiler, and build.
- Parsing includes construction and destruction of a fresh DOM each iteration.
  Each operation also reads the root container size for an observable checksum.
- Serialization uses a preparsed DOM and compact output, including allocation
  and destruction of the resulting string. A checksum prevents unused work
  from being discarded.
- Boost.JSON uses its default memory resource and a maximum depth of 128,
  matching JSONHandler. Other parser settings use library defaults.
- Qt uses `QJsonDocument::fromJson` and `toJson(Compact)`. The input `QByteArray`
  view is prepared outside timing. Scalar roots are skipped for Qt because
  QJsonDocument represents object/array documents.
- MiB/s is normalized to **input text bytes**, including for serialization;
  output sizes and escaping styles can differ.

Library order is fixed and no custom allocators or parser reuse are enabled.
Use an idle machine and inspect sample spread. Tiny inputs are sensitive to
clock resolution and call overhead. Run-to-run variation and implementation
differences limit conclusions; CI smoke tests check functionality, not speed.

## Local results

See [the array parsing optimization report](results/parse-optimization.md) for
paired before/after measurements on the current integer-capable parser.

See [results/windows-msvc1951.md](results/windows-msvc1951.md) and its raw CSV
for the historical local Release run at commit `fd56159`, before exact integer
storage was introduced. Rerun the harness to measure the current revision.

## Allocation and stage profiling

`MASKETJSON_BUILD_PROFILER=ON` adds `ParserProfile` to a static, unsanitized
build. It links replacement ordinary/aligned, array/scalar, throwing/nothrow
allocation/deallocation operators. Production and ParseBenchmark never link these.

```powershell
.\build-bench\benchmarks\Release\ParserProfile.exe --self-test
.\build-bench\benchmarks\Release\ParserProfile.exe benchmark-data/large.json > memory.csv
.\build-bench\benchmarks\Release\ParseBenchmark.exe --stages --csv timing.csv benchmark-data/large.json
.\build-bench\benchmarks\Release\ParseBenchmark.exe --generate-profile profile-data 256
```

ParserProfile emits **memory only**, once per input and stage: whole_parse,
lex_only, input_copy. The old optional ITERATIONS argument and timed CSV schema
are replaced. `ParseBenchmark --stages` supplies uninstrumented lexer/copy timings
in addition to ordinary parse/serialize comparisons. Stage times and peaks are
not additive; do not subtract them to claim an isolated DOM construction cost.
Use the default non-LTO build: the copy buffer escapes through an out-of-line
consumer so the compiler cannot replace a full copy with a size/first-byte read.

- `new_calls`/`new_bytes` are cumulative successful requests during the window.
  `peak_live_bytes` is the maximum simultaneous requested bytes, not cumulative volume.
  `retained_bytes` is sampled while the returned DOM/tokens/copy remain alive.
  `after_destroy_bytes` and `after_destroy_blocks` must both be zero.
- Input reading and a validation round-trip happen before capture. The caller's
  original input, root Json stack object, stack, allocator rounding/metadata,
  direct malloc calls, shared-runtime internals, and tracker bookkeeping are excluded.
  These numbers are **not RSS/working set or total process memory**. Tracking and
  live-block indexing use direct malloc, so their overhead cannot inflate counters.
- The captured owner thread allocates; frees from any thread are recognized.
  Pointers born before/after a window are safely ignored. Only one window is allowed.
  This matches the current single-threaded parser; it is not a background-thread
  allocation profiler. CTest self-tests cover resizing/tombstones, short-lived and
  surviving windows, cross-thread frees, zero-size, aligned/nothrow allocations,
  exception unwinding, and STL interior-aligned buffers.
- Retained blocks are labeled using public container data/entry addresses:
  strings (keys and values), array storage, map nodes, tokens, input copies, other.
  Container ownership wins over in-place SSO strings. Interior pointers are supported
  for MSVC's manually aligned large allocations. Block labels are implementation
  attribution, not portable STL node sizes; other includes unclassified bookkeeping.
  Categories partition retained bytes, not peak bytes. String capacity counts
  include SSO capacity and are character counts, not allocated heap bytes.
- The profiler measures only MasketJSON. Boost/Qt use different allocators and DLL
  paths; global new interception would not provide comparable memory coverage.
  Their uninstrumented timing comparisons validate semantics before measuring.

`--generate-profile DIR [BASE=256]` generates 18 deterministic fixtures: three
sizes each of wide objects, mixed numbers (within the common exact range), long
ASCII strings, JSON-escaped strings, raw UTF-8 strings, plus depth 8/64/128 arrays.
The original five `--generate` fixtures retain their original bytes. CI uses base 4
and checks generated input semantics and every memory CSV accounting invariant.

On Windows, `RunMemoryBaseline.ps1` generates both suites, pins children to one
logical CPU, captures memory once, and runs separate timing rounds. It saves
binary/input/tool-source hashes, CPU/OS, raw CSVs, and logs to a fresh directory:
Tool-source hashes normalize UTF-8 BOM and CRLF to UTF-8/LF for checkout portability;
binary and input hashes cover the actual measured bytes.

```powershell
.\benchmarks\RunMemoryBaseline.ps1 -BenchmarkExe .\build-bench\benchmarks\Release\ParseBenchmark.exe -ProfileExe .\build-bench\benchmarks\Release\ParserProfile.exe -OutputDirectory baseline -Iterations 10 -Samples 9 -Rounds 3 -Cpu 0
```

Optionally render a Markdown report using Python 3's standard library:
`python benchmarks/summarize_memory_baseline.py baseline report.md`.
See [the captured baseline](results/parser-memory-baseline.md) for raw memory,
three ordinary timing rounds, metadata, and findings. Generated JSON files and
transient logs are reproducible and are not checked into the repository.

## Comparing two builds on Windows

Keep baseline and candidate executables in separate build directories. Build
both as Release with the same compiler, static-library setting, external library
versions, and dataset generator. Disable sanitizers and instrumentation.

`CompareBuilds.ps1` pins its process to one logical CPU before starting children,
which inherit that affinity. It alternates before/after order across rounds and
saves individual CSVs, binary/input hashes, and run settings. Supply a fresh
output directory to avoid mixing incomplete runs with earlier results.

```powershell
.\benchmarks\CompareBuilds.ps1 -BeforeExe .\build-before\benchmarks\Release\ParseBenchmark.exe -AfterExe .\build-after\benchmarks\Release\ParseBenchmark.exe -Inputs @("benchmark-data/tiny.json","benchmark-data/small.json","benchmark-data/medium.json","benchmark-data/large.json","benchmark-data/nested.json") -OutputDirectory comparison-results -BeforeRevision BASE_COMMIT -AfterRevision CANDIDATE_COMMIT -Rounds 3 -Iterations 20 -Samples 9 -Cpu 0
```

Keep the machine idle during comparison. Affinity controls CPU placement, not
frequency or background activity. Boost/Qt rows act as unchanged controls; use
their variation and the per-sample ranges to judge small apparent differences.

## References

- [Boost.JSON 1.88 documentation](https://www.boost.org/doc/libs/1_88_0/libs/json/doc/html/index.html)
- [Qt QJsonDocument](https://doc.qt.io/qt-6/qjsondocument.html)
