# AGENTS.md — Spelling Blizzard Recomp

## What this is

- `Spelling Blizzard` is a 1994 Bright Star Technology, Inc. educational spelling game, distributed as a Windows CD product.
- The canonical executable is `original/BLIZZARD.EXE`, version `1.10.CD for Windows`, identified by its VERSIONINFO resource. It is a 229,472-byte Win16 NE executable. The paired CD resource DLL is `original/SB_CDRES.DLL` (9,808,128 bytes, containing 1,751 resources across 43 custom types). Preserve both files as immutable ground truth.
- The project goal is **Path 2: Full Decompilation to Clean C/C++ with a Thin Win16 API Shim**. We recover the original program's classes, methods, data structures, and control flow from the executable and Ghidra analysis, and translate them into clean, readable, maintainable C/C++ that compiles against a lightweight Win16 API shim (and can also be ABI-checked with Open Watcom 16-bit where helpful).
- The executable is a **16-bit Windows 3.x NE application** (`x86:LE:16:Protected Mode:default` in Ghidra). Imports come from `KERNEL`, `GDI`, `USER`, `KEYBOARD`, `MMSYSTEM`, and `SHELL`; graphics use GDI; audio uses Windows MMSYSTEM `waveOut` (streaming `BSW\0` 11,025 Hz 8-bit mono audio resources from `SB_CDRES.DLL`); and input uses Windows messages and keyboard state APIs.
- The analyzed executable contains 5 initialized code segments (`1000`, `1008`, `1010`, `1018`, `1020`) and 8 data segments (automatic data segment mapped at `1060` in Ghidra), with 1,422 code-segment functions, 130 import/thunk stubs, and 142 virtual method tables (1,088 virtual method slots).
- The working environment uses **Ghidra 12.2** at `.local/tools/ghidra_12.2_PUBLIC/` with project `ghidra/local/SpellingBlizzard.gpr` (`ghidra/local/SpellingBlizzard.rep/`), and **Open Watcom 2.0** at `.local/tools/openwatcom/`.

## Source-of-truth hierarchy

1. `original/BLIZZARD.EXE` & `original/SB_CDRES.DLL` — immutable binary ground truth.
2. Batch-exported Ghidra decompilation (`build/decomp/`), method-table JSON (`build/method_tables.json`), and the Ghidra database (`ghidra/local/SpellingBlizzard.gpr`).
3. Project documentation under `docs/` — architecture, subsystem maps, and vtable inventories.
4. Reconstructed source code under `src/` — derived representation, continuously refined to match the binary.

## Core Rules (Pragmatic Decompilation)

1. **Never modify `original/BLIZZARD.EXE` or `original/SB_CDRES.DLL`.**
   - Treat `original/` as read-only ground truth.

2. **Use Descriptive Working Names with Address Suffixes.**
   - Do **not** freeze progress over "semantic debt" or refuse to name fields, functions, or classes when their practical role is clear from context, callers, strings, or resource types.
   - Instead, use clear **working names suffixed with the original address or offset** (e.g., `Audio_LoadBswResource_1010_0fe6`, `RecordTable_InsertTop7_1018_a704`, `CView_1008_1aac`, `score_or_level_06`, `is_dirty_254`).
   - Including the hex address/offset in the identifier preserves 100% factual traceability while keeping the C/C++ code readable and fast to write.

3. **Decompile by Subsystem and Class, Not Isolated Micro-Getters.**
   - Partition the 1,422 code functions into three buckets:
     1. **Bucket A — Compiler Runtime / CRT (`1000:*` and standard helpers):** Identify standard C runtime functions (`malloc`, `free`, `memcpy`, `strcpy`, file I/O, startup) and replace or bridge them via standard C / shim routines rather than re-implementing 16-bit DOS/CRT internals.
     2. **Bucket B — Bright Star C++ Framework (`CView`, `CWindow`, `CDesktop`, `CEditText`, Resource Manager, Sound/WaveOut Streamer, Offscreen Layered Renderer):** Decompile class-by-class using the 142 recovered method tables in `docs/generated-method-tables.md`.
     3. **Bucket C — Game Logic (`SpellingDemons*`, match, grid, actors, levels, score/records):** Translate into clean, readable C/C++ methods and state structs backed by the framework and shim.

4. **Single Unified Codebase + Thin Win16 API Shim (No Dual Mock/Win16 Duplication).**
   - Do not write separate artificial "host behavioral mock resolvers" for every function alongside a second Win16 copy.
   - Write clean, portable C/C++ that uses lightweight macros/typedefs for Win16 conventions and calls a thin **Win16 API Shim** (`KERNEL` resource loading directly from `SB_CDRES.DLL`, `GDI` bitmap/palette/drawing operations, `USER` window/message pump, `MMSYSTEM` `waveOut` audio).

5. **Consult Exported Decompilations First (`build/decomp/`).**
   - All functions are batch-exported to `build/decomp/` (and indexed by segment, class/vtable, and subsystem).
   - Read the exported decompilation and cross-reference JSON directly instead of writing one-off headless Java Ghidra scripts for every question. Only run headless Ghidra scripts when updating the project database or performing bulk exports.

6. **Use Resource Types and Win16 Imports as Anchor Ground Truth.**
   - Resource types loaded from `SB_CDRES.DLL` (`0x83ee` BSW audio, `0x83f3` animation/sprites, `0x83ec` DIB bitmaps, `0x8403`/`0x8407`/`0x840c` game/level/map records) and Win16 imports (`WAVEOUT*`, `BITBLT`, `CREATEPALETTE`, `REGISTERCLASS`, `PEEKMESSAGE`) immediately anchor which subsystem a class or function belongs to.

## Critical files

| File | Role |
| --- | --- |
| `original/BLIZZARD.EXE` | Canonical immutable executable (SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`). |
| `original/SB_CDRES.DLL` | Canonical immutable CD resource library (SHA-256 `1c6c36bde51b8c9ad3b3a0eaba0283d0468d81326b3dbe4dbfbf7305863b70e1`). |
| `ghidra/local/SpellingBlizzard.gpr` | Working Ghidra 12.2 project for the executable. |
| `build/decomp/` | Batch-exported Ghidra C decompilations and call/reference index for all functions. |
| `build/method_tables.json` | Machine-extracted inventory of all 142 virtual method tables and constructor install chains. |
| `docs/generated-method-tables.md` | Human-readable vtable hierarchy, slot offsets, and constructor sites. |
| `docs/sb-cdres.md` | Resource inventory for `SB_CDRES.DLL` and the `BLIZZARD.EXE` resource-loading pipeline. |
| `docs/subsystem-map.md` | Classification of functions and classes into CRT, Framework, Audio, Graphics, Resource, and Game subsystems. |
| `src/` | Reconstructed C/C++ source code and Win16 shim. |

## Build and Verification Workflow

```text
original/BLIZZARD.EXE + original/SB_CDRES.DLL
        |
        v
Batch Ghidra Decompilation (build/decomp/) + Vtable Inventory (build/method_tables.json)
        |
        v
Generated Class/Vtable Headers + Subsystem Map (CRT / Framework / Game)
        |
        v
Clean C/C++ Decompilation + Thin Win16 API Shim (src/)
        |
        v
Automated Build, Unit/Subsystem Tests, & Runtime Verification
```

After completing each task or subsystem:
1. Verify `original/BLIZZARD.EXE` hash is untouched.
2. Ensure all CMake targets and tests build and pass cleanly.
3. Update progress documentation and commit changes with a clear summary.
