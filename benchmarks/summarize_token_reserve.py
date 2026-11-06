"""Validate the paired allocation/timing experiment and render its report."""
import csv
import json
import statistics
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("Usage: summarize_token_reserve.py RESULTS_DIRECTORY REPORT.md")
root, destination = map(Path, sys.argv[1:])
meta = json.loads((root / "metadata.json").read_text(encoding="utf-8-sig"))
experiment = json.loads((root / "experiment.json").read_text(encoding="utf-8-sig"))
inputs = {item["name"]: item for item in meta["inputs"]}
assert len(inputs) == len(meta["inputs"]) == 23
assert meta["stages"]
assert experiment["base_revision"] == meta["before_revision"]
assert experiment["configuration"]["configuration"] == "Release"
assert not experiment["configuration"]["shared"] and not experiment["configuration"]["sanitizers"]
assert experiment["qt_dll_before_sha256"] == experiment["qt_dll_after_sha256"]
assert any(item["file"] == "parser/JsonLexer.cpp" for item in experiment["after_sources"])
categories = ("other_bytes", "string_bytes", "array_bytes", "map_node_bytes", "token_bytes", "input_copy_bytes")
inventory = ("dom_nodes", "object_members", "array_elements", "array_capacity", "string_values",
             "string_chars", "string_capacity_chars", "token_count", "token_size", "json_size")
memory, times = {}, {}
expected_times = {(name, library, operation) for name in inputs
                  for library, operations in (("MasketJSON", ("parse", "serialize", "lex_only", "input_copy")),
                                               ("Boost.JSON", ("parse", "serialize")), ("Qt", ("parse", "serialize")))
                  for operation in operations}
for variant in ("before", "after"):
    rows = list(csv.DictReader((root / f"{variant}-memory.csv").open(encoding="utf-8-sig", newline="")))
    seen = set()
    for row in rows:
        key = row["dataset"], row["stage"]
        assert key not in seen
        seen.add(key)
        memory[variant, *key] = row
        assert int(row["input_bytes"]) == inputs[row["dataset"]]["bytes"]
        assert int(row["after_destroy_bytes"]) == int(row["after_destroy_blocks"]) == 0
        assert sum(int(row[c]) for c in categories) == int(row["retained_bytes"])
        assert int(row["peak_live_bytes"]) >= int(row["retained_bytes"])
    assert seen == {(name, stage) for name in inputs for stage in ("whole_parse", "lex_only", "input_copy")}
    for number in range(1, meta["rounds"] + 1):
        rows = list(csv.DictReader((root / f"{variant}-{number}.csv").open(encoding="utf-8-sig", newline="")))
        seen = set()
        for row in rows:
            key = row["dataset"], row["library"], row["operation"]
            assert key not in seen
            seen.add(key)
            assert int(row["bytes"]) == inputs[row["dataset"]]["bytes"]
            assert int(row["iterations"]) == meta["iterations"] and int(row["samples"]) == meta["samples"]
            assert row["configuration"] == "Release"
            assert row["compiler"] == "MSVC 1951"
            assert float(row["min_us"]) <= float(row["median_us"]) <= float(row["max_us"])
            times.setdefault((variant, *key), []).append(float(row["median_us"]))
        assert seen == expected_times
for name in inputs:
    for stage in ("whole_parse", "lex_only", "input_copy"):
        b, a = (memory[v, name, stage] for v in ("before", "after"))
        assert all(b[c] == a[c] for c in inventory)
        if stage != "lex_only":
            assert all(b[c] == a[c] for c in (*categories, "retained_bytes", "retained_blocks"))
assert all(len(values) == meta["rounds"] for values in times.values())


def mib(value):
    return int(value) / 1048576


def values(variant, name, library="MasketJSON", operation="parse"):
    return times[variant, name, library, operation]


def change(name, library="MasketJSON", operation="parse"):
    b, a = (statistics.median(values(v, name, library, operation)) for v in ("before", "after"))
    return 100 * (a / b - 1)


lines = [
    "# Adaptive token reservation", "", "## Implementation", "",
    "The lexer starts with at most 256 Token slots instead of source.size()/4 slots.",
    "When full, it estimates the total token count from tokens emitted and input bytes consumed,",
    "including the pending token and EOF. Growth is clamped between 1.5× and 64× capacity,",
    "with overflow-safe bounds against vector::max_size(). A zero-capacity vector starts with one slot.",
    "The estimate is an allocation hint; it adds no input scan or syntax decisions.",
    "Requested capacity is bounded by the initial 256 slots or 64 times the token count at the last growth.",
    "A dense prefix can still overestimate a sparse tail within that bound; a sparse prefix can",
    "underestimate a dense tail and cause further growth. Token contents, diagnostics, DOM and public API are unchanged.",
    "", "Exploratory runs rejected a simple 4,096-slot initial cap with ordinary vector growth:",
    "repeated large copies hurt wide arrays. A conservative 4× adaptive upper step also caused",
    "extra copying. A 64× upper step allows an estimate to cover wide inputs earlier while keeping",
    "reservation proportional to tokens already seen. A trial with a 12.5% prediction margin",
    "increased object-array memory; the final policy uses the observed density without that extra margin.",
    "These exploratory choices preceded the separate final paired experiment below.",
    "", "## Method", "",
    f"Baseline `{meta['before_revision']}`; candidate is that revision plus the lexer changes identified",
    "by normalized source hashes in [experiment.json](token-reserve-msvc1951/experiment.json).",
    f"{meta['cpu']}; {meta['os']}; logical CPU {meta['logical_cpu']}; static Release; MSVC 19.51.",
    "Boost.JSON 1.88 and Qt 6.11.0 are unchanged timing controls. Both builds use the same options,",
    "libraries and 23 deterministic inputs; frozen executable and input hashes are recorded.",
    f"{meta['rounds']} rounds × {meta['samples']} samples × {meta['iterations']} operations, plus warm-up; before/after order alternates by round.",
    "Timing uses ordinary uninstrumented executables. Parse includes result destruction; input loading",
    "and semantic/round-trip checks are outside timing. Tables use the median of round medians;",
    "Δ is after/before − 1 (negative means less time/memory). Paired range uses each round's ratio.",
    "CPU affinity controls placement, not frequency or background activity. Small differences within",
    "the controls' variation should not be treated as reliable improvements; these are local results.",
    "", "Memory is measured separately, once per stage/build, using captured successful C++ new requests.",
    "Peak means simultaneous requested bytes, including overlapping old/new token buffers during growth.",
    "Caller input, stack/root object, allocator rounding/metadata, direct malloc/CRT internals and profiler",
    "bookkeeping are excluded. This is not RSS; Boost/Qt memory is not compared. Stage peaks/times are not additive.",
    "", "## Whole-parse captured heap", "", "MiB = 1,048,576 bytes. Retained DOM is identical before/after for every input.", "",
    "| Input | Before peak MiB | After peak MiB | Δ peak | Retained DOM MiB |",
    "| --- | ---: | ---: | ---: | ---: |",
]
for name in sorted(inputs):
    b, a = (memory[v, name, "whole_parse"] for v in ("before", "after"))
    delta = 100 * (int(a["peak_live_bytes"]) / int(b["peak_live_bytes"]) - 1)
    lines.append(f"| {name} | {mib(b['peak_live_bytes']):.4f} | {mib(a['peak_live_bytes']):.4f} | {delta:+.1f}% | {mib(a['retained_bytes']):.4f} |")
lines += ["", "## Uninstrumented parse time", "", "Milliseconds per operation; unchanged Boost/Qt controls show their own parse-time Δ.", "",
          "| Input | Before ms | After ms | Δ | Paired Δ range | Boost Δ | Qt Δ |",
          "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    b, a = (values(v, name) for v in ("before", "after"))
    paired = [100 * (y / x - 1) for x, y in zip(b, a)]
    lines.append(f"| {name} | {statistics.median(b)/1000:.4f} | {statistics.median(a)/1000:.4f} | {change(name):+.1f}% | "
                 f"{min(paired):+.1f}…{max(paired):+.1f}% | {change(name,'Boost.JSON'):+.1f}% | {change(name,'Qt'):+.1f}% |")
lines += ["", "## Lexer allocation and time", "", "Allocation calls include token buffers and the lexer's temporary source copy; only tokens remain at the retained snapshot.",
          "The raw CSV also records peak blocks, cumulative requested bytes, and token counts/sizes.", "",
          "| Input | Tokens | Before capacity | After capacity | Before calls | After calls | Before ms | After ms |",
          "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    b, a = (memory[v, name, "lex_only"] for v in ("before", "after"))
    bt, at = (statistics.median(values(v, name, operation="lex_only"))/1000 for v in ("before", "after"))
    lines.append(f"| {name} | {a['token_count']} | {b['token_capacity']} | {a['token_capacity']} | {b['new_calls']} | {a['new_calls']} | {bt:.4f} | {at:.4f} |")
lines += ["", "## Interpretation", ""]
for name in ("long-ascii-4096.json", "wide-numbers-262144.json"):
    b, a = (memory[v, name, "whole_parse"] for v in ("before", "after"))
    lines.append(f"{name}: captured peak falls from {mib(b['peak_live_bytes']):.2f} to {mib(a['peak_live_bytes']):.2f} MiB. "
                 f"Median parse-time change is {change(name):+.1f}%.")
lines += ["", f"There is a measurable tradeoff on medium.json ({change('medium.json'):+.1f}% parse time) and "
          f"wide-objects-2048.json ({change('wide-objects-2048.json'):+.1f}%). Their paired ratios are positive in every round.",
          "The large mixed/object fixtures have almost unchanged captured peak; extra token-buffer copying",
          "makes their isolated lexer slower. The large.json whole-parse ratio crosses zero across rounds,",
          "so its lower aggregate time is not evidence of a consistent gain. Tiny-input percentages are",
          "sensitive to timer resolution; substantial control variation also limits claims about small changes.",
          "The policy is selected for its sparse-input memory reduction and wide-number result, accepting these dense-input costs.",
          "", "## Checks and limits", "",
          f"All {len(memory)} memory windows return captured bytes and blocks to zero after destruction; retained categories partition their totals.",
          "DOM inventories, retained DOM categories/bytes and emitted token counts match in both builds.",
          "Regressions cover long strings, whitespace-only and empty inputs, both directions of density change,",
          "every number/string range and EOF location across growth, and error locations after expansion.",
          "The change trades a few additional allocations for substantially less unused token storage on sparse inputs.",
          "Dense/object inputs can still pay for token copying; the tables expose any time or memory regressions.",
          "No timing threshold is imposed in CI. Sanitizer/fuzzer and shared/static platform checks run separately.",
          "", "## Reproduce", "",
          "Generate the same inputs with ParseBenchmark --generate DIR and --generate-profile DIR 256.",
          "Build the baseline and candidate as described in [benchmark instructions](../README.md), keeping executables separate.",
          "Capture `ParserProfile INPUT...` into before-memory.csv / after-memory.csv, then run:", "", "```powershell",
          ".\\benchmarks\\CompareBuilds.ps1 -BeforeExe BEFORE -AfterExe AFTER -Inputs $inputs -OutputDirectory comparison -BeforeRevision BASE -AfterRevision CANDIDATE -Rounds 3 -Samples 9 -Iterations 10 -Cpu 0 -Stages",
          "```", "", "The raw [comparison directory](token-reserve-msvc1951/) contains both memory files, six timing files,",
          "runner metadata and experiment source/profile hashes. After supplying the matching experiment metadata, render with:",
          "`python benchmarks/summarize_token_reserve.py comparison report.md`.", ""]
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_text("\n".join(lines), encoding="utf-8")
print(f"Validated {len(memory)} memory windows and {sum(map(len, times.values()))} timing rows; wrote {destination}")
