"""Validate decision structure, requested-capacity bounds and relocation counts."""
import csv
import io

def validate(text,goal,expected_names):
    rows=list(csv.DictReader(io.StringIO(text)))
    groups={}
    for row in rows:
        groups.setdefault((row["dataset"],row["stage"]),[]).append(row)
    assert set(groups)=={(n,s) for n in expected_names for s in ("parse","lex_only")}
    for (name,stage),events in groups.items():
        assert events[0]["reason"]=="initial"
        assert events[-1]["reason"] in ("finished","aborted")
        assert len(events)<=512
        moved=0
        previous_position=previous_tokens=0
        for index,event in enumerate(events):
            assert int(event["event"])==index
            position,tokens=int(event["position"]),int(event["tokens"])
            old,request,actual=(int(event[k]) for k in ("old_capacity","request","actual_capacity"))
            assert position>=previous_position and tokens>=previous_tokens
            previous_position,previous_tokens=position,tokens
            reason=event["reason"]
            if reason not in ("initial","finished","aborted","allocation_failed"):
                assert tokens==old and request>old and actual>=request
                assert request<=int(event["upper"]) or reason=="end_of_input"
                if reason!="end_of_input":
                    assert request>=int(event["lower"])
                    assert request<=(16 if goal=="speed" else 2)*tokens+258
                else:
                    assert request in (tokens+1,tokens+2)
                moved+=tokens
            assert int(event["moved_tokens"])==moved
        if events[-1]["reason"]=="finished":
            final=int(events[-1]["tokens"])
            assert moved<=(5 if goal=="speed" else 8)*final+4096
    return rows
