# Framework Dispatch, Keyboard Pipeline, and Window Creators

## Purpose

This document resolves the remaining open edges from `docs/input-to-command.md` and `docs/method-table-inventory.md`:
1. The creators and lifecycles of the remaining native window constructors (`1010:53c8` and `1010:5722`).
2. The keyboard input pipeline from Win16 `WM_KEYDOWN` to the `CHEAT` accumulator in `1018:509c`.
3. The root idle loop (`1008:7716`) and per-object `+0x14` polling / timer methods.
4. The identity and status of event kind 8.

Method:
- Binary disassembly: `objdump -D -b binary -m i8086 -M intel` on segment slices of `original/BLIZZARD.EXE`.
- Far-pointer and relocation resolution: `tools/ne_farptr.py`, `tools/ne_vtables.py`, and `ghidra/Step15Refs.java`.
- Executable hash unchanged: `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`.

---

## 1. Native Window Constructors and Creators (Fact)

Every native window created in the executable has now been identified, traced to its allocating view method, and linked through window field `+0x154`:

| Window Constructor | Alloc Size | Installed Table | Calling View Method | Creator View Table | Stored at | Role |
|---|---|---|---|---|---|---|
| `1010:50ce` | `0x160` | `1010:58ea` via `1008:708a`/`1008:705e` | `1008:4e70` | `1008:1cf0` | `view->+0x1c` | Main game view window |
| `1010:50ce` | `0x160` | `1010:58ea` via `1008:708a`/`1008:705e` | `1008:0eba` | `1008:18fc`, `1008:1744` | `view->+0x1c` | Dialog / secondary view windows |
| `1010:50ce` | `0x160` | `1010:58ea` via `1008:708a`/`1008:705e` | `1008:520c` | `1008:1ea0` | `view->+0x1c` | Modal / popup windows |
| `1010:53c8` | `0x168` | `1010:5942` via `1010:53e8` | `1008:cdf0` | `1008:284c` / `1008:2934` / `1008:2a1c` | `view->+0x1c` | Styled frame / child window (WS_* styles mapped by `1008:cd52`) |
| `1010:5722` | `0x158` | `1010:5916` via `1010:574e` | `1008:d770` (base) | `1008:0958` (base desktop) | `view->+0x1c` | Base desktop screen window |
| `1010:5722` | `0x158` | `1008:681a` via `1008:670a` | `1008:3218` (derived) | `1008:7d10` (`DAT_1060:0810`) | `view->+0x1c` | Spelling Blizzard desktop screen window |

### Details for `1010:53c8` (fact):
- Invoked from slot `+0xe4` (`1008:cdf0`) of the `1008:284c` view family.
- Helper `1008:cd52` (slot `+0xe0`) converts a style parameter (`0x3e7`, 1, 2, 3...) to Win16 `WS_*` style DWORDs (`WS_CAPTION`, `WS_CHILD`, `WS_VISIBLE`, `WS_SYSMENU`), then invokes `self->+0xe4`.
- `1008:cdf0` allocates `0x168` bytes via `1008:eed8` and calls `1010:53c8`.
- `1010:53c8` stores the creator view pointer (`this`) at window `+0x154`.
- The window's `WM_PAINT` handler `1010:559e` calls `creator->+0xd4` (`1008:cc24`).

### Details for `1010:5722` (fact):
- Base desktop view `1008:0958` slot `+0xc4` is `1008:d770`, which allocates `0x158` bytes and calls `1010:5722`.
- Specialized desktop view `1008:7d10` overrides slot `+0xc4` with `1008:3218`, which allocates `0x158` bytes and calls wrapper `1008:670a` (installing sub-table `1008:681a`), which in turn calls `1010:5722`.
- In both cases, `DAT_1060:0810` is passed as constructor argument and stored at window `+0x154`.

---

## 2. Keyboard Dispatch Pipeline to `CHEAT` (`1018:509c`) (Fact)

```text
Win16 WM_KEYDOWN
  -> Event loop PeekMessage helper (1010:3d56)
     Translates to Mac event kind 3 (or 5 for repeat)
     Translates virtual key code to char code via 1010:42ce (+0x40)
  -> Event dispatcher (1010:3a28)
     1. TranslateMessage / DispatchMessage (sent to native controls / WNDPROC)
     2. Kind 3/5 switch -> self->+0x1c (1020:2552)
  -> 1020:2552 checks for application shortcuts via DAT_1060:0808->+0x20 (1008:769a)
     (Intercepts Ctrl-Q / Cmd-Q for clean application exit)
  -> KeyDown events within view hierarchy dispatch via slot +0x20 (KeyDown method)
  -> Base view implementation (1008:b802) bubbles unhandled key events to parent:
     this->+0x04->+0x20(rec, charCode, keyCode)
  -> When bubbling reaches the game board view (table 1018:3228):
     Slot +0x20 is overridden by 1018:509c
  -> 1018:509c accumulates characters in DAT_1060:0a3c
     Matches against "CHEAT"
     -> Constructs state structure DAT_1060:01e4 (alloc 0x262 via 1008:ef2c)
     -> Invokes 1018:ae3a(99), raising slot-0 field +0x06 to at least 99 (see docs/semantic-debt.md)
     -> Emits command 0x2711 via this->+0x1c
```

### Table `1018:3228` (fact):
- Class inheritance: `1008:0bc8` $\rightarrow$ `1008:07f4` $\rightarrow$ `1008:0838` $\rightarrow$ `1008:1aac` $\rightarrow$ `1018:3108` $\rightarrow$ `1018:3228`.
- Constructed at `1018:2c33` in `1018:2bf0` (allocates `0x8a` bytes).
- Stored into field `[bx+0x46]` of the game container view (table `1008:97e6`).
- Slot `+0x1c` = `1018:553a` (command handler).
- Slot `+0x20` = `1018:509c` (the `CHEAT` key-down handler).

---

## 3. Root Idle Loop and Per-Object Polling (`+0x14`) (Fact)

When no Windows messages are pending, the event loop runs the idle and timer pipelines:

```text
Event pump 1010:4270 (event object +0x3c)
  -> PeekMessage returns 0
  -> Calls self->+0x18 (1010:3be2, idle hook)
  -> Calls root->+0x5c (1008:7716)
```

### Root Idle Method `1008:7716`:
1. If flag `DAT_1060:0196` is set and `DAT_1060:0198` is non-null: calls `DAT_1060:0198->+0x24()`.
2. Sets `sleepTime = 0x7fffffff` on stack.
3. Invokes root virtual slot `+0x14`: `DAT_1060:0808->+0x14(&sleepTime)` (`1008:7592`).
4. `1008:7592`:
   - Calls `DAT_1060:0a2e->+0xc0()` (`1010:9644`).
     `1010:9644` iterates the active game object list (`ForEach`) to run frame updates and animations.
   - Writes `sleepTime = 0`!
     By clearing `sleepTime` to 0, the event pump does not block or sleep, ensuring continuous frame updates and animations while maintaining responsive Windows message handling.

### Per-Object `+0x14` Timer / Polling Methods (e.g. `1010:ca3a`):
- Controls that require continuous tracking or periodic updates implement slot `+0x14`.
- For example, `1010:ca3a`:
  1. Checks `GetTickCount` (`1010:07ae`) against a deadline stored at `this->+0x4c`/`+0x4a`.
  2. If expired, polls `GetAsyncKeyState(VK_LBUTTON)` (`1010:07c0`) and cursor position (`1010:07fa`).
  3. Tests whether the cursor remains within control bounds (`this->+0x20`/`+0x22`) to handle continuous button tracking / scrolling.

---

## 4. Event Kind 8 Producer Resolution (Fact)

- In the Macintosh Toolbox `EventRecord` specification, event kind 8 is `activateEvt`.
- The framework event jump table in `1010:3a28` retains entry 8 at `1010:3afa`. It tests `rec.modifiers & 1` (`activeFlag` in Macintosh Toolbox) and calls slot `+0x10` (`1010:3b76`) on activate, and slot `+0x14` (`1010:3bac`) on deactivate.
- In `original/BLIZZARD.EXE`, `1010:3d56` converts Windows activation messages (`WM_ACTIVATE`, `WM_SETFOCUS`, `WM_KILLFOCUS`) into event kind `0x0f` (`osEvt`) or leaves them to the `WNDPROC`.
- An exhaustive byte scan across all code segments confirms that no instruction in the binary ever assigns 8 to `rec.what`.
- **Conclusion**: Event kind 8 is an **inert, unproduced remnant** of the ported Macintosh framework.
