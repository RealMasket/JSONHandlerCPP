"""Small-fixture exact comparisons and budget/tail proof checks for CI."""
import argparse,subprocess,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reservation'))
from check_matrix import executable,run,validate_memory
from check_journal import validate
p=argparse.ArgumentParser();p.add_argument('--bin',type=Path,required=True);p.add_argument('--fixed-bin',type=Path,required=True)
p.add_argument('--reference-bin',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--profiles',action='store_true');a=p.parse_args()
a.output.mkdir(parents=True,exist_ok=True);data=a.output/'data'
run(executable(a.reference_bin,'ParseBenchmark'),'--generate',data);run(executable(a.reference_bin,'ParseBenchmark'),'--generate-profile',data,4)
benches=Path(__file__).resolve().parents[1]
for script,options in ((benches/'reservation/generate_inputs.py',['--base','4']),(benches/'reservation/generate_holdout.py',['--small']),
    (benches/'auto/generate_holdout.py',['--small']),(Path(__file__).with_name('generate_holdout.py'),['--small'])):
    subprocess.run([sys.executable,str(script),str(data),*options],check=True)
inputs=sorted(data.glob('*.json'));assert len(inputs)==91
expected={f.name:f.stat().st_size for f in inputs};reference=run(executable(a.fixed_bin,'AutoInspect-balanced'),*inputs)
errors=run(executable(a.fixed_bin,'AutoInspect-balanced'),'--errors')
bad=a.output/'invalid';bad.mkdir(exist_ok=True)
for name,source in {'lex-error':'['+'0,'*2053+'\n @]','parse-error':'[1,]','utf8-error':'"\xff"'}.items():
    (bad/(name+'.json')).write_bytes(source.encode('latin1'))
invalid=sorted(bad.glob('*.json'))
for goal in ('speed','memory'):
    run(executable(a.bin,'AutoV2Verify-'+goal),*inputs)
    assert run(executable(a.bin,'AutoV2Inspect-'+goal),*inputs)==reference
    assert run(executable(a.bin,'AutoV2Inspect-'+goal),'--errors')==errors
    text=run(executable(a.bin,'AutoV2Trace-'+goal),*inputs,*invalid)
    (a.output/(goal+'-decisions.csv')).write_bytes(text.encode())
    validate(text,goal,[f.name for f in (*inputs,*invalid)])
    if a.profiles:validate_memory(run(executable(a.bin,'AutoV2Profile-'+goal),*inputs),expected)
print('91 V2 structures plus invalid traces passed exact, byte-budget, tail and relocation checks')
