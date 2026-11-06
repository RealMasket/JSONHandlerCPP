"""Repeated v1/v2/fixed-mode comparison using frozen, selected executables."""
import argparse,csv,hashlib,io,json,platform,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reservation'))
from check_matrix import executable,run,validate_memory,CATEGORIES
from run_matrix import pin,cpu_name,sha
from check_journal import validate
def source_hash(p):return hashlib.sha256(p.read_text(encoding='utf-8-sig').encode()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--bin',type=Path,required=True);p.add_argument('--v1-bin',type=Path,required=True)
    p.add_argument('--inputs',nargs='+',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--selection',type=Path,required=True);p.add_argument('--freeze',type=Path);p.add_argument('--stage',choices=('known','holdout'),required=True)
    args=p.parse_args();repo=Path(__file__).resolve().parents[2]
    selection=json.loads(args.selection.read_text());programs={
        'balanced':(args.bin,'Auto','balanced'),'v1-speed':(args.v1_bin,'Auto','speed'),'v2-speed':(args.bin,'AutoV2','speed'),
        'low-memory':(args.bin,'Auto','low-memory'),'v1-memory':(args.v1_bin,'Auto','memory'),'v2-memory':(args.bin,'AutoV2','memory')}
    binaries={label:{stem:sha(executable(directory,f'{prefix}{stem}-{goal}')) for stem in ('Time','Profile','Inspect')}
              for label,(directory,prefix,goal) in programs.items()}
    for goal in ('speed','memory'):
        binaries['v2-'+goal]['Trace']=sha(executable(args.bin,'AutoV2Trace-'+goal))
        binaries['v2-'+goal]['Verify']=sha(executable(args.bin,'AutoV2Verify-'+goal))
    if args.stage=='holdout':
        assert args.freeze
        frozen=json.loads(args.freeze.read_text());assert frozen['binaries']==binaries and frozen['selection_sha256']==sha(args.selection)
        for entry in frozen['policy_sources']:assert source_hash(repo/entry['file'])==entry['sha256']
    args.output.mkdir(parents=True)
    inputs=sorted((f for d in args.inputs for f in d.glob('*.json')),key=lambda f:f.name)
    expected={f.name:f.stat().st_size for f in inputs};assert len(inputs)==len(expected)==(75 if args.stage=='known' else 16)
    restore=pin(0)
    try:
        reference=run(executable(args.bin,'AutoInspect-balanced'),*inputs)
        (args.output/'shapes.csv').write_bytes(reference.encode());memory={}
        for label,(directory,prefix,goal) in programs.items():
            assert run(executable(directory,f'{prefix}Inspect-{goal}'),*inputs)==reference
            if prefix=='AutoV2':run(executable(directory,f'AutoV2Verify-{goal}'),*inputs)
            text=run(executable(directory,f'{prefix}Profile-{goal}'),*inputs);memory[label]=validate_memory(text,expected)
            (args.output/f'{label}-memory.csv').write_bytes(text.encode())
            for key,r in memory[label].items():
                assert r['token_count']==memory['balanced'][key]['token_count']
                if key[1]=='whole_parse':assert all(r[c]==memory['balanced'][key][c] for c in (*CATEGORIES,'retained_bytes','dom_nodes','array_capacity'))
            print(args.stage,label,'profile/exact checks complete',flush=True)
        for goal in ('speed','memory'):
            text=run(executable(args.bin,f'AutoV2Trace-{goal}'),*inputs);rows=validate(text,goal,expected)
            for n in expected:
                row=memory['v2-'+goal][n,'lex_only'];last=next(r for r in reversed(rows) if r['dataset']==n and r['stage']=='lex_only')
                assert int(last['moved_tokens'])*int(row['token_size'])==int(row['copied_token_bytes'])
                assert last['tokens']==row['token_count'] and last['actual_capacity']==row['token_capacity']
                assert int(last['budget_bytes'])==selection['chosen'][goal]['budget_bytes']
            (args.output/f'{goal}-decisions.csv').write_bytes(text.encode())
        policies=list(programs);orders=[]
        for index in range(6):
            order=policies[index:]+policies[:index];orders.append(order)
            for label in order:
                directory,prefix,goal=programs[label]
                text=run(executable(directory,f'{prefix}Time-{goal}'),3,5,*inputs)
                rows=list(csv.DictReader(io.StringIO(text)));assert len(rows)==len(inputs)*2 and all(r['configuration']=='Release' for r in rows)
                (args.output/f'{label}-{index+1}.csv').write_bytes(text.encode());print(f'{args.stage} {index+1}/6: {label}',flush=True)
        sources=[repo/'benchmarks/auto/AutoReservation.hpp',repo/'benchmarks/auto/generate_lexer.py']
        sources += [f for f in Path(__file__).resolve().parent.iterdir() if f.suffix in ('.hpp','.cpp','.py','.json','.txt')]
        meta=dict(stage=args.stage,calibration_revision='77c50bf9fbf9c254e7247e3094d7c34b1f2dd79b',v1_revision='5689e9cf1cd5458741523faa7b90996e05520100',
            cpu=cpu_name(),os=platform.platform(),logical_cpu=0,rounds=6,samples=5,iterations=3,policies=policies,round_order=orders,
            selection_sha256=sha(args.selection),freeze_sha256=sha(args.freeze) if args.freeze else None,binaries=binaries,
            inputs=[dict(name=f.name,bytes=f.stat().st_size,sha256=sha(f)) for f in inputs],
            sources=[dict(file=f.relative_to(repo).as_posix(),sha256=source_hash(f)) for f in sorted(set(sources))],
            source_hash_format='UTF-8 without BOM, LF line endings',configuration='MSVC 19.51 Release, static, /O2 /Ob2 /DNDEBUG; binary identities are frozen calibration artifacts')
        (args.output/'metadata.json').write_bytes((json.dumps(meta,indent=2)+'\n').encode())
    finally:restore()
if __name__=='__main__':main()
