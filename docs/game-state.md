# Initial Game-State Recovery

## Current status

No complete `GameState` struct is claimed yet. The current evidence establishes names and object-facing access patterns, but not enough field ownership or sizes to define a trustworthy semantic structure.

## Candidate application anchors

The executable contains strings for:

- `theMatch` at `1018:b32e`
- `HighScor` at `1018:b218`
- `CHEAT` at `1018:5cb6`
- `thePlacedLadder` at `1020:067a`
- `itsSpellingDemonObjectGrid` at `1010:cc16`
- `itsLayeredOffscreen`, `itsLayeredObjectList`, `itsImageHandlerList`, `itsGridCellArray`
- foreground/background redraw lists and order/add/remove/delete lists
- `itsAccessList`, `itsAttackList`, `itsPlanList`, `itsFrameSequenceList`, `itsSpecialEffectsList`, `itsSoundEffectsList`

Only `CHEAT` currently has a direct Ghidra string reference in the analyzed listing. It is referenced by `1018:509c` at several instruction locations. The other labels may be metadata, copied names, or accessed indirectly; their semantic use is not yet proven.

## 1018:509c observations

Ghidra's decompilation shows:

- a state-byte test at an offset from the second parameter;
- special handling for input bytes `0x3f`, `0x2f`, `0x03`, and `0x0d`;
- calls to `1008:e412(8)` before selected event/control operations;
- a comparison against the literal `CHEAT` string;
- allocation through `FUN_1008:ef2c(0x262)` when the cheat-related condition succeeds;
- initialization of a global object-like pointer at `DAT_1060:01e4`;
- callbacks at `1018:a9a0`, `1018:ae3a`, and `1018:aeaa`;
- reads of object-relative pointers at offsets represented by Ghidra as approximately `0x52`, `0x5e`, `0x66`, and `0x6a` from the first parameter.

The reference report adds that `1018:509c` directly calls `1018:a9a0`, `1018:ae3a`, `1018:aeaa`, and `1010:d0a6`; it has no direct code-reference caller in the current graph. This strengthens the interpretation that it is a callback/table target rather than an ordinary statically called routine.

These observations justify classifying it as a medium-confidence application/controller function, but not assigning meanings such as `score`, `match`, or `cheat_mode` to the fields.

## Provisional field ledger

| Location | Observed use | Semantic name | Confidence |
|---|---|---|---|
| first parameter + `0x52`-equivalent | pointer selected for input/control path | `field_52` | Confirmed offset use; unknown meaning |
| first parameter + `0x5e`-equivalent | object receiving callback calls | `field_5e` | Confirmed offset use; unknown meaning |
| first parameter + `0x66`-equivalent | pointer used by `n`/`p`/`P` input paths | `field_66` | Confirmed offset use; unknown meaning |
| first parameter + `0x6a`-equivalent | pointer used by `Q` path | `field_6a` | Confirmed offset use; unknown meaning |
| `DAT_1060:01e4` | global object-like pointer initialized in CHEAT path | `global_01e4` | Confirmed global use; unknown meaning |
| `DAT_1060:01de` | set to a callback result plus one | `global_01de` | Confirmed write; unknown meaning |
| `DAT_1060:01e1`, `DAT_1060:01e2` | gating bytes for input paths | `global_01e1`, `global_01e2` | Confirmed gating use; unknown meaning |

The ledger deliberately uses neutral field names until callers and allocations are traced.

## Candidate first semi-pure game/state target

`1018:aeaa` is currently the best first differential-testing candidate. It follows an object pointer at `object + 0x250`, follows the nested pointer, and returns the 16-bit value at nested offset `+6`. It has no Win16 API calls and no side effects visible in the decompilation.

Candidate contract:

```text
input: object with a valid nested object at field_0x250
output: nested_object->field_0x06
side effects: none observed
external dependencies: object layout only
confidence: high for data flow, unknown for semantic meaning
```

`1018:ae3a` is a related state mutation candidate: it updates the same nested value only when a candidate is larger, sets byte `object + 0x254`, and calls `1018:a704`. It is less suitable for a first pure test because of that callback/global dependency.

## Application command/input evidence

1. Find all callers of `1018:509c`.
2. Trace the allocation and constructor of the first parameter object.
3. Trace `FUN_1008:ef2c(0x262)` and the functions called during `DAT_1060:01e4` initialization.
4. Identify whether `global_01e4` belongs to a game object, cheat/debug controller, or framework service.
5. Search data references to the candidate offsets across callers before defining C structures.
6. Build a pure-function test only after a deterministic game-state transition is isolated.

## Phase 5 reconstruction update

`1018:aeaa` is now reconstructed as `sb_recovered_1018_aeaa` in `src/recovered/application/nested_field.c`. Its exact assembly follows `object + 0x250`, then holder offset `0`, then value offset `+0x06`; it returns the 16-bit value in `AX` with `RETF 0x4`. See `docs/first-reconstruction.md`.

`1018:ae3a` is the strongest confirmed writer for the same final `+0x06` field. It conditionally raises the field and sets a byte at parent `+0x254`; no semantic field name is claimed.

`1008:ac80` reads this value on its `0x2af9` branch, stores it plus one in `DAT_1060:01de`, and initializes/replaces `DAT_1060:01e4`. The five numeric branches and known effects are documented in `docs/command-flow.md`; their external origins remain unresolved.

## Phase 6 state-transition evidence

The `0x2af9` branch writes `DAT_1060:01de` as the 16-bit result of `1018:aeaa` plus one, then clears `DAT_1060:0a36`. The value in `DAT_1060:01de` is subsequently modified by handlers that increment/decrement it and clamp it: `1018:438e` enforces a maximum of `100`, while `1018:284c` compares it with threshold values including `0x32`, `0x4b`, and `0x5a`. This constrains it as a bounded application value, but does not prove whether it is an index, count, score, progress value, or another domain quantity.

`DAT_1060:0a36` is written as a byte and is set to `1` or `0` by `1018:267e` in response to event values, then cleared by `0x2af9`. Its exact role remains unresolved; a boolean interpretation is not claimed.

## Phase 7 nested value properties

`1018:ab1c` initializes nested `field_06` to zero. `1018:a704` reads it while comparing the value against the `+0x06` word in each nested repeated entry and inserting/replacing an entry. `1018:ae3a` writes the field only when its candidate is larger. These facts establish initialization, comparison, and conditional monotonic update operations, but do not establish a semantic name or a complete numeric range.

The nested object also has a count at `+0x38`, seven inline 8-byte entries beginning at `+0x00` (with slot 1 at `+0x08`), flag bytes near `+0x19a`, and an accumulated value around `+0x1a4`. These are layout observations only.

Phase 8 corrects the repeated-entry base: `+0x00` begins slot 0; `+0x08` begins slot 1. The slot-0 value at `+0x06` is the same word returned by `1018:aeaa`. `1018:a704` maintains the entries in descending signed-value order, removes at most one equal-text entry, caps active count at seven, and marks the destination parent at `+0x254`. Phase 9 connects an observed `1018:267e` event branch to this update through `1018:ae3a` and current `DAT_1060:01de`, and connects later `1018:adb8` indexed mutation to `+0x1a4/+0x1a6`. This is a behavioral property, not a domain name.

Phase 10 confirms that `1018:267e` has a two-step `0x2714` transition: `0a36 == 0` sets that byte and invokes receiver `+0x118`; a later `0a36 != 0` branch calls `ae3a`, optionally `ae7e`, clears `0a36`, increments `01de`, and dispatches through receiver `+0x114` or `+0x120`. The meaning of this sequencing byte remains unresolved.
