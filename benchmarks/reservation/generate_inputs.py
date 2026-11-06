"""Additional deterministic shape/transition fixtures; standard library only."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
parser.add_argument("--base", type=int, default=256)
args = parser.parse_args()
if not 1 <= args.base <= 4096:
    parser.error("base must be in 1..4096")
args.directory.mkdir(parents=True, exist_ok=True)
for scale in (1, 8, 64):
    rows = args.base * scale
    count = rows * 16
    zeros = ",".join(["0"] * count)
    sparse = '"' + "s" * (count * 8) + '"'
    compact = "[" + ",".join(["0"] * rows) + "]"
    padding = " \n\t " * (rows * 32)
    blocks = []
    for offset in range(0, count, 1024):
        blocks.extend(["0"] * min(1024, count - offset))
        blocks.append('"' + "m" * 8192 + '"')
    datasets = {
        f"dense-zeros-{count}": "[" + zeros + "]",
        f"dense-first-{count}": "[" + zeros + "," + sparse + "]",
        f"sparse-first-{count}": "[" + sparse + "," + zeros + "]",
        f"alternating-{count}": "[" + ",".join(blocks) + "]",
        f"whitespace-leading-{rows}": padding + compact,
        f"whitespace-trailing-{rows}": compact + padding,
        f"whitespace-interleaved-{rows}": "[" + (", \n\t" + " " * 128).join(["0"] * rows) + "]",
        f"flat-object-{rows}": "{" + ",".join(f'"key-{i}":{i}' for i in range(rows)) + "}",
    }
    for name, source in datasets.items():
        (args.directory / (name + ".json")).write_bytes(source.encode("ascii"))
print("Generated 24 reservation fixtures")
