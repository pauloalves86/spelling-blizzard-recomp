#!/usr/bin/env python3
"""Generate a per-function reconstruction ledger from Ghidra's function dump.

Usage: python3 tools/reconstruction_progress.py build/functions.json
The input is produced by ghidra/DumpCoverageData.java. Only explicitly mapped
functions receive a reconstructed/partial status; every other code function
stays visible as untriaged.
"""
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUT_SHA256 = "ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2"
CODE_SEGMENTS = {"1000", "1008", "1010", "1018", "1020"}

# A Win16 entry is distinct from a host-side semantic model: the former must
# preserve the original far ABI and stack cleanup as well as the body behavior.
MAPPED = {
    "1008:e412": ("host-model", "framework", "src/recovered/framework/message_pump.c",
                  "Message ranges/order modeled; Win16 calls supplied through a host seam."),
    "1018:a704": ("host-model", "application", "src/recovered/application/object_model.c",
                  "Ordered-entry behavior modeled under the documented valid-object preconditions."),
    "1018:adb8": ("host-model", "application", "src/recovered/application/object_model.c",
                  "Observed 16-bit arithmetic and selected-field update modeled."),
    "1018:ae3a": ("host-model", "application", "src/recovered/application/object_model.c",
                  "Nested maximum update and ordered-entry call modeled."),
    "1018:aeaa": ("win16-abi-checked", "application", "src/recovered/win16/application_primitives.c",
                  "Direct far entry follows parent +0x250 to holder +0 and reads nested +0x06; RETF 4 verified."),
    "1018:aede": ("win16-abi-checked", "application", "src/recovered/win16/application_primitives.c",
                  "Direct far entry writes nested +0x1a8 then parent +0x254; RETF 6 verified."),
    "1018:af08": ("win16-abi-checked", "application", "src/recovered/win16/application_primitives.c",
                  "Direct far entry returns AL=1 for signed argument <= nested +0x1a8; RETF 6 verified."),
    "1008:ac80": ("partial", "application", "src/recovered/application/application_state.c",
                  "Only the 0x2af9 global-state writes are represented; original function is incomplete."),
}


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    source = Path(sys.argv[1])
    dump = json.loads(source.read_text())
    funcs = [f for f in dump["functions"]
             if f["entry"].split(":", 1)[0].lower() in CODE_SEGMENTS]
    by_entry = {f["entry"].lower(): f for f in funcs}
    missing = sorted(set(MAPPED) - by_entry.keys())
    if missing:
        raise SystemExit("Mapped address missing from Ghidra inventory: " + ", ".join(missing))

    csv_path = ROOT / "docs/reconstruction-status.csv"
    fields = ["entry", "ghidra_name", "body_bytes", "status", "scope", "source", "notes"]
    counts = {"win16-abi-checked": 0, "host-model": 0, "partial": 0, "not-started": 0}
    byte_counts = dict.fromkeys(counts, 0)
    with csv_path.open("w", newline="") as stream:
        stream.write(f"# input_sha256={INPUT_SHA256}; ghidra=12.2; code_segments={','.join(sorted(CODE_SEGMENTS))}\n")
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for f in sorted(funcs, key=lambda x: x["entry"].lower()):
            entry = f["entry"].lower()
            status, scope, src, notes = MAPPED.get(
                entry, ("not-started", "untriaged", "", "Role and reconstruction status not yet classified."))
            counts[status] += 1
            byte_counts[status] += f["body_size"]
            writer.writerow({"entry": entry, "ghidra_name": f["name"], "body_bytes": f["body_size"],
                             "status": status, "scope": scope, "source": src, "notes": notes})

    total = len(funcs)
    covered = counts["win16-abi-checked"]
    percent = covered * 100 / total if total else 0
    report = [
        "# Reconstruction progress",
        "",
        f"Generated from Ghidra 12.2 function inventory for `original/BLIZZARD.EXE` (SHA-256 `{INPUT_SHA256}`).",
        "All functions in the five initialized code segments are listed in [reconstruction-status.csv](reconstruction-status.csv).",
        "",
        "## Function inventory",
        "",
        f"- Code-segment functions: **{total}**",
        f"- Win16 ABI entries implemented and stack-cleanup checked: **{covered} / {total} ({percent:.2f}%)** by function count",
        f"- Host-side behavioral models (not original entry ABIs): **{counts['host-model']}**",
        f"- Partial original functions: **{counts['partial']}**",
        f"- Not started or not yet classified: **{counts['not-started']}**",
        f"- ABI-checked entries' original Ghidra body bytes: **{byte_counts['win16-abi-checked']} / {sum(f['body_size'] for f in funcs)}**",
        "",
        "The percentage counts original entries with a Win16 C entrypoint whose far-return stack cleanup is checked. It does not claim byte identity, runtime validation against Windows, or complete game-only coverage; the inventory includes startup, runtime, and framework functions, and ownership is not established for every entry.",
        "This inventory comes from the newly created Ghidra 12.2 project plus the 595 method-table seeds. The previous computer's database is unavailable, and older reports list slightly different function totals; treat this as the reproducible local baseline until those inventories are reconciled.",
        "",
        "## Win16 ABI entrypoints",
        "",
        "| Original entry | C source | Recovered behavior / boundary |",
        "|---|---|---|",
    ]
    for entry, (_, _, src, note) in sorted(MAPPED.items()):
        if MAPPED[entry][0] == "win16-abi-checked":
            report.append(f"| `{entry}` | `{src}` | {note} |")
    report += [
        "",
        "## Host behavioral models",
        "",
        "These C functions model observed behavior but do not use the original function signatures/calling conventions.",
        "",
        "| Original entry | C source | Modeled behavior / boundary |",
        "|---|---|---|",
    ]
    for entry, (status, _, src, note) in sorted(MAPPED.items()):
        if status == "host-model":
            report.append(f"| `{entry}` | `{src}` | {note} |")
    report += [
        "",
        "## Partial work",
        "",
        "`1008:ac80` has a C fragment for the `0x2af9` state writes. It is not a reconstruction of the full original function.",
        "",
        "## Binary comparison limits",
        "",
        "The project now has an Open Watcom Win16 object-library target, but the original compiler and linker are not proven, and the objects are not linked into an NE executable with the original segment layout. Byte diffs are therefore not an exact recompilation percentage. Track direct ABI coverage and compare each mapped function's behavior, instructions, data accesses, and calling convention. No-op placeholders for unresolved callbacks are excluded from completed counts.",
        "",
        "Regenerate with `python3 tools/reconstruction_progress.py build/functions.json` after exporting a fresh function inventory with `ghidra/DumpCoverageData.java`.",
        "",
    ]
    (ROOT / "docs/reconstruction-progress.md").write_text("\n".join(report))
    print(f"win16_abi_checked={covered}/{total} ({percent:.2f}%) host_models={counts['host-model']} partial={counts['partial']} untriaged={counts['not-started']}")


if __name__ == "__main__":
    main()
