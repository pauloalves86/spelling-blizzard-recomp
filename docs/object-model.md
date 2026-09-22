# Initial Object Model Recovery

## Evidence

The executable contains C++-style names including `CWindow`, `CDesktop`, `CEditText`, `CAMEditText`, `SpellingDemonsWindow`, and `SpellingDemonsDesktop`. It also contains `Subclass Responsibility` and object-like names such as `theObject`, `theMissile`, and `itsSpellingDemonObjectGrid`.

These strings establish an object-oriented framework vocabulary. They do not prove that each string names a one-to-one source-level class.

## Confirmed framework shape

Functions `1010:596e` and `1010:5b18` both:

1. receive a large object pointer;
2. access the first words of that object as function-table pointers;
3. conditionally register a Windows class using `REGISTERCLASS`;
4. call `CREATEWINDOW`;
5. store the returned window handle;
6. obtain and store a DC with `GETDC`;
7. optionally invoke another object-relative callback.

This supports the following provisional shape:

```c
struct RecoveredWindowObject {
    /* field offsets are not yet semantically named */
    uint16_t field_00_vtable_segment;
    uint16_t field_02_vtable_offset;
    uint16_t field_04_window_handle;
    uint16_t field_06_device_context;
    uint8_t  unknown_08[...];
    /* class/window geometry and callback fields occur at larger offsets */
};
```

This is intentionally not emitted as a production C struct yet because the exact object size, pointer widths, and field ownership remain unresolved.

## Phase 5 nested accessor evidence

`1018:aeaa` confirms a nested pointer chain distinct from the generic window objects:

```text
parent + 0x250 -> holder
holder + 0x00 -> value object
value + 0x06  -> uint16_t returned in AX
```

This is a mechanically described layout only. The native test representation is in `src/recovered/application/nested_field.h`; no complete parent class is claimed.

## Vtable/function-pointer evidence

- `1010:596e` reads a method pointer from the object at a vtable-relative offset corresponding to `+0x14` before class/window creation and later invokes a method at `+0x18`.
- The `WNDCLASS`-like record passed to `REGISTERCLASS` is located at `object + 0x10a`; its window procedure is therefore supplied indirectly by the `+0x14` class-data initializer.
- `1010:3d56` invokes a callback at a vtable-relative offset corresponding to `+0x40` while processing messages.
- `1008:2bf4` constructs objects by writing repeated far offset/segment pairs into allocated memory, then invokes methods at slots such as `+0x58`, `+0x84`, and `+0x98`.
- `1018:509c` invokes callbacks through object-relative tables and performs game-facing state changes.

These are strong indicators of virtual dispatch or a structurally similar callback table. They do not establish C++ ABI details or prove that `1018:509c` is in one of the statically initialized tables.

## Current class/object classifications

| Evidence | Classification | Confidence |
|---|---|---|
| `CWindow`, `CDesktop`, `CEditText`, `CAMEditText` strings | framework/UI names or metadata | Medium |
| `SpellingDemonsWindow`, `SpellingDemonsDesktop` strings | likely application-specific window names | Medium |
| `1010:596e`, `1010:5b18` Win16 class/window creation | generic window framework | High |
| `itsLayeredOffscreen`, redraw/order/removal lists | rendering/object framework vocabulary | Medium |
| `theMatch`, `HighScor`, `CHEAT`, `thePlacedLadder` | likely game-specific state/debug vocabulary | Medium |
| `itsSpellingDemonObjectGrid` | likely game-specific object/grid state | Medium |

## Next object-model work

1. Trace callers of `1010:596e` and `1010:5b18`.
2. Record the object pointer passed at each call site and its allocation origin.
3. Trace the callback at the `+0x40`-equivalent vtable slot into concrete implementations.
4. Map `1018:509c` callers and the object fields at offsets `0x52`, `0x5e`, `0x66`, and `0x6a`.
5. Only then define evidence-backed C structs with `field_XX` names.

## Phase 6 replacement object

The `0x2af9` branch allocates `0x262` bytes, performs sequential method-table writes ending with `1008:2b04`, and stores the result in `DAT_1060:01e4`. A non-null old value is released through its first method before replacement. This is a confirmed object-like lifecycle, but `1008:2bf4` has not been shown to construct this particular object.

The replacement object is passed to `1018:a9a0`, `1018:aeaa`, `1010:d0a6`, and `1018:5496`. The nested chain used by `1018:aeaa` remains `+0x250 -> +0x00 -> +0x06`; `1018:a9a0` is known to initialize data around `+0x250`/`+0x256`, but the exact constructor that installs the nested pointer is not isolated.

## Phase 8 table boundary

The final table `1008:2b04` contains valid code entries even where Ghidra did not create function objects. In particular, `+0x1c -> 1008:e8b6` and `+0x68 -> 1008:e838` both decode as `RETF 8` no-op overrides. These are code facts, not missing pointers. The `+0x1c` override absorbs `1018:5496` event `0x271b` for final-table receivers.

## Phase 9 final-table lifecycle evidence

The same final table has `+0x00 -> 1018:aa84`, `+0x10 -> 1008:ce88`, `+0x20 -> 1008:cf6c`, `+0x44 -> 1008:cfc2`, `+0x54 -> 1008:d0f2`, and `+0x5c -> 1008:e812`. Known replacement construction paths finish with this table before storing `DAT_1060:01e4`; their old-object slot-0 release therefore reaches `1018:aa84`.

This establishes a concrete table-mediated nested release sequence without establishing a C++ class name or an ownership model beyond the observed release responsibility.

## Phase 10 event-interface boundary

`1020:4e8e` calls an arbitrary receiver's table slot `+0x1c` with event `0x2714` when an indirect state word equals `8`. `1018:267e` is a separate event switch with a confirmed `0x2714` case. The binary does not yet establish that this emitter's receiver table maps slot `+0x1c` to `1018:267e`; retain them as producer and handler candidates separated by an unresolved table edge.
