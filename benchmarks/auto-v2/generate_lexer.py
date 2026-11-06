"""Reuse the proven grammar/observer transform; change only the private controller."""
import subprocess
import sys
from pathlib import Path
source,destination=map(Path,sys.argv[1:])
subprocess.run([sys.executable,str(Path(__file__).resolve().parents[1]/"auto/generate_lexer.py"),str(source),str(destination)],check=True)
for goal in ("speed","memory"):
    path=destination/(goal+".cpp")
    text=path.read_text(encoding="utf-8-sig")
    assert text.count('#include "AutoReservation.hpp"')==1 and text.count('AutoReserve::Controller<AutoReserve::Goal::')==1
    text=text.replace('#include "AutoReservation.hpp"','#include "AutoReservationV2.hpp"').replace('AutoReserve::Controller<AutoReserve::Goal::','AutoReserveV2::Controller<AutoReserveV2::Goal::')
    path.write_bytes(text.encode())
