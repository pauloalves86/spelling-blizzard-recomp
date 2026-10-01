# Phase 3 Caller and Window-Procedure Trace

## Evidence and method

This trace uses Ghidra 12.1.3's existing `SpellingBlizzard` project and the read-only script `ghidra/MessageLoopTrace.java`. It was run with `-readOnly -noanalysis`; the report reads caller references, selected vtable cells, instruction windows, and decompilation without changing the Ghidra database. The unbounded callback at `1010:5db2` was decoded from bytes exported by that script with GNU `objdump` in 16-bit mode; no function boundary was added to the project.

Reproduce the Ghidra query with:

```text
<GHIDRA_HOME>/support/analyzeHeadless ghidra SpellingBlizzard \
  -readOnly -noanalysis -process BLIZZARD.EXE \
  -scriptPath ghidra -postScript MessageLoopTrace.java
```

The raw callback disassembly uses `objdump -D -b binary -m i8086 -M intel --adjust-vma=0x5db2` on the exported byte range; this is a derived analysis artifact, not an edit to the executable.

The executable remains `original/BLIZZARD.EXE`, the behavioral authority. Its SHA-256 must remain `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`.

## Direct callers of `1008:e412`

Ghidra reports 22 direct code-reference call sites in 13 distinct containing functions:

| Caller | Call site(s) |
|---|---|
| `1008:10dc` | `1008:1102` |
| `1008:16ce` | `1008:16ed` |
| `1010:9a6a` | `1010:9a73` |
| `1010:cd6c` | `1010:cd75` |
| `1018:284c` | `1018:2adf` |
| `1018:2af2` | `1018:2bdc` |
| `1018:2f20` | `1018:2f87` |
| `1018:509c` | `1018:50f9`, `5141`, `517d`, `519b`, `536f`, `5391` |
| `1018:5496` | `1018:54db` |
| `1018:9fe6` | `1018:a025`, `a05b` |
| `1020:51f4` | `1020:522d`, `5255`, `5273`, `52b9` |
| `1020:59cc` | `1020:59f6` |
| `1020:6616` | `1020:6637` |

Only one of these callers has a direct caller in the current code-reference graph: `1008:ac80` calls `1018:5496` at `1008:ad79`, and `1018:5496` calls `1008:e412` at `1018:54db` with flag `0x10`. `1008:ac80` itself has no direct code-reference caller. The other 12 `1008:e412` callers have no direct caller in the current Ghidra graph; that is consistent with indirect callback dispatch but does not prove how they are reached.

`1008:e412` directly uses `PEEKMESSAGE`, `TRANSLATEMESSAGE`, and `DISPATCHMESSAGE` to drain selected message ranges. It is not itself the application's outer loop. No direct path from the NE entry to `1008:e412` appears in the resolved direct-call graph.

## USER message API call sites

The message polling/dispatch APIs occur in four code areas: `1008:e412`, `1010:07fa`, `1010:08bc`, and `1010:3d56`.

- `1010:07fa` polls `0x0200` messages and reads a coordinate-like value; its role remains a framework helper, not a proven main loop.
- `1010:08bc` polls for `0x0202`; the direct calls found are inside command paths in `1008:ac80`.
- `1010:3d56` is a separate message/key translator using `PEEKMESSAGE`, `TRANSLATEMESSAGE`, `DISPATCHMESSAGE`, and `GETKEYSTATE`; its direct caller is `1020:2534`. It dispatches through a receiver-relative callback. It is not a direct callee of `1008:e412`.
- `POSTMESSAGE` is called in `1010:519a`, whose direct callers include `1008:6682` and `1008:6792`.
- `INVALIDATERECT` is called in `1010:1cde`; `VALIDATERECT` is called in `1008:e286` and `1008:9222`.
- `REGISTERCLASS` and `CREATEWINDOW` are called by `1010:596e` and `1010:5b18`.

The direct startup path is `1000:001a` -> `1000:00d8` (call at `1000:00c3`) -> `1008:2bf4` (call at `1000:00f6`). The direct-call graph does not reach `1010:596e`, `1010:5b18`, `1008:e412`, `1010:07fa`, `1010:08bc`, `1010:3d56`, or `1008:ac80` from the entry. Window creation and these message helpers are reached through additional callback/table paths that are not yet fully resolved.

## Registered Win16 window procedure

`1010:596e` and `1010:5b18` call the object's virtual class-data initializer at vtable slot `+0x14`, then register the `WNDCLASS`-like data at `object + 0x10a`. The shared initializer `1010:5bd8` writes:

- style at `object + 0x10a`;
- `lpfnWndProc` at `object + 0x10c` as the explicit far pointer `1010:5db2`;
- the class-name pointer at `object + 0x120`.

`1010:5348` and `1010:5824` call `1010:5bd8`; `1008:6fa0` calls `1010:5348`. Other table `+0x14` entries in the constructor paths lack Ghidra function boundaries, so this does not prove every registered class uses the same initializer.

Ghidra has no function boundary at `1010:5db2`. Raw 16-bit disassembly of the pointer target shows a window-procedure-shaped handler that:

- retrieves the object pointer with `GETWINDOWLONG(hwnd, 0)` and uses `DEFWINDOWPROC` when no object is attached;
- on message `0x0001`, reads the creation parameter, stores the object pointer with `SETWINDOWLONG(hwnd, 0, object)`, writes the HWND at object offset `+0x04`, and returns zero;
- on `0x000e`, calls `BEGINPAINT`, the object's `+0x10` method, and `ENDPAINT`;
- maps messages in the `0x0200`-`0x0209` range to the object's `+0x0c` method with small event values;
- routes other messages with an attached object through its `+0x04` method; and
- calls `POSTQUITMESSAGE(0)` for message `0x0010`.

The `+0x0c` methods identified in the constructor tables at `1008:0f80`, `1008:681a`, `1008:705e`, `1010:58ea`, `1010:5942`, and `1010:5916` are `1010:527c` or `1010:553e`. The decoded bodies translate coordinates and update cursor-related state. Paint slots vary: `1008:0f80`/`1008:705e` use `1008:6dfc`, `1010:5942` uses `1010:559e`, and several other listed tables point to the `RETF 8` stub at `1010:5d62`. The common `+0x18` post-create table target is `1010:5d66`; its raw bytes call `SHOWWINDOW`.

## Startup polling and paint/update dispatch

`1008:2bf4` installs the table at `1008:2e32` on global object `DAT_1060:07c4`. Its observed initialization calls include:

- slot `+0x98` -> `1008:72b6`, which initializes global `DAT_1060:07c8` with table `1008:7edc`;
- slot `+0x84` -> `1008:0694`, whose raw control flow loops while the receiver byte at `+0x1b` is zero, calling that receiver's `+0x78` and `+0x7c` methods (`1008:783a` and `1008:0596`); and
- slot `+0x58` -> `1008:76cc`.

This is an observed startup polling/synchronization loop, not evidence of the ordinary game idle loop. Its exact purpose and the downstream behavior of the unbounded `1008:783a`/`1008:0596` entries remain uncertain.

Step 15 correction (`docs/input-to-command.md`). The loop condition is inverted above: `1008:06ba` is `cmp byte [si+0x1b],0 / jne`, so the loop continues while the byte is **nonzero**. `1008:011b` sets it to 1 at construction, and `+0x7c` (`1008:0596`) runs one event-pump iteration. `1008:0694` is the application main loop.

A paint/update callback path is also visible in static table data:

Step 15 correction (`docs/method-table-inventory.md`). `1008:7edc` has only 12 slots, so the `+0xd4`/`+0xdc` cells listed in item 1 below lie in table `1008:7f84` (its `+0x2c` and `+0x34`), the event dispatcher, not in `1008:7edc`. The window `+0x154` field is a back-pointer to the view whose `+0xc4` created the window. Those views' own `+0xd4`/`+0xdc` are the paint callbacks.

1. The `1008:7edc` table has slot `+0xd4` -> `1010:3d32` and slot `+0xdc` -> `1010:3d52`.
2. Raw bytes at `1010:3d32` load global `DAT_1060:07c4` and call its table slots `+0x48` and `+0x4c`.
3. `DAT_1060:07c4` table `1008:2e32` has `+0x48` -> `1008:7b9a` and `+0x4c` -> `1008:7bce`. These methods call `1008:0684`/`1008:06ec` and conditionally invoke methods on the object at `DAT_1060:0198` when flag `DAT_1060:07fe` is set.
4. `1010:3d52` is a `RETF 8` stub; it is distinct from the function entry `1010:3d56`.

`1008:6dfc` and `1010:559e` obtain a far pointer from their window object's `+0x154` field and dispatch through its table at `+0xd4` or `+0xdc`. The constructors set that window field from an owner/creation parameter, but the current trace does not prove that the runtime pointer is `DAT_1060:07c8` (table `1008:7edc`). Thus the static paint-to-root callback chain is a strong candidate junction, not yet a complete runtime owner chain.

## Current conclusion and next edge

The evidence now joins Win16 `WM_PAINT` dispatch to per-window paint methods and identifies application-root callbacks at `1008:2e32 +0x48/+0x4c`. Separately, startup enters a short receiver-flag polling loop through `1008:0694` and slots `+0x78/+0x7c` on the same root table. This is the strongest current meeting point for startup work and application paint/update callbacks.

The trace does not show `1008:e412` being called from the window procedure, the root object's `+0x48/+0x4c` paint callbacks, or the root object's `+0x78/+0x7c` polling callbacks. The only resolved direct caller chain into `1008:e412` is `1008:ac80` -> `1018:5496` -> `1008:e412`; the incoming callback/table that reaches `1008:ac80` is unknown. Therefore the outer message/idle/update loop and a complete join to `1008:e412` remain unidentified.

Next, trace the runtime value installed at `window object +0x154`, the table owner behind the `+0xd4/+0xdc` callbacks, and the indirect caller of `1008:ac80`. Preserve the distinction between these confirmed table edges and the still-unproven owner/outer-loop relationship.

Step 14 update: the indirect caller of `1008:ac80` is resolved in `docs/ac80-indirect-caller.md`. `1008:ac80` is slot `+0x1c` of table `1018:6144`, reached from `1010:ce9f`. Note also that `1008:ac80` calls `1018:5496` with `receiver->+0x08` (the owner object), so the `1008:ac80 -> 1018:5496 -> 1008:e412` chain runs on the owner.