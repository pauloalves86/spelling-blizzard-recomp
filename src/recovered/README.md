# Recovered source

This directory contains derived source representations, not original source code. The executable and Ghidra project remain the source of truth.

Current recovery:

- `framework/message_pump.c` — small native seam derived from Ghidra function `1008:e412`.
- `application/object_model.h` / `.c` — canonical Win16-layout recovery for the
  parent/holder/nested structures, explicit far pointers, final table window,
  and recovered `1018:aeaa`, `1018:ae3a`, `1018:a704`, and `1018:adb8` behavior.
- `application/application_state.c` — isolated `1008:ad57..ad60` global writes
  over the same canonical object model.

The object-model resolver is a test seam, not a flat-pointer conversion or a
Win16 compatibility layer. See `docs/native-object-model-inventory.md` and
`docs/native-structural-comparison.md` for evidence and boundaries.

The implementation preserves the observed message-range ordering but abstracts Win16 APIs behind callbacks for Linux tests. It does not claim to be a complete Win16 compatibility layer or the game's main loop.
