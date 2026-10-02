# Initial Architecture Map

Status: Phase 1/2 mapping only. Names marked `FUN_...` are Ghidra-generated names and have not been mass-renamed. Interpretations are labeled Fact, Strong inference, or Hypothesis.

## Current diagram

```text
Win16 NE executable (MZ compatibility stub)
|
+- MZ Windows-only stub
|  +- prints "This program requires Microsoft Windows."
|  `- exits through DOS INT 21h
|
+- Win16 startup/runtime [Fact]
|  +- NE entry 1000:001a
|  +- KERNEL INITTASK / LOCKSEGMENT / GETVERSION / WAITEVENT
|  +- USER INITAPP
|  +- runtime argument/environment setup
|  `- error/termination handling
|
+- application/framework initialization [Strong inference]
|  +- startup functions in segment 1000
|  +- module/path and command-line processing
|  +- global-memory allocation and resource loading
|  +- custom C++-style object/vtable framework
|  `- configuration/preferences handling
|
+- application code [Strong inference]
|  +- spelling-demon/game object names and grids
|  +- window/desktop/edit-text objects
|  +- layered/offscreen rendering lists
|  +- score/high-score and match state
|  `- game data and object processing
|
+- Windows UI/message interface [Fact]
|  +- USER window creation/class/message APIs
|  +- PEEKMESSAGE, TRANSLATEMESSAGE, DISPATCHMESSAGE imports
|  +- GDI text, bitmap, blit, palette, DC and font APIs
|
+- audio [Fact]
|  +- MMSYSTEM waveOut device enumeration/capabilities
|  +- asynchronous waveOut open/prepare/write/reset/close
|  `- TIMEGETTIME timing API
|
+- filesystem/configuration [Fact]
|  +- KERNEL OPENFILE, _HREAD, _HWRITE, DOS3CALL
|  +- profile APIs and BRGHTSTR.INI string
|  +- external SB_CDRES.DLL and temporary-file strings
|
+- embedded resources/data [Fact]
   +- VERSIONINFO and textual resources
   +- initialized data and resource blocks
```

## Entry and startup path

Entry address: `1000:001a` (NE initial CS:IP segment 1, offset `0x1a`). Ghidra names this function `entry` and reports 189 address bytes, 14 calls, and no callers.

Fact: the first operations include `KERNEL!INITTASK`, `LOCKSEGMENT`, `GETVERSION`, a DOS environment/version path (`DOS3CALL` or an `INT 21h` thunk depending on a runtime flag), `WAITEVENT`, and `USER!INITAPP`.

Fact: after successful `INITAPP`, the entry path calls `FUN_1000_0104`, `FUN_1000_0346`, `FUN_1000_04f0`, `FUN_1000_2026`, a wrapper at `FUN_1000_00d8`, `FUN_1000_01e7`, and `FUN_1000_05b4`.

Strong inference: `1000:001a` is compiler/Windows startup and not the game's main loop. `FUN_1000_05b4` is a fatal-termination/error path because its decompilation calls `FATALAPPEXIT` and `FATALEXIT`; it is not treated as normal game shutdown without further tracing.

Hypothesis: the handoff from generic startup to the application framework occurs among `FUN_1000_0104`, `FUN_1000_0346`, and `FUN_1000_04f0`. Exact application-main ownership remains unresolved.

## Function inventory

Initial Ghidra auto-analysis identified 959 functions. After systematic virtual method table analysis across all 142 detected method tables (`tools/ne_vtables.py`), 1,088 unique virtual method entry points were exported as seeds (`ghidra/CreateSeeds.java`), resolving 595 previously unanalyzed functions. The program now contains 1,684 discovered functions across the 5 code segments, imported Win16 thunks, and runtime/framework code.

The following are useful anchors, not definitive semantic names:

| Address | Ghidra name | Proposed role | Evidence | Confidence |
|---|---|---|---|---|
| `1000:001a` | `entry` | Win16/CRT startup | NE entry; calls INITTASK, INITAPP, setup routines | High |
| `1000:0104` | `FUN_1000_0104` | startup/runtime environment and C runtime setup | handles DOS/Windows startup state and `_C_FILE_INFO=` | Medium |
| `1000:0346` | `FUN_1000_0346` | command-line/module initialization | calls GETMODULEFILENAME and parses startup arguments | Medium |
| `1000:04f0` | `FUN_1000_04f0` | environment/argument setup | calls GETDOSENVIRONMENT and builds argument structures | Medium |
| `1000:05b4` | `FUN_1000_05b4` | fatal error/termination handler | calls FATALAPPEXIT and FATALEXIT | High |
| `1000:071a` | `FUN_1000_071a` | path/file-related helper | manipulates path strings and calls file helpers | Low/medium |
| `1000:185c` | `FUN_1000_185c` | DOS compatibility/file operation helper | uses DOS3CALL/INT 21h path and copies result state | Medium |
| `1008:82b8` | `FUN_1008_82b8` | executable/configuration path discovery | calls GETMODULEFILENAME; decompilation has multiple path buffers | Medium |
| `1010:0000` | string/data region | runtime/framework text | contains subclass-responsibility message | High for data, no function role |
| `1010:5fe4` | string region | object/framework metadata | `WinClass`, `theObject`, `theMissile`, demon grid names | High for data |
| `1018:5cb6` | string region | cheat/configuration-related data | `CHEAT`, layered/offscreen/list names | Medium |
| `1018:b218` | string region | score/match-related data | `HighScor`, `theMatch` | Medium |
| `1020:10d8` | `FUN_1020_10d8` | object/game framework routine | large object-oriented call graph; function alone is not enough to assign loop role | Low/medium |
| `10a0:*` | imported thunks | Win16 API boundary | imports resolved by Ghidra symbol data | High |

No function is currently labeled `game_main_loop`: the available evidence supports a Windows message-driven application, but exact update/render dispatch and loop ownership need targeted call-graph tracing.

## Main loop evidence

Fact: the import table includes `USER!PEEKMESSAGE`, `TRANSLATEMESSAGE`, `DISPATCHMESSAGE`, `POSTMESSAGE`, `INVALIDATERECT`, and `VALIDATERECT`. It also includes `GETASYNCKEYSTATE` and `GETKEYSTATE`.

Strong inference: the application uses a Win16 message pump and window-procedure-driven input/UI rather than a DOS BIOS keyboard loop. A likely loop has the form “peek/process Windows messages, dispatch them, and run application work while idle,” but its exact function and timing behavior are not yet proven.

Fact: imported graphics interfaces include `BITBLT`, `STRETCHBLT`, `TEXTOUT`, `CREATEBITMAP`, `CREATECOMPATIBLEDC`, `GETBITMAPBITS`, `SETBITMAPBITS`, `CREATEPALETTE`, `SELECTPALETTE`, `REALIZEPALETTE`, and `ANIMATEPALETTE`.

Hypothesis: the names `itsLayeredOffscreen`, `itsLayeredObjectList`, `itsForeRedrawList`, and `itsBackRedrawList` indicate a layered/offscreen renderer, but this needs confirmation from callers and structure layouts.

## External interfaces

DOS/runtime: KERNEL `DOS3CALL`, `OPENFILE`, `_HREAD`, `_HWRITE`, `GETDOSENVIRONMENT`, profile APIs, and recognized `INT 21h` instructions. The executable also has a Windows-only MZ stub.

Graphics: Win16 GDI and USER APIs, including bitmap/DC/font/text/blit/palette operations. No application BIOS video `INT 10h` was recognized by Ghidra.

Input: USER keyboard-state APIs and Win16 message APIs; a `KEYBOARD` imported ordinal is present but not identified semantically.

Audio: MMSYSTEM `WAVEOUTGETNUMDEVS`, `WAVEOUTGETDEVCAPS`, `WAVEOUTOPEN`, `WAVEOUTPREPAREHEADER`, `WAVEOUTWRITE`, `WAVEOUTRESET`, `WAVEOUTUNPREPAREHEADER`, `WAVEOUTCLOSE`, and `TIMEGETTIME`. This is strong evidence for Windows wave output, not direct Sound Blaster/OPL programming.

## Assets and external data

Fact: the executable contains VERSIONINFO, textual resources, initialized data, and multiple resource blocks.

Fact: strings name `BRGHTSTR.INI`, `_DEMONS_.TMP`, and `SB_CDRES.DLL`.

Hypothesis: `SB_CDRES.DLL` likely supplies CD/resource content, but the binary alone does not establish what it exports or whether it is mandatory for every execution path. The application is therefore not safely considered self-contained.

## Major uncertainties

- Exact application-main function and message-loop function.
- Which `FUN_...` regions are reusable framework/runtime versus game-specific logic.
- Meaning of custom resource type IDs and the two `0x83e8` resources.
- Exact semantics of the KEYBOARD ordinal and SHELL ordinal 5.
- Whether `_DEMONS_.TMP` is a cache, generated file, or content file.
- Full contents and ABI of `SB_CDRES.DLL`.
- Whether all external files are required at startup or only by optional/CD paths.
- Correct object/class boundaries inferred from C++-style names.

## Recommended next steps

1. Use Ghidra call references to the imported USER message APIs and identify the function(s) containing the message pump.
2. Trace from startup functions to the first application object construction and window creation.
3. Label only high-confidence Win16 API wrappers and document their parameter/return conventions.
4. Parse the NE resource table into named resource records and inspect the custom resource payloads read-only.
5. Obtain or inventory the matching `SB_CDRES.DLL` and `BRGHTSTR.INI` only if they are available, without changing the canonical executable.
6. Map waveOut call sequences and buffers before making any audio replacement decisions.
7. Continue to preserve the distinction between direct facts, strong inferences, and hypotheses.
