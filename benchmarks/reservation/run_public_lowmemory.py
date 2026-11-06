"""Validate and measure public presets against the frozen pre-API control."""
import argparse
import csv
import hashlib
import io
import json
import platform
from pathlib import Path
from check_matrix import CATEGORIES, executable, run, validate_memory
from run_matrix import cpu_name, pin, sha

parser = argparse.ArgumentParser()
parser.add_argument("--bin", type=Path, required=True)
parser.add_argument("--before-bin", type=Path, required=True)
parser.add_argument("--training-inputs", type=Path, required=True)
parser.add_argument("--holdout-inputs", type=Path, required=True)
parser.add_argument("--selection", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--cpu", type=int, default=0)
parser.add_argument("--samples", type=int, default=7)
parser.add_argument("--iterations", type=int, default=5)
args = parser.parse_args()
assert 1 <= args.samples <= 101 and 1 <= args.iterations <= 1000
selection = json.loads(args.selection.read_text())
assert selection["chosen"] == "low100", "Public preset must match the frozen candidate"
args.output.mkdir(parents=True)
training = sorted(args.training_inputs.glob("*.json"))
holdout = sorted(args.holdout_inputs.glob("*.json"))
assert len(training) == 47 and len(holdout) == 12
inputs = training + holdout
expected = {p.name: p.stat().st_size for p in inputs}
assert len(expected) == 59
programs = {
    "before": (args.before_bin, "ReservationTimeCurrent", "ReservationInspectCurrent", "ReservationProfile-adaptive64"),
    "balanced": (args.bin, "ReservationTimeCurrent", "ReservationInspectCurrent", "ReservationProfile-adaptive64"),
    "low-memory": (args.bin, "ReservationTimeLowMemory", "ReservationInspectLowMemory", "ReservationProfileLowMemory"),
}
restore = pin(args.cpu)
try:
    reference = run(executable(args.before_bin, "ReservationInspectCurrent"), *inputs)
    errors = run(executable(args.before_bin, "ReservationInspectCurrent"), "--errors")
    (args.output / "shapes.csv").write_bytes(reference.encode())
    baseline = None
    observed = {}
    for name, (directory, time, inspect, profile) in programs.items():
        assert run(executable(directory, inspect), *inputs) == reference, name
        assert run(executable(directory, inspect), "--errors") == errors, name
        text = run(executable(directory, profile), *inputs)
        observed[name] = validate_memory(text, expected)
        (args.output / f"{name}-memory.csv").write_bytes(text.encode())
        if baseline is None:
            baseline = observed[name]
        for key, row in observed[name].items():
            assert row["token_count"] == baseline[key]["token_count"]
            if key[1] == "whole_parse":
                assert all(row[c] == baseline[key][c] for c in (*CATEGORIES, "retained_bytes", "retained_blocks", "dom_nodes", "array_capacity"))
            if name == "balanced":
                assert row == baseline[key], "Default allocation observations changed"
        print(f"{name}: memory/semantics/diagnostics complete", flush=True)
    prototype_text = run(executable(args.before_bin, "ReservationProfile-low100"), *inputs)
    prototype = validate_memory(prototype_text, expected)
    assert observed["low-memory"] == prototype, "Public API differs from frozen candidate allocation observations"
    (args.output / "prototype-low100-memory.csv").write_bytes(prototype_text.encode())
    orders = []
    policies = list(programs)
    for index in range(3):
        order = policies[index:] + policies[:index]
        orders.append(order)
        for name in order:
            directory, stem, _, _ = programs[name]
            text = run(executable(directory, stem), args.iterations, args.samples, *inputs)
            rows = list(csv.DictReader(io.StringIO(text)))
            assert len(rows) == 118 and all(r["configuration"] == "Release" for r in rows)
            (args.output / f"{name}-{index+1}.csv").write_bytes(text.encode())
            print(f"Public round {index+1}/3: {name}", flush=True)
    repo = Path(__file__).resolve().parents[2]
    paths = list((repo / "parser").glob("*.cpp")) + list((repo / "parser").glob("*.hpp"))
    paths += [p for p in Path(__file__).resolve().parent.iterdir() if p.suffix in (".cpp", ".hpp", ".py", ".txt", ".json")]
    paths += [repo / "CMakeLists.txt", repo / "benchmarks/CMakeLists.txt", repo / "benchmarks/ParserProfile.cpp",
              repo / "benchmarks/MemoryTracker.cpp", repo / "benchmarks/MemoryTracker.hpp", repo / "benchmarks/BenchmarkUtils.cpp"]
    bins = {name: {stem: sha(executable(directory, stem)) for stem in stems}
            for name, (directory, *stems) in programs.items()}
    bins["prototype-low100"] = sha(executable(args.before_bin, "ReservationProfile-low100"))
    meta = dict(prototype_revision="9d8f6ad18c33c0a683e5eb9c2c1c91b8e6d048e9",
        selection_sha256=sha(args.selection), cpu=cpu_name(), os=platform.platform(), logical_cpu=args.cpu,
        rounds=3, samples=args.samples, iterations=args.iterations, round_order=orders,
        build_info=(args.bin / "build-info-Release.txt").read_text(), binaries=bins,
        inputs=[dict(name=p.name, split="training" if p in training else "holdout", bytes=p.stat().st_size, sha256=sha(p)) for p in inputs],
        source_hash_format="UTF-8 without BOM, LF line endings",
        sources=[dict(file=p.relative_to(repo).as_posix(),sha256=hashlib.sha256(p.read_text(encoding="utf-8-sig").encode()).hexdigest()) for p in sorted(paths)])
    (args.output / "metadata.json").write_bytes((json.dumps(meta, indent=2)+"\n").encode())
finally:
    restore()
