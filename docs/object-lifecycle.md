# Phase 6 object lifecycle

## Object represented by DAT_1060:01e4

This is an object-like far pointer used by the `0x2af9` branch of `1008:ac80` and by the input/controller path at `1018:509c`. A source-level class name is not established.

## Allocation

The `0x2af9` branch calls `1008:ef2c` with size `0x262` after releasing the previous value of `DAT_1060:01e4`. The allocation result is treated as a segmented object pointer.

## Initialization

The branch performs sequential constructor-like writes to the object's first method-table pointer. Observed table values include:

```text
0bc8:1008
2810:1008
2b60:1008
2b04:1008  (final visible table pointer)
```

These overwrites are consistent with layered base/derived initialization, but the exact constructor boundaries are not proven. `1018:a9a0` then initializes nested/text-dependent state, including object-relative data around `+0x94`, `+0x250`, and `+0x256`.

## Storage

The new pointer is stored in `DAT_1060:01e4`. The global is subsequently consumed by other application paths, including `1018:509c`, which also initializes or uses it during its `CHEAT` input path.

## Users

- `1018:a9a0`: text/nested-state initialization
- `1018:aeaa`: reads `+0x250 -> +0x00 -> +0x06`
- `1010:d0a6`: releases/replaces the resource pointer at object `+0x04`, then invokes `1008:8e46`
- `1018:5496`: releases object field `+0x82`, invokes object method `+0x1c`, and performs callback/UI dispatch
- `1018:509c`: shares the global and several related application operations, but no common dispatch path is proven

## Replacement

Before allocation, the old `DAT_1060:01e4` value is tested. When non-null, its first virtual method is called with the old far pointer, and only then is a new allocation installed. This is direct evidence of an old-object release/destruction boundary.

## Destruction

For objects installed by the known `0x2af9` and `1018:509c` construction paths, destruction is now instruction-backed: final table `1008:2b04` slot `+0x00` is `1018:aa84`, and replacement calls the old object's slot 0. `1018:aa84` conditionally flushes parent/nested state when `+0x254` is nonzero, releases `+0x250/+0x252` through `1010:01d0`, then invokes `1008:e812(parent)`. `1008:e812` calls parent slot `+0x10` and `1008:ef8e`, which calls `1000:2032(parent)`.

This does not prove the source-level class or rule out an unsupported external write of a differently tabled pointer into `DAT_1060:01e4`.

## Survival

The replacement survives beyond the `0x2af9` branch because it remains in `DAT_1060:01e4` and is referenced by later application paths. No evidence currently establishes its final destruction point.

## Relationship to 1008:2bf4

`1008:2bf4` is a separate global/object initialization boundary that stores objects in `DAT_1060:0a14`, `DAT_1060:0a1e`, and `DAT_1060:07c4`. Current Phase 6 evidence does not prove that it allocates or initializes the object stored in `DAT_1060:01e4`. The matching framework style is suggestive, but the relationship remains unproven.

## Lifecycle diagram

```text
old DAT_1060:01e4
        |
        +-- final table 1008:2b04 slot +0x00 -> 1018:aa84
        |
        v
1008:ef2c(0x262)
        |
        v
constructor-like method-table writes
        |
        v
1018:a9a0(replacement, bounded_text)
        |
        v
DAT_1060:01e4 = replacement
        |
        +-- 1018:aeaa reads nested +0x06
        +-- 1010:d0a6 updates resource at +0x04
        +-- 1018:5496 releases +0x82 and dispatches callbacks
        |
        v
1010:01d0(nested +0x250) -> 1008:e812 -> parent cleanup
```

## Unknowns

- Exact class/type identity
- Constructor function boundaries

- Origin of nested pointer at `+0x250`
- Whether the allocation is shared with any other global
- Semantic meaning of `field_06`, `DAT_1060:01de`, and `DAT_1060:0a36`

## Phase 7 nested-object lifecycle

The `+0x250/+0x252` far pointer is installed during `1018:a9a0`, not by the raw allocator alone. The normal path obtains it from the replacement object's method slot `+0x44` and stores it at `1018:aa63`/`1018:aa68`. If the receiver method at `+0x18` returns zero, `1018:ab1c` is used instead; it obtains a far pointer through `1010:035c` and stores it at `1018:ab49`/`1018:ab4e`.

The fallback initializes the nested object's `+0x06` to zero, clears its repeated entry area, sets later entries to `10`, clears flag bytes near `+0x19a`, and writes `1000` at `+0x1a4`. This is the first direct lifecycle evidence for the object returned by `1018:aeaa`.

`1018:aa84` always passes the parent-held far pointer at `+0x250/+0x252` to `1010:01d0`, then calls `1008:e812`. It conditionally performs parent update callbacks only when parent byte `+0x254` is nonzero, and clears that byte afterward. Phase 9 proves this path is reached through the old `DAT_1060:01e4` first-slot release for objects produced by the known final-table construction paths.

Phase 10 found no direct static path from `1018:a704`'s `+0x254 = 1` write to an immediate `1018:aa84` call. The proven consumption remains part of later final-table release; mutation-to-release timing is unknown.
