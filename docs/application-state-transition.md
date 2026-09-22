# Phase 6 application state transition

## Command

`0x2af9`

## Entry function

`1008:ac80`, a four-parameter `__stdcall16far` dispatcher. The command is selected by comparing the first word parameter with `0x2af9`; the branch begins at `1008:acc0`.

## Inputs

The dispatcher receives a far pointer in its third parameter. In the `0x2af9` branch, the pointer is used as the object passed to `1018:a9a0`, `1018:aeaa`, `1010:d0a6`, and `1018:5496`. A bounded text value is obtained by `1010:d03c` from the dispatcher's first/second parameters before `1018:a9a0`.

## Objects involved

The branch releases the previous far pointer stored in `DAT_1060:01e4`, allocates `0x262` bytes through `1008:ef2c`, and applies a sequence of constructor-like method-table writes. The final visible table pointer is `1008:2b04`; intermediate writes include `1008:0bc8`, `1008:2810`, and `1008:2b60`. This establishes replacement of an object-like allocation, but not its source-level type.

`1018:a9a0` receives the newly allocated object and bounded text. It clears bytes around object offset `+0x256`, calls methods through the object's table, and stores a result near `+0x94`. The nested pointer used by `1018:aeaa` is at object offset `+0x250`; the construction of that pointer is not fully isolated.

## Globals read

- `DAT_1060:01e4`: previous object pointer for release; then replaced with the new allocation.
- `DAT_1060:01de`: not read in this branch.
- `DAT_1060:0a36`: not read in this branch.
- `DAT_1060:01ec`: read indirectly by `1018:5496` for callback/UI dispatch.

## Globals written

- `DAT_1060:01e4`: old object released, then new object stored.
- `DAT_1060:01de`: assigned `field_06 + 1` from `1018:aeaa`.
- `DAT_1060:0a36`: cleared to zero.
- `DAT_1060:01e2`: cleared inside `1018:5496`.
- `DAT_1060:01ec` is used by `1018:5496`, but is not written by this path based on current evidence.

## Calls, in observed order

1. `1010:d03c` obtains the bounded text value.
2. The previous `DAT_1060:01e4` object is released through its first method slot when non-null.
3. `1008:ef2c(0x262)` allocates the replacement object.
4. Constructor-like table writes initialize the replacement object.
5. `1018:a9a0(new_object, text)` initializes object/nested state.
6. `1018:aeaa(new_object)` returns nested `field_06`.
7. `DAT_1060:01de = field_06 + 1`, with 16-bit wraparound semantics.
8. `DAT_1060:0a36 = 0`.
9. `1010:d0a6(new_object)` releases/replaces the object resource at `+0x04`, then calls `1008:8e46`.
10. `1018:5496(new_object, 1)` performs cleanup/event dispatch and callback/UI operations.

## Callback behavior

`1018:5496` invokes the replacement object's method at vtable offset `+0x1c` with event value `0x271b`. When its third byte parameter is nonzero—as in this branch—it also calls `1008:e412(10)`, then invokes `DAT_1060:01ec` methods at offsets `+0x124` and `+0x128` with `(0x000e, 0x6d73)`, `(0x0012, 0x6d73)`, and `(0x0006, 0x6d73)`.

The `+0x1c` method and the `DAT_1060:01ec` methods are not yet semantically identified. The current evidence supports an object callback plus framework/UI dispatch, not a gameplay interpretation.

## State changes

The first isolated application-level state mutation is:

```c
DAT_1060:01de = recovered_1018_aeaa(new_object) + 1;
DAT_1060:0a36 = 0;
```

`DAT_1060:01de` is subsequently incremented/decremented by `1018:267e` and clamped to `100` by `1018:438e`. On its observed `0x2714` branch, `1018:267e` passes the current global to `1018:ae3a`, which conditionally changes source slot-0 `+0x06` and calls `1018:a704` to update the ordered destination record at `DAT_1060:0a46`. `1020:1a9a` also passes the global to `1018:adb8` as a one-based selector for `nested + 0x08 + 2 * (value - 1)`. Its semantic label remains unresolved.

## External dependencies

- Win16 segmented pointers and far calls
- allocator/object framework at `1008:ef2c`
- framework/resource helper `1008:8e46`
- message-range helper `1008:e412`
- object callback tables
- global callback object at `DAT_1060:01ec`

## Confidence

High for the instruction-level sequence and the three direct state writes (`DAT_1060:01e4`, `DAT_1060:01de`, `DAT_1060:0a36`). Medium for the object replacement/lifecycle interpretation because constructor/destructor ownership and the origin of the nested pointer at `+0x250` are not fully isolated. Low for source-level semantic names.

## Control-flow diagram

```text
1008:ac80
  |
  +-- command == 0x2af9
        |
        +-- obtain bounded text with 1010:d03c
        +-- release old DAT_1060:01e4 object if non-null
        +-- allocate 0x262-byte replacement with 1008:ef2c
        +-- apply constructor-like method-table writes
        +-- 1018:a9a0(replacement, text)
        +-- 1018:aeaa(replacement)
        +-- DAT_1060:01de = field_06 + 1
        +-- DAT_1060:0a36 = 0
        +-- 1010:d0a6(replacement)
        +-- 1018:5496(replacement, 1)
        +-- return from dispatcher
```

## Phase 7 nested initialization detail

`1018:a9a0` initializes the replacement object's text and pending fields, invokes object methods at slots `+0x2c` and `+0x18`, and then takes one of two nested-pointer paths:

```text
method +0x18 != 0
    -> method +0x20
    -> method +0x44 returns far pointer
    -> store at parent +0x250/+0x252

method +0x18 == 0
    -> 1018:ab1c
    -> 1010:035c(...) returns far pointer
    -> store at parent +0x250/+0x252
    -> nested +0x06 = 0
```

The exact implementation behind method `+0x44` and the allocator/resource wrapper `1010:035c` remains unresolved.

## Phase 8 event consequence

For the final table `1008:2b04`, the `+0x1c` entry is the raw three-byte function `1008:e8b6: RETF 8`. `1018:5496` supplies event `0x271b` to that slot, but this final override reads no arguments and performs no calls or writes. The event is therefore absorbed for receivers using the final table; no event consequence beyond the call boundary is supported.

## Phase 10 sibling event transition

The `0x2714` active-byte branch of `1018:267e` does not replace `DAT_1060:01e4`. It calls `1018:ae3a(DAT_1060:01e4, DAT_1060:01de)`, conditionally calls `1018:ae7e`, clears `DAT_1060:0a36`, increments `DAT_1060:01de`, then invokes receiver slot `+0x114(1)` unless the incremented value is `0x66`, when it calls slot `+0x120`. This is a mutation sibling to `0x2af9`, not a proven common-dispatch path.
