"""Affinity-controlled, rotating-order matrix runner. Python 3, stdlib only."""
import argparse
import csv
import hashlib
import io
import json
import os
import platform
import sys
from pathlib import Path
from check_matrix import POLICIES, executable, run, validate_memory


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pin(cpu):
    if sys.platform == "win32":
        import ctypes
        from ctypes import wintypes
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.GetCurrentProcess.restype = wintypes.HANDLE
        kernel.GetProcessAffinityMask.argtypes = [wintypes.HANDLE, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]
        kernel.SetProcessAffinityMask.argtypes = [wintypes.HANDLE, ctypes.c_size_t]
        process = kernel.GetCurrentProcess()
        original, system = ctypes.c_size_t(), ctypes.c_size_t()
        if not kernel.GetProcessAffinityMask(process, original, system):
            raise ctypes.WinError(ctypes.get_last_error())
        mask = 1 << cpu
        if not original.value & mask or not kernel.SetProcessAffinityMask(process, mask):
            raise RuntimeError("CPU outside allowed mask or affinity update failed")
        return lambda: kernel.SetProcessAffinityMask(process, original.value)
    original = os.sched_getaffinity(0)
    if cpu not in original:
        raise RuntimeError("CPU outside allowed affinity set")
    os.sched_setaffinity(0, {cpu})
    return lambda: os.sched_setaffinity(0, original)


def cpu_name():
    if sys.platform == "win32":
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as key:
            return winreg.QueryValueEx(key, "ProcessorNameString")[0].strip()
    return platform.processor()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", type=Path, required=True)
    parser.add_argument("--reference-bin", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--base-revision", required=True)
    parser.add_argument("--rounds", type=int, default=4)
    parser.add_argument("--samples", type=int, default=7)
    parser.add_argument("--iterations", type=int, default=5)
    parser.add_argument("--cpu", type=int, default=0)
    args = parser.parse_args()
    if args.rounds % len(POLICIES) or not 4 <= args.rounds <= 12 or not 1 <= args.samples <= 101 or not 1 <= args.iterations <= 1000:
        parser.error("rounds must be 4/8/12; samples 1..101; iterations 1..1000")
    args.output.mkdir(parents=True)  # Refuse existing/partial results.
    logs = args.output / "logs"
    logs.mkdir()
    restore = pin(args.cpu)
    try:
        data = args.output / "data"
        (logs / "generate.log").write_text(run(executable(args.reference_bin, "ParseBenchmark"), "--generate", data)
            + run(executable(args.reference_bin, "ParseBenchmark"), "--generate-profile", data, 256), encoding="utf-8")
        import subprocess
        subprocess.run([sys.executable, str(Path(__file__).with_name("generate_inputs.py")), str(data)], check=True)
        inputs = sorted(data.glob("*.json"))
        assert len(inputs) == 47
        expected = {p.name: p.stat().st_size for p in inputs}
        reference = run(executable(args.bin, "ReservationInspectCurrent"), *inputs)
        errors = run(executable(args.bin, "ReservationInspectCurrent"), "--errors")
        (args.output / "shapes.csv").write_text(reference, encoding="utf-8")
        (args.output / "diagnostics.csv").write_text(errors, encoding="utf-8")
        for policy in POLICIES:
            assert run(executable(args.bin, f"ReservationInspect-{policy}"), *inputs) == reference, policy
            assert run(executable(args.bin, f"ReservationInspect-{policy}"), "--errors") == errors, policy
            memory = run(executable(args.bin, f"ReservationProfile-{policy}"), *inputs)
            validate_memory(memory, expected)
            (args.output / f"{policy}-memory.csv").write_text(memory, encoding="utf-8")
            print(f"{policy}: inspection and memory complete", flush=True)
        order_log = []
        for number in range(args.rounds):
            order = list(POLICIES[number % len(POLICIES):] + POLICIES[:number % len(POLICIES)])
            # Production/default control brackets alternating rounds.
            order.insert(0 if number % 2 == 0 else len(order), "current")
            order_log.append(order)
            for policy in order:
                stem = "ReservationTimeCurrent" if policy == "current" else f"ReservationTime-{policy}"
                text = run(executable(args.bin, stem), args.iterations, args.samples, *inputs)
                rows = list(csv.DictReader(io.StringIO(text)))
                assert len(rows) == 94 and all(r["configuration"] == "Release" for r in rows)
                (args.output / f"{policy}-{number + 1}.csv").write_text(text, encoding="utf-8")
                print(f"Round {number + 1}/{args.rounds}: {policy}", flush=True)
        repo = Path(__file__).resolve().parents[2]
        source_paths = [repo / "CMakeLists.txt", repo / "benchmarks/CMakeLists.txt", repo / "benchmarks/ParserProfile.cpp",
            repo / "benchmarks/MemoryTracker.cpp", repo / "benchmarks/MemoryTracker.hpp",
            repo / "benchmarks/BenchmarkUtils.cpp", repo / "benchmarks/BenchmarkUtils.hpp"]
        source_paths += list((repo / "parser").glob("*.cpp")) + list((repo / "parser").glob("*.hpp"))
        source_paths += [p for p in Path(__file__).resolve().parent.iterdir() if p.suffix in (".cpp", ".hpp", ".py", ".txt")]
        bins = [p for p in args.bin.iterdir() if p.name.startswith("Reservation") and p.suffix == (".exe" if sys.platform == "win32" else "") and p.is_file()]
        build_info = (args.bin / "build-info-Release.txt").read_text(encoding="utf-8")
        metadata = dict(base_revision=args.base_revision, cpu=cpu_name(), os=platform.platform(), logical_cpu=args.cpu,
            rounds=args.rounds, samples=args.samples, iterations=args.iterations, base=256,
            policies=list(POLICIES), round_order=order_log, build_info=build_info,
            binaries={p.name: sha(p) for p in sorted(bins)},
            inputs=[dict(name=p.name, bytes=p.stat().st_size, sha256=sha(p)) for p in inputs],
            source_hash_format="UTF-8 without BOM, LF line endings",
            sources=[dict(file=p.relative_to(repo).as_posix(), sha256=hashlib.sha256(p.read_text(encoding="utf-8-sig").encode()).hexdigest()) for p in sorted(source_paths)])
        (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    finally:
        restore()


if __name__ == "__main__":
    main()
