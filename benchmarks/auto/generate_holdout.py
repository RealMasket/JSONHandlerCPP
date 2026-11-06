"""New Auto evaluation split; generate full cases only after freezing the controller."""
import argparse
import json
import random
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument("directory",type=Path)
parser.add_argument("--small",action="store_true")
args=parser.parse_args()
args.directory.mkdir(parents=True,exist_ok=True)
for n in ((11,43) if args.small else (73,2053)):
    rng=random.Random(20261107+n)
    dumps=lambda value:json.dumps(value,ensure_ascii=True,separators=(",",":"))
    length=31 if args.small else 4093
    padding=" \r\n\t"*(n*(2 if args.small else 128))
    zeros=",".join("0" for _ in range(n*7))
    spiky=[("\n\t\\\"\u041f\U0001f600"*length if i%23==0 else str(rng.randrange(100000))) for i in range(n)]
    blocks=[]
    for i in range(n):
        blocks.extend([i,None,{},[]])
        if i%17==0: blocks.append("r"*(length*3))
    keys={str(i)+(rng.choice("abc")*length if i%13==0 else "k"):bool(i%2) for i in range(n)}
    zigzag=0
    for level in range(9 if args.small else 43):
        zigzag=[level,{"child":zigzag,"rows":[None,True,False]}]
    data={
        f"auto-held-spiky-strings-{n}":dumps(spiky),
        f"auto-held-bursts-{n}":dumps(blocks),
        f"auto-held-long-keys-{n}":dumps(keys),
        f"auto-held-zigzag-{n}":padding+dumps(zigzag),
        f"auto-held-inner-padding-{n}":"["+zeros+","+padding+zeros+"]",
        f"auto-held-phases-{n}":padding+"["+zeros+","+dumps("q"*(length*n))+","+zeros+"]",
        f"auto-held-late-padding-{n}":"["+zeros+"]"+padding*3,
        f"auto-held-ragged-numbers-{n}":"["+(", \n"+" "*19).join(str(rng.randrange(-999999,999999)) for _ in range(n*7))+"]",
    }
    for name,source in data.items():
        (args.directory/(name+".json")).write_bytes(source.encode("ascii"))
print("Generated 16 Auto holdout fixtures")
