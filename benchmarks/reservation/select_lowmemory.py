"""Apply the pre-recorded training-only decision rule and freeze a candidate."""
import csv
import hashlib
import json
import math
import statistics
import sys
from datetime import datetime, timezone
from pathlib import Path

root, destination = map(Path, sys.argv[1:])
assert not destination.exists(), "Do not overwrite a frozen selection"
meta_path = root / "metadata.json"
meta = json.loads(meta_path.read_text())
assert meta["split"] == "training" and len(meta["inputs"]) == 47
plan_path = Path(__file__).with_name("lowmemory-plan.json")
plan = json.loads(plan_path.read_text())
names = [i["name"] for i in meta["inputs"]]
times, memory = {}, {}
for policy in meta["policies"]:
    memory[policy] = {r["dataset"]: r for r in csv.DictReader((root / f"{policy}-memory.csv").open(newline="")) if r["stage"] == "whole_parse"}
    for number in range(1, meta["rounds"] + 1):
        for r in csv.DictReader((root / f"{policy}-{number}.csv").open(newline="")):
            if r["operation"] == "parse":
                times.setdefault((policy, r["dataset"]), []).append(float(r["median_us"]))
assert all(len(v) == meta["rounds"] for v in times.values())
scores = {}
for policy in plan["candidates"]:
    ratio = math.exp(statistics.mean(math.log(statistics.median(times[policy,n]) / statistics.median(times["current",n])) for n in names))
    tail = int(memory[policy]["whitespace-trailing-16384.json"]["peak_live_bytes"])
    ascii_peak = int(memory[policy]["long-ascii-4096.json"]["peak_live_bytes"])
    eligible = tail <= int(memory["current"]["whitespace-trailing-16384.json"]["peak_live_bytes"]) * .25
    eligible &= ascii_peak <= int(memory["current"]["long-ascii-4096.json"]["peak_live_bytes"]) * 1.2
    scores[policy] = dict(eligible=eligible, geomean_parse_time_ratio=ratio,
        total_copied_token_bytes=sum(int(r["copied_token_bytes"]) for r in memory[policy].values()),
        tail_peak_bytes=tail, long_ascii_peak_bytes=ascii_peak)
eligible = [p for p in scores if scores[p]["eligible"]]
assert eligible, "No candidate met the planned memory gates"
best = min(scores[p]["geomean_parse_time_ratio"] for p in eligible)
tied = [p for p in eligible if scores[p]["geomean_parse_time_ratio"] <= best * 1.05]
chosen = min(tied, key=lambda p: scores[p]["total_copied_token_bytes"])
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
selection = dict(chosen=chosen, frozen_utc=datetime.now(timezone.utc).isoformat(),
    scores=scores, within_five_percent=tied, rules=plan["selection"],
    plan_sha256=sha(plan_path), training_metadata_sha256=sha(meta_path),
    holdout_generator_sha256=sha(Path(__file__).with_name("generate_holdout.py")),
    training_base_revision="74b629431520bac9b203e8a891ec73bd14a1a325")
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_bytes((json.dumps(selection,indent=2)+"\n").encode())
print(json.dumps(selection,indent=2))
