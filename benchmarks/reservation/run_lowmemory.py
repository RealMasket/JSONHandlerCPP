"""Compare bounded-reserve candidates on a fixed input split; no API tuning here."""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
from check_matrix import executable, run, validate_memory
from run_matrix import pin, cpu_name, sha

parser = argparse.ArgumentParser()
parser.add_argument("--bin", type=Path, required=True)
parser.add_argument("--inputs", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--policies", nargs="+", default=["low25", "low50", "low100", "current"])
parser.add_argument("--rounds", type=int, default=4)
parser.add_argument("--samples", type=int, default=5)
parser.add_argument("--iterations", type=int, default=3)
parser.add_argument("--cpu", type=int, default=0)
parser.add_argument("--split", choices=("training", "holdout", "public"), required=True)
args = parser.parse_args()
assert set(args.policies) <= {"low25", "low50", "low100", "current"}
assert len(set(args.policies)) == len(args.policies) and "current" in args.policies
assert args.rounds >= len(args.policies) and args.rounds % len(args.policies) == 0
assert 1 <= args.iterations <= 1000 and 1 <= args.samples <= 101
args.output.mkdir(parents=True)
inputs = sorted(args.inputs.glob("*.json"))
assert inputs
expected = {p.name: p.stat().st_size for p in inputs}
restore = pin(args.cpu)
try:
    reference = run(executable(args.bin, "ReservationInspectCurrent"), *inputs)
    (args.output / "shapes.csv").write_bytes(reference.encode())
    baseline = None
    bins = []
    for policy in args.policies:
        inspect = "ReservationInspectCurrent" if policy == "current" else f"ReservationInspect-{policy}"
        profile = "ReservationProfile-adaptive64" if policy == "current" else f"ReservationProfile-{policy}"
        time = "ReservationTimeCurrent" if policy == "current" else f"ReservationTime-{policy}"
        bins += [executable(args.bin, stem) for stem in (inspect, profile, time)]
        assert run(executable(args.bin, inspect), *inputs) == reference
        text = run(executable(args.bin, profile), *inputs)
        memory = validate_memory(text, expected)
        (args.output / f"{policy}-memory.csv").write_bytes(text.encode())
        if baseline is None:
            baseline = memory
        for key, row in memory.items():
            assert row["token_count"] == baseline[key]["token_count"]
            if key[1] == "whole_parse":
                assert all(row[c] == baseline[key][c] for c in ("retained_bytes", "retained_blocks", "dom_nodes", "array_capacity"))
        print(f"{policy}: memory/semantics complete", flush=True)
    orders = []
    for number in range(args.rounds):
        offset = number % len(args.policies)
        order = args.policies[offset:] + args.policies[:offset]
        orders.append(order)
        for policy in order:
            stem = "ReservationTimeCurrent" if policy == "current" else f"ReservationTime-{policy}"
            text = run(executable(args.bin, stem), args.iterations, args.samples, *inputs)
            rows = list(csv.DictReader(io.StringIO(text)))
            assert len(rows) == len(inputs) * 2 and all(r["configuration"] == "Release" for r in rows)
            (args.output / f"{policy}-{number+1}.csv").write_bytes(text.encode())
            print(f"{args.split} round {number+1}/{args.rounds}: {policy}", flush=True)
    repo = Path(__file__).resolve().parents[2]
    paths = list((repo / "parser").glob("*.cpp")) + list((repo / "parser").glob("*.hpp"))
    paths += [p for p in Path(__file__).resolve().parent.iterdir() if p.suffix in (".cpp", ".hpp", ".py", ".txt")]
    paths += [repo / "CMakeLists.txt", repo / "benchmarks/CMakeLists.txt", repo / "benchmarks/ParserProfile.cpp",
              repo / "benchmarks/MemoryTracker.cpp", repo / "benchmarks/MemoryTracker.hpp", repo / "benchmarks/BenchmarkUtils.cpp"]
    meta = dict(split=args.split, cpu=cpu_name(), logical_cpu=args.cpu, rounds=args.rounds, samples=args.samples,
        iterations=args.iterations, policies=args.policies, round_order=orders,
        build_info=(args.bin / "build-info-Release.txt").read_text(),
        binaries={p.name: sha(p) for p in sorted(set(bins))},
        inputs=[dict(name=p.name, bytes=p.stat().st_size, sha256=sha(p)) for p in inputs],
        source_hash_format="UTF-8 without BOM, LF line endings",
        sources=[dict(file=p.relative_to(repo).as_posix(),sha256=hashlib.sha256(p.read_text(encoding="utf-8-sig").encode()).hexdigest()) for p in sorted(paths)])
    (args.output / "metadata.json").write_bytes((json.dumps(meta, indent=2)+"\n").encode())
finally:
    restore()
