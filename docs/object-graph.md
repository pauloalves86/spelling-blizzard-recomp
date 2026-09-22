# Phase 7 object graph

## Confirmed graph

```text
DAT_1060:01e4
    |
    +-- replacement object
            |
            +-- +0x250/+0x252 far pointer
                    |
                    +-- nested value object
                            +-- +0x00..+0x37: seven inline 8-byte entries
                            |     slot 0 text: +0x00; slot 0 value: +0x06
                            |     slot 1 begins: +0x08
                            +-- +0x38: entry count
                            +-- +0x19a..: byte flags
                            +-- +0x1a4/+0x1a6: 32-bit accumulated value
                            +-- +0x1a8: 16-bit field
```

The `+0x250` pointer is a 4-byte far pointer. `1018:aeaa` performs the next `+0x00` far-pointer load and reads the nested object's `+0x06` word.

## Origin of +0x250

Two instruction-backed writers are present:

1. `1018:a9a0` calls the replacement object's method at vtable slot `+0x44`. It stores the returned far pointer at `object + 0x250` and `object + 0x252` (`1018:aa63` and `1018:aa68`), then clears `object + 0x254` (`1018:aa78`).
2. `1018:ab1c`, reached from `1018:a9a0` when the object's method at `+0x18` reports zero, calls `1010:035c` through a framework allocation/resource path and stores the returned far pointer at the same parent offsets (`1018:ab49` and `1018:ab4e`).

Therefore the pointer is not an uninitialized field in the `0x2af9` allocation. It is established during `1018:a9a0`, either by the object's `+0x44` method or by the fallback `1018:ab1c` path.

## Nested object initialization

The fallback `1018:ab1c` path:

- obtains a far pointer from `1010:035c(0x1aa, ...)` and stores it at parent `+0x250`;
- copies the input text into nested offset `+0x00`;
- initializes nested `+0x06` to zero;
- clears the repeated entry area beginning at nested `+0x08` through the observed range;
- initializes later entries to `10`;
- clears byte flags around `+0x19a`;
- writes `1000` at nested `+0x1a4`;
- sets parent byte `+0x254`, calls object methods, then clears `+0x254`.

The exact allocation size and ownership routine inside `1010:035c` remain unresolved because Ghidra identifies it as a wrapper around `1010:023e` with an imperfect prototype.

## Maintenance and users

- `1018:a704` performs ordered entry insertion/replacement against the nested object and sets parent `+0x254`.
- `1018:ae3a` conditionally raises nested `+0x06` and sets parent `+0x254`.
- `1018:ae7e` sets a nested flag byte and parent `+0x254`.
- `1018:aede` writes nested `+0x1a8` and parent `+0x254`.
- `1018:aec0` reads a nested flag byte.
- `1018:aa84` uses parent `+0x254` as an update indicator, releases the object at parent `+0x250/+0x252` through `1010:01d0`, then calls `1008:e812`.

## Phase 9 lifecycle closure

For known `DAT_1060:01e4` construction paths, final table `1008:2b04` has slot `+0x00 = 1018:aa84`. The old global object's first-slot release therefore reaches `1018:aa84` on its next replacement. That routine conditionally processes `+0x254`, always passes `+0x250/+0x252` to `1010:01d0`, then calls `1008:e812` for parent cleanup. `+0x250` is not cleared before the nested release.

## Ownership conclusion

The nested object is created or obtained during replacement-object initialization and is reachable through the parent. Known final-table parent destruction releases it through `1010:01d0`, supporting parent-held release responsibility. Exclusive ownership versus sharing remains unproven.

## Phase 8 correction and behavior

`1018:a704` proves that `+0x00` is the start of slot 0, not merely a standalone text field followed by entries at `+0x08`. Each slot is eight bytes: a NUL-terminated text area at slot `+0x00` and a signed word at slot `+0x06`. There are seven inline physical slots, and `+0x38` is the active count.

The `+0x06` field returned by `1018:aeaa` is slot-0's value. It is initialized to zero by `1018:ab1c`, conditionally raised by `1018:ae3a`, and transferred by `1018:a704` into the ordered destination record. See `docs/nested-object-behavior.md`.

## Confidence

- Far-pointer representation and `+0x250` writers: high.
- Nested `+0x00`, `+0x06`, entry-count, and flag offsets: high for observed layout.
- Exact nested allocation size and destructor ownership: medium/low.
- Source-level semantic names: intentionally unresolved.
