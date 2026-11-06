# Parser presets and optional parse reports (design proposal)

This is a design note supported by the [reservation matrix](../benchmarks/results/reservation-matrix.md).
No public preset/report API is implemented in this step. The production lexer and
Json representation retain their current behavior. Experiments live in optional,
isolated benchmark executables and do not add production allocation hooks.

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

| Proposed preset | Objective | Candidate mechanism to investigate |
| --- | --- | --- |
| Balanced | Avoid extreme memory/copy costs across workloads | Current observed-density strategy, then validate tighter extrapolation bounds |
| LowMemory | Limit unused token capacity and overlap during growth | Bound spare capacity or speculative reserve bytes; accept more copying |
| Throughput | Reduce reallocations on known dense inputs | More eager reservation, possibly with a caller-supplied token-count hint |
| Auto | Select/revise a strategy within explicit bounds | Online estimates with a conservative fallback when density changes |

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

## Possible API shape

Keep options separate from Json values. Preserve the existing parse(text) entry
point and default. New overloads could be considered after policies are validated:

```cpp
// Illustrative only; names and fields are not committed API.
ParseOptions options;
options.preset = ParsePreset::LowMemory;
auto value = JsonParser::parse(text, options);

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

## Next implementation gate

1. Collect real user inputs and repeat the matrix on another toolchain; keep raw
   input private when appropriate and publish reproducible shape-equivalent cases.
2. Define a spare-capacity budget and a workload-specific acceptable time penalty.
3. Measure candidate LowMemory/Throughput/Auto policies end to end, with held-out
   transition cases and the current implementation as a control.
4. Add an opt-in root report and options only after semantics and defaults are
   clear; test result/diagnostic equivalence and accounting without speed thresholds.

This step supplies the evidence and repeatable experiment, rather than prematurely
freezing heuristic names or an automatic classifier into the public API.
