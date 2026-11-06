# Opt-in LowMemory parsing

## Policy and API

ParseOptions adds Balanced (the existing default) and LowMemory. Both parser and lexer have new options overloads.
Json layout, values, numeric storage, token fields, diagnostics and the 128-level nesting limit are unchanged.

```cpp
const ParseOptions options{ParsePreset::LowMemory};
auto value = JsonParser::parse(text, options);
auto tokens = JsonLexer(text).tokenize(options);
```

Both modes initially request at most 256 Token slots. On a full buffer, LowMemory clamps the density prediction
between capacity + max(capacity/8, 1) and capacity + max(capacity, 256), with overflow-safe max_size bounds.
The strategy is selected once; there is no runtime preset branch per token. No pre-scan or production heap hooks are added.
This bounds speculative reserve requests, not total heap, allocator rounding, STL capacity or DOM memory.
Old/new token buffers can overlap during growth. LowMemory can be slower and can have a higher peak on some inputs.
Unknown preset values throw std::invalid_argument; JsonLexer rejects them before consuming input.
ParseStats, Throughput and Auto remain future proposals.

## Selection protocol

Prototype sources and original observations are committed as `9d8f6ad18c33c0a683e5eb9c2c1c91b8e6d048e9`, based on `74b6294`.
The [pre-recorded plan](../reservation/lowmemory-plan.json) defines three spare-capacity candidates: 25%, 50%, 100%.
The initial/small-buffer allowance is the same; all clamp the existing density estimate, with a 12.5% minimum growth step.
47 existing inputs form the training split. Require at least 75% tail-whitespace peak reduction and no more than
20% long-ASCII peak increase. Compare equal-weight geometric mean parse-time ratios; candidates within 5% of the best
are tied and the lower total token-relocation volume wins. These are workload-selection criteria, not a universal optimum.

| Candidate | Eligible | Geomean parse time/current | Total relocated token MiB | Tail peak MiB |
| --- | --- | ---: | ---: | ---: |
| low25 | True | 1.354 | 889.37 | 5.028 |
| low50 | True | 1.253 | 642.19 | 5.410 |
| low100 | True | 1.255 | 522.18 | 6.531 |

Selection was frozen at `2026-11-06T16:31:35.185315+00:00` in [selection.json](low-memory-msvc1951/selection.json).
low100 won the relocation tie-break against low50. Only then were 12 fixed held-out fixtures generated;
their results did not change the parameters. They cover long keys, nested records, empty-container mixtures,
sawtooth string lengths and escaped-string transitions, at counts 257/4099 instead of the training scales.

## Measurement

- training: 47 inputs; 4 rotating rounds × 5 samples × 3 operations; five warm-ups per operation.
- holdout: 12 inputs; 2 rotating rounds × 7 samples × 5 operations; five warm-ups per operation.
- public: 59 inputs; 3 rotating rounds × 7 samples × 5 operations; five warm-ups per operation.

12th Gen Intel(R) Core(TM) i5-12600KF; Windows-10-10.0.19045-SP0; logical CPU 0; static Release.

```text
compiler=MSVC
compiler_version=19.51.36257.0
configuration=Release
cxx_standard=17
flags=/DWIN32 /D_WINDOWS /EHsc
config_flags=/O2 /Ob2 /DNDEBUG
lto=
```

Timings come from uninstrumented executables; parse/lexer include construction and destruction of their results.
File reading, validation, shape inventory and profiling are outside timing. Tables use median of round medians.
The public run rotates the frozen pre-API control, public Balanced, and public LowMemory. Control variation
and sample min/max in raw CSV limit small comparisons; CPU affinity does not control frequency/background work.
Captured heap counts simultaneous requested C++ new bytes, including overlapping buffers. Caller input, stack/root Json,
allocator rounding/metadata, direct malloc/CRT internals and profiler bookkeeping are excluded. This is not RSS.
Relocated bytes mean sum(old live token count × sizeof(Token)) at expansions, not measured hardware traffic.
Memory is measured separately. Production LowMemory timing uses the public API; profiles instrument a generated
copy with the same allocation policy. Every memory/copy observation must equal the frozen low100 prototype.

## Public API results — all inputs

Δ is LowMemory/Balanced − 1; negative means less memory/time. Control ratio is public Balanced/pre-API time.

| Split / input | Balanced peak MiB | LowMemory peak MiB | Δ peak | Balanced ms | LowMemory ms | Δ time | Control ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| training / alternating-262144.json | 41.197 | 53.501 | +29.9% | 22.6543 | 35.6401 | +57.3% | 0.993 |
| training / alternating-32768.json | 5.150 | 6.688 | +29.9% | 2.7971 | 3.7825 | +35.2% | 1.164 |
| training / alternating-4096.json | 0.977 | 0.883 | -9.6% | 0.3119 | 0.5882 | +88.6% | 1.328 |
| training / deep-128.json | 0.017 | 0.017 | +0.0% | 0.0109 | 0.0108 | -0.6% | 0.971 |
| training / deep-64.json | 0.009 | 0.009 | +0.0% | 0.0058 | 0.0058 | -0.3% | 0.980 |
| training / deep-8.json | 0.001 | 0.001 | +0.0% | 0.0007 | 0.0008 | +5.4% | 0.949 |
| training / dense-first-262144.json | 62.500 | 74.500 | +19.2% | 21.7795 | 33.3577 | +53.2% | 0.986 |
| training / dense-first-32768.json | 16.813 | 9.313 | -44.6% | 2.5956 | 3.7430 | +44.2% | 0.987 |
| training / dense-first-4096.json | 0.977 | 1.164 | +19.2% | 0.3620 | 0.6387 | +76.5% | 1.028 |
| training / dense-zeros-262144.json | 34.500 | 51.500 | +49.3% | 17.0306 | 28.4854 | +67.3% | 1.017 |
| training / dense-zeros-32768.json | 4.313 | 6.438 | +49.3% | 1.9255 | 3.1970 | +66.0% | 0.922 |
| training / dense-zeros-4096.json | 0.539 | 0.805 | +49.2% | 0.1272 | 0.4138 | +225.3% | 0.997 |
| training / escaped-4096.json | 8.752 | 8.797 | +0.5% | 12.0849 | 12.2460 | +1.3% | 0.951 |
| training / escaped-512.json | 1.094 | 1.100 | +0.5% | 1.4631 | 1.3830 | -5.5% | 0.974 |
| training / escaped-64.json | 0.143 | 0.143 | +0.0% | 0.1550 | 0.1537 | -0.8% | 0.953 |
| training / flat-object-16384.json | 5.116 | 6.635 | +29.7% | 5.6568 | 6.0474 | +6.9% | 0.993 |
| training / flat-object-2048.json | 0.712 | 0.826 | +16.0% | 0.5467 | 0.5501 | +0.6% | 1.047 |
| training / flat-object-256.json | 0.082 | 0.103 | +25.9% | 0.0497 | 0.0497 | -0.1% | 0.974 |
| training / large.json | 21.915 | 26.584 | +21.3% | 16.9407 | 21.0857 | +24.5% | 0.963 |
| training / long-ascii-4096.json | 8.615 | 8.660 | +0.5% | 9.2375 | 8.7342 | -5.4% | 0.923 |
| training / long-ascii-512.json | 1.077 | 1.083 | +0.5% | 1.0986 | 1.0067 | -8.4% | 1.049 |
| training / long-ascii-64.json | 0.141 | 0.141 | +0.0% | 0.1279 | 0.1229 | -3.9% | 1.034 |
| training / medium.json | 2.709 | 3.321 | +22.6% | 1.8155 | 2.1248 | +17.0% | 1.018 |
| training / nested.json | 0.038 | 0.038 | +2.0% | 0.0267 | 0.0250 | -6.3% | 1.067 |
| training / small.json | 0.172 | 0.208 | +20.6% | 0.0965 | 0.0868 | -10.1% | 1.051 |
| training / sparse-first-262144.json | 45.792 | 48.224 | +5.3% | 34.1784 | 60.4949 | +77.0% | 1.002 |
| training / sparse-first-32768.json | 6.014 | 6.270 | +4.3% | 3.6036 | 6.7723 | +87.9% | 0.979 |
| training / sparse-first-4096.json | 0.790 | 0.825 | +4.5% | 0.2861 | 0.4102 | +43.4% | 0.957 |
| training / tiny.json | 0.003 | 0.002 | -16.0% | 0.0011 | 0.0012 | +5.5% | 0.965 |
| training / unicode-4096.json | 8.638 | 8.684 | +0.5% | 10.7009 | 11.4580 | +7.1% | 1.032 |
| training / unicode-512.json | 1.080 | 1.086 | +0.5% | 1.1967 | 1.1939 | -0.2% | 0.994 |
| training / unicode-64.json | 0.141 | 0.141 | +0.0% | 0.1486 | 0.1471 | -1.1% | 1.016 |
| training / whitespace-interleaved-16384.json | 4.328 | 5.266 | +21.7% | 2.9771 | 3.4247 | +15.0% | 0.975 |
| training / whitespace-interleaved-2048.json | 0.528 | 0.658 | +24.8% | 0.2870 | 0.3017 | +5.1% | 1.035 |
| training / whitespace-interleaved-256.json | 0.068 | 0.082 | +21.3% | 0.0355 | 0.0363 | +2.1% | 1.017 |
| training / whitespace-leading-16384.json | 4.565 | 5.111 | +12.0% | 3.7739 | 5.1264 | +35.8% | 1.032 |
| training / whitespace-leading-2048.json | 0.588 | 0.624 | +6.2% | 0.3493 | 0.4590 | +31.4% | 0.939 |
| training / whitespace-leading-256.json | 0.076 | 0.077 | +1.0% | 0.0416 | 0.0429 | +3.0% | 0.995 |
| training / whitespace-trailing-16384.json | 50.781 | 6.531 | -87.1% | 3.3216 | 3.6684 | +10.4% | 0.971 |
| training / whitespace-trailing-2048.json | 1.082 | 0.817 | -24.5% | 0.3339 | 0.3573 | +7.0% | 0.974 |
| training / whitespace-trailing-256.json | 0.794 | 0.102 | -87.1% | 0.0410 | 0.0429 | +4.6% | 0.957 |
| training / wide-numbers-262144.json | 39.161 | 52.697 | +34.6% | 23.6300 | 33.7242 | +42.7% | 1.014 |
| training / wide-numbers-32768.json | 4.650 | 6.573 | +41.4% | 2.7137 | 3.5307 | +30.1% | 1.065 |
| training / wide-numbers-4096.json | 0.615 | 0.820 | +33.4% | 0.2292 | 0.2409 | +5.1% | 1.044 |
| training / wide-objects-16384.json | 43.923 | 53.182 | +21.1% | 31.3832 | 42.6181 | +35.8% | 1.016 |
| training / wide-objects-2048.json | 5.449 | 6.644 | +21.9% | 3.2250 | 4.3237 | +34.1% | 0.993 |
| training / wide-objects-256.json | 0.692 | 0.830 | +20.1% | 0.3052 | 0.3269 | +7.1% | 0.976 |
| holdout / held-empty-mix-257.json | 0.227 | 0.241 | +5.8% | 0.0740 | 0.0780 | +5.4% | 0.996 |
| holdout / held-empty-mix-4099.json | 3.609 | 3.838 | +6.3% | 1.4498 | 1.9985 | +37.8% | 0.998 |
| holdout / held-escape-transition-257.json | 0.112 | 0.112 | +0.2% | 0.0912 | 0.0919 | +0.8% | 1.002 |
| holdout / held-escape-transition-4099.json | 0.807 | 0.841 | +4.2% | 0.2541 | 0.3027 | +19.1% | 0.969 |
| holdout / held-front-nested-257.json | 0.702 | 0.728 | +3.7% | 0.4301 | 0.4758 | +10.6% | 0.905 |
| holdout / held-front-nested-4099.json | 11.771 | 11.121 | -5.5% | 10.3720 | 14.5207 | +40.0% | 0.981 |
| holdout / held-long-keys-257.json | 0.043 | 0.043 | +0.0% | 0.0295 | 0.0300 | +2.0% | 0.981 |
| holdout / held-long-keys-4099.json | 0.548 | 0.547 | -0.0% | 0.4824 | 0.4977 | +3.2% | 0.960 |
| holdout / held-sawtooth-257.json | 0.277 | 0.268 | -3.2% | 0.2345 | 0.2340 | -0.2% | 1.002 |
| holdout / held-sawtooth-4099.json | 4.281 | 4.159 | -2.8% | 4.0366 | 3.9955 | -1.0% | 1.009 |
| holdout / held-tail-records-257.json | 1.133 | 0.764 | -32.6% | 0.4203 | 0.4165 | -0.9% | 1.026 |
| holdout / held-tail-records-4099.json | 54.116 | 12.219 | -77.4% | 8.4511 | 9.2036 | +8.9% | 1.007 |

## Aggregate tradeoffs

Equal-weight geometric mean time ratios are not total application throughput.

| Split | LowMemory/Balanced parse time | Largest peak ratio LowMemory/Balanced |
| --- | ---: | ---: |
| training | 1.213 | 1.493 |
| holdout | 1.097 | 1.063 |

Examples of the intended benefit:

- whitespace-trailing-16384.json: 50.78 → 6.53 MiB captured peak.
- held-tail-records-4099.json: 54.12 → 12.22 MiB captured peak.

Largest relative peak regressions:

- dense-zeros-262144.json: 34.500 → 51.500 MiB.
- dense-zeros-32768.json: 4.313 → 6.438 MiB.
- dense-zeros-4096.json: 0.539 → 0.805 MiB.

Choose LowMemory for a useful reserve bound and measured workload benefit; the name does not guarantee a lower total peak on every input.
The full tables and raw stage/copy counters retain unfavorable outcomes as well as improvements.

## Verification and reproduction

Both public presets match the frozen pre-API token-field/DOM fingerprints, shape descriptors and invalid-input diagnostics on all 59 inputs.
Fingerprints are consistency checks, not collision-free proofs. Default allocation observations are unchanged;
public LowMemory observations equal the frozen candidate on every memory field and stage. All captured windows
return live bytes and blocks to zero after destruction, with retained categories partitioning totals.
Unit tests cover range/EOF/error preservation, exact numeric storage/negative zero, nesting, long/trailing-whitespace
regressions, invalid presets without lexer consumption, and arithmetic near size_t/max_size limits.
Installed consumers exercise both header styles and DLL/static overloads. Corpus/mutation/fuzzer checks compare
Balanced and LowMemory acceptance, diagnostics, values and numeric kinds; shared/static and sanitizer CI run separately.

Raw [training, holdout and public observations](low-memory-msvc1951/) include source/binary/input hashes, selection and execution order.
Training/holdout source manifests refer to prototype commit 9d8f6ad; the public manifest identifies the final implementation.
LF attributes preserve cross-file hash references on checkout. Inputs/logs/binaries stay in ignored build output.
See [reproduction instructions](../README.md#lowmemory-selection-and-public-api-evaluation). Render this report with:
`python benchmarks/reservation/summarize_lowmemory.py benchmarks/results/low-memory-msvc1951 REPORT.md`.
