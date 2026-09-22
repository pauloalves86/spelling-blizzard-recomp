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

The current message flow is therefore:

```text
USER message APIs
    |
    v
1008:e412 / 1010:08bc message polling helpers
    |
    v
1010:3d56 generic message/key translator
    |
    v
object-relative callback or command dispatcher
    |
    +--> 1008:ac80 command/application handling
    +--> 1018:509c text-input/controller handling
```

This is a supported partial chain, not a claim that every path passes through both `1008:e412` and `1008:ac80`.
The current evidence does not justify naming `1008:e412` `main_loop`. It is better described as `framework_drain_message_ranges` until callers prove where idle/update work is performed.

## Native recovered seam

The first recovered source is:

- `src/recovered/framework/message_pump.h`
- `src/recovered/framework/message_pump.c`
- original address: `1008:e412`

It exposes an explicit callback table for `peek_message`, `translate_message`, and `dispatch_message`. This is a test seam, not an SDL layer and not a claim that the original Win16 ABI has been reproduced.

The native unit test verifies the recovered ordering for flags selecting the `0x0201`, `0x0207`, `0x0100`, and final `0x0200` passes.

## Open questions

- Which function invokes `1008:e412` as part of regular idle/update processing?
- Does the application run game updates between calls to `1008:e412`?
- What are the exact `wRemoveMsg` and HWND/filter arguments at each call site?
- Which object owns the callback reached by `1010:3d56`?
- Which messages correspond to game-specific input versus generic window handling?
