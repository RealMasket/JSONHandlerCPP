"""Audit calibration/freeze/final stages and evaluate unchanged joint targets."""
import csv,hashlib,json,math,statistics,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'reservation'))
from check_matrix import validate_memory
from check_journal import validate
root,report=map(Path,sys.argv[1:]);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
selection=json.loads((root/'selection.json').read_text());freeze=json.loads((root/'freeze.json').read_text())
assert freeze['selection_sha256']==sha(root/'selection.json') and freeze['known_metadata_sha256']==sha(root/'known/metadata.json')
for label,h in selection['calibration_metadata'].items():assert sha(root/'calibration'/label/'metadata.json')==h
data={}
for stage,count in (('known',75),('holdout',16)):
    d=root/stage;m=json.loads((d/'metadata.json').read_text());assert m['stage']==stage and m['rounds']==6
    assert m['selection_sha256']==freeze['selection_sha256'] and m['binaries']==freeze['binaries']
    if stage=='holdout':assert m['freeze_sha256']==sha(root/'freeze.json')
    inputs={i['name']:i['bytes'] for i in m['inputs']};assert len(inputs)==len(m['inputs'])==count
    memory={p:validate_memory((d/f'{p}-memory.csv').read_text(),inputs) for p in m['policies']};times={}
    for i in range(6):
        assert m['round_order'][i]==m['policies'][i:]+m['policies'][:i]
        for p in m['policies']:
            rows=list(csv.DictReader((d/f'{p}-{i+1}.csv').open(newline='')));seen=set()
            for row in rows:
                key=row['dataset'],row['operation'];assert key not in seen;seen.add(key)
                assert row['configuration']=='Release' and int(row['bytes'])==inputs[key[0]]
                assert int(row['samples'])==m['samples'] and int(row['iterations'])==m['iterations']
                assert 0<float(row['min_us'])<=float(row['median_us'])<=float(row['max_us'])
                times.setdefault((p,*key),[]).append(float(row['median_us']))
            assert seen=={(n,o) for n in inputs for o in ('parse','lex_only')}
    journals={}
    for goal in ('speed','memory'):
        rows=validate((d/f'{goal}-decisions.csv').read_text(),goal,inputs);journals[goal]=rows
        for n in inputs:
            last=next(r for r in reversed(rows) if r['dataset']==n and r['stage']=='lex_only');mem=memory['v2-'+goal][n,'lex_only']
            assert int(last['moved_tokens'])*int(mem['token_size'])==int(mem['copied_token_bytes'])
            assert last['tokens']==mem['token_count'] and last['actual_capacity']==mem['token_capacity']
            assert int(last['budget_bytes'])==selection['chosen'][goal]['budget_bytes']
    data[stage]=dict(meta=m,inputs=inputs,memory=memory,times=times,journals=journals)
def t(s,p,n):return statistics.median(data[s]['times'][p,n,'parse'])
def peak(s,p,n):return int(data[s]['memory'][p][n,'whole_parse']['peak_live_bytes'])
def ot(s,n):return min(t(s,'balanced',n),t(s,'low-memory',n))
def op(s,n):return min(peak(s,'balanced',n),peak(s,'low-memory',n))
def gm(v):return math.exp(statistics.mean(math.log(x) for x in v))

lines=['# Auto v2: byte budgets and joint resource targets','','## Root causes and changes','',
    'This remains a private experiment. The production parser, public presets and defaults are unchanged.',
    'The [v1 histories](auto-v2-msvc1951/v1-root-causes.json) explain two distinct failures:',
    '- Speed expanded 86,434 stored tokens to 1,383,202 slots; final count was only 90,180. Dense prefix stability did not describe the whitespace suffix.',
    '- Memory clipped a forecast of 65,538 to 65,534, then applied an 81,917-slot geometric floor near EOF. Old/new buffer overlap dominated peak memory.',
    '- V1 speed copy-pressure threshold >2n was inactive under its 1.5× floor over the representable range. V2 uses >1.5n; memory retains >3n.',
    '', 'V2 applies a byte ceiling to speculative slots beyond the already scanned pending token. Budgets are rounded down by sizeof(Token).',
    'If that ceiling is below the minimum geometric growth, the floor takes precedence and the actual exception is logged.',
    'This avoids additive tiny expansions and preserves amortized-linear token relocation. It is a soft reserve ceiling, not a total-memory or latency guarantee.',
    '', 'A second change uses a proven fact: each future non-EOF token consumes at least one remaining source byte.',
    'pending tokens + remaining bytes + required EOF therefore bounds the final count. When this is below the proposed reserve,',
    'v2 can reserve that bound instead of the geometric floor. The allocation fits all remaining tokens, so no further growth is needed.',
    'This performs no scan or restart. Saturating/checked arithmetic prevents overflow; if the bound cannot fit max_size it is not used.',
    '', '## Calibration and freeze','',
    'Three private candidate pairs were compared on 75 known inputs (all v1 development and evaluation fixtures).',
    'Calibration uses one round × 3 samples × 2 operations and is exploratory, not final performance evidence.',
    'The [pre-recorded plan](../auto-v2/plan.json) filters candidates by memory targets, treats times within 10% of fastest as ties,',
    'and breaks ties by the lowest worst peak/oracle ratio. The offline oracle is the better production fixed mode per document.',
    '', '| Candidate / goal | Budget MiB | Geomean time/Balanced | Worst peak/oracle | Memory eligible |',
    '| --- | ---: | ---: | ---: | --- |']
for key,score in selection['scores'].items():lines.append(f"| {key} | {score['budget_bytes']/1048576:.2f} | {score['geomean_time_balanced']:.3f} | {score['worst_peak_oracle']:.3f} | {score['memory_eligible']} |")
lines += ['',f"Selected speed budget: {selection['chosen']['speed']['budget_bytes']/1048576:g} MiB; memory budget: {selection['chosen']['memory']['budget_bytes']/1048576:g} MiB.",
    'Sources and original candidate observations are committed as `77c50bf9fbf9c254e7247e3094d7c34b1f2dd79b`.',
    'Each goal uses its selected frozen calibration executable; the unused other-goal macro in that artifact does not select behavior.',
    f"After repeated known-set measurement, policy sources, budgets and all comparison executables were frozen at `{freeze['frozen_utc']}`.",
    'Only then were 16 new cases generated with seed 20261108 and counts 149/3079: duplicate keys, UTF-8, chunked escaping, ragged numbers,',
    'internal gaps, space-filled string values and front/trailing padding around nested cells. No parameters were retuned using held-out observations.',
    '', '## Measurement and unchanged acceptance rules','',
    'Both stages compare six executables: production Balanced/LowMemory, frozen v1 speed/memory and selected v2 speed/memory.',
    'Six cyclic rounds put every policy in every position once; each round has 5 samples × 3 operations, plus five warm-ups.',
    'Tables use medians of round medians. Loading, semantic checks and shape inspection occur outside timing; parse includes result destruction.',
    'Timing has no journal hooks. Fixed-size stack journals and successful C++ new interception use separate builds.',
    'Captured peak is simultaneous requested heap bytes, including overlapping token buffers. Caller input, stack/root Json, allocator rounding/metadata,',
    'direct malloc/CRT internals and profiler bookkeeping are excluded. This is not RSS. Logical relocation bytes are old live tokens × sizeof(Token).',
    'Journal overlap_bytes is the logical old+requested token-buffer size at an expansion; it excludes source/DOM/allocator overhead.',
    '', 'Joint empirical targets on non-tiny inputs (Balanced ≥1 ms) were fixed before calibration:',
    '- Speed: worst time/oracle ≤1.5, peak/oracle ≤2, peak/Balanced ≤1.5.',
    '- Memory: worst time/Balanced ≤2 and peak/oracle ≤1.5.',
    'All inputs remain in the full tables. These are local evaluation thresholds, not runtime guarantees or CI speed assertions.',
    '', '| Split / mode | Geomean time/Balanced (all) | Worst time/oracle | Worst time/Balanced | Worst peak/oracle | Worst peak/Balanced | Joint target |',
    '| --- | ---: | ---: | ---: | ---: | ---: | --- |']
gates={}
for stage in data:
    names=list(data[stage]['inputs']);non=[n for n in names if t(stage,'balanced',n)>=1000];assert non
    for mode in ('v1-speed','v2-speed','v1-memory','v2-memory'):
        wt=max(t(stage,mode,n)/ot(stage,n) for n in non);wb=max(t(stage,mode,n)/t(stage,'balanced',n) for n in non)
        wp=max(peak(stage,mode,n)/op(stage,n) for n in non);pb=max(peak(stage,mode,n)/peak(stage,'balanced',n) for n in non)
        passed=wt<=1.5 and wp<=2 and pb<=1.5 if mode.endswith('speed') else wb<=2 and wp<=1.5
        gates[stage,mode]=passed
        lines.append(f"| {stage}/{mode} | {gm([t(stage,mode,n)/t(stage,'balanced',n) for n in names]):.3f} | {wt:.3f} | {wb:.3f} | {wp:.3f} | {pb:.3f} | {'PASS' if passed else 'FAIL'} |")
lines += ['', 'Original failure cases remeasured in the same run:','',
    '| Input | Balanced peak MiB | LowMemory peak MiB | V1 speed | V2 speed | V1 memory | V2 memory |',
    '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
for n in ('held-tail-records-4099.json','dense-zeros-32768.json'):
    lines.append('| '+n+' | '+' | '.join(f'{peak("known",p,n)/1048576:.3f}' for p in ('balanced','low-memory','v1-speed','v2-speed','v1-memory','v2-memory'))+' |')
lines += ['', '## Full parse-time results','', 'Milliseconds; every known/evaluation input is retained.','',
    '| Split / input | Balanced | LowMemory | V1 speed | V2 speed | V1 memory | V2 memory |',
    '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
for s in data:
    for n in data[s]['inputs']:lines.append('| '+s+' / '+n+' | '+' | '.join(f'{t(s,p,n)/1000:.4f}' for p in ('balanced','low-memory','v1-speed','v2-speed','v1-memory','v2-memory'))+' |')
lines += ['', '## Full captured-peak results','', 'MiB; retained DOM accounting is identical for every mode.','',
    '| Split / input | Balanced | LowMemory | V1 speed | V2 speed | V1 memory | V2 memory |',
    '| --- | ---: | ---: | ---: | ---: | ---: | ---: |']
for s in data:
    for n in data[s]['inputs']:lines.append('| '+s+' / '+n+' | '+' | '.join(f'{peak(s,p,n)/1048576:.3f}' for p in ('balanced','low-memory','v1-speed','v2-speed','v1-memory','v2-memory'))+' |')
lines += ['', '## Journal evidence','',
    '| Split / goal | Growths | Byte-limited growths | Actual floor overrides | Proven final-capacity reservations | Copy-pressure states |',
    '| --- | ---: | ---: | ---: | ---: | ---: |']
for s in data:
    for goal,rows in data[s]['journals'].items():
        growth=[r for r in rows if r['stage']=='lex_only' and r['reason'] not in ('initial','finished','aborted','allocation_failed')]
        lines.append(f"| {s}/{goal} | {len(growth)} | {sum(r['byte_limited']=='1' for r in growth)} | {sum(r['floor_override']=='1' for r in growth)} | {sum(r['tail_applied']=='1' for r in growth)} | {sum(r['state']=='copy_pressure' for r in growth)} |")
lines += ['', 'Actual floor overrides mean the configured byte ceiling was exceeded to preserve geometric progress. A final-capacity reservation can instead',
    'bypass the floor safely, because its remaining-byte proof excludes any subsequent growth. Both exceptions are explicit in the journal.',
    'The configured number is not a hard total-memory limit; a smaller value can increase copying and old/new overlap.',
    '', '## Checks, limits and reproduction','',
    'Exact comparators verify every token field, canonical numeric/zero representation, acceptance and full diagnostic class/location/message against both production parsers.',
    'Journal validation requires geometric progress or a proved final bound, justified byte-budget exceptions, zero dropped records, no growth after the final bound,',
    'and linear relocation-work bounds. Final journal movement/capacity/count equals the independent heap profile for every input.',
    'All tracked memory windows return live bytes and blocks to zero; profile categories partition retained bytes. Saturating math is checked near size_t/max_size limits.',
    'Windows/Linux smoke, sanitizers, corpus mutations and separate v2 coverage fuzzers exercise the same controller. CI does not enforce noisy timing targets.',
    '', 'Passed targets are evidence for this machine and these inputs; failed rows remain failures. Both priorities remain private experimental tools pending wider workloads.',
    'CPU frequency, background load and compiler/code-layout variation limit small time differences. Source/binary/input hashes and per-round sample spread are preserved.',
    '', 'Raw [calibration, selection, known/evaluation results and journals](auto-v2-msvc1951/) are reproducible through [the v2 instructions](../README.md#auto-v2-byte-budgets-and-joint-targets).',
    'Render with `python benchmarks/auto-v2/summarize.py benchmarks/results/auto-v2-msvc1951 REPORT.md`.','']
report.parent.mkdir(parents=True,exist_ok=True);report.write_bytes(('\n'.join(lines)).encode())
print(json.dumps({f'{s}/{m}':v for (s,m),v in gates.items()},indent=2))
