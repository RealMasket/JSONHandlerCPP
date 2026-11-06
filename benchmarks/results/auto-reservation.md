# Experimental Auto reservation

## Scope and controller

Two private controllers accept an explicit goal through the selected executable: speed or memory.
They do not infer user priorities from JSON. Production ParseOptions, defaults and Json layout are unchanged;
there is no public Auto preset. The experiment is enabled only with MASKETJSON_BUILD_AUTO_EXPERIMENT.
The generated lexer retains the production JSON grammar; private AutoParser/AutoLexer names allow exact
comparison with both independent production fixed modes in the same process.

Decisions occur only when the token buffer is full. The pending token has already been scanned; features use
consumed bytes, emitted/pending tokens, checkpoint density, stability streak and previous relocation volume.
No pre-scan, second parse or full-document classification is performed by the timed controller.
The evaluator's extra parsers and shape analysis run outside timing.

| Goal | Ordinary growth floor | Requested capacity ceiling | Forecast |
| --- | --- | --- | --- |
| Speed | n + max(n/2, 1) | 4n+258 while untrusted; 16n+258 when mature | global density, recent density on sharp change, 6.25% margin |
| Memory | n + max(n/4, 1); n + max(n/2, 1) under copy pressure | 2n+258 | conservative min(global, recent) density |

Here n is the full buffer size before the pending token is inserted. Requests saturate at vector::max_size().
Initial reservation is at most 256 slots. Density ratios below 0.25/above 4 detect sharp changes; ratios 0.5…2
advance stability. Maturity requires n≥1024, consumed bytes≥4096 and two stable checkpoints, or one under
copy pressure. Pressure is previous moved tokens >2n (speed) or >3n (memory). At source end only the pending
token and required EOF are reserved; no ordinary geometric floor is forced for this final known count.
These limits bound speculative requests and keep relocation amortized linear. They are not hard limits on
STL allocator behavior, total DOM memory or wall-clock time. Prior allocations/copies cannot be undone.
A density change can remain invisible until the next expansion, and a large existing reserve can hide it entirely.

## Decision journal

Separate AutoTrace executables record a fixed 512-entry stack journal; no vector/logging allocations occur inside it.
Ordinary timing builds have no journal/TLS trace hook. Profile builds separately count captured heap and token relocation.
Each row records source position, stored tokens, old/requested/actual capacity, ordinary lower/upper bounds,
cumulative moved tokens after successful reserve, checkpoint/global densities, prediction, stability and reason.
Reasons include warming, stable, density_drop/rise, copy_pressure, end_of_input, finish, abort and allocation failure.
clamped/floored flags describe prediction relative to the ordinary window; end_of_input explicitly bypasses its floor.
The stage identifies the lexer while doing whole parse or lex-only. A parser error can follow a successful lexer finish.
Invalid-input acceptance and diagnostic fingerprints are retained; ordinary exceptions still behave as in production.
Journal movement/capacity/count must exactly match the independent heap profile, and dropped records must be zero.

## Protocol and method

The existing 47 fixtures plus the previous 12 LowMemory holdout cases are now 59 known development inputs.
Sixteen new evaluation fixtures use seed 20261107, counts 73/2053, spiky escaped strings, phased densities,
long keys, zigzag nesting, internal/late padding and ragged numbers. Full-size cases were generated only after freeze.
The controller and binaries were frozen at `2026-11-06T17:20:07.091181+00:00` in [freeze.json](auto-msvc1951/freeze.json).
No parameters were changed using held-out results. Development and evaluation have identical executable hashes.
The [pre-recorded plan](../auto/plan.json) sets empirical promotion targets on non-tiny inputs (Balanced parse ≥1 ms):
Speed worst time/oracle ≤1.5; Memory worst time/Balanced ≤2 and worst peak/oracle ≤1.5.
The oracle is the better fixed mode per whole document, available only offline. It is not an online free signal.

12th Gen Intel(R) Core(TM) i5-12600KF; Windows-10-10.0.19045-SP0; logical CPU 0; static Release.
Each split uses 4 rotating rounds × 5 samples × 3 operations, plus five warm-ups.
Parse/lexer timing includes fresh construction and destruction. Input reading, validity checks and profiling are excluded.
Medians below are medians of round medians; raw CSV preserves sample ranges. Tiny inputs, clock frequency, background
activity and code layout limit small comparisons. Targets are experimental, not runtime guarantees or universal rankings.
Captured heap means simultaneous successful C++ new requests, including overlapping buffers. Caller input, stack/root Json,
allocator rounding/metadata, direct malloc/CRT internals and profiler bookkeeping are excluded. It is not RSS.
Copied-token volume means sum(old live tokens × sizeof(Token)) at expansion, not hardware traffic.

```text
compiler=MSVC
compiler_version=19.51.36257.0
configuration=Release
cxx_standard=17
flags=/DWIN32 /D_WINDOWS /EHsc
config_flags=/O2 /Ob2 /DNDEBUG
lto=
```

## Worst-case assessment

Targets are checked on non-tiny inputs only; the full tables retain every input, including small ones.

| Split / goal | Inputs ≥1 ms | Geomean time/Balanced (all inputs) | p95 time/oracle | Worst time/oracle | Worst time/Balanced | Worst peak/oracle | Targets |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| development / speed | 28 | 1.054 | 1.170 | 1.348 | 1.348 | 5.769 | PASS |
| development / memory | 28 | 1.153 | 1.670 | 1.802 | 1.802 | 1.580 | FAIL |
| holdout / speed | 9 | 0.996 | 1.116 | 1.116 | 1.116 | 1.427 | PASS |
| holdout / memory | 9 | 1.103 | 1.531 | 1.531 | 1.531 | 1.026 | PASS |

Largest non-tiny time/oracle losses:

- development/speed: whitespace-interleaved-16384.json, 4.141 ms versus offline oracle 3.072 ms.
- development/memory: dense-zeros-32768.json, 3.334 ms versus offline oracle 1.850 ms.
- holdout/speed: auto-held-bursts-2053.json, 4.790 ms versus offline oracle 4.293 ms.
- holdout/memory: auto-held-inner-padding-2053.json, 4.906 ms versus offline oracle 3.205 ms.

## Full parse-time matrix

Milliseconds; Speed/oracle and Memory/oracle expose the cost of a wrong decision.

| Split / input | Balanced | LowMemory | Auto speed | Auto memory | Speed/oracle | Memory/oracle |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| development / alternating-262144.json | 22.4829 | 34.9165 | 24.4313 | 28.7778 | 1.087 | 1.280 |
| development / alternating-32768.json | 2.3278 | 3.8335 | 2.6956 | 3.8882 | 1.158 | 1.670 |
| development / alternating-4096.json | 0.3554 | 0.6435 | 0.4972 | 0.6341 | 1.399 | 1.784 |
| development / deep-128.json | 0.0109 | 0.0108 | 0.0110 | 0.0110 | 1.015 | 1.018 |
| development / deep-64.json | 0.0061 | 0.0061 | 0.0062 | 0.0062 | 1.014 | 1.014 |
| development / deep-8.json | 0.0008 | 0.0008 | 0.0008 | 0.0008 | 1.021 | 1.021 |
| development / dense-first-262144.json | 22.1957 | 34.1125 | 23.4766 | 27.7538 | 1.058 | 1.250 |
| development / dense-first-32768.json | 2.8105 | 3.9806 | 2.6526 | 3.9081 | 0.944 | 1.391 |
| development / dense-first-4096.json | 0.3788 | 0.6539 | 0.4739 | 0.6122 | 1.251 | 1.616 |
| development / dense-zeros-262144.json | 17.6427 | 29.9073 | 18.7783 | 23.0063 | 1.064 | 1.304 |
| development / dense-zeros-32768.json | 1.8498 | 3.3395 | 1.9535 | 3.3342 | 1.056 | 1.802 |
| development / dense-zeros-4096.json | 0.1802 | 0.4604 | 0.3860 | 0.4508 | 2.142 | 2.501 |
| development / escaped-4096.json | 12.4194 | 12.3173 | 12.8033 | 12.5163 | 1.039 | 1.016 |
| development / escaped-512.json | 1.6081 | 1.6268 | 1.5685 | 1.5998 | 0.975 | 0.995 |
| development / escaped-64.json | 0.1721 | 0.1855 | 0.1811 | 0.1580 | 1.053 | 0.918 |
| development / flat-object-16384.json | 5.4901 | 6.5778 | 5.3161 | 6.5095 | 0.968 | 1.186 |
| development / flat-object-2048.json | 0.5744 | 0.8485 | 0.7183 | 0.8638 | 1.251 | 1.504 |
| development / flat-object-256.json | 0.0716 | 0.0694 | 0.0570 | 0.0593 | 0.822 | 0.854 |
| development / held-empty-mix-257.json | 0.1477 | 0.1979 | 0.1666 | 0.1784 | 1.127 | 1.208 |
| development / held-empty-mix-4099.json | 1.9050 | 2.5294 | 1.7046 | 2.1939 | 0.895 | 1.152 |
| development / held-escape-transition-257.json | 0.1254 | 0.1418 | 0.1112 | 0.1228 | 0.886 | 0.979 |
| development / held-escape-transition-4099.json | 0.7307 | 0.8365 | 0.6522 | 0.8282 | 0.893 | 1.133 |
| development / held-front-nested-257.json | 0.7876 | 0.9449 | 0.7745 | 0.9133 | 0.983 | 1.159 |
| development / held-front-nested-4099.json | 11.9659 | 16.0222 | 11.7058 | 12.2096 | 0.978 | 1.020 |
| development / held-long-keys-257.json | 0.0301 | 0.0308 | 0.0318 | 0.0309 | 1.055 | 1.024 |
| development / held-long-keys-4099.json | 0.5768 | 0.5119 | 0.5627 | 0.5481 | 1.099 | 1.071 |
| development / held-sawtooth-257.json | 0.2641 | 0.2469 | 0.2378 | 0.2449 | 0.963 | 0.992 |
| development / held-sawtooth-4099.json | 4.5807 | 4.2216 | 4.2945 | 4.2263 | 1.017 | 1.001 |
| development / held-tail-records-257.json | 0.4527 | 0.4838 | 0.6970 | 0.5510 | 1.540 | 1.217 |
| development / held-tail-records-4099.json | 8.7145 | 9.4437 | 9.6454 | 9.4787 | 1.107 | 1.088 |
| development / large.json | 16.6198 | 21.9620 | 17.2112 | 21.5699 | 1.036 | 1.298 |
| development / long-ascii-4096.json | 9.5149 | 9.0818 | 9.2817 | 9.0162 | 1.022 | 0.993 |
| development / long-ascii-512.json | 1.0408 | 1.0224 | 0.9743 | 0.9570 | 0.953 | 0.936 |
| development / long-ascii-64.json | 0.1234 | 0.1236 | 0.1207 | 0.1194 | 0.979 | 0.968 |
| development / medium.json | 1.5789 | 1.9558 | 1.6742 | 2.1026 | 1.060 | 1.332 |
| development / nested.json | 0.0248 | 0.0257 | 0.0239 | 0.0248 | 0.962 | 0.997 |
| development / small.json | 0.0872 | 0.0890 | 0.0874 | 0.0848 | 1.002 | 0.973 |
| development / sparse-first-262144.json | 34.3519 | 61.1822 | 40.1939 | 43.6887 | 1.170 | 1.272 |
| development / sparse-first-32768.json | 3.6234 | 6.2506 | 3.5485 | 3.4626 | 0.979 | 0.956 |
| development / sparse-first-4096.json | 0.2091 | 0.2751 | 0.2612 | 0.2294 | 1.249 | 1.097 |
| development / tiny.json | 0.0011 | 0.0013 | 0.0011 | 0.0012 | 1.000 | 1.061 |
| development / unicode-4096.json | 11.0180 | 10.6847 | 10.6503 | 11.3655 | 0.997 | 1.064 |
| development / unicode-512.json | 1.2139 | 1.2289 | 1.1749 | 1.1892 | 0.968 | 0.980 |
| development / unicode-64.json | 0.1514 | 0.1470 | 0.1431 | 0.1472 | 0.973 | 1.001 |
| development / whitespace-interleaved-16384.json | 3.0716 | 3.5376 | 4.1410 | 4.4291 | 1.348 | 1.442 |
| development / whitespace-interleaved-2048.json | 0.3051 | 0.2885 | 0.4234 | 0.4284 | 1.467 | 1.485 |
| development / whitespace-interleaved-256.json | 0.0390 | 0.0375 | 0.0525 | 0.0522 | 1.399 | 1.392 |
| development / whitespace-leading-16384.json | 3.9122 | 4.9291 | 4.0058 | 4.1833 | 1.024 | 1.069 |
| development / whitespace-leading-2048.json | 0.3542 | 0.3982 | 0.3731 | 0.3671 | 1.053 | 1.037 |
| development / whitespace-leading-256.json | 0.0422 | 0.0427 | 0.0426 | 0.0428 | 1.009 | 1.015 |
| development / whitespace-trailing-16384.json | 3.5652 | 3.7732 | 3.3682 | 3.8188 | 0.945 | 1.071 |
| development / whitespace-trailing-2048.json | 0.3456 | 0.3395 | 0.3282 | 0.3595 | 0.967 | 1.059 |
| development / whitespace-trailing-256.json | 0.0412 | 0.0430 | 0.0434 | 0.0413 | 1.054 | 1.004 |
| development / wide-numbers-262144.json | 24.5138 | 35.4929 | 25.3930 | 28.9936 | 1.036 | 1.183 |
| development / wide-numbers-32768.json | 2.6074 | 3.5420 | 2.6196 | 3.8184 | 1.005 | 1.464 |
| development / wide-numbers-4096.json | 0.2187 | 0.2344 | 0.2308 | 0.2327 | 1.055 | 1.064 |
| development / wide-objects-16384.json | 33.0577 | 43.6725 | 32.6057 | 44.6005 | 0.986 | 1.349 |
| development / wide-objects-2048.json | 3.1843 | 4.2639 | 3.2261 | 4.4387 | 1.013 | 1.394 |
| development / wide-objects-256.json | 0.3125 | 0.3287 | 0.3197 | 0.3393 | 1.023 | 1.086 |
| holdout / auto-held-bursts-2053.json | 4.2929 | 5.6679 | 4.7899 | 5.1884 | 1.116 | 1.209 |
| holdout / auto-held-bursts-73.json | 0.1745 | 0.1715 | 0.1746 | 0.1728 | 1.018 | 1.008 |
| holdout / auto-held-inner-padding-2053.json | 3.2052 | 5.9301 | 3.2183 | 4.9061 | 1.004 | 1.531 |
| holdout / auto-held-inner-padding-73.json | 0.1310 | 0.2078 | 0.1894 | 0.1606 | 1.446 | 1.226 |
| holdout / auto-held-late-padding-2053.json | 5.3962 | 5.8925 | 5.7704 | 6.1438 | 1.069 | 1.139 |
| holdout / auto-held-late-padding-73.json | 0.2340 | 0.2112 | 0.2168 | 0.2283 | 1.026 | 1.081 |
| holdout / auto-held-long-keys-2053.json | 2.2446 | 2.3139 | 2.1786 | 2.2881 | 0.971 | 1.019 |
| holdout / auto-held-long-keys-73.json | 0.0727 | 0.0736 | 0.0759 | 0.0741 | 1.045 | 1.020 |
| holdout / auto-held-phases-2053.json | 22.6636 | 25.9026 | 22.5980 | 24.2537 | 0.997 | 1.070 |
| holdout / auto-held-phases-73.json | 0.8460 | 0.9137 | 0.8664 | 0.8620 | 1.024 | 1.019 |
| holdout / auto-held-ragged-numbers-2053.json | 1.6189 | 2.0607 | 1.6247 | 1.9852 | 1.004 | 1.226 |
| holdout / auto-held-ragged-numbers-73.json | 0.0591 | 0.0512 | 0.0365 | 0.0792 | 0.713 | 1.546 |
| holdout / auto-held-spiky-strings-2053.json | 23.4137 | 24.2842 | 24.1975 | 23.7336 | 1.033 | 1.014 |
| holdout / auto-held-spiky-strings-73.json | 1.0894 | 1.1080 | 1.1034 | 1.1104 | 1.013 | 1.019 |
| holdout / auto-held-zigzag-2053.json | 1.5889 | 1.6026 | 1.6113 | 1.5862 | 1.014 | 0.998 |
| holdout / auto-held-zigzag-73.json | 0.1232 | 0.1324 | 0.1053 | 0.1229 | 0.855 | 0.998 |

## Captured whole-parse heap

MiB. Retained DOM categories and counts match all four policies.

| Split / input | Balanced | LowMemory | Auto speed | Auto memory | Memory/oracle |
| --- | ---: | ---: | ---: | ---: | ---: |
| development / alternating-262144.json | 41.197 | 53.501 | 40.372 | 38.597 | 0.937 |
| development / alternating-32768.json | 5.150 | 6.688 | 5.166 | 7.062 | 1.371 |
| development / alternating-4096.json | 0.977 | 0.883 | 0.735 | 0.860 | 0.973 |
| development / deep-128.json | 0.017 | 0.017 | 0.018 | 0.017 | 1.000 |
| development / deep-64.json | 0.009 | 0.009 | 0.009 | 0.009 | 1.000 |
| development / deep-8.json | 0.001 | 0.001 | 0.001 | 0.001 | 1.000 |
| development / dense-first-262144.json | 62.500 | 74.500 | 77.818 | 38.617 | 0.618 |
| development / dense-first-32768.json | 16.813 | 9.313 | 5.769 | 9.324 | 1.001 |
| development / dense-first-4096.json | 0.977 | 1.164 | 2.278 | 1.145 | 1.172 |
| development / dense-zeros-262144.json | 34.500 | 51.500 | 36.000 | 36.535 | 1.059 |
| development / dense-zeros-32768.json | 4.313 | 6.438 | 4.500 | 6.812 | 1.580 |
| development / dense-zeros-4096.json | 0.539 | 0.805 | 0.653 | 0.828 | 1.536 |
| development / escaped-4096.json | 8.752 | 8.797 | 8.774 | 8.831 | 1.009 |
| development / escaped-512.json | 1.094 | 1.100 | 1.097 | 1.094 | 1.000 |
| development / escaped-64.json | 0.143 | 0.143 | 0.143 | 0.143 | 1.000 |
| development / flat-object-16384.json | 5.116 | 6.635 | 5.582 | 7.010 | 1.370 |
| development / flat-object-2048.json | 0.712 | 0.826 | 0.690 | 0.850 | 1.193 |
| development / flat-object-256.json | 0.082 | 0.103 | 0.085 | 0.086 | 1.051 |
| development / held-empty-mix-257.json | 0.227 | 0.241 | 0.235 | 0.229 | 1.007 |
| development / held-empty-mix-4099.json | 3.609 | 3.838 | 3.750 | 3.832 | 1.062 |
| development / held-escape-transition-257.json | 0.112 | 0.112 | 0.112 | 0.114 | 1.020 |
| development / held-escape-transition-4099.json | 0.807 | 0.841 | 0.781 | 0.767 | 0.951 |
| development / held-front-nested-257.json | 0.702 | 0.728 | 0.663 | 0.815 | 1.160 |
| development / held-front-nested-4099.json | 11.771 | 11.121 | 11.110 | 11.744 | 1.056 |
| development / held-long-keys-257.json | 0.043 | 0.043 | 0.043 | 0.043 | 1.000 |
| development / held-long-keys-4099.json | 0.548 | 0.547 | 0.550 | 0.558 | 1.020 |
| development / held-sawtooth-257.json | 0.277 | 0.268 | 0.266 | 0.265 | 0.990 |
| development / held-sawtooth-4099.json | 4.281 | 4.159 | 4.136 | 4.193 | 1.008 |
| development / held-tail-records-257.json | 1.133 | 0.764 | 4.405 | 0.747 | 0.979 |
| development / held-tail-records-4099.json | 54.116 | 12.219 | 70.494 | 12.231 | 1.001 |
| development / large.json | 21.915 | 26.584 | 22.563 | 28.163 | 1.285 |
| development / long-ascii-4096.json | 8.615 | 8.660 | 8.637 | 8.694 | 1.009 |
| development / long-ascii-512.json | 1.077 | 1.083 | 1.080 | 1.077 | 1.000 |
| development / long-ascii-64.json | 0.141 | 0.141 | 0.141 | 0.141 | 1.000 |
| development / medium.json | 2.709 | 3.321 | 2.820 | 3.495 | 1.290 |
| development / nested.json | 0.038 | 0.038 | 0.038 | 0.040 | 1.067 |
| development / small.json | 0.172 | 0.208 | 0.176 | 0.194 | 1.125 |
| development / sparse-first-262144.json | 45.792 | 48.224 | 62.422 | 55.580 | 1.214 |
| development / sparse-first-32768.json | 6.014 | 6.270 | 5.573 | 5.418 | 0.901 |
| development / sparse-first-4096.json | 0.790 | 0.825 | 0.732 | 0.867 | 1.097 |
| development / tiny.json | 0.003 | 0.002 | 0.002 | 0.002 | 0.979 |
| development / unicode-4096.json | 8.638 | 8.684 | 8.660 | 8.718 | 1.009 |
| development / unicode-512.json | 1.080 | 1.086 | 1.083 | 1.080 | 1.000 |
| development / unicode-64.json | 0.141 | 0.141 | 0.141 | 0.141 | 1.000 |
| development / whitespace-interleaved-16384.json | 4.328 | 5.266 | 4.298 | 5.440 | 1.257 |
| development / whitespace-interleaved-2048.json | 0.528 | 0.658 | 0.538 | 0.657 | 1.245 |
| development / whitespace-interleaved-256.json | 0.068 | 0.082 | 0.069 | 0.068 | 1.000 |
| development / whitespace-leading-16384.json | 4.565 | 5.111 | 5.539 | 4.726 | 1.035 |
| development / whitespace-leading-2048.json | 0.588 | 0.624 | 0.716 | 0.607 | 1.033 |
| development / whitespace-leading-256.json | 0.076 | 0.077 | 0.076 | 0.083 | 1.100 |
| development / whitespace-trailing-16384.json | 50.781 | 6.531 | 6.613 | 6.525 | 0.999 |
| development / whitespace-trailing-2048.json | 1.082 | 0.817 | 0.579 | 0.795 | 0.974 |
| development / whitespace-trailing-256.json | 0.794 | 0.102 | 0.102 | 0.079 | 0.772 |
| development / wide-numbers-262144.json | 39.161 | 52.697 | 38.865 | 37.814 | 0.966 |
| development / wide-numbers-32768.json | 4.650 | 6.573 | 4.932 | 6.948 | 1.494 |
| development / wide-numbers-4096.json | 0.615 | 0.820 | 0.673 | 0.843 | 1.372 |
| development / wide-objects-16384.json | 43.923 | 53.182 | 45.227 | 56.366 | 1.283 |
| development / wide-objects-2048.json | 5.449 | 6.644 | 5.674 | 7.019 | 1.288 |
| development / wide-objects-256.json | 0.692 | 0.830 | 0.705 | 0.854 | 1.235 |
| holdout / auto-held-bursts-2053.json | 4.504 | 4.492 | 4.420 | 4.609 | 1.026 |
| holdout / auto-held-bursts-73.json | 0.183 | 0.178 | 0.186 | 0.175 | 0.988 |
| holdout / auto-held-inner-padding-2053.json | 50.154 | 6.163 | 6.110 | 6.310 | 1.024 |
| holdout / auto-held-inner-padding-73.json | 0.827 | 0.217 | 0.258 | 0.223 | 1.026 |
| holdout / auto-held-late-padding-2053.json | 51.785 | 5.285 | 7.540 | 5.270 | 0.997 |
| holdout / auto-held-late-padding-73.json | 0.878 | 0.178 | 0.186 | 0.226 | 1.265 |
| holdout / auto-held-long-keys-2053.json | 2.032 | 1.893 | 1.870 | 1.927 | 1.018 |
| holdout / auto-held-long-keys-73.json | 0.073 | 0.069 | 0.073 | 0.070 | 1.008 |
| holdout / auto-held-phases-2053.json | 26.711 | 20.884 | 27.582 | 21.099 | 1.010 |
| holdout / auto-held-phases-73.json | 0.752 | 0.742 | 0.779 | 0.750 | 1.011 |
| holdout / auto-held-ragged-numbers-2053.json | 2.455 | 2.455 | 2.337 | 2.446 | 0.996 |
| holdout / auto-held-ragged-numbers-73.json | 0.081 | 0.084 | 0.084 | 0.108 | 1.335 |
| holdout / auto-held-spiky-strings-2053.json | 18.631 | 18.577 | 18.566 | 18.586 | 1.001 |
| holdout / auto-held-spiky-strings-73.json | 0.827 | 0.827 | 0.827 | 0.827 | 1.000 |
| holdout / auto-held-zigzag-2053.json | 1.070 | 1.076 | 1.065 | 1.069 | 0.999 |
| holdout / auto-held-zigzag-73.json | 0.103 | 0.109 | 0.098 | 0.102 | 0.985 |

## Observed journal reasons

Lex-only events; whole-parse traces record the same lexer decisions separately.

| Split / goal | Recorded events | Warming | Stable | Density drop/rise | Copy pressure | End-of-input growth |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| development / speed | 310 | 74 | 104 | 13 | 0 | 1 |
| development / memory | 499 | 158 | 174 | 13 | 35 | 1 |
| holdout / speed | 68 | 18 | 12 | 6 | 0 | 0 |
| holdout / memory | 102 | 35 | 24 | 10 | 0 | 1 |

## Interpretation

Bounded requests prevent the previous 64× extrapolation from being applied blindly, while geometric floors
bound token-relocation work. Neither property makes the best fixed strategy inferable from an arbitrary prefix.
The goals express a caller priority; they do not change JSON semantics or discard work already performed.
Failed target rows are evidence against promoting that controller under the stated criteria. Passed rows are
local evidence only; they still need real workloads and more toolchains before a public Auto API.
Automatic mode selection remains experimental; callers retain explicit Balanced/LowMemory control.

A concrete memory counterexample for the speed goal:

- held-tail-records-4099.json: Auto speed 70.49 MiB, Balanced 54.12 MiB, LowMemory 12.22 MiB. Passing the time target does not satisfy a memory constraint.

This is why a caller's priority and resource constraints must remain explicit, and why these prototypes are not a universal selector.

## Verification and reproduce

AutoVerify compares every Token field, canonical storage/zero representation and full error class/location/message
against both independent production parsers in one process. It also checks compact/pretty round trips.
Journal validation enforces event order, requested bounds, endpoint exceptions and relocation-work limits
(Speed ≤5× final tokens+4096, Memory ≤8×+4096). Journal totals match heap-profile token copies/capacity exactly.
All memory windows satisfy category/retained invariants and release tracked bytes and blocks after destruction.
Windows/Linux smoke, sanitizer runs, deterministic corpus mutations and two coverage-instrumented Auto fuzz targets
compare both goals to production. Performance targets are not imposed in shared CI timing environments.

Raw [development, held-out results and journals](auto-msvc1951/) include binary/input/source hashes and execution order.
See [Auto experiment instructions](../README.md#experimental-auto-controller). Render this report with:
`python benchmarks/auto/summarize.py benchmarks/results/auto-msvc1951 REPORT.md`.
