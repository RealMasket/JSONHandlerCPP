"""Fixed, separate evaluation split. Do not use its results to tune a candidate."""
import argparse
import json
import random
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
parser.add_argument("--small", action="store_true", help="CI-size fixtures with the same structures")
args = parser.parse_args()
args.directory.mkdir(parents=True, exist_ok=True)
counts = (17, 67) if args.small else (257, 4099)
for index, count in enumerate(counts):
    randomizer = random.Random(20261106 + count)
    dumps = lambda value: json.dumps(value, ensure_ascii=True, separators=(",", ":"))
    padding = " \t\n" * (count * (4 if args.small else 256))
    records = [{"n": i, "children": [None, True, False, {"code": str(i)}]} for i in range(count)]
    keys = {"key-" + str(i) + "x" * (33 if args.small else 1031): i for i in range(max(3, count // 17))}
    sawtooth = ["s" * (randomizer.randrange(1, 47) if i % 19 else (127 if args.small else 8191)) for i in range(count)]
    escaped = "\n\t\"\\\u041f,[]{}" * (7 if args.small else 2053)
    datasets = {
        f"held-tail-records-{count}": dumps(records) + padding,
        f"held-long-keys-{count}": dumps(keys),
        f"held-front-nested-{count}": padding + dumps(records),
        f"held-sawtooth-{count}": dumps(sawtooth),
        f"held-empty-mix-{count}": dumps(([{}, [], None, True, False] * count)),
        f"held-escape-transition-{count}": dumps([escaped] + list(range(count))),
    }
    for name, source in datasets.items():
        (args.directory / (name + ".json")).write_bytes(source.encode("ascii"))
print("Generated 12 separate holdout fixtures")
