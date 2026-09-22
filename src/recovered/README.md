# Recovered source

This directory contains derived source representations, not original source code. The executable and Ghidra project remain the source of truth.

Current recovery:

- `framework/message_pump.c` — small native seam derived from Ghidra function `1008:e412`.

The implementation preserves the observed message-range ordering but abstracts Win16 APIs behind callbacks for Linux tests. It does not claim to be a complete Win16 compatibility layer or the game's main loop.
