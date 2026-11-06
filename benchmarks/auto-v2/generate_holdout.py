"""New independent v2 evaluation cases; full generation follows budget freeze."""
import argparse,json,random
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--small',action='store_true');a=p.parse_args()
a.directory.mkdir(parents=True,exist_ok=True)
for n in ((13,47) if a.small else (149,3079)):
    r=random.Random(20261108+n)
    dump=lambda value:json.dumps(value,ensure_ascii=True,separators=(',',':'))
    cells=[{'id':i,'nested':[None,bool(i%2),{}]} for i in range(n)]
    padding=' \t\r\n'*(n*(2 if a.small else 128))
    numbers=[r.randrange(-10000000,10000000) for _ in range(n*5+7)]
    chunks=[]
    for i in range(n):
        chunks.append([i,[],{},None])
        if i%41==0:chunks.append('\\\n\"[],{}'* (7 if a.small else 1021))
    unicode=[('Привет-世界-😀'*(3 if a.small else 127) if i%23==0 else str(i)) for i in range(n)]
    huge=[(' '* (127 if a.small else 8191) if i%211==0 else {'v':i}) for i in range(n)]
    duplicates='{'+','.join('"k'+str(i%19)+'":'+str(i) for i in range(n*3))+'}'
    documents={
        f'v2-held-tail-cells-{n}':dump(cells)+padding,
        f'v2-held-internal-gap-{n}':'{"a":'+dump(numbers[:n])+',"b":'+padding+dump(numbers[n:])+'}',
        f'v2-held-ragged-width-{n}':dump(numbers),
        f'v2-held-chunks-{n}':dump(chunks),
        f'v2-held-utf8-{n}':json.dumps(unicode,ensure_ascii=False,separators=(',',':')),
        f'v2-held-space-strings-{n}':dump(huge),
        f'v2-held-duplicate-keys-{n}':duplicates,
        f'v2-held-front-cells-{n}':padding+dump(cells),
    }
    for name,text in documents.items():(a.directory/(name+'.json')).write_bytes(text.encode('utf-8'))
print('Generated 16 independent Auto v2 holdout fixtures')
