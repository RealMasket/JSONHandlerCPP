# Adaptive token reservation

## Implementation

The lexer starts with at most 256 Token slots instead of source.size()/4 slots.
When full, it estimates the total token count from tokens emitted and input bytes consumed,
including the pending token and EOF. Growth is clamped between 1.5× and 64× capacity,
with overflow-safe bounds against vector::max_size(). A zero-capacity vector starts with one slot.
The estimate is an allocation hint; it adds no input scan or syntax decisions.
Requested capacity is bounded by the initial 256 slots or 64 times the token count at the last growth.
A dense prefix can still overestimate a sparse tail within that bound; a sparse prefix can
underestimate a dense tail and cause further growth. Token contents, diagnostics, DOM and public API are unchanged.

Exploratory runs rejected a simple 4,096-slot initial cap with ordinary vector growth:
repeated large copies hurt wide arrays. A conservative 4× adaptive upper step also caused
extra copying. A 64× upper step allows an estimate to cover wide inputs earlier while keeping
reservation proportional to tokens already seen. A trial with a 12.5% prediction margin
increased object-array memory; the final policy uses the observed density without that extra margin.
These exploratory choices preceded the separate final paired experiment below.

## Method

Baseline `07cbd8a5f7ed997e0753cd8d859c327f31e91d3f`; candidate is that revision plus the lexer changes identified
by normalized source hashes in [experiment.json](token-reserve-msvc1951/experiment.json).
12th Gen Intel(R) Core(TM) i5-12600KF; Microsoft Windows NT 10.0.19045.0; logical CPU 0; static Release; MSVC 19.51.
Boost.JSON 1.88 and Qt 6.11.0 are unchanged timing controls. Both builds use the same options,
libraries and 23 deterministic inputs; frozen executable and input hashes are recorded.
3 rounds × 9 samples × 10 operations, plus warm-up; before/after order alternates by round.
Timing uses ordinary uninstrumented executables. Parse includes result destruction; input loading
and semantic/round-trip checks are outside timing. Tables use the median of round medians;
Δ is after/before − 1 (negative means less time/memory). Paired range uses each round's ratio.
CPU affinity controls placement, not frequency or background activity. Small differences within
the controls' variation should not be treated as reliable improvements; these are local results.

Memory is measured separately, once per stage/build, using captured successful C++ new requests.
Peak means simultaneous requested bytes, including overlapping old/new token buffers during growth.
Caller input, stack/root object, allocator rounding/metadata, direct malloc/CRT internals and profiler
bookkeeping are excluded. This is not RSS; Boost/Qt memory is not compared. Stage peaks/times are not additive.

## Whole-parse captured heap

MiB = 1,048,576 bytes. Retained DOM is identical before/after for every input.

| Input | Before peak MiB | After peak MiB | Δ peak | Retained DOM MiB |
| --- | ---: | ---: | ---: | ---: |
| deep-128.json | 0.0251 | 0.0170 | -32.2% | 0.0049 |
| deep-64.json | 0.0126 | 0.0086 | -31.9% | 0.0024 |
| deep-8.json | 0.0015 | 0.0012 | -22.4% | 0.0003 |
| escaped-4096.json | 57.5001 | 8.7516 | -84.8% | 4.2813 |
| escaped-512.json | 7.1876 | 1.0941 | -84.8% | 0.5352 |
| escaped-64.json | 0.8985 | 0.1427 | -84.1% | 0.0669 |
| large.json | 21.9626 | 21.9152 | -0.2% | 7.6875 |
| long-ascii-4096.json | 56.4728 | 8.6149 | -84.7% | 4.2188 |
| long-ascii-512.json | 7.0592 | 1.0770 | -84.7% | 0.5274 |
| long-ascii-64.json | 0.8825 | 0.1405 | -84.1% | 0.0659 |
| medium.json | 2.7146 | 2.7088 | -0.2% | 0.9610 |
| nested.json | 0.0375 | 0.0375 | +0.0% | 0.0151 |
| small.json | 0.2549 | 0.1721 | -32.5% | 0.0601 |
| tiny.json | 0.0026 | 0.0026 | +0.0% | 0.0006 |
| unicode-4096.json | 56.7775 | 8.6383 | -84.8% | 4.2188 |
| unicode-512.json | 7.0973 | 1.0799 | -84.8% | 0.5274 |
| unicode-64.json | 0.8873 | 0.1409 | -84.1% | 0.0659 |
| wide-numbers-262144.json | 52.6076 | 39.1612 | -25.6% | 10.0000 |
| wide-numbers-32768.json | 6.1327 | 4.6496 | -24.2% | 1.2500 |
| wide-numbers-4096.json | 0.7102 | 0.6148 | -13.4% | 0.1563 |
| wide-objects-16384.json | 44.0182 | 43.9227 | -0.2% | 15.3750 |
| wide-objects-2048.json | 5.4605 | 5.4487 | -0.2% | 1.9219 |
| wide-objects-256.json | 0.6774 | 0.6915 | +2.1% | 0.2403 |

## Uninstrumented parse time

Milliseconds per operation; unchanged Boost/Qt controls show their own parse-time Δ.

| Input | Before ms | After ms | Δ | Paired Δ range | Boost Δ | Qt Δ |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| deep-128.json | 0.0130 | 0.0115 | -11.2% | -11.8…-4.9% | -6.3% | -5.7% |
| deep-64.json | 0.0058 | 0.0049 | -15.5% | -17.0…-7.5% | -7.1% | -5.0% |
| deep-8.json | 0.0009 | 0.0007 | -15.1% | -18.0…-7.6% | -11.1% | -54.4% |
| escaped-4096.json | 11.9637 | 12.0687 | +0.9% | -33.7…+2.9% | -4.0% | +2.5% |
| escaped-512.json | 1.5676 | 1.5433 | -1.6% | -2.2…+4.1% | -11.9% | +0.4% |
| escaped-64.json | 0.1595 | 0.1562 | -2.1% | -2.7…-1.8% | -10.2% | -0.8% |
| large.json | 17.2283 | 16.3624 | -5.0% | -8.2…+12.6% | -1.2% | +1.7% |
| long-ascii-4096.json | 9.5388 | 9.3414 | -2.1% | -2.4…-0.3% | -19.9% | +0.5% |
| long-ascii-512.json | 1.0767 | 1.0095 | -6.2% | -6.5…-4.8% | -1.5% | +0.9% |
| long-ascii-64.json | 0.1286 | 0.1227 | -4.6% | -8.2…+0.1% | -0.1% | -2.8% |
| medium.json | 1.5422 | 1.6485 | +6.9% | +2.8…+7.3% | -7.1% | -2.1% |
| nested.json | 0.0230 | 0.0233 | +1.3% | -6.1…+5.1% | -0.8% | +0.6% |
| small.json | 0.0862 | 0.0813 | -5.7% | -6.5…-1.7% | -4.0% | -2.1% |
| tiny.json | 0.0010 | 0.0011 | +7.8% | +2.9…+7.8% | +0.0% | +2.8% |
| unicode-4096.json | 11.8974 | 11.7570 | -1.2% | -3.1…+0.4% | +2.5% | -0.2% |
| unicode-512.json | 1.3830 | 1.3077 | -5.4% | -9.4…+2.5% | -0.2% | +2.2% |
| unicode-64.json | 0.1661 | 0.1598 | -3.8% | -9.0…-3.5% | -0.1% | -0.2% |
| wide-numbers-262144.json | 28.2429 | 24.1649 | -14.4% | -15.1…-13.2% | +1.7% | +0.1% |
| wide-numbers-32768.json | 3.0417 | 2.7134 | -10.8% | -16.7…-8.1% | +4.9% | +0.6% |
| wide-numbers-4096.json | 0.2314 | 0.2349 | +1.5% | -0.6…+5.9% | +4.1% | -0.9% |
| wide-objects-16384.json | 31.1159 | 30.8826 | -0.7% | -4.9…+0.9% | -2.3% | +1.7% |
| wide-objects-2048.json | 3.0731 | 3.2444 | +5.6% | +2.7…+10.7% | -3.0% | -0.2% |
| wide-objects-256.json | 0.3063 | 0.3152 | +2.9% | -1.2…+4.5% | -2.5% | -0.2% |

## Lexer allocation and time

Allocation calls include token buffers and the lexer's temporary source copy; only tokens remain at the retained snapshot.
The raw CSV also records peak blocks, cumulative requested bytes, and token counts/sizes.

| Input | Tokens | Before capacity | After capacity | Before calls | After calls | Before ms | After ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| deep-128.json | 258 | 324 | 258 | 6 | 3 | 0.0027 | 0.0019 |
| deep-64.json | 130 | 162 | 130 | 6 | 3 | 0.0014 | 0.0010 |
| deep-8.json | 18 | 19 | 18 | 6 | 3 | 0.0004 | 0.0003 |
| escaped-4096.json | 8194 | 1073152 | 8224 | 2 | 3 | 5.8907 | 5.7932 |
| escaped-512.json | 1026 | 134144 | 1028 | 2 | 3 | 0.6847 | 0.7466 |
| escaped-64.json | 130 | 16768 | 256 | 2 | 2 | 0.0794 | 0.0777 |
| large.json | 278530 | 284117 | 283081 | 2 | 4 | 4.5910 | 5.5193 |
| long-ascii-4096.json | 8194 | 1053696 | 8224 | 2 | 3 | 5.7377 | 5.5074 |
| long-ascii-512.json | 1026 | 131712 | 1028 | 2 | 3 | 0.6482 | 0.6089 |
| long-ascii-64.json | 130 | 16464 | 256 | 2 | 2 | 0.0772 | 0.0742 |
| medium.json | 34818 | 35029 | 34902 | 2 | 4 | 0.5709 | 0.6555 |
| nested.json | 386 | 468 | 468 | 4 | 4 | 0.0035 | 0.0034 |
| small.json | 2178 | 3232 | 2232 | 3 | 3 | 0.0225 | 0.0225 |
| tiny.json | 24 | 33 | 33 | 4 | 4 | 0.0004 | 0.0004 |
| unicode-4096.json | 8194 | 1059840 | 8224 | 2 | 3 | 8.0899 | 7.9114 |
| unicode-512.json | 1026 | 132480 | 1028 | 2 | 3 | 0.9447 | 0.8924 |
| unicode-64.json | 130 | 16560 | 256 | 2 | 2 | 0.1122 | 0.1088 |
| wide-numbers-262144.json | 524290 | 667294 | 599962 | 3 | 4 | 14.2931 | 10.0726 |
| wide-numbers-32768.json | 65538 | 77788 | 69940 | 3 | 4 | 1.3910 | 1.0916 |
| wide-numbers-4096.json | 8194 | 9007 | 9513 | 3 | 3 | 0.0721 | 0.0722 |
| wide-objects-16384.json | 557058 | 571981 | 569895 | 2 | 4 | 10.7377 | 11.5703 |
| wide-objects-2048.json | 69634 | 70613 | 70356 | 2 | 4 | 1.1053 | 1.2358 |
| wide-objects-256.json | 8706 | 8713 | 9021 | 2 | 3 | 0.0859 | 0.0911 |

## Interpretation

long-ascii-4096.json: captured peak falls from 56.47 to 8.61 MiB. Median parse-time change is -2.1%.
wide-numbers-262144.json: captured peak falls from 52.61 to 39.16 MiB. Median parse-time change is -14.4%.

There is a measurable tradeoff on medium.json (+6.9% parse time) and wide-objects-2048.json (+5.6%). Their paired ratios are positive in every round.
The large mixed/object fixtures have almost unchanged captured peak; extra token-buffer copying
makes their isolated lexer slower. The large.json whole-parse ratio crosses zero across rounds,
so its lower aggregate time is not evidence of a consistent gain. Tiny-input percentages are
sensitive to timer resolution; substantial control variation also limits claims about small changes.
The policy is selected for its sparse-input memory reduction and wide-number result, accepting these dense-input costs.

## Checks and limits

All 138 memory windows return captured bytes and blocks to zero after destruction; retained categories partition their totals.
DOM inventories, retained DOM categories/bytes and emitted token counts match in both builds.
Regressions cover long strings, whitespace-only and empty inputs, both directions of density change,
every number/string range and EOF location across growth, and error locations after expansion.
The change trades a few additional allocations for substantially less unused token storage on sparse inputs.
Dense/object inputs can still pay for token copying; the tables expose any time or memory regressions.
No timing threshold is imposed in CI. Sanitizer/fuzzer and shared/static platform checks run separately.

## Reproduce

Generate the same inputs with ParseBenchmark --generate DIR and --generate-profile DIR 256.
Build the baseline and candidate as described in [benchmark instructions](../README.md), keeping executables separate.
Capture `ParserProfile INPUT...` into before-memory.csv / after-memory.csv, then run:

```powershell
.\benchmarks\CompareBuilds.ps1 -BeforeExe BEFORE -AfterExe AFTER -Inputs $inputs -OutputDirectory comparison -BeforeRevision BASE -AfterRevision CANDIDATE -Rounds 3 -Samples 9 -Iterations 10 -Cpu 0 -Stages
```

The raw [comparison directory](token-reserve-msvc1951/) contains both memory files, six timing files,
runner metadata and experiment source/profile hashes. After supplying the matching experiment metadata, render with:
`python benchmarks/summarize_token_reserve.py comparison report.md`.
