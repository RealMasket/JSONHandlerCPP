# Reservation strategy matrix

## Experiment

Base `2708b83158b19a7a1a802b52c756f352519e32fe`; 12th Gen Intel(R) Core(TM) i5-12600KF; Windows-10-10.0.19045-SP0; logical CPU 0.
47 deterministic inputs: the previous 23 plus 24 fixtures for dense short values, flat objects,
leading/trailing/interleaved whitespace, both directions of density change, and alternating dense/sparse blocks.
Four isolated experimental builds change allocation policy only. The generator preserves the rest of
the production lexer text verbatim; a private class name and parser object route each executable to its variant.
The production lexer, public headers, default behavior and JSON representation are unchanged.

| Strategy | Initial reserve | Subsequent growth |
| --- | --- | --- |
| byte-quarter | input bytes / 4 | ordinary STL vector growth (historical policy) |
| geometric | min(input bytes / 4, 256) | ordinary STL vector growth |
| adaptive4 | min(input bytes / 4, 256) | observed density, bounded 1.5×…4× |
| adaptive64 | min(input bytes / 4, 256) | observed density, bounded 1.5×…64× (current policy) |

4 rounds × 7 samples × 5 operations, with five warm-ups per operation.
A cyclic order puts each strategy in every position relative to the other experimental strategies once per four rounds.
An unmodified production build (`current`) runs before/after alternating rounds as a same-algorithm control
for adaptive64. All timings use the same small harness and build flags; parse includes DOM destruction.
Lexer timing includes its source copy and result destruction. File loading, validation and profiling are outside timing.
Counters exist only in separate instrumented executables; memory and timings are never taken from the same executable.
No Boost/Qt timings are mixed into this experiment: it compares reservation policies within MasketJSON.

Captured heap is simultaneous requested C++ new bytes; it excludes caller input, stack, root Json object,
allocator rounding/metadata, direct malloc/CRT internals, and profiler bookkeeping. It is not RSS.
Copied-token bytes mean sum(old live token count × sizeof(Token)) at successful buffer expansions;
this is logical relocation volume, not measured hardware memory traffic. Peaks include overlapping buffers.
STL growth and allocation sizes are toolchain-specific; results are local, not universal rankings.

Build information:

```text
compiler=MSVC
compiler_version=19.51.36257.0
configuration=Release
cxx_standard=17
flags=/DWIN32 /D_WINDOWS /EHsc
config_flags=/O2 /Ob2 /DNDEBUG
lto=
```

## Shape inventory

Density includes EOF; whitespace excludes spaces inside string literals. String lengths are raw JSON bytes.

| Input | KiB | Tokens/KiB | Whitespace % | Max raw string KiB | Depth | Max array/object width | First/last 4 KiB tokens |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| alternating-262144.json | 2560.75 | 204.94 | 0.0 | 8.00 | 1 | 262400/0 | 2050/1 |
| alternating-32768.json | 320.09 | 204.95 | 0.0 | 8.00 | 1 | 32800/0 | 2050/1 |
| alternating-4096.json | 40.01 | 204.98 | 0.0 | 8.00 | 1 | 4100/0 | 2050/1 |
| deep-128.json | 0.25 | 1027.98 | 0.0 | 0.00 | 128 | 1/0 | 257/257 |
| deep-64.json | 0.13 | 1031.94 | 0.0 | 0.00 | 64 | 1/0 | 129/129 |
| deep-8.json | 0.02 | 1084.24 | 0.0 | 0.00 | 8 | 1/0 | 17/17 |
| dense-first-262144.json | 2560.00 | 204.80 | 0.0 | 2048.00 | 1 | 262145/0 | 4096/1 |
| dense-first-32768.json | 320.00 | 204.81 | 0.0 | 256.00 | 1 | 32769/0 | 4096/1 |
| dense-first-4096.json | 40.00 | 204.88 | 0.0 | 32.00 | 1 | 4097/0 | 4096/1 |
| dense-zeros-262144.json | 512.00 | 1024.00 | 0.0 | 0.00 | 1 | 262144/0 | 4096/4096 |
| dense-zeros-32768.json | 64.00 | 1024.02 | 0.0 | 0.00 | 1 | 32768/0 | 4096/4096 |
| dense-zeros-4096.json | 8.00 | 1024.12 | 0.0 | 0.00 | 1 | 4096/0 | 4096/4096 |
| escaped-4096.json | 4192.00 | 1.95 | 0.0 | 1.02 | 1 | 4096/0 | 8/7 |
| escaped-512.json | 524.00 | 1.96 | 0.0 | 1.02 | 1 | 512/0 | 8/7 |
| escaped-64.json | 65.50 | 1.98 | 0.0 | 1.02 | 1 | 64/0 | 8/7 |
| flat-object-16384.json | 266.30 | 246.10 | 0.0 | 0.01 | 1 | 0/16384 | 1234/911 |
| flat-object-2048.json | 29.83 | 274.66 | 0.0 | 0.01 | 1 | 0/2048 | 1234/1024 |
| flat-object-256.json | 3.29 | 312.22 | 0.0 | 0.01 | 1 | 0/256 | 1025/1025 |
| large.json | 1109.83 | 250.97 | 0.0 | 0.03 | 3 | 8192/7 | 1041/998 |
| long-ascii-4096.json | 4116.00 | 1.99 | 0.0 | 1.00 | 1 | 4096/0 | 8/7 |
| long-ascii-512.json | 514.50 | 1.99 | 0.0 | 1.00 | 1 | 512/0 | 8/7 |
| long-ascii-64.json | 64.31 | 2.02 | 0.0 | 1.00 | 1 | 64/0 | 8/7 |
| medium.json | 136.83 | 254.46 | 0.0 | 0.03 | 3 | 1024/7 | 1041/1002 |
| nested.json | 0.81 | 474.51 | 0.0 | 0.01 | 128 | 1/1 | 385/385 |
| small.json | 8.42 | 258.70 | 0.0 | 0.03 | 3 | 64/7 | 1041/1027 |
| sparse-first-262144.json | 2560.00 | 204.80 | 0.0 | 2048.00 | 1 | 262145/0 | 2/4096 |
| sparse-first-32768.json | 320.00 | 204.81 | 0.0 | 256.00 | 1 | 32769/0 | 2/4096 |
| sparse-first-4096.json | 40.00 | 204.88 | 0.0 | 32.00 | 1 | 4097/0 | 2/4096 |
| tiny.json | 0.06 | 402.89 | 0.0 | 0.01 | 2 | 3/4 | 23/23 |
| unicode-4096.json | 4140.00 | 1.98 | 0.0 | 1.01 | 1 | 4096/0 | 8/7 |
| unicode-512.json | 517.50 | 1.98 | 0.0 | 1.01 | 1 | 512/0 | 8/7 |
| unicode-64.json | 64.69 | 2.01 | 0.0 | 1.01 | 1 | 64/0 | 8/7 |
| whitespace-interleaved-16384.json | 2127.87 | 15.40 | 98.5 | 0.00 | 1 | 16384/0 | 63/62 |
| whitespace-interleaved-2048.json | 265.87 | 15.41 | 98.5 | 0.00 | 1 | 2048/0 | 63/62 |
| whitespace-interleaved-256.json | 33.12 | 15.52 | 98.5 | 0.00 | 1 | 256/0 | 63/62 |
| whitespace-leading-16384.json | 2080.00 | 15.75 | 98.5 | 0.00 | 1 | 16384/0 | 0/4096 |
| whitespace-leading-2048.json | 260.00 | 15.76 | 98.5 | 0.00 | 1 | 2048/0 | 0/4096 |
| whitespace-leading-256.json | 32.50 | 15.81 | 98.5 | 0.00 | 1 | 256/0 | 0/513 |
| whitespace-trailing-16384.json | 2080.00 | 15.75 | 98.5 | 0.00 | 1 | 16384/0 | 4096/0 |
| whitespace-trailing-2048.json | 260.00 | 15.76 | 98.5 | 0.00 | 1 | 2048/0 | 4096/0 |
| whitespace-trailing-256.json | 32.50 | 15.81 | 98.5 | 0.00 | 1 | 256/0 | 513/0 |
| wide-numbers-262144.json | 1737.75 | 301.71 | 0.0 | 0.00 | 1 | 262144/0 | 1511/1171 |
| wide-numbers-32768.json | 202.58 | 323.52 | 0.0 | 0.00 | 1 | 32768/0 | 1511/1261 |
| wide-numbers-4096.json | 23.46 | 349.31 | 0.0 | 0.00 | 1 | 4096/0 | 1511/1365 |
| wide-objects-16384.json | 2234.30 | 249.32 | 0.0 | 0.03 | 3 | 16384/7 | 1041/988 |
| wide-objects-2048.json | 275.83 | 252.45 | 0.0 | 0.03 | 3 | 2048/7 | 1041/998 |
| wide-objects-256.json | 34.04 | 255.79 | 0.0 | 0.03 | 3 | 256/7 | 1041/1014 |

## Captured whole-parse peak

MiB, lower is better. Retained DOM bytes/categories are identical for all strategies on every input.

| Input | byte-quarter | geometric | adaptive4 | adaptive64 |
| --- | ---: | ---: | ---: | ---: |
| alternating-262144.json | 44.533 | 45.793 | 38.680 | 41.197 |
| alternating-32768.json | 5.567 | 6.014 | 7.813 | 5.150 |
| alternating-4096.json | 0.696 | 0.790 | 0.852 | 0.977 |
| deep-128.json | 0.025 | 0.025 | 0.030 | 0.017 |
| deep-64.json | 0.013 | 0.013 | 0.015 | 0.009 |
| deep-8.json | 0.001 | 0.001 | 0.002 | 0.001 |
| dense-first-262144.json | 44.500 | 45.792 | 62.500 | 62.500 |
| dense-first-32768.json | 5.563 | 6.014 | 15.313 | 16.813 |
| dense-first-4096.json | 0.696 | 0.790 | 0.977 | 0.977 |
| dense-zeros-262144.json | 51.125 | 43.792 | 36.500 | 34.500 |
| dense-zeros-32768.json | 6.391 | 5.764 | 7.563 | 4.313 |
| dense-zeros-4096.json | 0.799 | 0.759 | 0.571 | 0.539 |
| escaped-4096.json | 57.500 | 8.826 | 8.750 | 8.752 |
| escaped-512.json | 7.188 | 1.106 | 1.117 | 1.094 |
| escaped-64.json | 0.899 | 0.143 | 0.143 | 0.143 |
| flat-object-16384.json | 5.006 | 5.961 | 7.760 | 5.116 |
| flat-object-2048.json | 0.903 | 0.780 | 0.637 | 0.712 |
| flat-object-256.json | 0.100 | 0.102 | 0.120 | 0.082 |
| large.json | 21.963 | 29.945 | 31.084 | 21.915 |
| long-ascii-4096.json | 56.473 | 8.689 | 8.614 | 8.615 |
| long-ascii-512.json | 7.059 | 1.089 | 1.100 | 1.077 |
| long-ascii-64.json | 0.882 | 0.141 | 0.141 | 0.141 |
| medium.json | 2.715 | 3.934 | 2.709 | 2.709 |
| nested.json | 0.038 | 0.038 | 0.038 | 0.038 |
| small.json | 0.255 | 0.231 | 0.170 | 0.172 |
| sparse-first-262144.json | 44.500 | 45.792 | 45.792 | 45.792 |
| sparse-first-32768.json | 5.563 | 6.014 | 6.014 | 6.014 |
| sparse-first-4096.json | 0.696 | 0.790 | 0.790 | 0.790 |
| tiny.json | 0.003 | 0.003 | 0.003 | 0.003 |
| unicode-4096.json | 56.777 | 8.712 | 8.637 | 8.638 |
| unicode-512.json | 7.097 | 1.092 | 1.103 | 1.080 |
| unicode-64.json | 0.887 | 0.141 | 0.141 | 0.141 |
| whitespace-interleaved-16384.json | 27.639 | 4.612 | 4.328 | 4.328 |
| whitespace-interleaved-2048.json | 3.454 | 0.593 | 0.729 | 0.528 |
| whitespace-interleaved-256.json | 0.430 | 0.076 | 0.068 | 0.068 |
| whitespace-leading-16384.json | 27.031 | 4.565 | 4.565 | 4.565 |
| whitespace-leading-2048.json | 3.379 | 0.588 | 0.588 | 0.588 |
| whitespace-leading-256.json | 0.423 | 0.076 | 0.076 | 0.076 |
| whitespace-trailing-16384.json | 27.031 | 4.565 | 5.781 | 50.781 |
| whitespace-trailing-2048.json | 3.379 | 0.588 | 1.192 | 1.082 |
| whitespace-trailing-256.json | 0.423 | 0.076 | 0.090 | 0.794 |
| wide-numbers-262144.json | 52.608 | 44.989 | 38.471 | 39.161 |
| wide-numbers-32768.json | 6.133 | 5.899 | 7.698 | 4.650 |
| wide-numbers-4096.json | 0.710 | 0.774 | 0.595 | 0.615 |
| wide-objects-16384.json | 44.018 | 45.474 | 43.507 | 43.923 |
| wide-objects-2048.json | 5.460 | 5.970 | 7.769 | 5.449 |
| wide-objects-256.json | 0.677 | 0.784 | 0.680 | 0.692 |

## Parse time

adaptive64 is milliseconds per operation (median of round medians). Other strategy columns are time/adaptive64 ratios:
below 1 means less time. The production/current ratio is an unchanged-policy control, not an optimization gain.
Raw CSV retains each round and its sample min/median/max; timer resolution, CPU frequency and background work limit small comparisons.

| Input | adaptive64 ms | byte-quarter ratio | geometric ratio | adaptive4 ratio | current control ratio |
| --- | ---: | ---: | ---: | ---: | ---: |
| alternating-262144.json | 22.8516 | 0.947 | 1.485 | 1.134 | 1.001 |
| alternating-32768.json | 2.3659 | 0.886 | 1.634 | 1.392 | 1.071 |
| alternating-4096.json | 0.2448 | 0.686 | 2.302 | 1.838 | 1.006 |
| deep-128.json | 0.0112 | 0.982 | 1.433 | 1.574 | 0.990 |
| deep-64.json | 0.0060 | 1.018 | 1.013 | 1.015 | 0.980 |
| deep-8.json | 0.0007 | 1.357 | 1.371 | 1.186 | 1.000 |
| dense-first-262144.json | 22.0838 | 0.954 | 1.515 | 1.183 | 1.001 |
| dense-first-32768.json | 2.7005 | 0.833 | 1.521 | 1.234 | 1.028 |
| dense-first-4096.json | 0.3534 | 0.943 | 1.463 | 1.177 | 0.998 |
| dense-zeros-262144.json | 16.8970 | 1.662 | 1.684 | 1.226 | 1.013 |
| dense-zeros-32768.json | 1.8137 | 1.654 | 1.766 | 1.517 | 1.007 |
| dense-zeros-4096.json | 0.1338 | 3.628 | 2.887 | 2.369 | 0.952 |
| escaped-4096.json | 12.4051 | 0.995 | 0.953 | 1.015 | 0.999 |
| escaped-512.json | 1.5877 | 0.947 | 0.848 | 0.959 | 1.008 |
| escaped-64.json | 0.1736 | 0.949 | 0.910 | 0.935 | 0.977 |
| flat-object-16384.json | 5.4195 | 0.971 | 1.138 | 1.145 | 0.982 |
| flat-object-2048.json | 0.5168 | 1.191 | 0.911 | 1.057 | 1.020 |
| flat-object-256.json | 0.0496 | 1.135 | 1.016 | 1.039 | 1.006 |
| large.json | 17.4667 | 0.945 | 1.320 | 1.230 | 0.998 |
| long-ascii-4096.json | 10.3884 | 0.951 | 0.894 | 0.910 | 0.950 |
| long-ascii-512.json | 1.1245 | 0.931 | 0.936 | 0.938 | 0.972 |
| long-ascii-64.json | 0.1240 | 1.005 | 1.045 | 0.991 | 1.036 |
| medium.json | 1.9124 | 0.822 | 1.215 | 0.912 | 0.954 |
| nested.json | 0.0256 | 0.917 | 0.920 | 0.975 | 0.977 |
| small.json | 0.0999 | 1.013 | 1.017 | 0.955 | 1.052 |
| sparse-first-262144.json | 34.2790 | 0.616 | 0.970 | 0.994 | 1.007 |
| sparse-first-32768.json | 3.7091 | 0.566 | 0.914 | 0.962 | 1.014 |
| sparse-first-4096.json | 0.2839 | 0.694 | 0.705 | 0.751 | 0.897 |
| tiny.json | 0.0011 | 0.963 | 0.945 | 1.000 | 0.991 |
| unicode-4096.json | 10.6195 | 1.034 | 1.008 | 1.000 | 1.017 |
| unicode-512.json | 1.1986 | 1.028 | 1.004 | 0.999 | 1.017 |
| unicode-64.json | 0.1472 | 1.007 | 0.987 | 0.995 | 1.016 |
| whitespace-interleaved-16384.json | 3.0984 | 1.295 | 1.452 | 0.998 | 1.020 |
| whitespace-interleaved-2048.json | 0.2909 | 1.602 | 1.573 | 1.005 | 0.973 |
| whitespace-interleaved-256.json | 0.0352 | 1.544 | 1.511 | 0.997 | 1.001 |
| whitespace-leading-16384.json | 3.8370 | 0.869 | 1.023 | 1.033 | 1.004 |
| whitespace-leading-2048.json | 0.3611 | 1.058 | 0.963 | 0.997 | 1.026 |
| whitespace-leading-256.json | 0.0419 | 0.961 | 0.980 | 1.001 | 1.044 |
| whitespace-trailing-16384.json | 3.3991 | 0.972 | 1.060 | 1.012 | 1.081 |
| whitespace-trailing-2048.json | 0.3308 | 1.159 | 1.034 | 1.063 | 1.051 |
| whitespace-trailing-256.json | 0.0410 | 0.996 | 0.982 | 1.005 | 1.049 |
| wide-numbers-262144.json | 23.6152 | 1.182 | 1.478 | 1.165 | 0.998 |
| wide-numbers-32768.json | 2.7039 | 1.113 | 1.342 | 1.186 | 0.949 |
| wide-numbers-4096.json | 0.2269 | 1.012 | 1.054 | 0.994 | 0.994 |
| wide-objects-16384.json | 32.2712 | 0.950 | 1.322 | 1.083 | 0.955 |
| wide-objects-2048.json | 3.2666 | 0.933 | 1.321 | 1.262 | 1.055 |
| wide-objects-256.json | 0.3353 | 0.899 | 0.997 | 0.980 | 0.949 |

## Families

Equal-weight geometric means of per-input ratios, relative to adaptive64; these are not total workload throughput.
Each cell is parse-time ratio / peak-memory ratio. Families receive no assumed application frequency.

| Family | Inputs | byte-quarter | geometric | adaptive4 |
| --- | ---: | ---: | ---: | ---: |
| alternating | 3 | 0.832 / 0.941 | 1.774 / 1.016 | 1.426 / 1.075 |
| deep | 3 | 1.107 / 1.408 | 1.258 / 1.408 | 1.237 / 1.693 |
| dense-first | 3 | 0.908 / 0.552 | 1.499 / 0.596 | 1.198 / 0.969 |
| dense-zeros | 3 | 2.152 / 1.482 | 2.048 / 1.336 | 1.639 / 1.252 |
| escaped | 3 | 0.963 / 6.478 | 0.903 / 1.007 | 0.969 / 1.007 |
| flat-object | 3 | 1.095 / 1.147 | 1.018 / 1.168 | 1.079 / 1.260 |
| long-ascii | 3 | 0.962 / 6.462 | 0.956 / 1.007 | 0.946 / 1.007 |
| mixed-records | 4 | 0.933 / 1.104 | 1.114 / 1.277 | 1.017 / 1.088 |
| nested | 1 | 0.917 / 1.000 | 0.920 / 1.000 | 0.975 / 1.000 |
| sparse-first | 3 | 0.623 / 0.925 | 0.855 / 1.000 | 0.895 / 1.000 |
| unicode | 3 | 1.023 / 6.479 | 1.000 / 1.007 | 0.998 / 1.007 |
| whitespace-interleaved | 3 | 1.474 / 6.426 | 1.511 / 1.105 | 1.000 / 1.114 |
| whitespace-leading | 3 | 0.960 / 5.746 | 0.988 / 1.000 | 1.010 / 1.000 |
| whitespace-trailing | 3 | 1.039 / 0.960 | 1.025 / 0.167 | 1.027 / 0.243 |
| wide-numbers | 3 | 1.100 / 1.270 | 1.279 / 1.224 | 1.112 / 1.163 |
| wide-objects | 3 | 0.927 / 0.995 | 1.203 / 1.088 | 1.102 / 1.116 |

## Token relocation volume and expansions

Each cell is copied-token MiB / growth events, excluding the initial reserve. No DOM relocation is counted here.

| Input | byte-quarter | geometric | adaptive4 | adaptive64 |
| --- | ---: | ---: | ---: | ---: |
| alternating-262144.json | 0.000 / 0 | 51.927 / 19 | 15.996 / 6 | 0.762 / 2 |
| alternating-32768.json | 0.000 / 0 | 6.818 / 14 | 3.996 / 5 | 0.762 / 2 |
| alternating-4096.json | 0.000 / 0 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| deep-128.json | 0.024 / 4 | 0.024 / 4 | 0.015 / 2 | 0.003 / 1 |
| deep-64.json | 0.012 / 4 | 0.012 / 4 | 0.007 / 2 | 0.001 / 1 |
| deep-8.json | 0.001 / 4 | 0.001 / 4 | 0.001 / 2 | 0.000 / 1 |
| dense-first-262144.json | 0.000 / 0 | 51.927 / 19 | 15.996 / 6 | 0.762 / 2 |
| dense-first-32768.json | 0.000 / 0 | 6.818 / 14 | 3.996 / 5 | 0.762 / 2 |
| dense-first-4096.json | 0.000 / 0 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| dense-zeros-262144.json | 48.750 / 4 | 51.927 / 19 | 15.996 / 6 | 0.762 / 2 |
| dense-zeros-32768.json | 6.094 / 4 | 6.818 / 14 | 3.996 / 5 | 0.762 / 2 |
| dense-zeros-4096.json | 0.762 / 4 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| escaped-4096.json | 0.000 / 0 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| escaped-512.json | 0.000 / 0 | 0.095 / 4 | 0.059 / 2 | 0.012 / 1 |
| escaped-64.json | 0.000 / 0 | 0.000 / 0 | 0.000 / 0 | 0.000 / 0 |
| flat-object-16384.json | 0.000 / 0 | 6.818 / 14 | 3.996 / 5 | 0.762 / 2 |
| flat-object-2048.json | 0.350 / 1 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| flat-object-256.json | 0.038 / 1 | 0.095 / 4 | 0.059 / 2 | 0.012 / 1 |
| large.json | 0.000 / 0 | 34.610 / 18 | 15.996 / 6 | 0.762 / 2 |
| long-ascii-4096.json | 0.000 / 0 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| long-ascii-512.json | 0.000 / 0 | 0.095 / 4 | 0.059 / 2 | 0.012 / 1 |
| long-ascii-64.json | 0.000 / 0 | 0.000 / 0 | 0.000 / 0 | 0.000 / 0 |
| medium.json | 0.000 / 0 | 4.538 / 13 | 0.996 / 4 | 0.762 / 2 |
| nested.json | 0.024 / 2 | 0.024 / 2 | 0.024 / 2 | 0.024 / 2 |
| small.json | 0.099 / 1 | 0.244 / 6 | 0.059 / 2 | 0.012 / 1 |
| sparse-first-262144.json | 0.000 / 0 | 51.927 / 19 | 51.927 / 19 | 51.927 / 19 |
| sparse-first-32768.json | 0.000 / 0 | 6.818 / 14 | 6.818 / 14 | 6.818 / 14 |
| sparse-first-4096.json | 0.000 / 0 | 0.878 / 9 | 0.878 / 9 | 0.878 / 9 |
| tiny.json | 0.002 / 2 | 0.002 / 2 | 0.002 / 2 | 0.002 / 2 |
| unicode-4096.json | 0.000 / 0 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| unicode-512.json | 0.000 / 0 | 0.095 / 4 | 0.059 / 2 | 0.012 / 1 |
| unicode-64.json | 0.000 / 0 | 0.000 / 0 | 0.000 / 0 | 0.000 / 0 |
| whitespace-interleaved-16384.json | 0.000 / 0 | 3.017 / 12 | 0.996 / 4 | 0.762 / 2 |
| whitespace-interleaved-2048.json | 0.000 / 0 | 0.377 / 7 | 0.246 / 3 | 0.012 / 1 |
| whitespace-interleaved-256.json | 0.000 / 0 | 0.029 / 2 | 0.012 / 1 | 0.012 / 1 |
| whitespace-leading-16384.json | 0.000 / 0 | 3.017 / 12 | 3.017 / 12 | 3.017 / 12 |
| whitespace-leading-2048.json | 0.000 / 0 | 0.377 / 7 | 0.377 / 7 | 0.377 / 7 |
| whitespace-leading-256.json | 0.000 / 0 | 0.029 / 2 | 0.029 / 2 | 0.029 / 2 |
| whitespace-trailing-16384.json | 0.000 / 0 | 3.017 / 12 | 0.996 / 4 | 0.762 / 2 |
| whitespace-trailing-2048.json | 0.000 / 0 | 0.377 / 7 | 0.246 / 3 | 0.012 / 1 |
| whitespace-trailing-256.json | 0.000 / 0 | 0.029 / 2 | 0.012 / 1 | 0.012 / 1 |
| wide-numbers-262144.json | 20.364 / 1 | 51.927 / 19 | 15.996 / 6 | 0.762 / 2 |
| wide-numbers-32768.json | 2.374 / 1 | 6.818 / 14 | 3.996 / 5 | 0.762 / 2 |
| wide-numbers-4096.json | 0.275 / 1 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |
| wide-objects-16384.json | 0.000 / 0 | 51.927 / 19 | 15.996 / 6 | 0.762 / 2 |
| wide-objects-2048.json | 0.000 / 0 | 6.818 / 14 | 3.996 / 5 | 0.762 / 2 |
| wide-objects-256.json | 0.000 / 0 | 0.878 / 9 | 0.246 / 3 | 0.012 / 1 |

## Interpretation and preset design

No preset is selected merely by a single fastest median. The tables expose a Pareto tradeoff between time,
peak memory and relocation volume; repeated rounds and the same-policy control are needed to judge margins.
A small initial reserve alone does not guarantee low peak memory: geometric expansion can keep old/new buffers alive together.
An eager byte-based reserve avoids some copies but can waste large token buffers on whitespace and long strings.
Early density is not a reliable document classification: dense-first/sparse-first/alternating and leading/trailing
whitespace deliberately present misleading prefixes. The full-file shape inventory is an offline descriptor,
not a free signal available before parsing. Auto-selection needs online estimates and bounded fallback.

Keep the current default until user workloads and additional toolchains validate a replacement. Future Balanced,
LowMemory and Throughput names should express budgets/priorities rather than promise universally best performance.
See [the preset design note](../../docs/parser-presets.md) for proposed options and optional document reports.

Concrete examples from this run:

- long-ascii-4096.json — byte-quarter: 56.47 MiB; geometric: 8.69 MiB; adaptive4: 8.61 MiB; adaptive64: 8.61 MiB.
- whitespace-trailing-16384.json — byte-quarter: 27.03 MiB; geometric: 4.57 MiB; adaptive4: 5.78 MiB; adaptive64: 50.78 MiB.
- dense-first-262144.json — byte-quarter: 44.50 MiB; geometric: 45.79 MiB; adaptive4: 62.50 MiB; adaptive64: 62.50 MiB.
- sparse-first-262144.json — byte-quarter: 44.50 MiB; geometric: 45.79 MiB; adaptive4: 45.79 MiB; adaptive64: 45.79 MiB.

Trailing whitespace makes a dense prefix overpredict the remaining token count; aggressive adaptive reserve
can then use more peak memory than even the old byte-quarter policy. Dense-first and sparse-first have the same
counts and string payload lengths but trigger different allocation histories. These counterexamples are why
an Auto design needs a speculative-reserve budget and a fallback, rather than a permanent prefix-derived label.

## Verification and reproduction

All strategies match the production token-field fingerprint, canonical DOM fingerprint, and every shape descriptor on all 47 inputs.
Fingerprint comparisons are consistency checks, not a collision-free mathematical proof. Invalid-input acceptance/diagnostic
fingerprints also match for 11 cases, through both lexer and parser, including errors after buffer growth.
All 564 memory windows satisfy peak/retained/category invariants and return tracked bytes AND blocks to zero.
Whole-parse and isolated-lexer relocation counters match. CI runs the same cross-strategy smoke on smaller fixtures on Windows/Linux
without speed thresholds; normal production tests, sanitizer and fuzz jobs remain separate.

Raw files: [reservation-matrix-msvc1951](reservation-matrix-msvc1951/). Metadata records source/binary/input SHA-256,
compiler/flags, affinity and per-round order. Generated inputs and transient logs stay outside Git.
Reproduce using [the reservation matrix instructions](../README.md#reservation-strategy-matrix), then render:
`python benchmarks/reservation/summarize_matrix.py RESULTS REPORT.md`.
