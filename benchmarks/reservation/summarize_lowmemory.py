"""Validate the three experiment stages and render all LowMemory tradeoffs."""
import csv
import hashlib
import json
import math
import statistics
import sys
from pathlib import Path
from check_matrix import validate_memory

root, report = map(Path, sys.argv[1:])
selection = json.loads((root / "selection.json").read_text())
assert selection["chosen"] == "low100"
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(root / "training/metadata.json") == selection["training_metadata_sha256"]
data = {}
for stage, count in (("training", 47), ("holdout", 12), ("public", 59)):
    directory = root / stage
    meta = json.loads((directory / "metadata.json").read_text())
    inputs = {i["name"]: i["bytes"] for i in meta["inputs"]}
    assert len(inputs) == len(meta["inputs"]) == count
    policies = meta.get("policies", ["before", "balanced", "low-memory"])
    memory = {p: validate_memory((directory / f"{p}-memory.csv").read_text(), inputs) for p in policies}
    times = {}
    for policy in policies:
        for number in range(1, meta["rounds"] + 1):
            rows = list(csv.DictReader((directory / f"{policy}-{number}.csv").open(newline="")))
            seen = set()
            for row in rows:
                key = row["dataset"], row["operation"]
                assert key not in seen
                seen.add(key)
                assert int(row["bytes"]) == inputs[key[0]] and row["configuration"] == "Release"
                assert int(row["iterations"]) == meta["iterations"] and int(row["samples"]) == meta["samples"]
                assert 0 < float(row["min_us"]) <= float(row["median_us"]) <= float(row["max_us"])
                times.setdefault((policy, *key), []).append(float(row["median_us"]))
            assert seen == {(n, op) for n in inputs for op in ("parse", "lex_only")}
    data[stage] = dict(meta=meta, inputs=inputs, policies=policies, memory=memory, times=times)
assert data["holdout"]["meta"]["policies"] == [selection["chosen"], "current"]
assert data["public"]["meta"]["selection_sha256"] == sha(root / "selection.json")
for stage in ("training", "holdout"):
    split = {r["name"]: r for r in data[stage]["meta"]["inputs"]}
    public = {r["name"]: r for r in data["public"]["meta"]["inputs"] if r["split"] == stage}
    assert split.keys() == public.keys()
    assert all(split[n]["sha256"] == public[n]["sha256"] for n in split)
    for name in split:
        for mode, old in (("balanced", "current"), ("low-memory", "low100")):
            for phase in ("whole_parse", "lex_only", "input_copy"):
                assert data["public"]["memory"][mode][name,phase] == data[stage]["memory"][old][name,phase]
assert data["public"]["memory"]["before"] == data["public"]["memory"]["balanced"]
prototype = validate_memory((root / "public/prototype-low100-memory.csv").read_text(), data["public"]["inputs"])
assert prototype == data["public"]["memory"]["low-memory"]


def median(stage, mode, name, op="parse"):
    return statistics.median(data[stage]["times"][mode, name, op])


def peak(stage, mode, name):
    return int(data[stage]["memory"][mode][name,"whole_parse"]["peak_live_bytes"])


def geomean(values):
    return math.exp(statistics.mean(math.log(v) for v in values))


def percent(a, b):
    return 100 * (a/b - 1)


lines = ["# Opt-in LowMemory parsing", "", "## Policy and API", "",
    "ParseOptions adds Balanced (the existing default) and LowMemory. Both parser and lexer have new options overloads.",
    "Json layout, values, numeric storage, token fields, diagnostics and the 128-level nesting limit are unchanged.", "", "```cpp",
    "const ParseOptions options{ParsePreset::LowMemory};",
    "auto value = JsonParser::parse(text, options);",
    "auto tokens = JsonLexer(text).tokenize(options);", "```", "",
    "Both modes initially request at most 256 Token slots. On a full buffer, LowMemory clamps the density prediction",
    "between capacity + max(capacity/8, 1) and capacity + max(capacity, 256), with overflow-safe max_size bounds.",
    "The strategy is selected once; there is no runtime preset branch per token. No pre-scan or production heap hooks are added.",
    "This bounds speculative reserve requests, not total heap, allocator rounding, STL capacity or DOM memory.",
    "Old/new token buffers can overlap during growth. LowMemory can be slower and can have a higher peak on some inputs.",
    "Unknown preset values throw std::invalid_argument; JsonLexer rejects them before consuming input.",
    "ParseStats, Throughput and Auto remain future proposals.", "", "## Selection protocol", "",
    "Prototype sources and original observations are committed as `9d8f6ad18c33c0a683e5eb9c2c1c91b8e6d048e9`, based on `74b6294`.",
    "The [pre-recorded plan](../reservation/lowmemory-plan.json) defines three spare-capacity candidates: 25%, 50%, 100%.",
    "The initial/small-buffer allowance is the same; all clamp the existing density estimate, with a 12.5% minimum growth step.",
    "47 existing inputs form the training split. Require at least 75% tail-whitespace peak reduction and no more than",
    "20% long-ASCII peak increase. Compare equal-weight geometric mean parse-time ratios; candidates within 5% of the best",
    "are tied and the lower total token-relocation volume wins. These are workload-selection criteria, not a universal optimum.", "",
    "| Candidate | Eligible | Geomean parse time/current | Total relocated token MiB | Tail peak MiB |",
    "| --- | --- | ---: | ---: | ---: |"]
for p, score in selection["scores"].items():
    lines.append(f"| {p} | {score['eligible']} | {score['geomean_parse_time_ratio']:.3f} | {score['total_copied_token_bytes']/1048576:.2f} | {score['tail_peak_bytes']/1048576:.3f} |")
lines += ["", f"Selection was frozen at `{selection['frozen_utc']}` in [selection.json](low-memory-msvc1951/selection.json).",
    "low100 won the relocation tie-break against low50. Only then were 12 fixed held-out fixtures generated;",
    "their results did not change the parameters. They cover long keys, nested records, empty-container mixtures,",
    "sawtooth string lengths and escaped-string transitions, at counts 257/4099 instead of the training scales.",
    "", "## Measurement", ""]
for stage in ("training", "holdout", "public"):
    m = data[stage]["meta"]
    lines.append(f"- {stage}: {len(data[stage]['inputs'])} inputs; {m['rounds']} rotating rounds × {m['samples']} samples × {m['iterations']} operations; five warm-ups per operation.")
m = data["public"]["meta"]
lines += ["", f"{m['cpu']}; {m['os']}; logical CPU {m['logical_cpu']}; static Release.", "", "```text", m["build_info"].strip(), "```", "",
    "Timings come from uninstrumented executables; parse/lexer include construction and destruction of their results.",
    "File reading, validation, shape inventory and profiling are outside timing. Tables use median of round medians.",
    "The public run rotates the frozen pre-API control, public Balanced, and public LowMemory. Control variation",
    "and sample min/max in raw CSV limit small comparisons; CPU affinity does not control frequency/background work.",
    "Captured heap counts simultaneous requested C++ new bytes, including overlapping buffers. Caller input, stack/root Json,",
    "allocator rounding/metadata, direct malloc/CRT internals and profiler bookkeeping are excluded. This is not RSS.",
    "Relocated bytes mean sum(old live token count × sizeof(Token)) at expansions, not measured hardware traffic.",
    "Memory is measured separately. Production LowMemory timing uses the public API; profiles instrument a generated",
    "copy with the same allocation policy. Every memory/copy observation must equal the frozen low100 prototype.",
    "", "## Public API results — all inputs", "",
    "Δ is LowMemory/Balanced − 1; negative means less memory/time. Control ratio is public Balanced/pre-API time.", "",
    "| Split / input | Balanced peak MiB | LowMemory peak MiB | Δ peak | Balanced ms | LowMemory ms | Δ time | Control ratio |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"]
for item in m["inputs"]:
    n = item["name"]
    b, low = (peak("public",p,n) for p in ("balanced","low-memory"))
    bt, lt, old = (median("public",p,n) for p in ("balanced","low-memory","before"))
    lines.append(f"| {item['split']} / {n} | {b/1048576:.3f} | {low/1048576:.3f} | {percent(low,b):+.1f}% | {bt/1000:.4f} | {lt/1000:.4f} | {percent(lt,bt):+.1f}% | {bt/old:.3f} |")
lines += ["", "## Aggregate tradeoffs", "", "Equal-weight geometric mean time ratios are not total application throughput.", "",
    "| Split | LowMemory/Balanced parse time | Largest peak ratio LowMemory/Balanced |", "| --- | ---: | ---: |"]
for stage in ("training", "holdout"):
    names = data[stage]["inputs"]
    gm = geomean([median("public","low-memory",n)/median("public","balanced",n) for n in names])
    worst = max(peak("public","low-memory",n)/peak("public","balanced",n) for n in names)
    lines.append(f"| {stage} | {gm:.3f} | {worst:.3f} |")
lines += ["", "Examples of the intended benefit:", ""]
for n in ("whitespace-trailing-16384.json", "held-tail-records-4099.json"):
    lines.append(f"- {n}: {peak('public','balanced',n)/1048576:.2f} → {peak('public','low-memory',n)/1048576:.2f} MiB captured peak.")
regressions = sorted(data["public"]["inputs"], key=lambda n: peak("public","low-memory",n)/peak("public","balanced",n), reverse=True)[:3]
lines += ["", "Largest relative peak regressions:", ""]
for n in regressions:
    lines.append(f"- {n}: {peak('public','balanced',n)/1048576:.3f} → {peak('public','low-memory',n)/1048576:.3f} MiB.")
lines += ["", "Choose LowMemory for a useful reserve bound and measured workload benefit; the name does not guarantee a lower total peak on every input.",
    "The full tables and raw stage/copy counters retain unfavorable outcomes as well as improvements.",
    "", "## Verification and reproduction", "",
    "Both public presets match the frozen pre-API token-field/DOM fingerprints, shape descriptors and invalid-input diagnostics on all 59 inputs.",
    "Fingerprints are consistency checks, not collision-free proofs. Default allocation observations are unchanged;",
    "public LowMemory observations equal the frozen candidate on every memory field and stage. All captured windows",
    "return live bytes and blocks to zero after destruction, with retained categories partitioning totals.",
    "Unit tests cover range/EOF/error preservation, exact numeric storage/negative zero, nesting, long/trailing-whitespace",
    "regressions, invalid presets without lexer consumption, and arithmetic near size_t/max_size limits.",
    "Installed consumers exercise both header styles and DLL/static overloads. Corpus/mutation/fuzzer checks compare",
    "Balanced and LowMemory acceptance, diagnostics, values and numeric kinds; shared/static and sanitizer CI run separately.",
    "", "Raw [training, holdout and public observations](low-memory-msvc1951/) include source/binary/input hashes, selection and execution order.",
    "Training/holdout source manifests refer to prototype commit 9d8f6ad; the public manifest identifies the final implementation.",
    "LF attributes preserve cross-file hash references on checkout. Inputs/logs/binaries stay in ignored build output.",
    "See [reproduction instructions](../README.md#lowmemory-selection-and-public-api-evaluation). Render this report with:",
    "`python benchmarks/reservation/summarize_lowmemory.py benchmarks/results/low-memory-msvc1951 REPORT.md`.", ""]
report.parent.mkdir(parents=True, exist_ok=True)
report.write_bytes(("\n".join(lines)).encode())
print("Validated training/holdout/public observations, source-selection references and public/prototype allocation equality")
