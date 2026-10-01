# Phase 3 Main-Loop and Message Handling

## Confirmed message-pump helper

Ghidra identifies `1008:e412` as `FUN_1008_e412`. It repeatedly calls:

1. `USER!PEEKMESSAGE`
2. `USER!TRANSLATEMESSAGE`
3. `USER!DISPATCHMESSAGE`

The function drains multiple message ranges selected by a flags argument and always performs a final `0x0200` range pass. It does not itself contain the game's update logic or a blocking `GetMessage` loop.

Recovered control shape:

```text
FUN_1008_e412(flags)
  if flags & 0x02:
      drain 0x0201, 0x0207, 0x0204, 0x0203, 0x0209, 0x0206
  if flags & 0x04:
      drain 0x0202
      drain unfiltered/uncertain range
      drain unfiltered/uncertain range
  if flags & 0x08:
      drain 0x0100
  if flags & 0x10:
      drain 0x0101
  if flags & 0x40:
      drain 0x000f
  drain 0x0200
```

The repeated `PEEKMESSAGE` calls in Ghidra's decompilation carry segmented-pointer and Win16 argument artifacts. The six exact single-message values above are directly visible in the decompiled control flow. The two `0x04` subloops whose filters decompile as no explicit range are intentionally marked uncertain in the native recovery.

## Related event processing

`1010:07fa` drains `0x0200` messages and then reads a global/object coordinate, optionally adds `GETWINDOWORGEX` output, and returns a 32-bit coordinate-like value. This appears to be a framework polling/coordinate helper, not the top-level application loop.

`1010:08bc` checks for a `0x0202` message with `PEEKMESSAGE` and returns whether none was present.

`1010:3d56` receives an object-like pointer and message-state structure. It polls a message when the object's state byte is clear, copies the `MSG`-like record, switches over the message value, calls an object callback through a vtable-like table, and combines modifier/key state from `GETKEYSTATE`. It handles keyboard messages (`0x100`, `0x101`, `0x104`, `0x105`) and mouse/window ranges (`0x200`-`0x209`, among others).

## Application-facing dispatcher

`1008:ac80` is now the strongest application/controller candidate. It handles command-like values `0x2af9`, `0x2afa`, `0x2afb`, `0x32c9`, and `0x32ca`; it invokes game-segment routines and object methods, and it uses `1010:08bc` to poll for pending `0x0202` messages in two paths. It is not yet proven to be the outermost message loop.

The caller trace found one direct application path into the message-draining helper:

```text
1008:ac80 --(1008:ad79)--> 1018:5496 --(1018:54db)--> 1008:e412
```

Ghidra found 22 direct call sites to `1008:e412` across 13 caller functions. Only `1018:5496` has a direct caller in the current graph (`1008:ac80`); the other 12 callers have no direct caller there, consistent with but not proof of indirect callback dispatch. `1008:ac80` itself has no direct code-reference caller. See `docs/message-loop-trace.md` for the complete caller list and the read-only analysis procedure.

Step 14 update: the indirect entry into `1008:ac80` is now traced. It is slot `+0x1c` of table `1018:6144`, called from `1010:ce9f` (`docs/ac80-indirect-caller.md`).

The USER window path is now partially recovered separately:

```text
1008:e412 --PEEKMESSAGE / TRANSLATEMESSAGE / DISPATCHMESSAGE--> USER
    - - - Win16 dispatch semantics - - -> WNDPROC 1010:5db2
                                          WM_PAINT -> window object's +0x10 method
                                                    -> nested object callback
```

`1010:5db2` is the `lpfnWndProc` value written by `1010:5bd8` into the `WNDCLASS`-like record. Ghidra has no function boundary at that target; raw 16-bit disassembly shows `GETWINDOWLONG`/`SETWINDOWLONG`, `BEGINPAINT`/`ENDPAINT`, and object-relative dispatch. The paint callbacks reach per-window object methods; static tables then lead toward application-root methods. The runtime owner pointer at window `+0x154` is not yet proven, so the full paint-to-root chain remains conditional.

Startup also reaches `1008:2bf4` directly via `1000:00d8`. The initialized root table at `1008:2e32` has a polling loop through slots `+0x78/+0x7c` inside `1008:0694` and paint/update callbacks at `+0x48/+0x4c`. This is a confirmed shared object-table surface, but the `1008:0694` loop is observed during initialization and is not evidence of the ordinary game idle loop. The caller trace does not show `1008:e412` called directly from these root-table slots or from the WNDPROC.

`1010:3d56` is a separate message/key polling helper, not a proven callee of `1008:e412`; its direct caller is `1020:2534`. Do not merge these paths solely because both use USER message APIs.

The evidence still does not justify naming `1008:e412` `main_loop`. It remains a message-draining helper. The outer loop, ordinary idle work, and the runtime owner joining the WNDPROC paint callbacks to the `1008:e412` callers remain unresolved.

## Native recovered seam

The first recovered source is:

- `src/recovered/framework/message_pump.h`
- `src/recovered/framework/message_pump.c`
- original address: `1008:e412`

It exposes an explicit callback table for `peek_message`, `translate_message`, and `dispatch_message`. This is a test seam, not an SDL layer and not a claim that the original Win16 ABI has been reproduced.

The native unit test verifies the recovered ordering for flags selecting the `0x0201`, `0x0207`, `0x0100`, and final `0x0200` passes.

## Open questions

- Which indirect dispatch paths reach the 12 direct callers of `1008:e412` that have no direct callers in the current graph?
- Does the window's runtime owner pointer at `+0x154` equal the startup-created `DAT_1060:07c8` object?
- Is the `1008:0694` polling loop initialization-only, or does a related object method participate in ordinary idle work?
- Does the application run game updates between calls to `1008:e412`?
- What are the exact `wRemoveMsg` and HWND/filter arguments at each call site?
- Which object owns the callback reached by `1010:3d56`?
- Which messages correspond to game-specific input versus generic window handling?
