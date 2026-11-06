import csv
import io

def validate(text,goal,names):
    rows=list(csv.DictReader(io.StringIO(text))); groups={}
    for r in rows:groups.setdefault((r['dataset'],r['stage']),[]).append(r)
    assert set(groups)=={(n,s) for n in names for s in ('parse','lex_only')}
    for events in groups.values():
        assert events[0]['reason']=='initial' and events[-1]['reason'] in ('finished','aborted')
        assert len(events)<=512
        moved=0;last_pos=last_count=0;tail_seen=False
        for index,r in enumerate(events):
            assert int(r['event'])==index
            pos,n=int(r['position']),int(r['tokens'])
            assert pos>=last_pos and n>=last_count
            last_pos,last_count=pos,n
            reason=r['reason'];request=int(r['request'])
            if reason not in ('initial','finished','aborted','allocation_failed'):
                assert not tail_seen,'Growth after guaranteed final token capacity'
                assert n==int(r['old_capacity']) and request>n and int(r['actual_capacity'])>=request
                assert request<=int(r['effective_upper'])
                if r['floor_override']=='1':
                    assert int(r['lower'])>int(r['byte_upper']) and request<=int(r['lower'])
                else:assert request<=int(r['byte_upper'])
                if r['tail_applied']=='1':
                    assert request==int(r['tail_bound']);tail_seen=True
                else:assert request>=int(r['lower'])
                assert int(r['spare_bytes'])==(request-n-1)*int(r['token_size'])
                moved+=n
            assert int(r['moved_tokens'])==moved
        if events[-1]['reason']=='finished':
            assert moved<=(5 if goal=='speed' else 8)*int(events[-1]['tokens'])+4096
    return rows
