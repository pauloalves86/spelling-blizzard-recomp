# Phase 5 Command Flow

## Target

`1008:ac80` is a `__stdcall16far` function with four parameters according to Ghidra:

```text
param_1: far pointer to an object-like structure
param_2: 16-bit segment component associated with param_1
param_3: 16-bit command/event value
param_4: 16-bit secondary value
```

The function returns no value. If `param_4 != 0`, it immediately delegates to `1008:b7de` and returns.

## Command-flow table

| Command ID | Origin | Branch and observed effects | Calls | Confidence |
|---|---|---|---|---|
| `0x2af9` | The immediate producer is unresolved; it arrives in `param_3` | Uses `DAT_1060:0a38` and object offset `+0x66`; conditionally releases/replaces `DAT_1060:01e4`; allocates `0x262` bytes through `1008:ef2c`; initializes method-table entries; calls `1018:a9a0`; reads `1018:aeaa`; stores result + 1 in `DAT_1060:01de`; clears `DAT_1060:0a36`; calls `1010:d0a6`; calls `1018:5496` with control value `1` | `1010:d03c`, indirect object callback, `1008:ef2c`, `1018:a9a0`, `1018:aeaa`, `1010:d0a6`, `1018:5496` | High for branch effects; origin unresolved |
| `0x2afa` | The immediate producer is unresolved; it arrives in `param_3` | Selects control value `0` and calls `1018:5496` with object field `param_1[2]` | `1018:5496` | High for branch effects; origin unresolved |
| `0x2afb` | The immediate producer is unresolved; it arrives in `param_3` | Same observed `1018:5496` path and control value `0` as `0x2afa` | `1018:5496` | High for branch effects; origin unresolved |
| `0x32c9` | The immediate producer is unresolved; it arrives in `param_3` | Repeatedly invokes the callback at object/vtable-relative offset `+0x110`, passing `1`, until `1010:08bc` returns zero | indirect callback, `1010:08bc` | High for branch effects; origin unresolved |
| `0x32ca` | The immediate producer is unresolved; it arrives in `param_3` | Repeatedly invokes the callback at object/vtable-relative offset `+0x114`, passing `1`, until `1010:08bc` returns zero | indirect callback, `1010:08bc` | High for branch effects; origin unresolved |

## State transition candidate

The `0x2af9` branch is the first clear application-specific state transition candidate:

```text
command 0x2af9
    -> construct/replace DAT_1060:01e4 object
    -> initialize it from local data via 1018:a9a0
    -> read nested field +0x06 via 1018:aeaa
    -> DAT_1060:01de = field_06 + 1
    -> DAT_1060:0a36 = 0
    -> UI/framework update via 1010:d0a6
    -> 1018:5496(..., 1)
```

This is a state transition candidate, not yet a complete behavioral reconstruction. The meaning of the object and command remains neutral.

## Command origins

FACT: No direct code-reference caller to `1008:ac80` was found in the current Ghidra reference graph. The function appears to be an indirect callback target, consistent with the framework's object/function-table design.

FACT: The five command constants occur as comparisons inside `1008:ac80`. Their concrete source event has not been established.

The current evidence does not justify claiming that they originate from a menu, timer, keyboard message, or window procedure.

## Relationship to other Phase 5 targets

`1008:ac80` directly calls `1018:aeaa` and writes `DAT_1060:01de` from its result. It also writes/replaces `DAT_1060:01e4`, the object passed to `1018:a9a0` and `1018:aeaa`.

The known `1018:509c` path also references `DAT_1060:0a3c` and application-object fields, but the current reports do not establish that it calls `1008:ac80` or shares the same command source. That relationship remains open.

`1010:08bc` is used as a loop termination/polling predicate only in the `0x32c9` and `0x32ca` branches. Its semantic role is unresolved.

## Phase 6 exact 0x2af9 sequence

The branch at `1008:acc0` has now been traced at instruction level:

```text
1010:d03c -> bounded text value
old DAT_1060:01e4 non-null -> call its first method slot
1008:ef2c(0x262) -> replacement allocation
constructor-like writes: 0bc8:1008, 2810:1008, 2b60:1008, final 2b04:1008
1018:a9a0(replacement, text)
1018:aeaa(replacement)
DAT_1060:01de = returned AX + 1
DAT_1060:0a36 = 0
1010:d0a6(replacement)
1018:5496(replacement, 1)
```

`1010:d0a6` calls `1010:01d0` on the object field at `+4`, then calls `1008:8e46`. It is therefore more specifically a resource/object-field replacement helper than a proven redraw-only helper.

`1018:5496` clears `DAT_1060:01e2`, releases the far object at `+0x82` through its first method, clears that field, invokes the receiver's method at `+0x1c` with `0x271b`, drains message range `10` when its control byte is nonzero, and invokes the object at `DAT_1060:01ec` through slots `+0x124` and `+0x128`. See `docs/application-state-transition.md`.

## Phase 6 command-origin boundary

One concrete sibling command origin is now established: `1010:2a7e` pushes `0x2afa` and calls an object callback at offset `+0x68` (the function begins at `1010:2a74`). No equivalent literal producer for `0x2af9` was found in the executable instruction scan. The current evidence therefore narrows the `0x2af9` origin to an unresolved command argument supplied to `1008:ac80`, while showing that related command values can be emitted by indirect object callbacks.

## Phase 7 object/callback refinement

The `+0x250` nested pointer used by `1018:aeaa` is now traced to `1018:a9a0`:

```text
1018:a9a0 -> method +0x18
  nonzero -> method +0x20 -> method +0x44 -> store returned far pointer at +0x250
  zero    -> 1018:ab1c -> 1010:035c -> store returned far pointer at +0x250
```

The callback used by `1018:5496` is the replacement object's `+0x1c` table entry. For the final table pointer `1008:2b04`, the entry bytes encode `1008:e8b6`; Ghidra has not established a function boundary at that address. This is an exact indirect-target boundary, not a semantic callback identification.

## Phase 10 event sibling

`0x2714` is not a command case in `1008:ac80`; it is handled in `1018:267e`. `1020:4e8e` emits its literal through a receiver slot `+0x1c` only when a word reached through `+0x4a/+0x46/+0x68` equals `8`. The known `0x2714` mutation path is documented in `docs/application-state-machine.md`; its dispatch table connection remains unresolved.
