# Code Coverage and Gap Classification

## Summary

This document presents the complete byte-level coverage measurement and gap classification across all five code segments of `original/BLIZZARD.EXE`.

Measuring coverage simply against the raw code segment size (221,968 bytes) is misleading because code segments in this Win16 executable embed significant non-code artifacts: virtual method tables, switch jump tables, embedded ASCII strings, and inter-function alignment padding.

Evaluating coverage as **`covered / (covered + candidate code)`** reveals that the reverse-engineering workspace has already covered **93.44%** of the executable's actual code.

```text
================================================================================
Total code segment space:            221,968 bytes (100.00%)
--------------------------------------------------------------------------------
  Covered by known functions:        180,530 bytes  (81.33%)
  Uncovered candidate code:           12,664 bytes   (5.71%)
  Virtual method tables:              23,624 bytes  (10.64%)
  Switch jump tables:                  1,746 bytes   (0.79%)
  Embedded strings & metadata:         1,149 bytes   (0.52%)
  Alignment padding (00/90/CC):        2,105 bytes   (0.95%)
  Other data / fixup records:            150 bytes   (0.07%)
--------------------------------------------------------------------------------
  => TRUE CODE COVERAGE:              93.44%  [covered / (covered + candidate)]
================================================================================
```

---

## 1. Segment-by-Segment Coverage Breakdown (Fact)

| Segment | Selector | Segment Length | Covered Functions | Candidate Code | Vtables | Jump Tables | Strings | Padding | Other Data | True Coverage | Raw Ratio |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Seg 1 | `1000` | 9,191 | 8,448 | 581 | 0 | 68 | 0 | 92 | 2 | **93.57%** | 91.92% |
| Seg 2 | `1008` | 61,402 | 49,419 | 1,057 | 9,520 | 454 | 400 | 486 | 66 | **97.91%** | 80.48% |
| Seg 3 | `1010` | 61,418 | 52,806 | 3,681 | 3,300 | 590 | 340 | 666 | 35 | **93.48%** | 85.98% |
| Seg 4 | `1018` | 61,307 | 47,423 | 5,660 | 6,752 | 528 | 270 | 638 | 36 | **89.34%** | 77.35% |
| Seg 5 | `1020` | 28,650 | 22,434 | 1,685 | 4,052 | 106 | 139 | 223 | 11 | **93.01%** | 78.30% |
| **Total** | | **221,968** | **180,530** | **12,664** | **23,624** | **1,746** | **1,149** | **2,105** | **150** | **93.44%** | **81.33%** |

Every single byte across all five segments (221,968 bytes) is categorized with zero overlaps and zero unclassified gaps.

---

## 2. Validation of the 595 New Virtual Method Functions (Fact)

Following the creation of 595 missing functions from vtable seeds (`ghidra/CreateSeeds.java`), an automated **slot-ABI consistency verification** was performed across all 142 method tables (`tools/ne_vtables.py` and `ghidra/CheckSeeds.java`):

1. **Principle**: In 16-bit segmented x86 virtual method dispatch, every override of a virtual slot must accept the identical argument count and calling convention as the base method it overrides. Therefore, the stack purge instruction (`RETF n` vs `RETF`) must strictly agree across parent and child slots.
2. **Direct Override Verification**:
   - Compared every slot override between child and parent tables across all 142 tables.
   - Total direct override comparisons: **363**.
   - Agreements: **363 (100.0%)**.
   - Mismatches: **0**.
3. **Descendant Path Verification**:
   - Traced all slots from their initial introducing table through every inheriting descendant across the class hierarchy.
   - Total descendant slot checks: **5,097**.
   - Agreements: **5,097 (100.0%)**.
   - Mismatches: **0**.
4. **Resolution of Previously Unanalyzed Cells**:
   - Cell `1008:b82e` (present at slot 9 in 67 separate method tables): successfully created as `FUN_1008_b82e` (43 bytes, `RETF 0xc`). Its `RETF 0xc` purge perfectly matches all sibling and parent slot 9 implementations.

Conclusion: All 595 newly created functions are confirmed to be valid, correctly aligned, and structurally consistent virtual method bodies.

---

## 3. Analysis of Uncovered Gaps and Non-Code Artifacts (Fact & Strong Inference)

Detailed analysis of the 41,438 uncovered bytes reveals four distinct categories of non-code content embedded within code segments:

### A. Virtual Method Tables (23,624 bytes)
- 142 detected method tables (100% located in code segments: Segments 2, 3, 4, 5).
- Completely unreferenced by linear instruction flow; accessed only via indirect far pointers and NE relocation selector fixup chains.

### B. Switch Jump Tables (1,746 bytes)
- 36 indirect jump tables embedded directly inside functions.
- Example `1018:26a6`: 11-target near jump table (`0x26d4`, `0x26ea`, `0x26c3`, `0x2704`...) accessed via `jmp word ptr cs:[bx + 0x26a6]`.
- Example `1010:c47a`: 125-entry jump table (250 bytes) for command routing.
- Example `1018:df06`: 14-entry jump table (28 bytes).
- Between individual switch cases, the Microsoft C compiler inserted `0x90` (NOP) bytes for 16-bit word alignment (e.g. `1018:26d3`, `1018:26e9`, `1018:2703`).

### C. Embedded Strings and String Tables (1,149 bytes)
- Embedded ASCII strings, error diagnostics, and resource name tables:
  - `1010:0000` (117 bytes): `"A function subclass responsibility function was called"`
  - `1008:6cb8` (72 bytes): `"%s requires a sound card that supports 11.025 KHz 8-bit mono output."`
  - `1008:6d5a` (67 bytes): `"%s requires more memory.  Consider closing other applications."`
  - `1008:6c76` (66 bytes): `"%s requires a sound driver that allows for simultaneous output."`
  - `1008:6d00` (54 bytes): `"%s requires a screen resolution of 640x480 or higher."`
  - `1008:6c2a` (36 bytes): `"%s requires Windows 3.10 or higher."`
  - `1008:5554` (20 bytes): `"FailNIL(%s) failed!"`
  - `1008:efc2` (24 bytes): `"Subclass Responsibility"`
  - `1018:7f32` (31 bytes): `"itsForeRedrawList[theSubLayer]"`
  - `1018:7f52` (31 bytes): `"itsBackRedrawList[theSubLayer]"`
  - `1010:cc16` (27 bytes): `"itsSpellingDemonObjectGrid"`

### D. Inter-Function and Inter-Block Padding (2,105 bytes)
- 1,195 gaps consisting purely of `0x00`, `0x90`, or `0xcc` alignment bytes inserted between functions and data structures.

---

## 4. Remaining Candidate Code and Relocation Targets (Fact)

The remaining **12,664 bytes** (5.71% of code space) represent non-virtual code that was not entered through vtables.

### Relocation Seeds vs. Vtable Seeds:
- Total NE selector relocation links in the binary: **8,936**.
- Relocation links targeting code segments: **8,825**.
- Unique code targets resolved via far pointer cells and far calls (`call 9A`): **1,493**.
- Overlap with vtable seeds: **1,088 (100.0% of vtable seeds are relocated)**.
- Relocation targets outside vtables: **405**.

Status of the 405 non-vtable relocation targets in Ghidra:
- **255** targets already have functions defined at entry.
- **57** targets land inside existing function bodies (internal branch labels / far call targets).
- **93** targets land in uncovered gaps (missing functions).

Key functions identified among the candidate code:
1. `1010:5db2`: The primary Win16 `WndProc` (window procedure), written as paired immediates at `1010:5c1d` (`mov word ptr [bx+0x10c], 0x5db2; mov word ptr [bx+0x10e], seg 1010`).
2. `1008:0000`: Segment 2 initialization / dispatch entry point (`enter 0x8, 0`).
3. `1008:21ec`: Specialized view callback helper (`enter 0x4, 0`).
4. `1008:53b2`: Control frame callback (`enter 0x20, 0`).
5. `1000:166d`: C runtime / DOS startup path (`push bp; mov bp, sp`).

Total candidate function prologues identified across all uncovered gaps: **67**.

---

## 5. Exported Artifacts

The complete data generated during this phase is preserved in the following artifacts:

| Artifact | Contents |
|---|---|
| `build/vtable_seeds.txt` | Exactly 1,088 unique virtual method entry points extracted from all 142 tables. |
| `build/functions.json` | Historical export of 1,554 functions with entry addresses, body size, stack purge, and return instructions. |
| `build/functions.txt` | Historical human-readable listing of those 1,554 functions and their return signatures. |
| `build/coverage_classification.json` | Segment-by-segment and overall byte-level coverage classification. |
| `build/vtable_ranges.txt` | Table extent ranges (segment, start, end, byte size) for all 142 method tables. |
| `docs/semantic-debt.md` | Grounded registry distinguishing machine operations from unproven source semantics. |

The current local Ghidra 12.2 project exports 1,552 records (1,422 code-segment functions plus 130 import/thunk records). Its code function bodies still total 180,530 bytes. The two-record boundary-count difference from the historical 1,554 export is unresolved because the prior Ghidra database is unavailable; the figures above remain as historical provenance rather than being silently replaced.
