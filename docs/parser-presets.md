# Parser presets and optional parse reports (design proposal)

This is a design note supported by the [reservation matrix](../benchmarks/results/reservation-matrix.md).
`ParseOptions`, `ParsePreset::Balanced` and `ParsePreset::LowMemory` are now
implemented; see the [LowMemory experiment and evaluation](../benchmarks/results/low-memory.md).
Balanced remains the default. Json layout is unchanged, and experimental allocation
hooks remain isolated from production. Throughput, Auto and root-level ParseStats
are future proposals. Private Auto controllers and decision journals are now
available in the optional [Auto experiment](../benchmarks/results/auto-reservation.md).
Its speed goal passes the stated time targets locally, but it can use much more
memory; the memory goal fails a development-set peak target. Neither is a public preset.

## Evidence to collect

Compare time, simultaneous captured C++ heap, token-buffer growth events and
logical relocated token bytes. Record raw timings, spread, compiler/STL, source,
binary/input hashes and execution order. Include multiple scales and deliberately
misleading prefixes, rather than only homogeneous documents. The current matrix
has 47 deterministic fixtures and four policies; further decisions also need real
application inputs and additional toolchains. Windows/Linux smoke checks establish
functional consistency, not cross-platform performance.

Input descriptors include token density, whitespace outside strings, raw string
lengths/escapes, numeric token count, depth, maximum container widths and first/last
4 KiB token counts. These are offline full-document observations. They must not be
treated as available before parsing, or as free features for an automatic selector.

## Preset meanings

| Preset / status | Objective | Mechanism |
| --- | --- | --- |
| Balanced — implemented | Existing behavior | Observed density, 1.5×…64× growth |
| LowMemory — implemented | Limit speculative capacity | At growth request at most live token count + max(live token count, 256) slots; accept more copying |
| Throughput — proposed | Reduce reallocations on known dense inputs | More eager reservation, possibly with a caller-supplied token-count hint |
| Auto — proposed | Select/revise a strategy within explicit bounds | Online estimates with a conservative fallback when density changes |

These are objectives, not names for the fastest/lowest-memory row on one fixture.
The matrix already shows that small initial reserve alone does not ensure a low
peak, and aggressive density extrapolation can waste memory on dense prefixes
followed by long strings or trailing whitespace. A sparse prefix followed by dense
values can instead cause repeated copying. Finite prefix sampling cannot reliably
classify an arbitrary remaining suffix.

An initial Auto policy should use observations already available during lexing,
such as consumed bytes and emitted tokens, and bound speculative growth. It should
reconsider its estimate as data arrives. Evaluating a separate pre-scan is valid
only if its time and memory are included in the end-to-end benchmark. A static
shape decision learned from these synthetic fixtures needs validation on held-out
inputs; the same fixtures cannot both tune and prove a classifier's generality.

An unused-capacity budget is not a total parser memory limit. Source copies,
required live tokens, DOM nodes and old/new-buffer overlap still consume memory.
A hard overall limit would need defined allocation-failure semantics or another
parser architecture; do not promise that through a reservation preset alone.

## Options API and possible report API

Options are separate from Json values. The existing parse(text) entry point and
default are preserved. Both parser and lexer accept ParseOptions:

```cpp
ParseOptions options;
options.preset = ParsePreset::LowMemory;
auto value = JsonParser::parse(text, options);
auto tokens = JsonLexer(text).tokenize(options);
```

The following report API is illustrative and not implemented:

```cpp
auto result = JsonParser::parseWithStats(text, options);
use(result.value);
inspect(result.stats);
```

Do not store a preset/statistics member in every Json node. A reservation strategy
is relevant before/during construction, while a Json value may also be constructed
manually. An optional root-level report avoids increasing the value layout or
changing serialized JSON, equality, copying and the binary interface.

The report could contain selected/effective strategy, input bytes, token count,
initial/final token capacity, expansions and logical relocated token bytes. These
are parse observations, not permanent attributes of the resulting value. Tree
mutation does not update them, and they must be labeled as statistics of the
original parse. Retained capacity belongs to the temporary lexer buffer, which
has already been freed when parsing returns.

Do not advertise the profiler's captured whole-parse peak as a cheap production
ParseStats field. It currently requires separate allocation interception and
excludes several allocation classes. Cheap logical counters, offline DOM walks and
instrumented heap measurements have different costs and meanings.

## Further validation and future modes

The [Auto v2 experiment](../benchmarks/results/auto-v2-reservation.md) adds byte
ceilings for speculative token slots and a remaining-byte bound near EOF. Minimum
geometric growth can exceed the ceiling; each exception is logged. This is not a
total-memory limit. The selected private budgets are 4 MiB for speed and 1 MiB for
memory, calibrated on 75 known documents before freezing a new 16-document split.

Both goals pass the joint targets on the new split, but fail the known-set CPU
targets: speed reaches 1.723 times the better fixed mode (limit 1.5), and memory
reaches 2.421 times Balanced (limit 2). Original heap failures improve from 70.494
to 15.132 MiB and from 6.812 to 5.557 MiB. These mixed results do not qualify Auto
for public promotion; retain the explicit Balanced/LowMemory choice while
collecting representative user workloads. No thresholds or budgets were retuned
using the new split. Raw measurements and decision journals remain in the report.

1. Collect real user inputs and repeat the matrix on another toolchain; keep raw
   input private when appropriate and publish reproducible shape-equivalent cases.
2. Revisit the spare-capacity/time tradeoff for actual user workloads. LowMemory
   currently caps each request as described above; this is an advisory reserve
   bound, not a guarantee on STL allocator behavior or total live parser memory.
3. Measure candidate LowMemory/Throughput/Auto policies end to end, with held-out
   transition cases and the current implementation as a control.
4. Add an opt-in root report or further options only after semantics and defaults are
   clear; test result/diagnostic equivalence and accounting without speed thresholds.

LowMemory was selected on the 47-input training set before the 12-input holdout
was generated. The final public API is also checked against that frozen prototype.
The parameters were not changed in response to held-out results.
