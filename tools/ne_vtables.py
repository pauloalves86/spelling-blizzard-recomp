#!/usr/bin/env python3
"""Method-table inventory for original/BLIZZARD.EXE (Step 15).

Read-only.  Built on ne_farptr.py's NE selector-fixup-chain resolution.

Detection rules (all on raw segment bytes; no Ghidra state is used):

* install site: two consecutive instructions
      [26] C7 /0 <ea>      , imm16 OFF
      [26] C7 /0 <ea + 2>  , imm16 SEL      (SEL word is a selector-fixup location)
  i.e. a 16:16 far pointer written into memory as two word stores.  The pair
  (target segment, OFF) is the installed value.
* constructor chain: install sites writing through the same base register and
  displacement, each starting at most 12 bytes after the previous one.  The
  last value written is the "final" table; earlier ones are the base tables
  written by inlined base-class constructor steps (Strong inference: this is the
  usual inlined-constructor pattern; the binary does not name it).
* allocation: a `push imm; call far 1008:ef2c` within 48 bytes before the chain.
* store: the first store of the new pointer within 40 bytes after the chain
  (`mov es:[bx+F], si` / `mov dword es:[bx+F], eax` / `mov ds:[X], ax`).
* destructor-shaped chain: the exact reverse of another chain, in a function that
  takes only `this` (RETF 4) or has no Ghidra boundary, while the forward chain's
  function takes more arguments.  Excluded from parent inference.
* table extent: contiguous fixup-resolved 4-byte cells from the installed
  offset, stopping before the next installed table or a non-cell.

Usage:  ne_vtables.py [--json PATH] [--md PATH] [--sites PATH] [--functions PATH]
"""
import json
import struct
import sys
from collections import defaultdict

import ne_farptr as N

CODE = {1, 2, 3, 4, 5}
ALLOC = (2, 0xEF2C)  # 1008:ef2c, the allocator used by every observed constructor
REG16 = ["bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"]


def blob(s):
    fo, ln, _ = N.segs[s]
    return N.data[fo:fo + ln]


def ea_len(m):
    mod, rm = m >> 6, m & 7
    if mod == 0:
        return 2 if rm == 6 else 0
    return {1: 1, 2: 2, 3: None}[mod]


def ea_disp(b, p, m):
    mod, rm = m >> 6, m & 7
    if mod == 0:
        return struct.unpack_from("<H", b, p)[0] if rm == 6 else 0
    if mod == 1:
        return struct.unpack_from("<b", b, p)[0] & 0xFFFF
    return struct.unpack_from("<H", b, p)[0]


def find_installs():
    out = []
    for s in CODE:
        b = blob(s)
        for (ss, loc), t in N.selector_target.items():
            if ss != s or t not in CODE:
                continue
            for dl in (0, 1, 2):
                p = loc - dl - 2
                if p < 1 or b[p] != 0xC7:
                    continue
                m = b[p + 1]
                if (m >> 3) & 7 or ea_len(m) != dl:
                    continue
                d2 = ea_disp(b, p + 2, m)
                q = p - 1 if b[p - 1] == 0x26 else p
                for back in range(4, 9):
                    r = q - back
                    if r < 0 or b[r] != 0xC7:
                        continue
                    m1 = b[r + 1]
                    l1 = ea_len(m1)
                    if l1 is None or (m1 & 7) != (m & 7) or (m1 >> 3) & 7 or r + 2 + l1 + 2 != q:
                        continue
                    d1 = ea_disp(b, r + 2, m1)
                    if (d1 + 2) & 0xFFFF != d2:
                        continue
                    start = r - 1 if r and b[r - 1] == 0x26 else r
                    out.append(dict(seg=s, at=start, end=loc + 2, target=(t, struct.unpack_from("<H", b, r + 2 + l1)[0]),
                                    rm=m1 & 7, mod=m1 >> 6, disp=d1))
    out.sort(key=lambda i: (i["seg"], i["at"]))
    return out


def is_far_call_to(s, p, target):
    b = blob(s)
    return b[p] == 0x9A and struct.unpack_from("<H", b, p + 1)[0] == target[1] and N.selector_target.get((s, p + 3)) == target[0]


def far_call_target(s, p):
    b = blob(s)
    t = N.selector_target.get((s, p + 3))
    return (t, struct.unpack_from("<H", b, p + 1)[0]) if b[p] == 0x9A and t in CODE else None


def find_alloc(s, start):
    """Return (allocation size or None, far calls between the allocation and the chain)."""
    b = blob(s)
    for p in range(start - 5, max(start - 48, 0), -1):
        if is_far_call_to(s, p, ALLOC):
            pre = [far_call_target(s, q) for q in range(p + 5, start - 4)]
            pre = [fmt(*x) for x in pre if x]
            if b[p - 2] == 0x6A:
                return b[p - 1], pre
            if b[p - 3] == 0x68:
                return struct.unpack_from("<H", b, p - 2)[0], pre
            return None, pre
    return None, []


def find_store(s, end):
    """Return a short description of where the new object pointer is stored."""
    b = blob(s)
    for p in range(end, min(end + 40, len(b) - 6)):
        x = b[p:p + 7]
        if x[0] == 0xA3:  # mov ds:[X], ax
            return f"ds:{struct.unpack_from('<H', x, 1)[0]:04x}"
        if x[0] == 0x26 and x[1] == 0x89 and (x[2] >> 6) in (1, 2) and ((x[2] >> 3) & 7) in (6, 7):  # mov es:[r+F], si/di
            d = x[3] if x[2] >> 6 == 1 else struct.unpack_from("<H", x, 3)[0]
            return f"[{REG16[x[2] & 7]}+{d:#x}]"
        if x[:3] == b"\x66\x26\x89" and (x[3] >> 6) in (1, 2) and ((x[3] >> 3) & 7) == 0:  # mov dword es:[r+F], eax
            d = x[4] if x[3] >> 6 == 1 else struct.unpack_from("<H", x, 4)[0]
            return f"[{REG16[x[3] & 7]}+{d:#x}]"
        if x[0] in (0xCA, 0xCB, 0xC3):
            return None
    return None


def build_chains(installs):
    chains = []
    for i in installs:
        c = chains[-1] if chains else None
        if c and c[-1]["seg"] == i["seg"] and c[-1]["rm"] == i["rm"] and c[-1]["disp"] == i["disp"] and 0 <= i["at"] - c[-1]["end"] <= 12:
            c.append(i)
        else:
            chains.append([i])
    return chains


def table_cells(t, off, starts):
    cells = []
    o = off
    while (t, o + 2) in N.selector_target and N.selector_target[(t, o + 2)] in CODE:
        if o != off and (t, o) in starts:
            break
        cells.append((N.selector_target[(t, o + 2)], N.word(t, o)))
        o += 4
    return cells


def raw_entry(s, at):
    """Heuristic: nearest preceding `enter n,0` or `push bp; mov bp,sp` prologue."""
    b = blob(s)
    for p in range(at - 1, max(at - 0x800, 0), -1):
        if (b[p] == 0xC8 and b[p + 3] == 0) or b[p:p + 3] == b"\x55\x8b\xec":
            return p
    return None


def fmt(t, o):
    return f"{N.sel(t):04x}:{o:04x}"


def main(argv):
    opts = dict(zip(argv[1::2], argv[2::2]))
    funcs = {}
    if "--functions" in opts:  # lines "<site> <entry|-> <purge|->" from ghidra/Step15Functions.java
        for line in open(opts["--functions"]):
            parts = line.split()
            if len(parts) == 3:
                funcs[parts[0]] = parts[1] if parts[1] == "-" else f"`{parts[1]}` (RETF {int(parts[2]):#x})"
    # funcs values: "-" or "`SEG:OFF` (RETF 0xN)"
    installs = find_installs()
    chains = build_chains(installs)
    starts = {i["target"] for i in installs}
    tables = {}
    for t, off in sorted(starts):
        cells = table_cells(t, off, starts)
        if len(cells) >= 2:
            tables[(t, off)] = cells
    parents = defaultdict(set)
    sites = defaultdict(list)
    ctor = []
    kept = [c for c in chains if c[-1]["target"] in tables]  # others: single far-pointer stores (callback fields)
    seqs = [tuple(i["target"] for i in c) for c in kept]

    def purge(c):
        v = funcs.get(fmt(c[0]["seg"], c[0]["at"]), "-")
        return None if v == "-" else int(v.rsplit("RETF ", 1)[1].rstrip(")"), 16)

    # Destructor-shaped chain: the exact reverse of another chain, inside a function that
    # takes only `this` (RETF 4) or has no Ghidra boundary, while the forward chain's
    # function takes more arguments.  Its edges are not used for parent inference.
    dtor = set()
    for i, c in enumerate(kept):
        if len(seqs[i]) < 2:
            continue
        for j, d in enumerate(kept):
            if seqs[j] == seqs[i][::-1] and purge(c) in (None, 4) and (purge(d) or 0) > 4:
                dtor.add(i)
    for i, c in enumerate(kept):
        seq = list(seqs[i])
        final = seq[-1]
        if i not in dtor:
            for a, b_ in zip(seq, seq[1:]):
                parents[b_].add(a)
        alloc, pre = find_alloc(c[0]["seg"], c[0]["at"])
        store = find_store(c[-1]["seg"], c[-1]["end"])
        rec = dict(site=fmt(c[0]["seg"], c[0]["at"]), kind="destructor-shaped" if i in dtor else "constructor",
                   final=fmt(*final), chain=[fmt(*x) for x in seq],
                   via=f"[{REG16[c[0]['rm']]}+{c[0]['disp']:#x}]", alloc=alloc, pre=pre, store=store)
        rec["function"] = funcs.get(rec["site"])
        if rec["function"] in (None, "-"):
            e = raw_entry(c[0]["seg"], c[0]["at"])
            rec["function"] = f"raw≈`{fmt(c[0]['seg'], e)}`" if e is not None else None
        ctor.append(rec)
        for x in seq:
            sites[x].append(rec["site"])

    inv = []
    for key, cells in tables.items():
        ps = sorted(parents.get(key, ()))
        parent = ps[0] if len(ps) == 1 else None
        overrides = []
        if parent and parent in tables:
            pc = tables[parent]
            for k, cell in enumerate(cells):
                if k >= len(pc):
                    overrides.append((k * 4, cell, "new"))
                elif cell != pc[k]:
                    overrides.append((k * 4, cell, pc[k]))
        inv.append(dict(table=fmt(*key), slots=len(cells), parents=[fmt(*p) for p in ps],
                        final_in=sum(1 for r in ctor if r["final"] == fmt(*key) and r["kind"] == "constructor"),
                        base_in=sum(1 for r in ctor if fmt(*key) in r["chain"][:-1]),
                        slot1c=fmt(*cells[7]) if len(cells) > 7 else None,
                        overrides=[(f"{o:#x}", fmt(*c), p if p == "new" else fmt(*p)) for o, c, p in overrides],
                        cells=[fmt(*c) for c in cells]))

    result = dict(input=str(N.EXE.name), sha256=N.SHA256, install_sites=len(installs), constructors=ctor, tables=inv)
    if "--json" in opts:
        with open(opts["--json"], "w") as f:
            json.dump(result, f, indent=1)
    if "--sites" in opts:
        with open(opts["--sites"], "w") as f:
            for r in ctor:
                f.write(r["site"] + "\n")
    if "--md" in opts:
        write_md(opts["--md"], result)
    print(f"install sites={len(installs)} constructors={len(ctor)} tables={len(inv)}")


def write_md(path, r):
    L = []
    L.append("# Generated Method-Table Inventory\n")
    L.append(f"Generated by `tools/ne_vtables.py` from `original/{r['input']}` (SHA-256 `{r['sha256']}`). "
             "Do not edit by hand; regenerate. Detection rules are in the tool's docstring; interpretation is in "
             "`docs/method-table-inventory.md`.\n")
    L.append(f"- install sites: {r['install_sites']}\n- constructor chains (final value is a table): {len(r['constructors'])}\n"
             f"- tables (>= 2 cells): {len(r['tables'])}\n")
    L.append("## Tables\n")
    L.append("| Table | Slots | Parent(s) | Final in | Base in | `+0x1c` | Overrides vs parent |")
    L.append("|---|---|---|---|---|---|---|")
    for t in r["tables"]:
        ov = "; ".join(f"{o}={c}" for o, c, p in t["overrides"]) if t["parents"] and len(t["parents"]) == 1 else "n/a"
        if len(ov) > 600:
            ov = ov[:600] + " …"
        L.append(f"| `{t['table']}` | {t['slots']} | {', '.join(t['parents']) or '—'} | {t['final_in']} | {t['base_in']} | "
                 f"{t['slot1c'] or '—'} | {ov} |")
    L.append("\n## Inheritance tree\n")
    L.append("Parent = the table written immediately before the final one in a constructor chain. "
             "`final` = number of constructor chains ending in this table.\n")
    L.append("```text")
    kids = {}
    for t in r["tables"]:
        for p_ in t["parents"]:
            kids.setdefault(p_, []).append(t["table"])
    by = {t["table"]: t for t in r["tables"]}

    def walk(n, depth, seen):
        t = by[n]
        L.append(f"{'  ' * depth}{n}  slots={t['slots']} final={t['final_in']} +1c={t['slot1c'] or '-'}")
        if n in seen:
            return
        for k in sorted(kids.get(n, [])):
            walk(k, depth + 1, seen | {n})

    for t in r["tables"]:
        if not t["parents"]:
            walk(t["table"], 0, set())
    L.append("```")
    L.append("\n## Constructor sites\n")
    L.append("| Site | Kind | Function | Chain (first → last write) | Pre-call | Written via | Alloc size | Stored to |")
    L.append("|---|---|---|---|---|---|---|---|")
    for c in r["constructors"]:
        a = f"{c['alloc']:#x}" if c["alloc"] is not None else "—"
        fn = c.get("function") or "—"
        L.append(f"| `{c['site']}` | {c['kind']} | {fn} | {' → '.join(c['chain'])} | {', '.join(c['pre']) or '—'} | `{c['via']}` | {a} | {c['store'] or '—'} |")
    with open(path, "w") as f:
        f.write("\n".join(L) + "\n")


if __name__ == "__main__":
    main(sys.argv)
