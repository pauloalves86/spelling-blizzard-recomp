# Phase 3 Application Entry Path

Status: initial Phase 3 recovery. Claims are labeled Fact, Strong inference, Hypothesis, or Unknown. The canonical executable was not modified.

## Evidence-producing analysis

Ghidra 12.1.3 headless analysis was rerun on `original/BLIZZARD.EXE` using `ApplicationReport.java` and `TargetedDecomp.java` in `ghidra/`. The reports are derived from the existing `ghidra/SpellingBlizzard` project.

## Current call-flow

```text
NE entry 1000:001a (entry)
  |
  +-- KERNEL INITTASK / LOCKSEGMENT / GETVERSION / DOS3CALL-or-INT21 / WAITEVENT
  +-- USER INITAPP
  |
  +-- 1000:0104 FUN_1000_0104
  |     runtime/C startup state; references _C_FILE_INFO=
  |
  +-- 1000:0346 FUN_1000_0346
  |     GETMODULEFILENAME; command-line/module setup
  |
  +-- 1000:04f0 FUN_1000_04f0
  |     GETDOSENVIRONMENT; environment/argument setup
  |
  +-- 1000:00d8 -> 1008:2bf4
  |     direct startup-to-object initialization path
  |
  +-- 1000:2026
        separate startup helper (entry call at 1000:00ac); not a direct caller of 1008:2bf4
  |
  +-- 1000:01e7
  +-- 1000:05b4
        FATALAPPEXIT/FATALEXIT path; likely error termination

Later Win16 framework anchors:
  1008:e412  message-filter pump helper
  1010:596e / 1010:5b18  CWindow-like class registration/window creation
  1010:3d56  message normalization/keyboard-state handling
  1010:07fa / 1010:08bc  message polling helpers

Likely application-specific anchors:
  1008:ac80  command/application dispatcher
  1018:509c  text-input/controller handler with CHEAT and game-object callbacks
  1018:649a  nested-object construction/initialization candidate
```

## Classification

### RUNTIME / startup

`1000:001a` is confirmed startup/runtime based on the NE entry point and its use of `INITTASK`, `LOCKSEGMENT`, `GETVERSION`, `WAITEVENT`, and `INITAPP`. `1000:0104` references `_C_FILE_INFO=` and performs startup environment work. `1000:0346` parses the startup/module argument area after calling `GETMODULEFILENAME`. `1000:04f0` uses `GETDOSENVIRONMENT` and constructs environment/argument records.

Confidence: High for startup/runtime classification.

### FRAMEWORK / Win16 window layer

`1010:596e` and `1010:5b18` call `GETCLASSINFO`, conditionally `REGISTERCLASS`, `CREATEWINDOW`, and `GETDC`. They initialize a large object whose first words are function-table pointers and whose fields include window/class/DC-related values. This is strong evidence for a generic CWindow-like framework layer, not game logic itself.

Confidence: High for framework classification; exact class hierarchy unknown.

`1010:3d56` normalizes Windows messages, calls `GETKEYSTATE`, and dispatches through a function pointer at an object-relative vtable offset. It handles message values including `0x100`, `0x101`, `0x104`, `0x105`, `0x200`-`0x209`, and `0x406`-`0x41c` paths. This is a framework/window event translator with an application callback boundary.

Confidence: High for generic event-dispatch role; exact message constants need Win16 header verification.

### APPLICATION / game boundary

`1018:509c` receives an object pointer plus a character/byte input (`param_4` in Ghidra's output), calls the message pump helper for selected input cases, examines object-relative fields, checks the literal `CHEAT`, allocates or initializes an object through `FUN_1008_ef2c(0x262)`, and invokes functions at `1018:a9a0`, `1018:ae3a`, and `1018:aeaa`. It also dispatches through a function pointer in the first parameter's object table.

The reference report confirms direct callers of the related routines: `1018:509c` calls `1018:a9a0`, `1018:ae3a`, and `1010:d0a6`; `1018:ae3a` is also called by `1018:267e`; `1018:aeaa` is called by `1018:3c70` and `1018:509c`. `1018:509c` itself has no direct code-reference caller in the current Ghidra reference graph, so its entry may be reached through a function pointer/table or data-driven dispatch.

Strong inference: this is application-specific input/control code, or a game-facing controller layered on the framework. The `CHEAT` reference and object-specific callbacks make it a better game-code candidate than the generic Win16 functions.

Confidence: Medium. The exact object and state semantics remain unknown.

## Application/controller layer

`1008:2bf4` is the clearest current framework/application initialization boundary. It allocates several objects, writes method-table offset/segment pairs, stores them in global pointers (`DAT_1060_0a14`, `DAT_1060_0a1e`, and `DAT_1060_07c4`), invokes initialization methods, and returns an object field from `DAT_1060_07c4`. The direct startup path is `1000:001a` -> `1000:00d8` -> `1008:2bf4`; `1000:2026` is a separate direct callee of the entry function, not a direct caller of `1008:2bf4`. The object initialization then proceeds through method-table calls, including the `1008:0694` polling path and `1008:72b6` global-object setup.

`1008:ac80` is an application-facing dispatcher. It handles command-like values `0x2af9`, `0x2afa`, `0x2afb`, `0x32c9`, and `0x32ca`. The `0x2af9` path constructs/initializes `DAT_1060_01e4`, calls `1018:a9a0` and `1018:aeaa`, updates `DAT_1060:01de`, clears `DAT_1060:0a36`, calls `1010:d0a6`, and then invokes an object callback. The `0x32c9` and `0x32ca` paths repeatedly invoke object methods while polling `1010:08bc`.

`1018:509c` is a separate application input/controller candidate. It receives a byte input, appends it to `DAT_1060:0a3c` while the buffer is shorter than five characters, and compares the buffer with `CHEAT`. The equality path constructs/initializes the global object at `DAT_1060:01e4`, then calls `1018:a9a0`, `1018:ae3a` with `99`, `1018:aeaa`, and `1010:d0a6`.

This establishes an application/control layer but does not prove that `1008:ac80` is the top-level application entry or that `1018:509c` is reached directly from the USER window procedure.

## Phase 5 first reconstruction

`1008:ac80` is now documented as a command dispatcher with five numeric branches. Its `0x2af9` branch is the first meaningful state-transition candidate because it allocates/replaces `DAT_1060:01e4`, initializes it, reads nested field `+0x06`, updates `DAT_1060:01de`, clears `DAT_1060:0a36`, and invokes a UI/framework helper. The external source of `param_3` remains unresolved.

`1018:aeaa` has a native unit-tested reconstruction and is not yet a differential test against the original executable.

## Window construction edge

Direct callers of the generic constructors are documented in `docs/application-windows.md`. The shared class-data helper `1010:5bd8` writes the concrete `lpfnWndProc` far pointer `1010:5db2` into the WNDCLASS-like structure at `object + 0x10a` (offset and segment at `+0x10c/+0x10e`). Ghidra has no function boundary at the target; raw disassembly shows its window-message dispatch behavior. The runtime owner pointer at window `+0x154` and the full paint-to-application-owner path remain unresolved.

Direct string references for `SpellingDemonsWindow`, `SpellingDemonsDesktop`, `CEditText`, `CAMEditText`, `CWindow`, and `CDesktop` were not recovered by Ghidra's reference manager. Their presence is fact, but they may be debug/class metadata, copied name tables, or strings used through indirect data structures.

The strongest executable evidence for actual window construction is instead:

- `1010:596e` -> `GETCLASSINFO`, `REGISTERCLASS`, `CREATEWINDOW`, `GETDC`.
- `1010:5b18` -> same sequence.
- `1010:596e` stores the returned window handle and DC into the object.
- The object contains a class-name-related pointer at a large positive offset and callback/vtable pointers at the beginning.

The application-specific window instance that supplies these objects has not yet been connected conclusively to a single constructor. The direct-call graph from `1008:2bf4` does not reach `1010:596e`, `1010:5b18`, `1008:ac80`, or `1010:3d56`; those paths remain behind indirect object/table dispatch.

## Updated application flow

```text
Win16 entry 1000:001a
    |
    +--> 1000:00d8 -> 1008:2bf4 object/global initialization
    |                    |
    |                    +--> indirect initialization methods
    |                    +--> root table 1008:2e32
    |                         +--> 1008:0694 startup polling loop
    |                         +--> 1008:72b6 creates DAT_1060:07c8 / table 1008:7edc
    |
    +--> 1000:2026 separate startup helper

Later, through unresolved object/table edges:
    generic window constructors 1010:596e / 1010:5b18
        -> REGISTERCLASS / CREATEWINDOW / GETDC
        -> lpfnWndProc 1010:5db2
        -> WM_PAINT -> per-window paint method -> nested callback path

    application dispatcher 1008:ac80
        -> 1018:5496 -> 1008:e412 message drain

    message/key helper 1010:3d56
        <- direct caller 1020:2534
```

The direct startup edges and registered callback target are now known. The incoming callback/table edges connecting the window owner, `1008:ac80`, and the `1008:e412` callers—and the outer loop—remain unresolved. See `docs/message-loop-trace.md` for the detailed evidence and confidence boundaries.
