"""Freeze the initial controller before opening the new full-size evaluation split."""
import hashlib
import json
import sys
from datetime import datetime,timezone
from pathlib import Path
development,destination=map(Path,sys.argv[1:])
assert not destination.exists()
meta_path=development/"metadata.json"
meta=json.loads(meta_path.read_text())
assert meta["stage"]=="development" and len(meta["inputs"])==59
repo=Path(__file__).resolve().parents[2]
paths=[Path(__file__).with_name(n).resolve() for n in ("AutoReservation.hpp","generate_lexer.py","generate_holdout.py","plan.json")]
hash_source=lambda p:hashlib.sha256(p.read_text(encoding="utf-8-sig").encode()).hexdigest()
frozen=dict(frozen_utc=datetime.now(timezone.utc).isoformat(),
    development_metadata_sha256=hashlib.sha256(meta_path.read_bytes()).hexdigest(),
    policy_sources=[dict(file=p.relative_to(repo).as_posix(),sha256=hash_source(p)) for p in paths],
    binaries=meta["binaries"],decision="Freeze this experimental controller without held-out retuning; assess promotion targets after evaluation.")
destination.parent.mkdir(parents=True,exist_ok=True)
destination.write_bytes((json.dumps(frozen,indent=2)+"\n").encode())
print(f"Frozen at {frozen['frozen_utc']}")
