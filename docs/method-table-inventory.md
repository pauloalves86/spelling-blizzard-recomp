# Method-Table Inventory (Step 15)

## Purpose and method

This step produces one machine-generated inventory of every method table, every site that installs one, and the parent order implied by each constructor. It then re-checks earlier "no owner / no reference" conclusions against it.

- Input: `original/BLIZZARD.EXE`, SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2` (checked by the tools on every run).
- Generator: `tools/ne_vtables.py` (Python 3, read-only). It builds on `tools/ne_farptr.py`'s NE selector-fixup-chain resolution (see `docs/ac80-indirect-caller.md`). Its detection rules are in the tool's docstring.
- Optional function boundaries: `ghidra/Step15Functions.java`, run read-only, maps each install site to its Ghidra function and stack purge. Sites without a boundary are labelled `raw≈<entry>` from the nearest preceding prologue (heuristic).
- Output: `docs/generated-method-tables.md`. It is deterministic: two runs produced byte-identical output.

Regenerate with:

```text
python3 tools/ne_vtables.py --sites /tmp/sites.txt
<GHIDRA_HOME>/support/analyzeHeadless ghidra SpellingBlizzard -readOnly -noanalysis \
  -process BLIZZARD.EXE -scriptPath ghidra -postScript Step15Functions.java /tmp/sites.txt /tmp/functions.txt
python3 tools/ne_vtables.py --functions /tmp/functions.txt --md docs/generated-method-tables.md
```

## Counts (fact)

| Item | Count |
|---|---|
| Two-word far-pointer stores (`mov [x],OFF; mov [x+2],SEG` with a selector fixup) | 1022 |
| Contiguous store chains whose last value is a table | 228 (3 destructor-shaped) |
| Tables (≥ 2 contiguous fixup-resolved cells, bounded by the next installed table) | 142 |
| Tables of the `+0x04 = 1008:ef9e`, `+0x08 = 1008:efa4` shape | 117 |
| Of those 117, tables with no detected installer | **0** |

Every table of the common shape has an installer, so the earlier "no installation constant" results were all artifacts of the fixup chains.

**Strong inference.** These are compiler-generated virtual tables written by inlined constructor steps. The repeated order base → … → final, the matching reverse order in destructor-shaped functions, and the shared slot prefixes all fit that pattern. Class names are not in the binary; table addresses stand in for them. This is not a claim about the original source hierarchy (`AGENTS.md`, non-negotiable 3).

## Shared slot conventions

These conventions are observed on the `1008:1aac`-derived family, the largest subtree, which includes the window, button and owner objects of Steps 14 and 15. Each row cites the code it rests on.

| Slot | Base target in `1008:1aac` | Observed role | Evidence |
|---|---|---|---|
| `+0x04` field (not a slot) | — | parent/owner link | `1008:b6fa` writes it; `1008:b7de` forwards to it |
| `+0x1c` | `1008:b7de` | event handler `(this, dword event)`, `RETF 8`; unhandled events bubble to `+0x04` | Step 14 |
| `+0x30` | `1010:45ea` | create from resource record `(id, parent, …)` | Step 14 |
| `+0x34` | `1010:15f0` | class initializer (overridden) | Step 14 |
| `+0x50` | `1010:491e` | route a mouse event record down the child tree | below |
| `+0x58` / `+0x5c` / `+0x60` | `RETF 0xe` stubs | left (default), middle and right button-down handlers | `1010:491e` dispatch on `rec+0x12` |
| `+0x64` / `+0x68` / `+0x6c` | `RETF 8` stubs | button-up notifications | `1010:3c82` |
| `+0x70` | `1010:4a6e` | hit test over the child list at `+0x0c` | `1010:491e` calls it with the point |
| `+0xc4` | `1010:4e28` | creates the object's native window (overridden in window-owning views) | below |
| `+0xd4` / `+0xdc` | `1010:183c` / `1010:18ec` | paint callbacks called by the window's `WM_PAINT` method | `1008:6dfc`, `1010:559e` |

## Corrections to earlier documents

1. **`1008:7f80` (Step 13 candidate)** is not a table base. `1008:7f80` is the last cell of table `1008:7f0c`. The candidate's "`+0x3c → 1020:2534`" cell is slot `+0x38` of table **`1008:7f84`**, which derives from `1008:0b84`. `1008:7f84` is the application's event-dispatcher class (see `docs/input-to-command.md`), installed at `1008:77ec` in `1008:77d0`, the root's slot `+0x70`. It is allocated with size `0x1a` and stored at the root's `+0x0e`. `1020:2534` wraps the base event fetcher `1010:3d56`.
2. **`1008:7edc` has only 12 slots** (`+0x00`–`+0x2c`). The Step 13 statement "the `1008:7edc` table has slot `+0xd4 → 1010:3d32` and `+0xdc → 1010:3d52`" read past its end. Those cells are slots `+0x2c` and `+0x34` of `1008:7f84`, the event dispatcher, and are not paint callbacks.
3. **`1018:3230` → `1018:3228`** and **`1008:97c2` → `1008:97e6`**: already corrected in Step 14. The inventory confirms both bases, their installers (`1018:2c33` in raw `1018:2bf0`, and `1008:92ff` in `1008:9222`), and their parents (`1018:3108` and `1008:1aac`).

## What owns the window's `+0x154`, and the `+0xd4`/`+0xdc` callbacks (Step 13 item 1)

**Fact.**
- The window constructors `1010:50ce`, `1010:53c8` and `1010:5722` store a constructor argument at `+0x154` (`1010:510c`, `1010:53f9`, `1010:575f`).
- `1010:50ce` is reached only through `1008:6db2` and `1008:6ef6`. Both pass their own `[bp+0x10]` argument through.
- Their three call sites allocate `0x160` bytes via `1008:eed8`. Each passes the current receiver (`[bp+0x8]:si/di`) as that argument:
  - `1008:4ea9` in `1008:4e70`;
  - `1008:0ef3` in `1008:0eba`;
  - `1008:5245` in `1008:520c`.
- Those three functions are slot `+0xc4` of tables `1008:1cf0`, `1008:18fc`/`1008:1744`, and `1008:1ea0` respectively.

So, for windows made by these paths, **`+0x154` is a back-pointer to the view object whose `+0xc4` created the window.** `1008:6dfc` and `1010:559e` paint through it. The callback tables are the creators' own tables:

| Creator table | `+0xd4` | `+0xdc` |
|---|---|---|
| `1008:1cf0`, `1008:1ea0` | `1008:4802` | `1010:18ec` (base) |
| `1008:18fc` | `1008:0c78` | `1010:18ec` (base) |
| `1008:1744` | `1008:1252` | `1010:18ec` (base) |

`1010:53c8` (`1008:ce33`) and `1010:5722` (`1008:d79f`) were not followed to their creators in this step.
`1010:5722` (`1008:671d` / `1008:670a`) was resolved in `docs/root-view-lifecycle.md`: its creator is `1008:3218` (slot `+0xc4` of `1008:7d10`, the root desktop view `DAT_1060:0810`).
All remaining window constructors (`1010:53c8` via `1008:cdf0`, `1010:5722` via `1008:d770`) and slot `+0x14` callers were resolved in `docs/framework-dispatch-edges.md`.

## Open items

- Reversed chains are identified only when the matching forward chain exists. A destructor with no counterpart is still counted as a constructor chain, although it contributes no parent edge unless it has ≥ 2 writes.
- Store-target detection (`Stored to` column) is a 40-byte forward heuristic. `—` means it was not found, not that the object is unstored.
