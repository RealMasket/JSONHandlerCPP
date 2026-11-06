"""Small candidate sweep on known data only. Each run writes a fresh snapshot."""
import argparse,csv,hashlib,io,json,platform,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reservation'))
from check_matrix import executable,run,validate_memory,CATEGORIES
from run_matrix import pin,cpu_name,sha
from check_journal import validate

parser=argparse.ArgumentParser()
parser.add_argument('--bin',type=Path,required=True)
parser.add_argument('--inputs',nargs='+',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--candidate',required=True)
args=parser.parse_args();args.output.mkdir(parents=True)
inputs=sorted((p for d in args.inputs for p in d.glob('*.json')),key=lambda p:p.name)
expected={p.name:p.stat().st_size for p in inputs};assert len(expected)==len(inputs)==75
programs={'balanced':('AutoTime-balanced','AutoProfile-balanced','AutoInspect-balanced'),
    'speed':('AutoV2Time-speed','AutoV2Profile-speed','AutoV2Inspect-speed'),
    'low-memory':('AutoTime-low-memory','AutoProfile-low-memory','AutoInspect-low-memory'),
    'memory':('AutoV2Time-memory','AutoV2Profile-memory','AutoV2Inspect-memory')}
restore=pin(0)
try:
    reference=run(executable(args.bin,'AutoInspect-balanced'),*inputs)
    memories={}
    for policy,(timing,profile,inspect) in programs.items():
        assert run(executable(args.bin,inspect),*inputs)==reference
        text=run(executable(args.bin,profile),*inputs)
        memories[policy]=validate_memory(text,expected)
        (args.output/f'{policy}-memory.csv').write_bytes(text.encode())
        if policy in ('speed','memory'):
            run(executable(args.bin,f'AutoV2Verify-{policy}'),*inputs)
            journal=run(executable(args.bin,f'AutoV2Trace-{policy}'),*inputs)
            rows=validate(journal,policy,expected)
            for n in expected:
                last=next(r for r in reversed(rows) if r['dataset']==n and r['stage']=='lex_only')
                row=memories[policy][n,'lex_only']
                assert int(last['moved_tokens'])*int(row['token_size'])==int(row['copied_token_bytes'])
                assert last['actual_capacity']==row['token_capacity']
            (args.output/f'{policy}-decisions.csv').write_bytes(journal.encode())
        text=run(executable(args.bin,timing),2,3,*inputs)
        rows=list(csv.DictReader(io.StringIO(text)));assert len(rows)==150
        (args.output/f'{policy}-1.csv').write_bytes(text.encode())
        print(args.candidate,policy,'complete',flush=True)
    suffix='.exe' if sys.platform=='win32' else ''
    bins=[p for p in args.bin.iterdir() if p.is_file() and p.suffix==suffix]
    repo=Path(__file__).resolve().parents[2]
    source_paths=[p for p in Path(__file__).resolve().parent.iterdir() if p.suffix in ('.hpp','.cpp','.py','.json','.txt')]
    meta=dict(candidate=args.candidate,cpu=cpu_name(),os=platform.platform(),logical_cpu=0,rounds=1,samples=3,iterations=2,
        build_info=(args.bin/'build-info-Release.txt').read_text(),inputs=[dict(name=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in inputs],
        binaries={p.name:sha(p) for p in sorted(bins)},source_hash_format='UTF-8 without BOM, LF line endings',
        sources=[dict(file=p.relative_to(repo).as_posix(),sha256=hashlib.sha256(p.read_text(encoding='utf-8-sig').encode()).hexdigest()) for p in sorted(source_paths)])
    (args.output/'metadata.json').write_bytes((json.dumps(meta,indent=2)+'\n').encode())
finally:restore()
