# Parser memory and timing baseline

Local Windows Release baseline on 12th Gen Intel(R) Core(TM) i5-12600KF; Microsoft Windows NT 10.0.19045.0; logical CPU 0.
Source base: `e2881cb0ff570b99df0feb4e3957043d4deedf33` plus the profiling-only files and hashes in metadata.
Parser, lexer, value representation, serializer, and file I/O implementations are unchanged in this step.

## Method

23 deterministic inputs; memory captured once per stage. Ordinary timings use 3 rounds × 9 samples × 10 operations, plus warm-up.
Tables show the median of round medians; raw CSV includes each sample range. Fixed library order,
CPU frequency and background activity limit small comparisons. These are local results, not universal rankings.
File loading and round-trip/semantic validation are outside measurements. Parse timing includes result destruction.
Libraries: MasketJSON 1.1.0 (MSVC 1951); Boost.JSON 1_88 (MSVC 1951); Qt 6.11.0 (MSVC 1951).

Captured heap counts successful C++ new/new[] requests, including aligned/nothrow forms.
Peak is simultaneous requested bytes; retained is sampled before result destruction. Caller input, stack,
root Json object, allocator rounding/metadata, direct malloc/CRT internals, and profiler bookkeeping are excluded.
This is not process RSS/working set. Boost/Qt memory is not measured because allocator coverage would differ.
Retained labels use owning container addresses (including interior-aligned MSVC buffers); SSO stays with its owner.
Other contains unclassified STL bookkeeping. Categories partition retained memory, not peak memory.
Stage times/peaks are separate observations, not additive components; subtraction is not a DOM-only measurement.

## Captured MasketJSON heap

MiB = 1,048,576 bytes. Small-document ratios include fixed container/token overhead.

| Input | Input MiB | Peak MiB | Retained DOM MiB | Peak/input |
| --- | ---: | ---: | ---: | ---: |
| deep-128.json | 0.0002 | 0.0251 | 0.0049 | 102.22× |
| deep-64.json | 0.0001 | 0.0126 | 0.0024 | 102.19× |
| deep-8.json | 0.0000 | 0.0015 | 0.0003 | 92.24× |
| escaped-4096.json | 4.0938 | 57.5001 | 4.2813 | 14.05× |
| escaped-512.json | 0.5117 | 7.1876 | 0.5352 | 14.05× |
| escaped-64.json | 0.0640 | 0.8985 | 0.0669 | 14.05× |
| large.json | 1.0838 | 21.9626 | 7.6875 | 20.26× |
| long-ascii-4096.json | 4.0195 | 56.4728 | 4.2188 | 14.05× |
| long-ascii-512.json | 0.5024 | 7.0592 | 0.5274 | 14.05× |
| long-ascii-64.json | 0.0628 | 0.8825 | 0.0659 | 14.05× |
| medium.json | 0.1336 | 2.7146 | 0.9610 | 20.31× |
| nested.json | 0.0008 | 0.0375 | 0.0151 | 47.21× |
| small.json | 0.0082 | 0.2549 | 0.0601 | 31.01× |
| tiny.json | 0.0001 | 0.0026 | 0.0006 | 44.33× |
| unicode-4096.json | 4.0430 | 56.7775 | 4.2188 | 14.04× |
| unicode-512.json | 0.5054 | 7.0973 | 0.5274 | 14.04× |
| unicode-64.json | 0.0632 | 0.8873 | 0.0659 | 14.04× |
| wide-numbers-262144.json | 1.6970 | 52.6076 | 10.0000 | 31.00× |
| wide-numbers-32768.json | 0.1978 | 6.1327 | 1.2500 | 31.00× |
| wide-numbers-4096.json | 0.0229 | 0.7102 | 0.1563 | 31.00× |
| wide-objects-16384.json | 2.1819 | 44.0182 | 15.3750 | 20.17× |
| wide-objects-2048.json | 0.2694 | 5.4605 | 1.9219 | 20.27× |
| wide-objects-256.json | 0.0332 | 0.6774 | 0.2403 | 20.38× |

## Retained allocation attribution

| Input | String MiB | Array MiB | Map node MiB | Other MiB |
| --- | ---: | ---: | ---: | ---: |
| large.json | 0.2500 | 0.9375 | 5.6875 | 0.8125 |
| wide-objects-16384.json | 0.5000 | 1.8750 | 11.3750 | 1.6250 |
| wide-numbers-262144.json | 0.0000 | 10.0000 | 0.0000 | 0.0000 |
| long-ascii-4096.json | 4.0625 | 0.1563 | 0.0000 | 0.0000 |
| escaped-4096.json | 4.1250 | 0.1563 | 0.0000 | 0.0000 |

## Uninstrumented parse/stage timings

Milliseconds per operation.

| Input | MasketJSON | Boost.JSON | Qt | MasketJSON lexer |
| --- | ---: | ---: | ---: | ---: |
| deep-128.json | 0.0130 | 0.0062 | 0.0170 | 0.0024 |
| deep-64.json | 0.0057 | 0.0042 | 0.0096 | 0.0014 |
| deep-8.json | 0.0008 | 0.0003 | 0.0016 | 0.0004 |
| escaped-4096.json | 11.7810 | 6.6033 | 20.2699 | 5.9084 |
| escaped-512.json | 1.4595 | 0.7281 | 2.4235 | 0.6779 |
| escaped-64.json | 0.1429 | 0.0862 | 0.2833 | 0.0799 |
| large.json | 16.9778 | 4.7764 | 10.2588 | 4.7124 |
| long-ascii-4096.json | 9.3048 | 0.8952 | 8.0027 | 5.7703 |
| long-ascii-512.json | 1.0719 | 0.0500 | 0.8643 | 0.6597 |
| long-ascii-64.json | 0.1278 | 0.0074 | 0.0919 | 0.0763 |
| medium.json | 1.5141 | 0.5967 | 1.2384 | 0.5692 |
| nested.json | 0.0229 | 0.0114 | 0.0190 | 0.0032 |
| small.json | 0.0881 | 0.0443 | 0.0830 | 0.0225 |
| tiny.json | 0.0010 | 0.0003 | 0.0007 | 0.0004 |
| unicode-4096.json | 11.6962 | 8.3636 | 7.5946 | 8.0583 |
| unicode-512.json | 1.3604 | 0.8825 | 0.8167 | 0.9323 |
| unicode-64.json | 0.1655 | 0.1104 | 0.0864 | 0.1106 |
| wide-numbers-262144.json | 28.4376 | 5.5888 | 11.7465 | 14.7361 |
| wide-numbers-32768.json | 3.0589 | 0.3436 | 1.2864 | 1.4156 |
| wide-numbers-4096.json | 0.2243 | 0.0398 | 0.1435 | 0.0721 |
| wide-objects-16384.json | 29.5251 | 9.7529 | 20.9824 | 10.5597 |
| wide-objects-2048.json | 3.1322 | 1.1751 | 2.4986 | 1.1084 |
| wide-objects-256.json | 0.3073 | 0.1504 | 0.3111 | 0.0827 |

## Findings and next experiment

Long ASCII strings: 4.02 MiB input, 56.47 MiB captured peak, 4.22 MiB retained DOM. Lexing retains 48.23 MiB of token storage for 8194 tokens with capacity 1053696.
The lexer currently reserves source.size()/4 Token records. This is evidence that byte-based token
reservation is costly for long strings with few tokens. A bounded/adaptive reservation policy is
the first proposed experiment; it must be checked against wide-number/object inputs and depth cases.

Wide numbers: lexer peak 52.61 MiB; whole-parse peak 52.61 MiB; retained DOM 10.00 MiB.
Inspect token-vector growth as well as initial reservation; reducing reservation can trade memory
against reallocation/copying cost. No parser optimization is included in this baseline.

All 69 memory windows returned captured live bytes AND blocks to zero after destruction.
This validates captured allocation lifetimes, not all process allocations or a general leak-free claim.

## Reproduce

See [benchmark instructions](../README.md) and RunMemoryBaseline.ps1.
Generate the original five fixtures plus --generate-profile BASE=256; use a static unsanitized Release build.
Enable local Boost.JSON and Qt backends for the same semantic-checked timing comparison.
Tracker self-tests and CTest fixture/accounting checks are correctness checks, never performance thresholds.

Raw files: [metadata.json](parser-memory-msvc1951/metadata.json), [memory.csv](parser-memory-msvc1951/memory.csv), [timing-1.csv](parser-memory-msvc1951/timing-1.csv), [timing-2.csv](parser-memory-msvc1951/timing-2.csv), [timing-3.csv](parser-memory-msvc1951/timing-3.csv).
