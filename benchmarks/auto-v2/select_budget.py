"""Apply pre-recorded exploratory selection; avoid shadowing Python's select module."""
import csv,hashlib,json,math,statistics,sys
from datetime import datetime,timezone
from pathlib import Path
root,destination=map(Path,sys.argv[1:]);assert not destination.exists()
plan_path=Path(__file__).with_name('plan.json');plan=json.loads(plan_path.read_text())
scores={}
for label,config in plan['candidates'].items():
    directory=root/label;meta=json.loads((directory/'metadata.json').read_text());assert len(meta['inputs'])==75
    times={p:{r['dataset']:float(r['median_us']) for r in csv.DictReader((directory/f'{p}-1.csv').open()) if r['operation']=='parse'} for p in ('balanced','low-memory','speed','memory')}
    peaks={p:{r['dataset']:int(r['peak_live_bytes']) for r in csv.DictReader((directory/f'{p}-memory.csv').open()) if r['stage']=='whole_parse'} for p in times}
    names=list(times['balanced']);non_tiny=[n for n in names if times['balanced'][n]>=1000];assert non_tiny
    for goal in ('speed','memory'):
        worst=max(peaks[goal][n]/min(peaks['balanced'][n],peaks['low-memory'][n]) for n in non_tiny)
        worst_balanced=max(peaks[goal][n]/peaks['balanced'][n] for n in non_tiny)
        eligible=worst<=plan['joint_targets'][goal]['max_peak_oracle']
        if goal=='speed':eligible &= worst_balanced<=plan['joint_targets']['speed']['max_peak_balanced']
        gm=math.exp(statistics.mean(math.log(times[goal][n]/times['balanced'][n]) for n in names))
        scores[label,goal]=dict(memory_eligible=eligible,geomean_time_balanced=gm,worst_peak_oracle=worst,worst_peak_balanced=worst_balanced,budget_bytes=config[goal+'_bytes'])
chosen={}
for goal in ('speed','memory'):
    eligible=[label for label in plan['candidates'] if scores[label,goal]['memory_eligible']]
    if eligible:
        best=min(scores[label,goal]['geomean_time_balanced'] for label in eligible)
        tied=[label for label in eligible if scores[label,goal]['geomean_time_balanced']<=best*1.10]
        label=min(tied,key=lambda l:scores[l,goal]['worst_peak_oracle'])
    else:label=min(plan['candidates'],key=lambda l:scores[l,goal]['worst_peak_oracle'])
    chosen[goal]=dict(candidate=label,**scores[label,goal])
out=dict(frozen_utc=datetime.now(timezone.utc).isoformat(),chosen=chosen,scores={f'{l}/{g}':s for (l,g),s in scores.items()},
    plan_sha256=hashlib.sha256(plan_path.read_bytes()).hexdigest(),calibration_metadata={l:hashlib.sha256((root/l/'metadata.json').read_bytes()).hexdigest() for l in plan['candidates']},
    note='Single-round calibration is exploratory; evaluate joint CPU and memory targets on repeated final measurements. No holdout results used.')
destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes((json.dumps(out,indent=2)+'\n').encode());print(json.dumps(out,indent=2))
