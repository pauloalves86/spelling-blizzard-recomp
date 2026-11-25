# Reconstruction progress

Generated from Ghidra 12.2 function inventory for `original/BLIZZARD.EXE` (SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`).
All functions in the five initialized code segments are listed in [reconstruction-status.csv](reconstruction-status.csv).

## Function inventory

- Code-segment functions: **1422**
- Win16 ABI entries implemented and stack-cleanup checked: **3 / 1422 (0.21%)** by function count
- Host-side behavioral models (not original entry ABIs): **4**
- Partial original functions: **1**
- Not started or not yet classified: **1414**
- ABI-checked entries' original Ghidra body bytes: **99 / 180530**

The percentage counts original entries with a Win16 C entrypoint whose far-return stack cleanup is checked. It does not claim byte identity, runtime validation against Windows, or complete game-only coverage; the inventory includes startup, runtime, and framework functions, and ownership is not established for every entry.
This inventory comes from the newly created Ghidra 12.2 project plus the 595 method-table seeds. The previous computer's database is unavailable, and older reports list slightly different function totals; treat this as the reproducible local baseline until those inventories are reconciled.

## Win16 ABI entrypoints

| Original entry | C source | Recovered behavior / boundary |
|---|---|---|
| `1018:aeaa` | `src/recovered/win16/application_primitives.c` | Direct far entry follows parent +0x250 to holder +0 and reads nested +0x06; RETF 4 verified. |
| `1018:aede` | `src/recovered/win16/application_primitives.c` | Direct far entry writes nested +0x1a8 then parent +0x254; RETF 6 verified. |
| `1018:af08` | `src/recovered/win16/application_primitives.c` | Direct far entry returns AL=1 for signed argument <= nested +0x1a8; RETF 6 verified. |

## Host behavioral models

These C functions model observed behavior but do not use the original function signatures/calling conventions.

| Original entry | C source | Modeled behavior / boundary |
|---|---|---|
| `1008:e412` | `src/recovered/framework/message_pump.c` | Message ranges/order modeled; Win16 calls supplied through a host seam. |
| `1018:a704` | `src/recovered/application/object_model.c` | Ordered-entry behavior modeled under the documented valid-object preconditions. |
| `1018:adb8` | `src/recovered/application/object_model.c` | Observed 16-bit arithmetic and selected-field update modeled. |
| `1018:ae3a` | `src/recovered/application/object_model.c` | Nested maximum update and ordered-entry call modeled. |

## Partial work

`1008:ac80` has a C fragment for the `0x2af9` state writes. It is not a reconstruction of the full original function.

## Binary comparison limits

The project now has an Open Watcom Win16 object-library target, but the original compiler and linker are not proven, and the objects are not linked into an NE executable with the original segment layout. Byte diffs are therefore not an exact recompilation percentage. Track direct ABI coverage and compare each mapped function's behavior, instructions, data accesses, and calling convention. No-op placeholders for unresolved callbacks are excluded from completed counts.

Regenerate with `python3 tools/reconstruction_progress.py build/functions.json` after exporting a fresh function inventory with `ghidra/DumpCoverageData.java`.
