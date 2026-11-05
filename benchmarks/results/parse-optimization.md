# Array parsing optimization

Measured on 2026-11-05, Windows x64, Intel Core i5-12600KF, MSVC 19.51 Release.
Baseline: source commit d21853c (already includes exact int64_t/uint64_t).
Candidate: the parser sources in the revision introducing this report.
Both use static JSONHandler and Boost.JSON 1.88.0; Qt 6.11.0 is the same prebuilt
MSVC 2022 x64 kit. No sanitizers, profiling hooks, LTO changes, or compiler-flag
changes are present in the comparison executables.

## Measurements

All processes were pinned to logical CPU 0 before launch. Three rounds alternate
before/after, after/before, before/after. Each operation has five warm-ups, then
nine samples of twenty iterations. Values below are medians of the three
per-round sample medians; raw sample minima/maxima remain in each CSV.

| Dataset | Before, us | After, us | Speed ratio (before / after) |
| --- | ---: | ---: | ---: |
| tiny.json | 1.205 | 1.200 | 1.004x |
| small.json | 275.515 | 184.130 | 1.496x |
| medium.json | 3,189.795 | 2,084.975 | 1.530x |
| large.json | 28,293.775 | 15,851.740 | 1.785x |
| nested.json | 21.105 | 22.150 | 0.953x |

The large dataset contains 8,192 mixed records (1,136,469 input bytes).

| Large dataset parsing | Before, ms | After, ms |
| --- | ---: | ---: |
| JsonHandlerCPP | 28.294 | 15.852 |
| Boost.JSON | 4.834 | 4.915 |
| Qt | 10.723 | 10.705 |

Boost and Qt are unchanged controls. Their differences show machine/timing
variation, not an effect of the JSONHandler patch. Tiny inputs and singleton
nesting do not receive the same benefit as wide arrays. This is a local result;
it is not a universal speedup claim or a CI performance threshold.
The 128-level singleton nesting fixture measured about 5% slower in this run;
the optimization trades an extra capacity-discovery check for wide-array gains.

## Instrumented evidence and change

Separate ParserProfile executables use the same Release toolchain for the
baseline and candidate. This is manual stage/allocation instrumentation, not an
OS CPU-sampling profile. The large input produces 278,530 tokens of 48 bytes,
with token-vector capacity 284,117. Lexing performs two ordinary C++ allocations
(source copy and token storage); the rest of the allocation volume is in tree
construction and container movement/copying.

In this MSVC build, is_nothrow_move_constructible_v<Json> is false. The installed
MSVC vector growth implementation chooses copying when a throwing move and a
copy constructor are available. Growing arrays of already-built nested objects
therefore causes repeated deep copies.

| Whole parse, one operation | Before | After |
| --- | ---: | ---: |
| Ordinary C++ new/new[] calls | 349,362 | 98,331 |
| Cumulative bytes requested from new/new[] | 48,920,482 | 25,122,219 |

Allocation calls fell by 71.9%; cumulative requested bytes fell by 48.6%.
These are not peak or retained memory measurements and exclude aligned
allocation and direct CRT malloc. Instrumented wall times are diagnostic;
only the ordinary comparison CSVs are used for speed ratios above.

The parser now makes one token pass to collect immediate array element counts
in opening order. It reserves each array after the first value parses. Nested
objects, commas inside strings, duplicate keys, and empty arrays keep their
existing semantics. No-comma sources skip metadata via a raw string search.
Malformed delimiters and excessive depth fall back to the original parser's
diagnostics. No public JSON value types or numeric policies changed.

A preliminary candidate showed overhead on singleton nesting; the no-comma
fast path removes unnecessary reservation metadata in that case. Exploratory
uncontrolled and preliminary measurements were not used in the final ratios.

## Reproduce and inspect

- [Runner, build and profiling instructions](../README.md)
- [Run metadata, input hashes and binary hashes](parse-optimization-final/metadata.json)
- [Baseline round 1](parse-optimization-final/before-1.csv), [round 2](parse-optimization-final/before-2.csv), [round 3](parse-optimization-final/before-3.csv)
- [Candidate round 1](parse-optimization-final/after-1.csv), [round 2](parse-optimization-final/after-2.csv), [round 3](parse-optimization-final/after-3.csv)
- [Instrumented baseline](parse-profile-before.csv) and [candidate](parse-profile-after.csv)

Use separate baseline/candidate source and build directories. To run ParserProfile
on the baseline, copy the profiling-only benchmark source and target definition
into a checkout of d21853c; do not copy the candidate parser sources.

Correctness validation includes wide nested arrays, duplicate-key cases, the
normal CTest suite, ASan tests, and bounded mutation replay. Linux CI also
runs UBSan and libFuzzer; measured performance runs are always unsanitized.
