"""Portable small-fixture Auto checks, optionally with instrumented heap profiles."""
import argparse
import subprocess
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"reservation"))
from check_matrix import executable,run,validate_memory
from check_journal import validate

parser=argparse.ArgumentParser()
parser.add_argument("--bin",type=Path,required=True)
parser.add_argument("--reference-bin",type=Path,required=True)
parser.add_argument("--output",type=Path,required=True)
parser.add_argument("--profiles",action="store_true")
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
data=args.output/"data"
run(executable(args.reference_bin,"ParseBenchmark"),"--generate",data)
run(executable(args.reference_bin,"ParseBenchmark"),"--generate-profile",data,4)
reservation=Path(__file__).resolve().parents[1]/"reservation"
for script,extra in ((reservation/"generate_inputs.py",["--base","4"]),(reservation/"generate_holdout.py",["--small"]),(Path(__file__).with_name("generate_holdout.py"),["--small"])):
    subprocess.run([sys.executable,str(script),str(data),*extra],check=True)
inputs=sorted(data.glob("*.json"))
assert len(inputs)==75
expected={p.name:p.stat().st_size for p in inputs}
reference=run(executable(args.bin,"AutoInspect-balanced"),*inputs)
errors=run(executable(args.bin,"AutoInspect-balanced"),"--errors")
bad=args.output/"invalid"
bad.mkdir(exist_ok=True)
for name,source in {"lex-error":"["+"0,"*2053+"\n @]","parse-error":"[1,]","utf8-error":'"\xff"'}.items():
    (bad/(name+".json")).write_bytes(source.encode("latin1"))
invalid=sorted(bad.glob("*.json"))
for goal in ("speed","memory"):
    run(executable(args.bin,f"AutoVerify-{goal}"),*inputs)
    assert run(executable(args.bin,f"AutoInspect-{goal}"),*inputs)==reference
    assert run(executable(args.bin,f"AutoInspect-{goal}"),"--errors")==errors
    text=run(executable(args.bin,f"AutoTrace-{goal}"),*inputs,*invalid)
    validate(text,goal,[p.name for p in (*inputs,*invalid)])
    (args.output/f"{goal}-decisions.csv").write_bytes(text.encode())
    if args.profiles:
        memory=run(executable(args.bin,f"AutoProfile-{goal}"),*inputs)
        validate_memory(memory,expected)
    timing=run(executable(args.bin,f"AutoTime-{goal}"),1,1,*inputs)
    assert timing.count("\n")==151
print("75 Auto fixtures and invalid traces passed exact comparison, bounds and relocation checks")
