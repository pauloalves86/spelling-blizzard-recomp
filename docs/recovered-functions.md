# Recovered Function Inventory

Confidence levels: CONFIRMED, HIGH CONFIDENCE, MEDIUM CONFIDENCE, LOW CONFIDENCE, UNKNOWN.

## 1008:e412 — framework_drain_message_ranges

- Current Ghidra name: `FUN_1008_e412`
- Category: FRAMEWORK / WIN16_API
- Calling convention: Ghidra: `__cdecl16far`
- Parameters: one 16-bit flags value; exact semantic type unknown
- Return value: none
- Globals accessed: `DAT_1060_01d6` is cleared when the `0x02` group completes
- APIs: `PEEKMESSAGE`, `TRANSLATEMESSAGE`, `DISPATCHMESSAGE`
- Purpose: drain selected Win16 message ranges in a fixed order
- Evidence: repeated API loops and message filter constants in Ghidra decompilation
- Confidence: HIGH CONFIDENCE for message-drain behavior
- Open questions: exact Win16 filter/remove arguments and caller-level idle/update semantics
- Native source: `src/recovered/framework/message_pump.c`

## 1010:3d56 — framework_translate_message

- Current Ghidra name: `FUN_1010_3d56`
- Category: FRAMEWORK / WIN16_API
- Calling convention: Ghidra: `__stdcall16far`
- Parameters: an object/vtable-like pointer and a message-state pointer; exact segmented types unknown
- Return value: 16-bit status/result
- APIs: `PEEKMESSAGE`, `GETKEYSTATE`
- Behavior: obtains/copies a MSG-like record, switches over Windows message IDs, invokes an object-relative callback, combines modifier-key state, and records status/coordinates
- Evidence: direct Ghidra decompilation and API cross-references
- Confidence: HIGH CONFIDENCE for generic event translator role; MEDIUM for exact field semantics
- Open questions: callback prototype and complete message structure layout

## 1010:596e — window_create_or_initialize

- Current Ghidra name: `FUN_1010_596e`
- Category: FRAMEWORK / WIN16_API
- Calling convention: Ghidra: `__stdcall16far`
- Parameters: large object pointer plus Win16 segment/value arguments; exact prototype unresolved
- Return value: far pointer to object
- APIs: `GETCLASSINFO`, `REGISTERCLASS`, `CREATEWINDOW`, `GETDC`
- Behavior: initializes vtable/class fields, registers a class if absent, creates a window, stores the handle and DC, invokes an optional post-create callback
- Confidence: HIGH CONFIDENCE for generic window-object constructor role
- Open questions: actual class name field and callback/vtable layout

## 1010:5b18 — window_create

- Current Ghidra name: `FUN_1010_5b18`
- Category: FRAMEWORK / WIN16_API
- Calling convention: Ghidra: `__stdcall16far`
- Parameters: large window object pointer and segment/value argument
- APIs: `GETCLASSINFO`, `REGISTERCLASS`, `CREATEWINDOW`, `GETDC`
- Behavior: same core sequence as `1010:596e`, without the apparent conditional initialization branch
- Confidence: HIGH CONFIDENCE for generic window creation
- Open questions: relationship to `1010:596e` and inheritance/constructor dispatch

## 1018:509c — game_input_or_controller_handler

- Current Ghidra name: `FUN_1018_509c`
- Category: GAME / FRAMEWORK boundary
- Calling convention: Ghidra: `__stdcall16far`
- Parameters: `undefined4 *param_1`, `undefined4 param_2`, `undefined1 param_3`, `byte param_4`
- Return value: none in current decompilation
- APIs/calls: `1008:e412`, object-relative virtual callbacks, `FUN_1008:ef2c`, `1018:a9a0`, `1018:ae3a`, `1018:aeaa`, `1010:d0a6`
- Evidence: direct references to `CHEAT`, a five-character input buffer at `DAT_1060:0a3c`, special values `?`, `/`, Ctrl-C, and Enter, plus object-specific callbacks
- Input behavior: if the second parameter's state byte is clear, special controls are handled; otherwise lower-case command bytes such as `n`, `N`, `P`, `Q`, and `r` select object fields and eventually call `FUN_1008:b802` or an object method at a `+0x118`-equivalent slot
- CHEAT behavior: the handler appends an input byte to `DAT_1060:0a3c` while the buffer length is below five, then compares that buffer against the literal `CHEAT`; equality triggers construction/initialization of `DAT_1060:01e4`
- Direct code-reference caller: none found; likely reached through an indirect dispatch path, but the owning table is not yet recovered
- Confidence: MEDIUM for application input/controller role; HIGH for the observed command-buffer comparison
- Open questions: exact object layouts, callback-table owner, meanings of command bytes, and whether the command buffer accepts only five characters by design

## 1018:aeaa — nested_state_value_getter

- Current Ghidra name: `FUN_1018_aeaa`
- Category: GAME / STATE
- Calling convention: `__stdcall16far`
- Parameters: one far object pointer
- Return value: 16-bit value
- Behavior: follows the object field at `+0x250`, then a nested pointer, and returns the 16-bit value at nested offset `+6`
- Evidence: direct decompilation; called from `1018:509c`, `1018:3c70`, and `1008:ac80`
- Confidence: HIGH for data-flow behavior; UNKNOWN for semantic name of the value
- Candidate use: first small semi-pure differential target once a valid object fixture is recovered

## 1018:ae3a — nested_state_value_update

- Current Ghidra name: `FUN_1018_ae3a`
- Category: GAME / STATE
- Calling convention: `__stdcall16far`
- Parameters: object pointer, 16-bit value, 16-bit candidate value
- Return value: none
- Behavior: follows `object + 0x250` to a nested object, replaces nested offset `+6` if the candidate is larger, sets object byte `+0x254`, and calls `1018:a704(DAT_1060:0a46, object, param_2)`
- Evidence: direct decompilation; called from `1018:509c` and `1018:267e`
- Confidence: HIGH for max-update/dirty-flag behavior; MEDIUM for game semantics
- External dependency: `1018:a704` and global `DAT_1060:0a46`

## 1018:a9a0 — nested_object_initialize_from_text

- Current Ghidra name: `FUN_1018_a9a0`
- Category: GAME / STATE / OBJECT
- Calling convention: `__stdcall16far`
- Parameters: object pointer and text pointer, with Ghidra showing additional segmented/stack artifacts
- Return value: none
- Behavior: clears a nested array around object offset `+0x256`, copies a bounded input string into a 64-byte local buffer, calls `1010:14c2` and `1008:8caa`, invokes object methods at slots around `+0x2c`, `+0x18`, `+0x20`, and `+0x44`, stores a returned far value at `+0x250/+0x252`, and clears byte `+0x254`
- Evidence: direct decompilation and callers
- Confidence: HIGH for initialization operations; MEDIUM for interpretation as text/state initialization

## 1010:d0a6 — framework_or_UI_update_helper

- Current Ghidra name: `FUN_1010_d0a6`
- Category: FRAMEWORK / UI boundary
- Calling convention: `__stdcall16far`
- Parameters: object pointer and 16-bit value
- Return value: none
- Behavior: calls `1010:01d0` on a field at object offset `+4`, then calls `1008:8e46`
- Evidence: direct decompilation; called from `1018:509c` and `1008:ac80`
- Confidence: HIGH for call sequence; LOW/MEDIUM for exact UI semantics

## 1000:001a — Win16_entry

- Current Ghidra name: `entry`
- Category: RUNTIME
- Calling convention: startup entry; not a normal C prototype
- Evidence: NE initial entry, `INITTASK`, `LOCKSEGMENT`, `GETVERSION`, `WAITEVENT`, `INITAPP`
- Confidence: CONFIRMED runtime/startup classification

## 1008:82b8 — configuration_path_setup

- Current Ghidra name: `FUN_1008_82b8`
- Category: FRAMEWORK / CONFIGURATION
- Calling convention: Ghidra: `__cdecl16far`
- APIs: `GETMODULEFILENAME`, `GETPRIVATEPROFILESTRING`
- Evidence: multiple 256-byte path buffers, `Spelling Blizzard CD`, configuration strings, profile read
- Confidence: HIGH for path/configuration role
- Open questions: exact `BRGHTSTR.INI` sections/keys and error path

## 1008:ac80 — application_command_dispatcher

- Current Ghidra name: `FUN_1008_ac80`
- Category: GAME / APPLICATION CONTROL
- Calling convention: `__stdcall16far`
- Parameters: object pointer, segment value, command/event value, flag/value
- Behavior: handles command values `0x2af9`, `0x2afa`, `0x2afb`, `0x32c9`, and `0x32ca`; initializes `DAT_1060:01e4` on the `0x2af9` path; invokes `1018:a9a0`, `1018:aeaa`, `1010:d0a6`, and object methods; polls `1010:08bc` for two command paths
- Confidence: MEDIUM for application/controller classification; HIGH for observed command dispatch
- Open questions: command ID origin and relation to the window procedure

## 1008:2bf4 — application_object_initialization_candidate

- Current Ghidra name: `FUN_1008_2bf4`
- Category: FRAMEWORK / APPLICATION INITIALIZATION
- Calling convention: `__stdcall16far`
- Behavior: allocates multiple objects, writes far offset/segment method-table entries, stores global object pointers, invokes object initialization methods, and returns an object field
- Globals: `DAT_1060:07d0`, `DAT_1060:0a14`, `DAT_1060:0a1e`, `DAT_1060:07c4`
- Confidence: HIGH for object-initialization behavior; MEDIUM for the application-boundary interpretation

## Window constructor callers

- `1010:596e` callers: `1010:50ce`, `1010:53c8`, `1010:5722`
- `1010:5b18` callers: `1008:0eba`, `1008:6ef6`, `1008:670a`, `1010:50ce`, `1010:53c8`, `1010:5722`
- `1008:0eba` allocates a `0x160`-byte object, initializes it through `1008:6ef6`, calls `1010:5b18`, and stores the object in owner fields at `+0x1c`/`+0x1e`
- Concrete application `lpfnWndProc` remains unknown because the WNDCLASS-like record is populated through an object method at the `+0x14` slot

- `1018:aeaa` now has a native unit-tested reconstruction in `src/recovered/application/nested_field.c` and `tests/test_recovered_1018_aeaa.c`.
- `1008:ac80` command branches are documented in `docs/command-flow.md`; no direct static caller or concrete command source event has yet been established.
- `DAT_1060:01e4` is allocated/replaced on the `0x2af9` path and passed through `1018:a9a0`/`1018:aeaa`; it remains semantically unnamed.
- The relation between `DAT_1060:0a3c` and `1008:ac80` remains unproven.

## Not yet recovered

- Exact application main function.
- Exact game-state object and match loop.
- Concrete application window procedure.
- Static callback table owning `1018:509c`.
- Differential harness for `1018:aeaa`; its object fixture is not yet defined.

- The native message pump remains intact. The Phase 4 work adds evidence and reports without claiming an unsupported callback-table mapping.

## Phase 6 additions

### 1018:5496 — object_release_and_application_dispatch

- Calling convention: `__stdcall16far`, three arguments including a far object pointer and a byte control flag
- Behavior: clears `DAT_1060:01e2`; releases and clears object field `+0x82`; invokes receiver method `+0x1c` with event `0x271b`; when the control flag is nonzero, calls `1008:e412(10)` and dispatches through `DAT_1060:01ec` slots `+0x124` and `+0x128`
- Confidence: HIGH for instruction-level sequence; LOW for semantic names of the callbacks

### 1010:d0a6 — object_resource_replace_helper

- Behavior: calls `1010:01d0` on object field `+0x04`, then calls `1008:8e46`
- Callers: `1008:ac80` and `1018:509c`
- Confidence: HIGH for sequence; MEDIUM for resource/object replacement classification

### 1008:ac80 — phase 6 lifecycle sequence

- The `0x2af9` branch releases/replaces `DAT_1060:01e4`, initializes the replacement through `1018:a9a0`, derives `DAT_1060:01de` from `1018:aeaa`, clears `DAT_1060:0a36`, then crosses `1010:d0a6` and `1018:5496` boundaries.
- A native helper now reconstructs the isolated writes in `src/recovered/application/application_state.c`; it does not emulate allocation, segmented callbacks, or Win16 framework behavior.

### 1018:ab1c — nested_object_initialize_fallback

- Behavior: obtains a far pointer through `1010:035c`, stores it at parent `+0x250/+0x252`, copies text into nested `+0x00`, initializes nested `+0x06` to zero, initializes repeated-entry storage and related counters/flags, invokes object methods, and clears the parent pending byte
- Confidence: HIGH for observed writes and call order; LOW/MEDIUM for ownership and semantic interpretation

### 1018:a704 — nested_entry_update

- Behavior: reads nested `+0x06`, compares it with repeated entry values, removes or shifts entries, inserts text/value data, increments a nested count capped at `7`, and sets parent byte `+0x254`
- Confidence: HIGH for data-flow behavior; semantic meaning unresolved

### 1018:aa84 — nested_object_release_or_update

- Behavior: when parent byte `+0x254` is set, performs framework/object update callbacks and clears that byte; it always passes the far object at parent `+0x250` to `1010:01d0` and calls `1008:e812`
- Confidence: HIGH for call sequence; exact ownership semantics unresolved

### 1018:a704 — nested_ordered_entry_update

- Calling convention: `__stdcall16far`; two far parent-pointer parameters; `RETF 0x8`
- Behavior: captures source nested slot-0 text/value, removes the first equal-text destination entry, ranks descending signed values, shifts physical slots through slot 6, inserts when rank is below 7, clamps count at 7, and sets destination parent `+0x254`
- Confidence: HIGH for instruction-level data flow and ordering; UNKNOWN for domain semantics
- Native source/test: `src/recovered/application/nested_entries.c`, `tests/test_recovered_1018_a704.c`

### 1008:e8b6 — final_table_event_noop

- Current Ghidra function: no recovered boundary
- Raw code: `CA 08 00` (`RETF 8`)
- Behavior: consumes four argument words without reads, calls, or writes; absorbs `1018:5496` event `0x271b` for final-table receivers
- Confidence: HIGH

## Phase 9 additions

### 1018:aa84 — final_table_release_path

- Calling convention: `__stdcall16far`; one far parent pointer; `RETF 4`
- Table entry: final replacement table `1008:2b04`, slot `+0x00`
- Behavior: if parent `+0x254` is nonzero, calls slots `+0x20`, `+0x54`, and `+0x10` around `1010:00e8`/`1010:0122` on parent `+0x250`, then clears `+0x254`; always calls `1010:01d0(parent +0x250)` and `1008:e812(parent)`
- Lifecycle: known `0x2af9` and `1018:509c` constructors install `1008:2b04`; an old non-null `DAT_1060:01e4` is released through slot 0 on replacement
- Confidence: CONFIRMED for the known construction/replacement paths

### 1018:adb8 — indexed_nested_word_adjustment

- Calling convention and complete prototype: unresolved
- Behavior: selects `nested + 0x08 + 2 * (parameter - 1)`, applies `+0x28` or `-0x14`, clamps the selected word to `[1,200]`, adds the signed change to `+0x1a4/+0x1a6`, and writes parent `+0x254`
- Direct callers: `1020:1a9a` (two sites) and `1020:6af6`
- Confidence: HIGH for arithmetic/data flow; UNKNOWN for semantic field names

### 1008:cfc2 / 1008:d0f2 — final-table nested boundaries

- `1008:cfc2` (`+0x44`): `RETF 4`; returns `0:0` if receiver `+0x24c` is null, otherwise returns an acquired far pointer after framework calls. It is the method `1018:a9a0` uses to obtain parent `+0x250` on its normal path.
- `1008:d0f2` (`+0x54`): `RETF 8`; conditionally consumes/forwards a far input through receiver `+0x24c`; called by `1018:aa84`'s nonzero-`+0x254` sequence.
- Confidence: HIGH for ABI and bounded side effects; source-level roles unresolved.

## Phase 10 additions

### 1018:267e — event_state_transition_dispatcher

- Calling convention: `__stdcall16far`; `RETF 8`
- `0x2714`: first transition sets `DAT_1060:0a36` and calls receiver `+0x118`; active transition calls `1018:ae3a(DAT_1060:01e4, DAT_1060:01de)`, optionally `1018:ae7e`, clears `0a36`, increments `01de`, and calls receiver `+0x114` or `+0x120`
- Confidence: HIGH for branch instructions; event source/table target remains unresolved

### 1018:adb8 / 1018:aede / 1018:af08 — nested state primitives

- `adb8`: `__stdcall16far`, `RETF 8`; unchecked one-based word selection from nested `+0x08`, signed `+40`/`-20` adjustment, signed clamp `[1,200]`, signed 32-bit accumulation at `+0x1a4/+0x1a6`, parent `+0x254=1`.
- `aede`: `__stdcall16far`, `RETF 6`; writes an unmodified word to `+0x1a8`, parent `+0x254=1`.
- `af08`: `__stdcall16far`, `RETF 6`; returns `AL=1` when signed argument is `<=` signed `+0x1a8`.
- Native reconstruction: intentionally omitted; `adb8` has unchecked 16-bit segmented-offset selection, so a bounded native array model would be speculative.
