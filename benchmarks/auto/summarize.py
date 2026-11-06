"""Validate both splits and expose worst Auto regret, not just average gains."""
import csv
import hashlib
import json
import math
import statistics
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"reservation"))
from check_matrix import validate_memory
from check_journal import validate

root,report=map(Path,sys.argv[1:])
frozen=json.loads((root/"freeze.json").read_text())
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert frozen["development_metadata_sha256"]==sha(root/"development/metadata.json")
data={}
for stage,count in (("development",59),("holdout",16)):
    directory=root/stage
    meta=json.loads((directory/"metadata.json").read_text())
    assert meta["stage"]==stage
    assert meta["binaries"]==frozen["binaries"]
    if stage=="holdout": assert meta["freeze_sha256"]==sha(root/"freeze.json")
    inputs={p["name"]:p["bytes"] for p in meta["inputs"]}
    assert len(inputs)==count
    memory={p:validate_memory((directory/f"{p}-memory.csv").read_text(),inputs) for p in meta["policies"]}
    times={}
    for number in range(1,meta["rounds"]+1):
        expected_order=meta["policies"][number-1:]+meta["policies"][:number-1] if meta["rounds"]==4 else meta["policies"][(number-1)%4:]+meta["policies"][:(number-1)%4]
        assert meta["round_order"][number-1]==expected_order
        for policy in meta["policies"]:
            rows=list(csv.DictReader((directory/f"{policy}-{number}.csv").open(newline="")))
            seen=set()
            for row in rows:
                key=row["dataset"],row["operation"]
                assert key not in seen
                seen.add(key)
                assert row["configuration"]=="Release" and int(row["bytes"])==inputs[key[0]]
                assert int(row["iterations"])==meta["iterations"] and int(row["samples"])==meta["samples"]
                assert 0<float(row["min_us"])<=float(row["median_us"])<=float(row["max_us"])
                times.setdefault((policy,*key),[]).append(float(row["median_us"]))
            assert seen=={(n,s) for n in inputs for s in ("parse","lex_only")}
    journals={}
    for goal in ("speed","memory"):
        text=(directory/f"{goal}-decisions.csv").read_text()
        names=set(inputs)|{"lex-error.json","parse-error.json","utf8-error.json"}
        rows=validate(text,goal,names)
        journals[goal]=rows
        for n in inputs:
            final=next(r for r in reversed(rows) if r["dataset"]==n and r["stage"]=="lex_only")
            row=memory[goal][n,"lex_only"]
            assert int(final["moved_tokens"])*int(row["token_size"])==int(row["copied_token_bytes"])
            assert final["tokens"]==row["token_count"] and final["actual_capacity"]==row["token_capacity"]
    data[stage]=dict(meta=meta,inputs=inputs,memory=memory,times=times,journals=journals)

def time(stage,policy,n): return statistics.median(data[stage]["times"][policy,n,"parse"])
def peak(stage,policy,n): return int(data[stage]["memory"][policy][n,"whole_parse"]["peak_live_bytes"])
def oracle_time(s,n): return min(time(s,"balanced",n),time(s,"low-memory",n))
def oracle_peak(s,n): return min(peak(s,"balanced",n),peak(s,"low-memory",n))
def gm(values): return math.exp(statistics.mean(math.log(v) for v in values))
def p95(values): return sorted(values)[math.ceil(.95*len(values))-1]

lines=["# Experimental Auto reservation", "", "## Scope and controller", "",
    "Two private controllers accept an explicit goal through the selected executable: speed or memory.",
    "They do not infer user priorities from JSON. Production ParseOptions, defaults and Json layout are unchanged;",
    "there is no public Auto preset. The experiment is enabled only with MASKETJSON_BUILD_AUTO_EXPERIMENT.",
    "The generated lexer retains the production JSON grammar; private AutoParser/AutoLexer names allow exact",
    "comparison with both independent production fixed modes in the same process.", "",
    "Decisions occur only when the token buffer is full. The pending token has already been scanned; features use",
    "consumed bytes, emitted/pending tokens, checkpoint density, stability streak and previous relocation volume.",
    "No pre-scan, second parse or full-document classification is performed by the timed controller.",
    "The evaluator's extra parsers and shape analysis run outside timing.", "",
    "| Goal | Ordinary growth floor | Requested capacity ceiling | Forecast |",
    "| --- | --- | --- | --- |",
    "| Speed | n + max(n/2, 1) | 4n+258 while untrusted; 16n+258 when mature | global density, recent density on sharp change, 6.25% margin |",
    "| Memory | n + max(n/4, 1); n + max(n/2, 1) under copy pressure | 2n+258 | conservative min(global, recent) density |", "",
    "Here n is the full buffer size before the pending token is inserted. Requests saturate at vector::max_size().",
    "Initial reservation is at most 256 slots. Density ratios below 0.25/above 4 detect sharp changes; ratios 0.5…2",
    "advance stability. Maturity requires n≥1024, consumed bytes≥4096 and two stable checkpoints, or one under",
    "copy pressure. Pressure is previous moved tokens >2n (speed) or >3n (memory). At source end only the pending",
    "token and required EOF are reserved; no ordinary geometric floor is forced for this final known count.",
    "These limits bound speculative requests and keep relocation amortized linear. They are not hard limits on",
    "STL allocator behavior, total DOM memory or wall-clock time. Prior allocations/copies cannot be undone.",
    "A density change can remain invisible until the next expansion, and a large existing reserve can hide it entirely.",
    "", "## Decision journal", "",
    "Separate AutoTrace executables record a fixed 512-entry stack journal; no vector/logging allocations occur inside it.",
    "Ordinary timing builds have no journal/TLS trace hook. Profile builds separately count captured heap and token relocation.",
    "Each row records source position, stored tokens, old/requested/actual capacity, ordinary lower/upper bounds,",
    "cumulative moved tokens after successful reserve, checkpoint/global densities, prediction, stability and reason.",
    "Reasons include warming, stable, density_drop/rise, copy_pressure, end_of_input, finish, abort and allocation failure.",
    "clamped/floored flags describe prediction relative to the ordinary window; end_of_input explicitly bypasses its floor.",
    "The stage identifies the lexer while doing whole parse or lex-only. A parser error can follow a successful lexer finish.",
    "Invalid-input acceptance and diagnostic fingerprints are retained; ordinary exceptions still behave as in production.",
    "Journal movement/capacity/count must exactly match the independent heap profile, and dropped records must be zero.",
    "", "## Protocol and method", "",
    "The existing 47 fixtures plus the previous 12 LowMemory holdout cases are now 59 known development inputs.",
    "Sixteen new evaluation fixtures use seed 20261107, counts 73/2053, spiky escaped strings, phased densities,",
    "long keys, zigzag nesting, internal/late padding and ragged numbers. Full-size cases were generated only after freeze.",
    f"The controller and binaries were frozen at `{frozen['frozen_utc']}` in [freeze.json](auto-msvc1951/freeze.json).",
    "No parameters were changed using held-out results. Development and evaluation have identical executable hashes.",
    "The [pre-recorded plan](../auto/plan.json) sets empirical promotion targets on non-tiny inputs (Balanced parse ≥1 ms):",
    "Speed worst time/oracle ≤1.5; Memory worst time/Balanced ≤2 and worst peak/oracle ≤1.5.",
    "The oracle is the better fixed mode per whole document, available only offline. It is not an online free signal.", ""]
meta=data["holdout"]["meta"]
lines += [f"{meta['cpu']}; {meta['os']}; logical CPU {meta['logical_cpu']}; static Release.",
    f"Each split uses {meta['rounds']} rotating rounds × {meta['samples']} samples × {meta['iterations']} operations, plus five warm-ups.",
    "Parse/lexer timing includes fresh construction and destruction. Input reading, validity checks and profiling are excluded.",
    "Medians below are medians of round medians; raw CSV preserves sample ranges. Tiny inputs, clock frequency, background",
    "activity and code layout limit small comparisons. Targets are experimental, not runtime guarantees or universal rankings.",
    "Captured heap means simultaneous successful C++ new requests, including overlapping buffers. Caller input, stack/root Json,",
    "allocator rounding/metadata, direct malloc/CRT internals and profiler bookkeeping are excluded. It is not RSS.",
    "Copied-token volume means sum(old live tokens × sizeof(Token)) at expansion, not hardware traffic.", "",
    "```text",meta["build_info"].strip(),"```", "", "## Worst-case assessment", "",
    "Targets are checked on non-tiny inputs only; the full tables retain every input, including small ones.", "",
    "| Split / goal | Inputs ≥1 ms | Geomean time/Balanced (all inputs) | p95 time/oracle | Worst time/oracle | Worst time/Balanced | Worst peak/oracle | Targets |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |"]
gates={}
for stage in data:
    names=list(data[stage]["inputs"])
    non_tiny=[n for n in names if time(stage,"balanced",n)>=1000]
    assert non_tiny
    for goal in ("speed","memory"):
        regrets=[time(stage,goal,n)/oracle_time(stage,n) for n in non_tiny]
        fixed_regrets=[time(stage,goal,n)/time(stage,"balanced",n) for n in non_tiny]
        peak_regrets=[peak(stage,goal,n)/oracle_peak(stage,n) for n in non_tiny]
        passed=max(regrets)<=1.5 if goal=="speed" else max(fixed_regrets)<=2 and max(peak_regrets)<=1.5
        gates[stage,goal]=passed
        lines.append(f"| {stage} / {goal} | {len(non_tiny)} | {gm([time(stage,goal,n)/time(stage,'balanced',n) for n in names]):.3f} | "
            f"{p95(regrets):.3f} | {max(regrets):.3f} | {max(fixed_regrets):.3f} | {max(peak_regrets):.3f} | {'PASS' if passed else 'FAIL'} |")
lines += ["", "Largest non-tiny time/oracle losses:", ""]
for stage in data:
    for goal in ("speed","memory"):
        names=[n for n in data[stage]["inputs"] if time(stage,"balanced",n)>=1000]
        worst=max(names,key=lambda n:time(stage,goal,n)/oracle_time(stage,n))
        lines.append(f"- {stage}/{goal}: {worst}, {time(stage,goal,worst)/1000:.3f} ms versus offline oracle {oracle_time(stage,worst)/1000:.3f} ms.")
lines += ["", "## Full parse-time matrix", "", "Milliseconds; Speed/oracle and Memory/oracle expose the cost of a wrong decision.", "",
    "| Split / input | Balanced | LowMemory | Auto speed | Auto memory | Speed/oracle | Memory/oracle |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
for stage in data:
    for n in data[stage]["inputs"]:
        cells=" | ".join(f"{time(stage,p,n)/1000:.4f}" for p in ("balanced","low-memory","speed","memory"))
        lines.append(f"| {stage} / {n} | {cells} | {time(stage,'speed',n)/oracle_time(stage,n):.3f} | {time(stage,'memory',n)/oracle_time(stage,n):.3f} |")
lines += ["", "## Captured whole-parse heap", "", "MiB. Retained DOM categories and counts match all four policies.", "",
    "| Split / input | Balanced | LowMemory | Auto speed | Auto memory | Memory/oracle |",
    "| --- | ---: | ---: | ---: | ---: | ---: |"]
for stage in data:
    for n in data[stage]["inputs"]:
        cells=" | ".join(f"{peak(stage,p,n)/1048576:.3f}" for p in ("balanced","low-memory","speed","memory"))
        lines.append(f"| {stage} / {n} | {cells} | {peak(stage,'memory',n)/oracle_peak(stage,n):.3f} |")
lines += ["", "## Observed journal reasons", "", "Lex-only events; whole-parse traces record the same lexer decisions separately.", "",
    "| Split / goal | Recorded events | Warming | Stable | Density drop/rise | Copy pressure | End-of-input growth |",
    "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
for stage in data:
    for goal,rows in data[stage]["journals"].items():
        valid=[r for r in rows if r["stage"]=="lex_only" and r["dataset"] in data[stage]["inputs"]]
        count=lambda reasons:sum(r["reason"] in reasons for r in valid)
        lines.append(f"| {stage} / {goal} | {len(valid)} | {count(('warming',))} | {count(('stable',))} | {count(('density_drop','density_rise'))} | {count(('copy_pressure',))} | {count(('end_of_input',))} |")
lines += ["", "## Interpretation", "",
    "Bounded requests prevent the previous 64× extrapolation from being applied blindly, while geometric floors",
    "bound token-relocation work. Neither property makes the best fixed strategy inferable from an arbitrary prefix.",
    "The goals express a caller priority; they do not change JSON semantics or discard work already performed.",
    "Failed target rows are evidence against promoting that controller under the stated criteria. Passed rows are",
    "local evidence only; they still need real workloads and more toolchains before a public Auto API.",
    "Automatic mode selection remains experimental; callers retain explicit Balanced/LowMemory control.",
    "", "A concrete memory counterexample for the speed goal:", ""]
n="held-tail-records-4099.json"
lines.append(f"- {n}: Auto speed {peak('development','speed',n)/1048576:.2f} MiB, Balanced {peak('development','balanced',n)/1048576:.2f} MiB, "
             f"LowMemory {peak('development','low-memory',n)/1048576:.2f} MiB. Passing the time target does not satisfy a memory constraint.")
lines += ["", "This is why a caller's priority and resource constraints must remain explicit, and why these prototypes are not a universal selector.",
    "", "## Verification and reproduce", "",
    "AutoVerify compares every Token field, canonical storage/zero representation and full error class/location/message",
    "against both independent production parsers in one process. It also checks compact/pretty round trips.",
    "Journal validation enforces event order, requested bounds, endpoint exceptions and relocation-work limits",
    "(Speed ≤5× final tokens+4096, Memory ≤8×+4096). Journal totals match heap-profile token copies/capacity exactly.",
    "All memory windows satisfy category/retained invariants and release tracked bytes and blocks after destruction.",
    "Windows/Linux smoke, sanitizer runs, deterministic corpus mutations and two coverage-instrumented Auto fuzz targets",
    "compare both goals to production. Performance targets are not imposed in shared CI timing environments.",
    "", "Raw [development, held-out results and journals](auto-msvc1951/) include binary/input/source hashes and execution order.",
    "See [Auto experiment instructions](../README.md#experimental-auto-controller). Render this report with:",
    "`python benchmarks/auto/summarize.py benchmarks/results/auto-msvc1951 REPORT.md`.", ""]
report.parent.mkdir(parents=True,exist_ok=True)
report.write_bytes(("\n".join(lines)).encode())
print(json.dumps({f"{s}/{g}":v for (s,g),v in gates.items()},indent=2))
