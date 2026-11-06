"""Extract reproducible root-cause evidence from the frozen v1 observations."""
import csv,json,sys
from pathlib import Path
source,destination=map(Path,sys.argv[1:]);findings=[]
for goal,name in (('speed','held-tail-records-4099.json'),('memory','dense-zeros-32768.json')):
    journal=[r for r in csv.DictReader((source/f'{goal}-decisions.csv').open()) if r['dataset']==name and r['stage']=='lex_only']
    expansions=[r for r in journal if r['reason'] not in ('initial','finished','aborted')]
    last=expansions[-1];final=journal[-1]
    memory=next(r for r in csv.DictReader((source/f'{goal}-memory.csv').open()) if r['dataset']==name and r['stage']=='whole_parse')
    size=int(memory['token_size']);old=int(last['old_capacity']);request=int(last['request'])
    findings.append(dict(goal=goal,dataset=name,last_growth=last,final_tokens=int(final['tokens']),final_capacity=int(final['actual_capacity']),
        requested_peak_bytes=int(memory['peak_live_bytes']),input_bytes=int(memory['input_bytes']),
        logical_token_overlap_bytes=(old+request)*size,
        unused_final_token_bytes=(int(final['actual_capacity'])-int(final['tokens']))*size,
        inferred_cause='Dense prefix extrapolated across unseen whitespace tail; relative ceiling still allowed a huge reserve.' if goal=='speed'
            else 'Forecast 65538 was clipped to 65534; next ordinary floor 81917 forced a late overlapping allocation.'))
out=dict(source_revision='5689e9cf1cd5458741523faa7b90996e05520100',findings=findings,
    pressure_observation='Speed emitted zero copy_pressure events. For full initial 256 and 1.5x floor, 2*c-next moved remains positive over the representable growth count; >2*n is not a useful speed trigger.')
destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes((json.dumps(out,indent=2)+'\n').encode());print('Extracted two v1 failure histories and logical buffer overlap')
