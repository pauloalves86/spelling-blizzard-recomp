# Phase 4 Callback and Dispatch Analysis

## Target: 1018:509c

### Fact

`1018:509c` has no direct code reference in Ghidra's reference manager. The `getReferencesTo()` scan also found no direct reference from initialized data to that address.

A raw executable scan found the 16-bit little-endian value `0x509c` at these loaded code locations:

- `1018:3248`
- `1018:8687`
- `1018:86a1`
- `1018:86bc`
- `1018:86d7`
- `1018:ae8d`
- `1018:aeef`

These are not yet valid callback references. Instruction-context inspection shows the locations in the `0x86xx` and `0xae8x` ranges are instruction displacement bytes, not function-pointer table entries. The `1018:3248` occurrence is in an undefined code-segment/data-looking region, with neighboring words that do not form the expected far pointer `0x509c:0x1018`. No Ghidra reference is attached to it.

The expected far-pointer byte sequence `9c 50 18 10` was not found in the original file. The original file was scanned directly; it was not modified.

### Current conclusion

The evidence does not yet prove that `1018:509c` is stored in a conventional static far-pointer callback table. It remains plausible that the function is reached through a near-pointer table, a dynamically populated table, an unrecognized data structure, or an indirect dispatch path whose pointer value is not preserved as a literal `0x509c` in the file.

Do not claim a recovered table containing `1018:509c` yet.

## Phase 5 data-flow follow-up

The unresolved `1018:509c` path shares application-state evidence with the `0x2af9` branch of `1008:ac80`: both can initialize/use `DAT_1060:01e4`, and both call `1018:a9a0`, `1018:aeaa`, and `1010:d0a6` in their respective paths. This is a shared-state relationship, not proof that one dispatches to the other.

`DAT_1060:0a3c` remains directly tied to the input accumulation and `CHEAT` comparison in `1018:509c`. Current decompilation does not show `1008:ac80` accessing it.

## Confirmed indirect-dispatch conventions

The executable does contain many object-relative indirect calls. A common form is:

```text
load object vtable pointer
add method-slot offset
load near code pointer
call through pointer
```

Examples:

- `1010:596e` invokes object methods at vtable-relative slots corresponding to `+0x14` and `+0x18`.
- `1010:3d56` invokes an event callback at an object-relative slot corresponding to `+0x40`.
- `1018:509c` invokes object-relative callbacks at offsets represented by approximately `+0x1c`, `+0x52`, `+0x5e`, `+0x62`, `+0x66`, and `+0x6a` in Ghidra's output.

These establish an indirect-dispatch architecture, but not ownership of the particular `1018:509c` entry.

## Callback/function-table initialization evidence

`1008:2bf4` allocates several objects through `FUN_1008_ef2c` and writes repeated offset/segment pairs such as:

```text
0xbc8:1008
0x824:1008
0x904:1008
0xb08:1008
0x2ece:1008
0x2da6:1008
0x2ff2:1008
0x2f5a:1008
0x3042:1008
0x2e32:1008
```

It stores these object pointers in globals including `DAT_1060_0a14`, `DAT_1060_0a1e`, and `DAT_1060_07c4`, then invokes methods through the resulting tables. This is confirmed function-table/vtable-style initialization, but none of the literal entries identified in this path is `1018:509c`.

`1018:649a` similarly allocates and initializes several nested objects with offset/segment pairs in the `1008` and `1018` segments. This is a strong application/object construction candidate, but it is not yet connected to `1018:509c`.

## Event path currently supported by evidence

```text
USER message APIs
    |
    v
1008:e412  -- drains selected message ranges
    |
    v
1010:3d56  -- generic message/key translator and object callback
    |
    v
object-relative callback tables
    |
    +-- 1018:509c is a game/application handler candidate,
        but its exact table entry/owner is unresolved
```

This diagram intentionally stops at the unresolved edge. The binary does not yet support a more specific claim.

## Highest-value next trace

Search the owners of the object fields used by `1018:509c`, especially the object passed as its first parameter and the nested objects at offsets represented as `0x52`, `0x5e`, `0x62`, `0x66`, and `0x6a`. The callback table is more likely to be recovered by identifying the object's constructor than by treating every `0x509c` byte sequence as a pointer.

## Phase 7 callback findings

The replacement object's final method-table pointer is `1008:2b04`. Reading its far entries gives:

| Slot | Entry | Status |
|---:|---|---|
| `+0x1c` | `1008:e8b6` | target address established; Ghidra function boundary unresolved |
| `+0x20` | `1008:cf6c` | function identified; selects a resource/string pointer based on a byte and stores it near object `+0x24c` |
| `+0x44` | `1008:cfc2` | target address established; implementation boundary unresolved |
| `+0x54` | `1008:d0f2` | target address established; implementation boundary unresolved |
| `+0x68` | `1008:e838` | target address established; implementation boundary unresolved |

`1018:a9a0` uses `+0x20` and `+0x44` while establishing `+0x250`; `1018:5496` uses `+0x1c` for event `0x271b`.

The global object at `DAT_1060:01ec` is read at `1018:5496`, `1018:284c`, `1020:1a9a`, `1008:75b2`, `1008:16ce`, and `1018:9f90`. Its analyzed image bytes are zero, but raw code at `1020:5f29..5f58` first calls current receiver slot `+0x11c` with one, then assigns that receiver from stack `BP+6/BP+8` into this global, clears `DAT_1060:01f0`, and sets receiver byte `+0x15a`. Later `1020:678c` stores its receiver in `DAT_1060:01f0` and clears `+0x15a` after `+0x128/+0x148` dispatches. This supports an activation/completion-context inference, not ownership or concrete callback identities.

Phase 10 confines this object to documented boundaries: `1018:284c` calls its `+0x124`, `+0x128`, `+0x12c`, and `+0x118` slots while applying `01de` thresholds, and `1020:4e8e` may drain its `+0x118` loop before emitting `0x2718`. No direct `0x2714` ordered-record consequence through this runtime object is proven.
