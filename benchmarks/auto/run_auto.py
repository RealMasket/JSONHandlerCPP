"""Four-policy, rotating-order experiment; trace/profile/timing are separate builds."""
import argparse
import csv
import hashlib
import io
import json
import platform
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"reservation"))
from check_matrix import CATEGORIES,executable,run,validate_memory
from run_matrix import cpu_name,pin,sha
from check_journal import validate

POLICIES=("balanced","speed","low-memory","memory")
def source_hash(path):
    return hashlib.sha256(path.read_text(encoding="utf-8-sig").encode()).hexdigest()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--bin",type=Path,required=True)
    parser.add_argument("--inputs",nargs="+",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--stage",choices=("development","holdout"),required=True)
    parser.add_argument("--freeze",type=Path)
    parser.add_argument("--rounds",type=int,default=4)
    parser.add_argument("--samples",type=int,default=5)
    parser.add_argument("--iterations",type=int,default=3)
    parser.add_argument("--cpu",type=int,default=0)
    args=parser.parse_args()
    assert args.rounds in (4,8) and 1<=args.samples<=101 and 1<=args.iterations<=1000
    repo=Path(__file__).resolve().parents[2]
    if args.stage=="holdout":
        assert args.freeze is not None
        frozen=json.loads(args.freeze.read_text())
        for entry in frozen["policy_sources"]:
            assert source_hash(repo/entry["file"])==entry["sha256"],entry["file"]
        for name,digest in frozen["binaries"].items():
            assert sha(args.bin/name)==digest,name
    args.output.mkdir(parents=True)
    inputs=sorted((p for directory in args.inputs for p in directory.glob("*.json")),key=lambda p:p.name)
    expected={p.name:p.stat().st_size for p in inputs}
    assert len(expected)==len(inputs)==(59 if args.stage=="development" else 16)
    restore=pin(args.cpu)
    try:
        reference=run(executable(args.bin,"AutoInspect-balanced"),*inputs)
        errors=run(executable(args.bin,"AutoInspect-balanced"),"--errors")
        (args.output/"shapes.csv").write_bytes(reference.encode())
        memory={}
        for policy in POLICIES:
            assert run(executable(args.bin,f"AutoInspect-{policy}"),*inputs)==reference,policy
            assert run(executable(args.bin,f"AutoInspect-{policy}"),"--errors")==errors,policy
            if policy in ("speed","memory"):
                run(executable(args.bin,f"AutoVerify-{policy}"),*inputs)
            text=run(executable(args.bin,f"AutoProfile-{policy}"),*inputs)
            memory[policy]=validate_memory(text,expected)
            (args.output/f"{policy}-memory.csv").write_bytes(text.encode())
            for key,row in memory[policy].items():
                assert row["token_count"]==memory["balanced"][key]["token_count"]
                if key[1]=="whole_parse":
                    assert all(row[c]==memory["balanced"][key][c] for c in (*CATEGORIES,"retained_bytes","retained_blocks","dom_nodes","array_capacity"))
            print(f"{args.stage}: {policy} memory and exact checks complete",flush=True)
        invalid=args.output/"invalid"
        invalid.mkdir()
        for name,source in {"lex-error":"["+"0,"*2053+"\n @]","parse-error":"[1,]","utf8-error":'"\xff"'}.items():
            (invalid/(name+".json")).write_bytes(source.encode("latin1"))
        bad=sorted(invalid.glob("*.json"))
        for goal in ("speed","memory"):
            text=run(executable(args.bin,f"AutoTrace-{goal}"),*inputs,*bad)
            rows=validate(text,goal,[p.name for p in (*inputs,*bad)])
            for name in expected:
                last=next(r for r in reversed(rows) if r["dataset"]==name and r["stage"]=="lex_only")
                row=memory[goal][name,"lex_only"]
                assert int(last["moved_tokens"])*int(row["token_size"])==int(row["copied_token_bytes"])
                assert last["actual_capacity"]==row["token_capacity"] and last["tokens"]==row["token_count"]
            (args.output/f"{goal}-decisions.csv").write_bytes(text.encode())
        orders=[]
        for index in range(args.rounds):
            order=POLICIES[index%4:]+POLICIES[:index%4]
            orders.append(order)
            for policy in order:
                text=run(executable(args.bin,f"AutoTime-{policy}"),args.iterations,args.samples,*inputs)
                rows=list(csv.DictReader(io.StringIO(text)))
                assert len(rows)==len(inputs)*2 and all(r["configuration"]=="Release" for r in rows)
                (args.output/f"{policy}-{index+1}.csv").write_bytes(text.encode())
                print(f"{args.stage} round {index+1}/{args.rounds}: {policy}",flush=True)
        source_paths=list(Path(__file__).resolve().parent.glob("*.hpp"))+list(Path(__file__).resolve().parent.glob("*.cpp"))
        source_paths+=list(Path(__file__).resolve().parent.glob("*.py"))+list(Path(__file__).resolve().parent.glob("*.json"))
        source_paths+=[repo/"benchmarks/auto/CMakeLists.txt",repo/"benchmarks/CMakeLists.txt",repo/"CMakeLists.txt",
            repo/"benchmarks/ParserProfile.cpp",repo/"benchmarks/MemoryTracker.cpp",repo/"benchmarks/MemoryTracker.hpp",
            repo/"benchmarks/reservation/Data.hpp",repo/"benchmarks/reservation/Timing.cpp",repo/"benchmarks/reservation/Inspect.cpp",
            repo/"benchmarks/BenchmarkUtils.cpp",repo/"benchmarks/BenchmarkUtils.hpp"]
        source_paths+=list((repo/"parser").glob("*.cpp"))+list((repo/"parser").glob("*.hpp"))
        suffix=".exe" if sys.platform=="win32" else ""
        bins=[p for p in args.bin.iterdir() if p.is_file() and p.name.startswith(("AutoTime-","AutoProfile-","AutoTrace-","AutoVerify-","AutoInspect-")) and p.suffix==suffix]
        meta=dict(base_revision="b94639e6a70ae467db7ae095daca1bc3a4d231e3",stage=args.stage,cpu=cpu_name(),os=platform.platform(),logical_cpu=args.cpu,
            rounds=args.rounds,samples=args.samples,iterations=args.iterations,policies=POLICIES,round_order=orders,
            build_info=(args.bin/"build-info-Release.txt").read_text(),
            freeze_sha256=sha(args.freeze) if args.freeze else None,
            inputs=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in inputs],
            binaries={p.name:sha(p) for p in sorted(bins)},source_hash_format="UTF-8 without BOM, LF line endings",
            sources=[dict(file=p.relative_to(repo).as_posix(),sha256=source_hash(p)) for p in sorted(set(source_paths))])
        (args.output/"metadata.json").write_bytes((json.dumps(meta,indent=2)+"\n").encode())
    finally:
        restore()
if __name__=="__main__":
    main()
