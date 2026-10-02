# Main Loop and Mouse-Input Path to `1008:ac80` (Step 15)

## Question

How does a user's mouse click reach the game command dispatcher `1008:ac80`? Answering it also settles where the application's outer loop is, which Step 13 left open.

## Method

- Raw disassembly: `objdump -D -b binary -m i8086 -M intel` over segment bytes extracted from `original/BLIZZARD.EXE`.
- Far-pointer resolution: `tools/ne_farptr.py`. Table data: `docs/generated-method-tables.md`.
- Imported API names: `ghidra/Step15Refs.java` (read-only). It reads the relocated far-call target from Ghidra's memory image, which works where Ghidra has not disassembled the code:

```text
<GHIDRA_HOME>/support/analyzeHeadless ghidra SpellingBlizzard -readOnly -noanalysis \
  -process BLIZZARD.EXE -scriptPath ghidra -postScript Step15Refs.java 1010:3d75 1010:3a3c 1010:3a43 …
```

The executable hash is unchanged (`ddf0a4c1…f3dce2`).

## 1. The main loop (fact)

```text
1000:001a NE entry -> 1000:00d8 -> 1008:2bf4
1008:2bf4: builds root object DAT_1060:07c4 (table 1008:2e32); at 1008:2d53 calls root->+0x84
root +0x84 = 1008:0694:
      self->+0x78()                 ; 1008:783a, parses the command-line string at self+0x20
      do { self->+0x7c() }          ; 1008:0596
      while (self->byte_1b != 0)    ; 1008:06ba: cmp byte [si+0x1b],0 / jne 06a9
root +0x7c = 1008:0596:
      self->+0x0e->+0x3c()          ; one event-loop iteration (1010:4270)
      if self->byte_1a: self->+0x16->+0x6c(…), ->+0x58(); self->byte_1a = 0
      1010:4aec()
```

Run-flag writers:
- `1008:011b`: the base constructor sets `+0x1b = 1`.
- `1008:0606` (root `+0x40`): clears it, then sets it back to 1 if any window, enumerated through `DAT_1060:0810->+0xd8`, refuses via its `+0x40`. This is a quit request the windows can veto (strong inference).
- `1008:8a36`: after `MessageBox(…, 0x2010)` (`MB_TASKMODAL | MB_ICONHAND`), sets `DAT_1060:07c4->+0x1b = 0`.

**Correction to `docs/message-loop-trace.md`.** Step 13 described `1008:0694` as looping "while the receiver byte at `+0x1b` is zero", and called it a startup polling loop "not … the ordinary game idle loop". The loop actually continues while the byte is **nonzero**, the byte is set to 1 at construction, and each iteration runs the event pump. **`1008:0694` is the application main loop** (strong inference; the loop structure itself is fact).

## 2. One loop iteration: event fetch and dispatch (fact)

The root's `+0x0e` object is created by root `+0x70` = `1008:77d0`, with table `1008:7f84`, which derives from `1008:0b84`.

| Slot | Target | Behaviour |
|---|---|---|
| `+0x3c` | `1010:4270` | calls `DAT_1060:0810->+0xa4`; `ok = self->+0x38(&rec)`; then `self->+0x0c(&rec)` if an event was fetched, otherwise `self->+0x18(&rec)` |
| `+0x38` | `1020:2534` | calls `1010:3d56` |
| `1010:3d56` | | `PeekMessage(&rec.msg, 0, 0, 0, PM_REMOVE)`; copies the `MSG` to `rec+0x10` (so `rec+0x12` = message, `rec+0x1e` = point); writes an event kind at `rec+0x00` |
| `+0x0c` | `1010:3a28` | `TranslateMessage`/`DispatchMessage(&rec.msg)`, so the Win16 window procedure still runs; then dispatches on `rec+0x00` |
| `+0x18` | `1010:3be2` | idle path: `DAT_1060:07c4->+0x5c(&rec)` (root `+0x5c` = `1008:7716`) |

Event kinds written by `1010:3d56` (`rec+0x00`) and their handlers in `1010:3a28`:

| Kind | Produced from | `1010:3a28` handler |
|---|---|---|
| 0 | anything not listed below | `+0x28` |
| 1 | `WM_LBUTTONDOWN`, `WM_LBUTTONDBLCLK`, `WM_RBUTTONDOWN`, `WM_RBUTTONDBLCLK`, `WM_MBUTTONDOWN`, `WM_MBUTTONDBLCLK` | `+0x20` |
| 2 | `WM_LBUTTONUP`, `WM_RBUTTONUP`, `WM_MBUTTONUP` | `+0x24` |
| 3 / 5 | `WM_KEYDOWN`/`WM_SYSKEYDOWN` (5 when lParam bit 30, the repeat flag, is set) | `+0x1c` |
| 4 | `WM_KEYUP`/`WM_SYSKEYUP` | `+0x1c` |
| 6 | `WM_PAINT` | `+0x34` |
| 8 | (jump-table entry; producer not traced) | `+0x10`/`+0x14` by `rec+0x0e` bit 0 |
| `0x0f` | `WM_ACTIVATE`, `WM_ACTIVATEAPP`, `0x406`, `0x41c` (`rec+0x02` = 1 if wParam ≠ 0); `WM_SETFOCUS`, `0x407` (`rec+0x02` = 1); `WM_KILLFOCUS`, `0x408` (`rec+0x02` = 0) | `1010:3b38` |
| ? | `WM_MOUSEMOVE` | branch `1010:3fa2` calls `1010:07d2` (at `1010:3fba`) to record the cursor; the resulting kind was not decoded |

**Strong inference.** These kind values match the Macintosh Toolbox `EventRecord.what` codes: 0 null, 1 mouseDown, 2 mouseUp, 3 keyDown, 4 keyUp, 5 autoKey, 6 update, 8 activate, 15 osEvt (suspend/resume). The framework presents a Mac-style event model on top of Win16.

**Hypothesis.** The application framework was ported from, or designed to share code with, a Macintosh version. This is not established by this binary.

## 3. Mouse-down to the clicked object (fact)

```text
kind 1 -> 1008:7f84 +0x20 = 1008:7c02 -> base 1010:3bfe
1010:3bfe: DAT_1060:0810->+0x50(&rec)          ; route from the root view
           then saves a per-button copy of rec in segment [ds:0x5de] (+0x00/+0x22/+0x44)
+0x50 = 1010:491e (base of the 1008:1aac view family):
           child = self->+0x70(rec.point)       ; hit test, base 1010:4a6e
           if child: return child->+0x50(&rec)  ; recurse
           DAT_1060:0a22 = self                 ; last mouse-down target
           self->+0xa4(); self->+0x88(&dx,&dy)  ; local-coordinate adjustment
           switch rec+0x12:
             0x204, 0x206 (right)  -> self->+0x60(…)
             0x207, 0x209 (middle) -> self->+0x5c(…)
             otherwise (left)      -> self->+0x58(…)   ; RETF 0xe arguments
```

Mouse-up (kind 2): `1008:7c5c` → base `1010:3c82` notifies the stored targets: `WM_LBUTTONUP` → `DAT_1060:0a22->+0x64`, `WM_RBUTTONUP` → `DAT_1060:0a2a->+0x6c`, `WM_MBUTTONUP` → `DAT_1060:0a26->+0x68`.

## 4. Click to command (fact, joining Step 14)

```text
1008:b0c8 child +0x58 = 1010:cd6c
    1008:e412(2)                       ; drain pending button messages
    self->+0x118(self->byte_55)        ; 1010:ce0a
1010:ce0a -> 1010:ce9f: self->+0x04->+0x1c((long)self->word_4c)
    +0x04 = the 1018:6144 object; its +0x1c = 1008:ac80
```

## Complete path

```text
NE entry 1000:001a -> 1008:2bf4 -> root+0x84 (1008:0694, main loop)
  -> root+0x7c (1008:0596) -> event object+0x3c (1010:4270)
  -> +0x38 PeekMessage/translate kind (1010:3d56)
  -> +0x0c Translate/DispatchMessage + kind switch (1010:3a28)
  -> kind 1: +0x20 (1008:7c02 -> 1010:3bfe) -> DAT_1060:0810->+0x50
  -> +0x50 hit-test recursion (1010:491e)
  -> clicked 1008:b0c8 object +0x58 (1010:cd6c) -> +0x118 (1010:ce0a)
  -> parent +0x1c = 1008:ac80
```

Every edge above is a direct instruction or a fixup-resolved table cell, so the static path is a fact. Two things are not established:

- **Runtime identity.** Which concrete object is at `DAT_1060:0810` at runtime, and that a given click's hit test lands on a particular `1008:b0c8` child, are static inferences. A runtime trace is needed to observe them.
- **The window-procedure mouse path.** The window procedure's `+0x0c` mouse method `1010:527c` does **not** feed this path. It only calls `ClientToScreen` and records the position (`1010:07d2`) for polling helpers such as `1010:07fa`.

## Open edges

1. **Resolved (`docs/root-view-lifecycle.md`).** `DAT_1060:0810` is the 0x30-byte Desktop Screen View (table `1008:7d10`), constructed by root slot `+0x6c` (`1008:7194`). Its slot `+0x50` is `1008:d410`, which delegates to the active front view `DAT_1060:01ca` (updated on window activation/focus).
2. **Resolved (`docs/framework-dispatch-edges.md`).** Root idle `+0x5c` (`1008:7716`) calls `root->+0x14` (`1008:7592`), which invokes `DAT_1060:0a2e->+0xc0()` (`1010:9644`) to update active game objects and sets `sleepTime = 0` for continuous animation. Per-object `+0x14` methods (e.g. `1010:ca3a`) poll timers and mouse states for controls.
3. **Resolved (`docs/framework-dispatch-edges.md`).** Event kinds 3/5 reach `+0x1c` (`1020:2552`), which intercepts Ctrl-Q via `root->+0x20` (`1008:769a`). View keystrokes dispatch via virtual slot `+0x20` (`KeyDown`), bubbling up parents (`1008:b802`) to the board view (`1018:3228`), whose `+0x20` override `1018:509c` accumulates characters in `DAT_1060:0a3c`, matches `"CHEAT"`, creates `DAT_1060:01e4`, raises slot-0 `+0x06` to at least 99 via `1018:ae3a(99)`, and emits command `0x2711`.
4. **Resolved (`docs/framework-dispatch-edges.md`).** Event kind 8 is Macintosh `activateEvt`. Entry `1010:3afa` remains in the framework jump table, but `1010:3d56` never produces it on Win16; it is an inert artifact of the Macintosh port.
