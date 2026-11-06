"""Validate raw baseline files and write a Markdown report (Python 3, stdlib only)."""
import csv
import json
import statistics
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("Usage: summarize_memory_baseline.py RESULTS_DIRECTORY REPORT.md")
root, destination = map(Path, sys.argv[1:])
meta = json.loads((root / "metadata.json").read_text(encoding="utf-8-sig"))
memory = list(csv.DictReader((root / "memory.csv").open(encoding="utf-8-sig", newline="")))
inputs = {item["file"]: item for item in meta["inputs"]}
assert len(memory) == len(inputs) * 3
indexed = {}
categories = ("other_bytes", "string_bytes", "array_bytes", "map_node_bytes", "token_bytes", "input_copy_bytes")
for row in memory:
    key = (row["dataset"], row["stage"])
    assert key not in indexed
    indexed[key] = row
    assert int(row["input_bytes"]) == inputs[row["dataset"]]["bytes"]
    assert int(row["after_destroy_bytes"]) == int(row["after_destroy_blocks"]) == 0
    assert sum(int(row[c]) for c in categories) == int(row["retained_bytes"])
    assert int(row["peak_live_bytes"]) >= int(row["retained_bytes"])
times = {}
for number in range(1, meta["rounds"] + 1):
    rows = list(csv.DictReader((root / f"timing-{number}.csv").open(encoding="utf-8-sig", newline="")))
    seen = set()
    for row in rows:
        key = (row["dataset"], row["library"], row["operation"])
        assert key not in seen
        seen.add(key)
        assert row["configuration"] == "Release"
        assert int(row["iterations"]) == meta["iterations"] and int(row["samples"]) == meta["samples"]
        assert int(row["bytes"]) == inputs[row["dataset"]]["bytes"]
        times.setdefault(key, []).append(float(row["median_us"]))
assert all(len(values) == meta["rounds"] for values in times.values())

def mib(value):
    return int(value) / (1024 * 1024)

def milliseconds(dataset, library, operation="parse"):
    values = times.get((dataset, library, operation))
    return f"{statistics.median(values)/1000:.4f}" if values else "—"

lines = [
    "# Parser memory and timing baseline", "",
    f"Local Windows Release baseline on {meta['cpu']}; {meta['os']}; logical CPU {meta['logical_cpu']}.",
    f"Source base: `{meta['base_revision']}` plus the profiling-only files and hashes in metadata.",
    "Parser, lexer, value representation, serializer, and file I/O implementations are unchanged in this step.",
    "", "## Method", "",
    f"{len(inputs)} deterministic inputs; memory captured once per stage. Ordinary timings use "
    f"{meta['rounds']} rounds × {meta['samples']} samples × {meta['iterations']} operations, plus warm-up.",
    "Tables show the median of round medians; raw CSV includes each sample range. Fixed library order,",
    "CPU frequency and background activity limit small comparisons. These are local results, not universal rankings.",
    "File loading and round-trip/semantic validation are outside measurements. Parse timing includes result destruction.",
    "Libraries: " + "; ".join(f"{x['library']} {x['version']} ({x['compiler']})" for x in meta["libraries"]) + ".",
    "", "Captured heap counts successful C++ new/new[] requests, including aligned/nothrow forms.",
    "Peak is simultaneous requested bytes; retained is sampled before result destruction. Caller input, stack,",
    "root Json object, allocator rounding/metadata, direct malloc/CRT internals, and profiler bookkeeping are excluded.",
    "This is not process RSS/working set. Boost/Qt memory is not measured because allocator coverage would differ.",
    "Retained labels use owning container addresses (including interior-aligned MSVC buffers); SSO stays with its owner.",
    "Other contains unclassified STL bookkeeping. Categories partition retained memory, not peak memory.",
    "Stage times/peaks are separate observations, not additive components; subtraction is not a DOM-only measurement.",
    "", "## Captured MasketJSON heap", "",
    "MiB = 1,048,576 bytes. Small-document ratios include fixed container/token overhead.", "",
    "| Input | Input MiB | Peak MiB | Retained DOM MiB | Peak/input |",
    "| --- | ---: | ---: | ---: | ---: |",
]
for name in sorted(inputs):
    row = indexed[name, "whole_parse"]
    lines.append(f"| {name} | {mib(row['input_bytes']):.4f} | {mib(row['peak_live_bytes']):.4f} | "
                 f"{mib(row['retained_bytes']):.4f} | {int(row['peak_live_bytes'])/int(row['input_bytes']):.2f}× |")
lines += ["", "## Retained allocation attribution", "",
          "| Input | String MiB | Array MiB | Map node MiB | Other MiB |",
          "| --- | ---: | ---: | ---: | ---: |"]
selected = ["large.json", "wide-objects-16384.json", "wide-numbers-262144.json", "long-ascii-4096.json", "escaped-4096.json"]
for name in selected:
    if (name, "whole_parse") not in indexed:
        continue
    row = indexed[name, "whole_parse"]
    lines.append("| " + name + " | " + " | ".join(f"{mib(row[c]):.4f}" for c in
                 ("string_bytes", "array_bytes", "map_node_bytes", "other_bytes")) + " |")
lines += ["", "## Uninstrumented parse/stage timings", "", "Milliseconds per operation.", "",
          "| Input | MasketJSON | Boost.JSON | Qt | MasketJSON lexer |",
          "| --- | ---: | ---: | ---: | ---: |"]
for name in sorted(inputs):
    lines.append(f"| {name} | {milliseconds(name,'MasketJSON')} | {milliseconds(name,'Boost.JSON')} | "
                 f"{milliseconds(name,'Qt')} | {milliseconds(name,'MasketJSON','lex_only')} |")
lines += ["", "## Findings and next experiment", ""]
if ("long-ascii-4096.json", "whole_parse") in indexed:
    row, lex = indexed["long-ascii-4096.json", "whole_parse"], indexed["long-ascii-4096.json", "lex_only"]
    lines += [f"Long ASCII strings: {mib(row['input_bytes']):.2f} MiB input, {mib(row['peak_live_bytes']):.2f} MiB captured peak, "
              f"{mib(row['retained_bytes']):.2f} MiB retained DOM. Lexing retains {mib(lex['token_bytes']):.2f} MiB of token storage "
              f"for {lex['token_count']} tokens with capacity {lex['token_capacity']}.",
              "The lexer currently reserves source.size()/4 Token records. This is evidence that byte-based token",
              "reservation is costly for long strings with few tokens. A bounded/adaptive reservation policy is",
              "the first proposed experiment; it must be checked against wide-number/object inputs and depth cases.", ""]
if ("wide-numbers-262144.json", "whole_parse") in indexed:
    row, lex = indexed["wide-numbers-262144.json", "whole_parse"], indexed["wide-numbers-262144.json", "lex_only"]
    lines += [f"Wide numbers: lexer peak {mib(lex['peak_live_bytes']):.2f} MiB; whole-parse peak "
              f"{mib(row['peak_live_bytes']):.2f} MiB; retained DOM {mib(row['retained_bytes']):.2f} MiB.",
              "Inspect token-vector growth as well as initial reservation; reducing reservation can trade memory",
              "against reallocation/copying cost. No parser optimization is included in this baseline.", ""]
lines += [f"All {len(memory)} memory windows returned captured live bytes AND blocks to zero after destruction.",
          "This validates captured allocation lifetimes, not all process allocations or a general leak-free claim.", "",
          "## Reproduce", "", "See [benchmark instructions](../README.md) and RunMemoryBaseline.ps1.",
          "Generate the original five fixtures plus --generate-profile BASE=256; use a static unsanitized Release build.",
          "Enable local Boost.JSON and Qt backends for the same semantic-checked timing comparison.",
          "Tracker self-tests and CTest fixture/accounting checks are correctness checks, never performance thresholds.", "",
          "Raw files: " + ", ".join(f"[{name}](parser-memory-msvc1951/{name})" for name in
          ("metadata.json", "memory.csv", "timing-1.csv", "timing-2.csv", "timing-3.csv")) + ".", ""]
destination.write_text("\n".join(lines), encoding="utf-8")
print(f"Validated {len(inputs)} inputs, {len(memory)} memory rows, {sum(map(len,times.values()))} timing rows")
