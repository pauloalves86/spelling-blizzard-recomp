#!/usr/bin/env python3
"""Check RETF cleanup in the Open Watcom disassembly of known Win16 entries."""
import re
import sys
from pathlib import Path

EXPECTED = {
    "SB_WIN16_1018_AEAA": 4,
    "SB_WIN16_1018_AEDE": 6,
    "SB_WIN16_1018_AF08": 6,
}


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_win16_abi.py <wdis-output>")
    text = Path(sys.argv[1]).read_text()
    failures = []
    for symbol, purge in EXPECTED.items():
        label = re.search(rf"(?m)^\w+\s+{symbol}:\s*$", text)
        if not label:
            failures.append(f"{symbol}: symbol missing")
            continue
        end = re.search(r"(?m)^Routine Size:", text[label.end():])
        body = text[label.end():label.end() + end.start()] if end else text[label.end():]
        actual = re.search(r"(?im)\bretf\s+0x([0-9a-f]+)", body)
        if not actual:
            failures.append(f"{symbol}: no immediate far return found")
        elif int(actual.group(1), 16) != purge:
            failures.append(f"{symbol}: RETF {int(actual.group(1), 16):#x}, expected RETF {purge:#x}")
    if failures:
        raise SystemExit("\n".join(failures))
    print(f"Win16 ABI cleanup verified for {len(EXPECTED)} entrypoints")


if __name__ == "__main__":
    main()
