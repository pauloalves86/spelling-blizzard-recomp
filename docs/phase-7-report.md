# Phase 7 report

## Summary

Phase 7 established the origin and maintenance paths for the nested object used by `1018:aeaa`. The central result is that `replacement + 0x250` is installed during `1018:a9a0`, not left as an unexplained field of the `0x2af9` allocation.

The canonical executable was used read-only. No differential execution against the original was performed.

## Proven facts

- `1018:aeaa` reads `parent + 0x250`, then nested `+0x00`, then returns nested `+0x06`.
- `1018:a9a0` writes the parent far pointer at `+0x250/+0x252` from the return of its object method `+0x44`.
- If the object's method `+0x18` returns zero, `1018:a9a0` calls `1018:ab1c`, which obtains another far pointer through `1010:035c` and writes it to the same offsets.
- `1018:ab1c` initializes nested `+0x06` to zero.
- `1018:a704` reads nested `+0x06` as part of ordered entry processing.
- `1018:ae3a` conditionally writes nested `+0x06` when its candidate is larger.
- `1018:5496` explicitly pushes event `0x271b` to the replacement object's method slot `+0x1c`.
- The only literal `0x271b` found by the focused scan is the push in `1018:5496`.
- `DAT_1060:01de` has readers/writers in application handlers, including `1018:509c`, `1018:267e`, `1018:284c`, `1018:438e`, `1020:1a9a`, `1020:678c`, `1018:3ee0`, and `1008:ac80`.
- `DAT_1060:0a36` is read/written by `1018:267e`, written by `1018:509c`, and cleared by `1008:ac80`.

## Object graph

```text
DAT_1060:01e4
  -> replacement object
       +0x250/+0x252 -> nested object
                            +0x00 text
                            +0x06 field returned by 1018:aeaa
                            +0x08 repeated entry storage
                            +0x38 entry count
                            +0x19a flags
                            +0x1a4/+0x1a6 accumulated value
                            +0x1a8 auxiliary word
```

The graph and instruction-backed writers are documented in `docs/object-graph.md`.

## +0x250 origin

The strongest direct sequence is:

```text
1018:a9a0
  -> receiver method +0x18
  -> if nonzero: receiver method +0x20, then receiver method +0x44
  -> store returned far pointer at parent +0x250/+0x252
  -> clear parent +0x254
```

The fallback is:

```text
1018:a9a0
  -> 1018:ab1c
  -> 1010:035c(0x1aa, ...)
  -> store returned far pointer at parent +0x250/+0x252
```

The exact nested allocator behind `1010:035c -> 1010:023e` was not fully reconstructed.

## field_06 data flow

Confirmed operations:

- initialization to zero in `1018:ab1c`;
- read and return in `1018:aeaa`;
- candidate-maximum update in `1018:ae3a`;
- read as `local_e` in `1018:a704` while comparing/inserting nested entries;
- use by `1008:ac80` as `DAT_1060:01de = field_06 + 1`.

The value is not proven to be a score, level, index, or player identifier. It participates in nested entry ordering and has a monotonic-update helper, but no complete range invariant has been established.

## DAT_1060:01de

Producer/consumer map:

```text
DAT_1060:01de
├── writer: 1008:ac80 / 0x2af9 -> field_06 + 1
├── writer: 1018:509c
├── increment/decrement/read-write: 1018:267e
├── clamp/write: 1018:438e, maximum 100
├── comparisons/branching: 1018:284c
├── reads: 1018:3ee0
├── reads: 1020:1a9a
├── reads: 1020:678c
└── additional reads: 1018:2646 and other framework/application paths
```

Supported behavioral description: a 16-bit bounded application value derived from nested state during `0x2af9`, adjusted by event handlers, and compared with several thresholds. Its domain name remains unresolved.

## DAT_1060:0a36

`1018:267e` reads and writes this byte for events `0x2712`, `0x2714`, `0x2718`, and `0x2719`. `1018:267e` uses it to choose whether to update `DAT_1060:01de` or dispatch through object slots around `+0x114`, `+0x118`, and `+0x124`. `1008:ac80` clears it after deriving `DAT_1060:01de`; `1018:509c` also writes it.

The evidence supports a transition/control byte. It does not establish a boolean, timer, dirty flag, or rendering-specific interpretation.

## 1018:5496 callbacks

The receiver callback target is selected indirectly through the replacement object's method table at `+0x1c`; the table pointer installed by the `0x2af9` path ends at `1008:2b04`, whose `+0x1c` far entry is `1008:e8b6`. Ghidra does not currently define a function at that target, so its implementation remains an unresolved callback boundary.

The global object at `DAT_1060:01ec` is read through slots `+0x124` and `+0x128`, but its runtime pointer is zero in the analyzed file image and no static initializer was identified. Its concrete callback targets therefore cannot be resolved from current static evidence.

## Event 0x271b

`0x271b` is an application/object event value explicitly passed by `1018:5496` to the receiver's method at `+0x1c`. The focused literal scan found no other literal consumer. `1018:267e` has a separate dynamic case for `0x271b` that dispatches through its receiver's `+0x128` method, but that value arrives as a parameter rather than a literal in the scanned instructions. The event category is unresolved.

## Command 0x2af9 origin

No literal producer for `0x2af9` was found beyond the comparison in `1008:ac80`. A related callback at `1010:2a7e` explicitly emits `0x2afa` through an object method at `+0x68`, proving that sibling commands can be generated by indirect object callbacks. The origin of `0x2af9` remains unresolved at the command-argument boundary.

## Relationship to 1018:509c

The paths share `DAT_1060:01e4`, `1018:a9a0`, `1018:aeaa`, and `1010:d0a6`. `1018:509c` also writes `DAT_1060:0a36` and `DAT_1060:01de` in its own input path. No evidence proves that it calls `1018:5496`, uses the same final method table, or receives the same dispatch event. Shared state and callees establish application coupling, not a common dispatch origin.

## Native code added

No new helper beyond the Phase 6 state-transition helper was required. Existing native components remain:

- `nested_field.c`: literal `1018:aeaa` pointer chain;
- `application_state.c`: literal `field_06 + 1` and `DAT_1060:0a36 = 0` writes.

The nested initializer was not ported because it depends on unresolved segmented allocation, object methods, text/resource helpers, and callback side effects.

## Analysis artifacts

Focused reports added:

- `ghidra/NestedObjectTrace.java`
- `ghidra/Field06Trace.java`
- `ghidra/StateVariableTrace.java`
- `ghidra/CallbackTargets.java`
- `ghidra/CallbackFunctionDump.java`
- `ghidra/CallbackLinearDump.java`
- `ghidra/Event271bScan.java`

Reports were run with the project's Ghidra installation and the existing Win16 NE project.

## Remaining unresolved items

- exact allocation size and constructor internals behind `1010:035c`/`1010:023e`;
- exact implementation at `1008:e8b6`;
- runtime value and callback targets of `DAT_1060:01ec`;
- exact release chain from old `DAT_1060:01e4` to its nested object;
- semantic names for nested `+0x06`, `DAT_1060:01de`, and `DAT_1060:0a36`;
- ultimate producer of command `0x2af9`.

## Verification

Native build and tests completed successfully after the Phase 7 analysis. The canonical executable hashes remain unchanged and match the project identity.
