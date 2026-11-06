"""Portable functional checks of all experimental strategies (no timing thresholds)."""
import argparse
import csv
import io
import subprocess
import sys
from pathlib import Path

POLICIES = ("byte-quarter", "geometric", "adaptive4", "adaptive64")
CATEGORIES = ("other_bytes", "string_bytes", "array_bytes", "map_node_bytes", "token_bytes", "input_copy_bytes")


def executable(directory, stem):
    return directory / (stem + (".exe" if sys.platform == "win32" else ""))


def run(exe, *args):
    return subprocess.run([str(exe), *map(str, args)], check=True, capture_output=True).stdout.decode("utf-8-sig").replace("\r\n", "\n")


def validate_memory(text, expected_inputs):
    rows = list(csv.DictReader(io.StringIO(text)))
    indexed = {(r["dataset"], r["stage"]): r for r in rows}
    assert len(indexed) == len(rows) == len(expected_inputs) * 3
    assert set(indexed) == {(n, s) for n in expected_inputs for s in ("whole_parse", "lex_only", "input_copy")}
    for row in rows:
        assert int(row["input_bytes"]) == expected_inputs[row["dataset"]]
        assert int(row["after_destroy_bytes"]) == int(row["after_destroy_blocks"]) == 0
        assert sum(int(row[c]) for c in CATEGORIES) == int(row["retained_bytes"])
        assert int(row["peak_live_bytes"]) >= int(row["retained_bytes"])
        assert int(row["copied_token_bytes"]) % int(row["token_size"]) == 0
        assert int(row["token_buffer_allocations"]) <= int(row["new_calls"])
    for name in expected_inputs:
        p, lex = (indexed[name, s] for s in ("whole_parse", "lex_only"))
        assert all(p[c] == lex[c] for c in ("initial_token_capacity", "token_buffer_allocations", "token_growth_events", "copied_token_bytes"))
        assert int(lex["token_capacity"]) >= int(lex["token_count"])
    return indexed


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", type=Path, required=True)
    parser.add_argument("--reference-bin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    data = args.output / "data"
    run(executable(args.reference_bin, "ParseBenchmark"), "--generate", data)
    run(executable(args.reference_bin, "ParseBenchmark"), "--generate-profile", data, 4)
    subprocess.run([sys.executable, str(Path(__file__).with_name("generate_inputs.py")), str(data), "--base", "4"], check=True)
    inputs = sorted(data.glob("*.json"))
    assert len(inputs) == 47
    expected = {p.name: p.stat().st_size for p in inputs}
    reference = run(executable(args.bin, "ReservationInspectCurrent"), *inputs)
    errors = run(executable(args.bin, "ReservationInspectCurrent"), "--errors")
    baseline = None
    for policy in POLICIES:
        assert run(executable(args.bin, f"ReservationInspect-{policy}"), *inputs) == reference, policy
        assert run(executable(args.bin, f"ReservationInspect-{policy}"), "--errors") == errors, policy
        memory_text = run(executable(args.bin, f"ReservationProfile-{policy}"), *inputs)
        memory = validate_memory(memory_text, expected)
        (args.output / f"{policy}-memory.csv").write_text(memory_text, encoding="utf-8")
        if baseline is None:
            baseline = memory
        for key, row in memory.items():
            b = baseline[key]
            assert row["token_count"] == b["token_count"]
            if key[1] == "whole_parse":
                assert all(row[c] == b[c] for c in (*CATEGORIES, "retained_bytes", "retained_blocks", "dom_nodes", "array_capacity"))
        rows = list(csv.DictReader(io.StringIO(run(executable(args.bin, f"ReservationTime-{policy}"), 1, 1, *inputs))))
        assert len(rows) == 94 and {r["operation"] for r in rows} == {"parse", "lex_only"}
    heldout = args.output / "holdout"
    subprocess.run([sys.executable, str(Path(__file__).with_name("generate_holdout.py")), str(heldout), "--small"], check=True)
    public_inputs = inputs + sorted(heldout.glob("*.json"))
    expected_public = {p.name: p.stat().st_size for p in public_inputs}
    assert len(public_inputs) == 59
    reference_public = run(executable(args.bin, "ReservationInspectCurrent"), *public_inputs)
    assert run(executable(args.bin, "ReservationInspectLowMemory"), *public_inputs) == reference_public
    assert run(executable(args.bin, "ReservationInspectLowMemory"), "--errors") == errors
    validate_memory(run(executable(args.bin, "ReservationProfileLowMemory"), *public_inputs), expected_public)
    public_rows = list(csv.DictReader(io.StringIO(run(executable(args.bin, "ReservationTimeLowMemory"), 1, 1, *public_inputs))))
    assert len(public_rows) == 118
    print("47 strategy fixtures + 12 holdout structures: public LowMemory semantics, counters and timing smoke passed")


if __name__ == "__main__":
    main()
