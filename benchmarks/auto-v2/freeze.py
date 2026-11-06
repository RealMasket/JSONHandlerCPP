import hashlib,json,sys
from datetime import datetime,timezone
from pathlib import Path
known,selection,destination=map(Path,sys.argv[1:]);assert not destination.exists()
meta=json.loads((known/'metadata.json').read_text());assert meta['stage']=='known' and len(meta['inputs'])==75
repo=Path(__file__).resolve().parents[2]
paths=[Path(__file__).with_name(n).resolve() for n in ('AutoReservationV2.hpp','generate_lexer.py','generate_holdout.py','plan.json')]
paths += [repo/'benchmarks/auto/AutoReservation.hpp',repo/'benchmarks/auto/generate_lexer.py']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
out=dict(frozen_utc=datetime.now(timezone.utc).isoformat(),selection_sha256=sha(selection),known_metadata_sha256=sha(known/'metadata.json'),binaries=meta['binaries'],
    policy_sources=[dict(file=f.relative_to(repo).as_posix(),sha256=hashlib.sha256(f.read_text(encoding='utf-8-sig').encode()).hexdigest()) for f in paths])
destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes((json.dumps(out,indent=2)+'\n').encode());print('Frozen selected v2 controllers and all comparison binaries')
