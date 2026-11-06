"""Validate saved matrix observations and render a reproducible Markdown report."""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path
from check_matrix import POLICIES, CATEGORIES, validate_memory

parser = argparse.ArgumentParser()
parser.add_argument("results", type=Path)
parser.add_argument("report", type=Path)
args = parser.parse_args()
meta = json.loads((args.results / "metadata.json").read_text(encoding="utf-8-sig"))
inputs = {p["name"]: p["bytes"] for p in meta["inputs"]}
assert len(inputs) == 47 and meta["policies"] == list(POLICIES)
assert meta["rounds"] % 4 == 0
for index, order in enumerate(meta["round_order"]):
    assert [p for p in order if p != "current"] == list(POLICIES[index % 4:] + POLICIES[:index % 4])
shapes = list(csv.DictReader((args.results / "shapes.csv").open(encoding="utf-8-sig", newline="")))
shape = {r["dataset"]: r for r in shapes}
assert len(shape) == len(shapes) == len(inputs) and set(shape) == set(inputs)
for name, row in shape.items():
    assert int(row["input_bytes"]) == inputs[name]
    assert 0 <= int(row["whitespace_bytes"]) <= inputs[name]
    assert int(row["max_depth"]) <= 128
    assert int(row["first_4k_tokens"]) < int(row["token_count"])
memory, times = {}, {}
for policy in POLICIES:
    rows = validate_memory((args.results / f"{policy}-memory.csv").read_text(encoding="utf-8-sig"), inputs)
    memory.update({(policy, *key): row for key, row in rows.items()})
    for name in inputs:
        assert rows[name, "lex_only"]["token_count"] == shape[name]["token_count"]
        current = rows[name, "whole_parse"]
        if policy != POLICIES[0]:
            baseline = memory[POLICIES[0], name, "whole_parse"]
            assert all(current[c] == baseline[c] for c in (*CATEGORIES, "retained_bytes", "retained_blocks", "dom_nodes", "object_members", "array_elements", "array_capacity"))
for policy in (*POLICIES, "current"):
    for number in range(1, meta["rounds"] + 1):
        rows = list(csv.DictReader((args.results / f"{policy}-{number}.csv").open(encoding="utf-8-sig", newline="")))
        seen = set()
        for row in rows:
            key = row["dataset"], row["operation"]
            assert key not in seen
            seen.add(key)
            assert row["configuration"] == "Release" and int(row["bytes"]) == inputs[key[0]]
            assert int(row["samples"]) == meta["samples"] and int(row["iterations"]) == meta["iterations"]
            assert 0 < float(row["min_us"]) <= float(row["median_us"]) <= float(row["max_us"])
            times.setdefault((policy, *key), []).append(float(row["median_us"]))
        assert seen == {(n, op) for n in inputs for op in ("parse", "lex_only")}


def time(policy, name, op="parse"):
    return statistics.median(times[policy, name, op])


def peak(policy, name):
    return int(memory[policy, name, "whole_parse"]["peak_live_bytes"])


def family(name):
    stem = Path(name).stem
    if stem in ("tiny", "small", "medium", "large"):
        return "mixed-records"
    return stem.rsplit("-", 1)[0] if stem.rsplit("-", 1)[-1].isdigit() else stem


def geomean(values):
    return math.exp(statistics.mean(math.log(v) for v in values))


groups = {}
for name in inputs:
    groups.setdefault(family(name), []).append(name)
lines = ["# Reservation strategy matrix", "", "## Experiment", "",
    f"Base `{meta['base_revision']}`; {meta['cpu']}; {meta['os']}; logical CPU {meta['logical_cpu']}.",
    "47 deterministic inputs: the previous 23 plus 24 fixtures for dense short values, flat objects,",
    "leading/trailing/interleaved whitespace, both directions of density change, and alternating dense/sparse blocks.",
    "Four isolated experimental builds change allocation policy only. The generator preserves the rest of",
    "the production lexer text verbatim; a private class name and parser object route each executable to its variant.",
    "The production lexer, public headers, default behavior and JSON representation are unchanged.", "",
    "| Strategy | Initial reserve | Subsequent growth |", "| --- | --- | --- |",
    "| byte-quarter | input bytes / 4 | ordinary STL vector growth (historical policy) |",
    "| geometric | min(input bytes / 4, 256) | ordinary STL vector growth |",
    "| adaptive4 | min(input bytes / 4, 256) | observed density, bounded 1.5×…4× |",
    "| adaptive64 | min(input bytes / 4, 256) | observed density, bounded 1.5×…64× (current policy) |", "",
    f"{meta['rounds']} rounds × {meta['samples']} samples × {meta['iterations']} operations, with five warm-ups per operation.",
    "A cyclic order puts each strategy in every position relative to the other experimental strategies once per four rounds.",
    "An unmodified production build (`current`) runs before/after alternating rounds as a same-algorithm control",
    "for adaptive64. All timings use the same small harness and build flags; parse includes DOM destruction.",
    "Lexer timing includes its source copy and result destruction. File loading, validation and profiling are outside timing.",
    "Counters exist only in separate instrumented executables; memory and timings are never taken from the same executable.",
    "No Boost/Qt timings are mixed into this experiment: it compares reservation policies within MasketJSON.", "",
    "Captured heap is simultaneous requested C++ new bytes; it excludes caller input, stack, root Json object,",
    "allocator rounding/metadata, direct malloc/CRT internals, and profiler bookkeeping. It is not RSS.",
    "Copied-token bytes mean sum(old live token count × sizeof(Token)) at successful buffer expansions;",
    "this is logical relocation volume, not measured hardware memory traffic. Peaks include overlapping buffers.",
    "STL growth and allocation sizes are toolchain-specific; results are local, not universal rankings.",
    "", "Build information:", "", "```text", meta["build_info"].strip(), "```", "",
    "## Shape inventory", "", "Density includes EOF; whitespace excludes spaces inside string literals. String lengths are raw JSON bytes.", "",
    "| Input | KiB | Tokens/KiB | Whitespace % | Max raw string KiB | Depth | Max array/object width | First/last 4 KiB tokens |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    s = shape[name]
    lines.append(f"| {name} | {inputs[name]/1024:.2f} | {int(s['token_count'])*1024/inputs[name]:.2f} | "
        f"{100*int(s['whitespace_bytes'])/inputs[name]:.1f} | {int(s['max_raw_string_bytes'])/1024:.2f} | {s['max_depth']} | "
        f"{s['max_array_width']}/{s['max_object_width']} | {s['first_4k_tokens']}/{s['last_4k_tokens']} |")
lines += ["", "## Captured whole-parse peak", "", "MiB, lower is better. Retained DOM bytes/categories are identical for all strategies on every input.", "",
    "| Input | byte-quarter | geometric | adaptive4 | adaptive64 |", "| --- | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    lines.append("| " + name + " | " + " | ".join(f"{peak(p,name)/1048576:.3f}" for p in POLICIES) + " |")
lines += ["", "## Parse time", "",
    "adaptive64 is milliseconds per operation (median of round medians). Other strategy columns are time/adaptive64 ratios:",
    "below 1 means less time. The production/current ratio is an unchanged-policy control, not an optimization gain.",
    "Raw CSV retains each round and its sample min/median/max; timer resolution, CPU frequency and background work limit small comparisons.", "",
    "| Input | adaptive64 ms | byte-quarter ratio | geometric ratio | adaptive4 ratio | current control ratio |",
    "| --- | ---: | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    base = time("adaptive64", name)
    lines.append(f"| {name} | {base/1000:.4f} | " + " | ".join(f"{time(p,name)/base:.3f}" for p in (*POLICIES[:3], "current")) + " |")
lines += ["", "## Families", "", "Equal-weight geometric means of per-input ratios, relative to adaptive64; these are not total workload throughput.",
    "Each cell is parse-time ratio / peak-memory ratio. Families receive no assumed application frequency.", "",
    "| Family | Inputs | byte-quarter | geometric | adaptive4 |", "| --- | ---: | ---: | ---: | ---: |"]
for group, names in sorted(groups.items()):
    cells = [f"{geomean([time(p,n)/time('adaptive64',n) for n in names]):.3f} / "
             f"{geomean([peak(p,n)/peak('adaptive64',n) for n in names]):.3f}" for p in POLICIES[:3]]
    lines.append(f"| {group} | {len(names)} | " + " | ".join(cells) + " |")
lines += ["", "## Token relocation volume and expansions", "", "Each cell is copied-token MiB / growth events, excluding the initial reserve. No DOM relocation is counted here.", "",
    "| Input | byte-quarter | geometric | adaptive4 | adaptive64 |", "| --- | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    cells = []
    for p in POLICIES:
        r = memory[p, name, "lex_only"]
        cells.append(f"{int(r['copied_token_bytes'])/1048576:.3f} / {r['token_growth_events']}")
    lines.append(f"| {name} | " + " | ".join(cells) + " |")
lines += ["", "## Interpretation and preset design", "",
    "No preset is selected merely by a single fastest median. The tables expose a Pareto tradeoff between time,",
    "peak memory and relocation volume; repeated rounds and the same-policy control are needed to judge margins.",
    "A small initial reserve alone does not guarantee low peak memory: geometric expansion can keep old/new buffers alive together.",
    "An eager byte-based reserve avoids some copies but can waste large token buffers on whitespace and long strings.",
    "Early density is not a reliable document classification: dense-first/sparse-first/alternating and leading/trailing",
    "whitespace deliberately present misleading prefixes. The full-file shape inventory is an offline descriptor,",
    "not a free signal available before parsing. Auto-selection needs online estimates and bounded fallback.", "",
    "Keep the current default until user workloads and additional toolchains validate a replacement. Future Balanced,",
    "LowMemory and Throughput names should express budgets/priorities rather than promise universally best performance.",
    "See [the preset design note](../../docs/parser-presets.md) for proposed options and optional document reports.",
    "", "Concrete examples from this run:", ""]
for name in ("long-ascii-4096.json", "whitespace-trailing-16384.json", "dense-first-262144.json", "sparse-first-262144.json"):
    values = "; ".join(f"{p}: {peak(p,name)/1048576:.2f} MiB" for p in POLICIES)
    lines.append(f"- {name} — {values}.")
lines += ["", "Trailing whitespace makes a dense prefix overpredict the remaining token count; aggressive adaptive reserve",
    "can then use more peak memory than even the old byte-quarter policy. Dense-first and sparse-first have the same",
    "counts and string payload lengths but trigger different allocation histories. These counterexamples are why",
    "an Auto design needs a speculative-reserve budget and a fallback, rather than a permanent prefix-derived label.",
    "", "## Verification and reproduction", "",
    "All strategies match the production token-field fingerprint, canonical DOM fingerprint, and every shape descriptor on all 47 inputs.",
    "Fingerprint comparisons are consistency checks, not a collision-free mathematical proof. Invalid-input acceptance/diagnostic",
    "fingerprints also match for 11 cases, through both lexer and parser, including errors after buffer growth.",
    f"All {len(memory)} memory windows satisfy peak/retained/category invariants and return tracked bytes AND blocks to zero.",
    "Whole-parse and isolated-lexer relocation counters match. CI runs the same cross-strategy smoke on smaller fixtures on Windows/Linux",
    "without speed thresholds; normal production tests, sanitizer and fuzz jobs remain separate.", "",
    "Raw files: [reservation-matrix-msvc1951](reservation-matrix-msvc1951/). Metadata records source/binary/input SHA-256,",
    "compiler/flags, affinity and per-round order. Generated inputs and transient logs stay outside Git.",
    "Reproduce using [the reservation matrix instructions](../README.md#reservation-strategy-matrix), then render:",
    "`python benchmarks/reservation/summarize_matrix.py RESULTS REPORT.md`.", ""]
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text("\n".join(lines), encoding="utf-8")
print(f"Validated {len(memory)} memory windows and {sum(map(len, times.values()))} timing rows; wrote {args.report}")
