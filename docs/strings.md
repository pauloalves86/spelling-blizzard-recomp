# Initial String Analysis

Source: `original/BLIZZARD.EXE`, GNU `strings -a -n 4`, and Ghidra 12.1.3 ASCII-string analysis. Ghidra addresses use segmented Win16 notation. This is an initial classification, not a complete semantic interpretation.

## Product/version identity

- `Spelling Blizzard`
- `SpellingBlizzard`
- `BLIZZARD`
- `Spelling Blizzard CD`
- `1.10.CD for Windows`
- `1994 by Bright Star Technology, Inc.`
- `Bright Star Technology, Inc., a wholly-owned subsidiary of Sierra On-Line, Inc.`
- `Advanced Spelling Tricks`
- `First Retail Product Name`

Ghidra VERSIONINFO resource locations include `1078:0004` (`VS_VERSION_INFO`), `1078:00e4` (`Spelling Blizzard`), `1078:0108` (`1.10.CD for Windows`), `1078:0134` (`BLIZZARD`), `1078:0228` (`Spelling Blizzard`), and `1078:0250` (`1.10.CD for Windows`).

## External files, registry/profile paths, and temporary data

- `BRGHTSTR.INI`
- `SB_CDRES.DLL`
- `_DEMONS_.TMP`
- `\\BLIZZARD`
- `Extensions`
- `programs`
- `\\shell\\open\\command`
- `_C_FILE_INFO=`

Facts: `BRGHTSTR.INI` is a literal configuration filename; `SB_CDRES.DLL` is a literal DLL name; `_DEMONS_.TMP` is a literal temporary-file name. The strings alone do not prove which code path opens each item.

## Configuration/preferences and UI text

- `Preferences`
- `Volume`
- `Configuration`
- `ColorText`
- `CWindow`
- `CDesktop`
- `SpellingDemonsWindow`
- `SpellingDemonsDesktop`
- `CEditText`
- `CAMEditText`
- `WinClass`

Likely framework/object metadata is present in strings such as `theObject`, `theMissile`, `itsSpellingDemonObjectGrid`, `itsLayer[i]`, `itsLayeredOffscreen`, `itsLayeredObjectList`, `itsImageHandlerList`, `itsGridCellArray`, `itsForeRedrawList[theSubLayer]`, `itsBackRedrawList[theSubLayer]`, `itsOrdersList`, `itsRemovalList`, `itsAdditionList`, and `itsDeletionList`.

Classification: strong evidence of object-oriented application/framework vocabulary. It does not by itself prove original source-level class boundaries.

## Game/state vocabulary

- `Advanced Spelling Tricks`
- `First Retail Product Name`
- `Magic`
- `Players`
- `CHEAT`
- `HighScor`
- `theMatch`
- `thePlacedLadder`
- `itsAccessList`
- `itsAttackList`
- `itsPlanList`
- `itsFrameSequenceList`
- `itsSpecialEffectsList`
- `itsSoundEffectsList`

These are useful anchors for future string-reference tracing. Their presence is fact; their exact runtime roles are hypotheses until callers and data structures are mapped.

## Runtime/library diagnostics

Windows requirement and capability diagnostics:

- `%s requires Windows 3.10 or higher.`
- `%s requires an installed sound driver.`
- `%s requires a sound driver that allows for asynchronous playback.`
- `%s requires a sound card that supports 11 kHz or better playback rates.`
- `%s requires a screen resolution of 640x480 or larger.`
- `%s requires support for 256 colors.`
- `%s requires more memory.  Consider closing some other application.`
- `File %s is missing.`
- `A writeable directory cannot be found.`

Microsoft-runtime-looking diagnostics:

- `R6000\r\n- stack overflow\r\n`
- `R6003\r\n- integer divide by 0\r\n`
- `R6009\r\n- not enough space for environment\r\n`
- `R6018\r\n- unexpected heap error\r\n`
- `R6020\r\n- unexpected QuickWin error\r\n`
- `R6021\r\n- no main procedure\r\n`
- `R6008\r\n- not enough space for arguments\r\n`
- `run-time error `
- `R6002\r\n- floating-point support not loaded\r\n`
- `ABNORMAL PROGRAM TERMINATION`
- ` Temporarily out of memory.  Recovering! `
- `FailNIL(%s) failed!`
- `Subclass Responsibility`
- `A function subclass responsibility function was called.\nSuch functions should be overridden and not called directly.`

Strong inference: these diagnostics and imported Win16 startup/runtime symbols are consistent with a Microsoft 16-bit C/C++ runtime or libraries. See `docs/toolchain.md`; this remains a toolchain hypothesis, not a definitive compiler identification.

## Fonts

The binary contains a font-name list:

`Chicago`, `MS Sans Serif`, `New York`, `Roman`, `Geneva`, `Arial`, `Monaco`, `Venice`, `London`, `Athens`, `San Francisco`, `Toronto`, `Cairo`, `Los Angeles`, `Times`, `Times New Roman`, `Helvetica`, `Courier`, `Courier New`, `Symbol`, `Taliesin`, `Palatino`, `Arial Bold`.

These names support the Win16/GDI interpretation and indicate runtime font selection or compatibility handling. They are not evidence of embedded font files.

## Resource commentary

The executable includes:

- `Silly Comments:`
- `Why are you looking at the resoures in this executable file?`
- `This product used to be called Advanced Spelling Tricks.`
- `We changed it.  Perhaps that sounded too hard.`

The misspelling `resoures` is preserved exactly as found.

## Important Ghidra locations

| Address | String(s) | Classification |
|---|---|---|
| `1008:554e` | `macTE`, `FailNIL(%s) failed!` | runtime/framework diagnostics |
| `1008:5568-563a` | font names | Win16/GDI font vocabulary |
| `1008:6c2a-6d9e` | capability and missing-file messages | Windows startup/runtime checks |
| `1008:8ffc-90d6` | product, registry/profile, temp/config/game strings | application configuration and identity |
| `1010:0ef2` | temporary out-of-memory message | framework/runtime diagnostic |
| `1010:5fe4-cc16` | object/grid vocabulary | application/framework metadata |
| `1018:25be-87dc` | layered lists, CHEAT, preferences | game/UI/rendering metadata |
| `1018:b218-b32e` | score/match metadata | game-state metadata |
| `1020:067a-1f3c` | ladder and object-list metadata | game object metadata |
| `1060:0100-0138` | CD/config/DLL and class names | startup/config/resource metadata |
| `1060:064c-0790` | Microsoft-style runtime diagnostics | runtime/toolchain evidence |
| `1070:0002-0088` | resource comments | embedded resource commentary |
| `1078:0004-0250` | VERSIONINFO fields | exact product/version evidence |

## Negative findings

The initial printable-string scan did not find obvious `.WAV`, `.MID`, `.VOC`, `.MOD`, `.DAT`, or word-list filenames. This does not prove that no such data exists: audio or game content may be embedded, loaded through `SB_CDRES.DLL`, encoded/compressed, or referenced by numeric/resource identifiers.
