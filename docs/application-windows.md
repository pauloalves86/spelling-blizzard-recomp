# Phase 4 Application Window Recovery

## Generic window constructors

Ghidra identifies the following direct callers:

### 1010:596e

- `1010:50ce`
- `1010:53c8`
- `1010:5722`

These callers are framework constructors/wrappers. They pass an object pointer and creation data to `1010:596e`, then set framework vtable pointers and object fields. The application-specific object supplying the final window data has not been proven.

### 1010:5b18

- `1008:0eba`
- `1008:6ef6`
- `1008:670a`
- `1010:50ce`
- `1010:53c8`
- `1010:5722`

These are also framework construction paths. `1008:0eba` allocates a `0x160`-byte object with `FUN_1008:eed8`, initializes it through `1008:6ef6`, calls `1010:5b18`, and stores the resulting pointer into fields at offsets `0x1c`/`0x1e` of its owner.

## WNDCLASS construction

`1010:596e` and `1010:5b18` pass the address `object + 0x10a` to `REGISTERCLASS`. Before this, they invoke the object's virtual method at a vtable-relative slot corresponding to `+0x14`.

The function then:

1. calls `GETCLASSINFO` using the class-name data at object-relative field `+0x120`;
2. calls `REGISTERCLASS` with the structure at `object + 0x10a` if the class is absent;
3. calls `CREATEWINDOW` using the object's class/name, geometry, style, parent, instance, and creation data fields;
4. stores the returned window handle at the beginning of the object;
5. obtains and stores a DC with `GETDC`;
6. invokes a second object method at the `+0x18`-equivalent vtable slot.

## Window procedure status

The `lpfnWndProc` value is part of the `WNDCLASS`-like structure at `object + 0x10a`, but the current callers do not write that structure directly. The virtual method at `+0x14` is responsible for preparing it, so the concrete window procedure is currently hidden behind object dispatch.

No single function can yet be named as the application's registered window procedure. The strongest current statement is:

```text
window object constructor
    -> virtual class-data initializer (+0x14 slot)
    -> WNDCLASS-like data at object+0x10a
    -> REGISTERCLASS
    -> USER window dispatch
    -> generic/object event handling
```

Confidence: High for the construction sequence; Unknown for the concrete `lpfnWndProc`.

## Application-level dispatcher

`1008:ac80` is a stronger application-facing event dispatcher than the generic constructors. It handles command-like values `0x2af9`, `0x2afa`, `0x2afb`, `0x32c9`, and `0x32ca`. For `0x2af9` it initializes the global object at `DAT_1060_01e4`, calls `1018:a9a0`, reads `1018:aeaa`, sets `DAT_1060_01de`, clears `DAT_1060_0a36`, calls `1010:d0a6`, and then calls an object callback. For `0x32c9` and `0x32ca` it repeatedly invokes object methods at slots corresponding to `+0x110` and `+0x114` while polling `1010:08bc`.

This function is likely in the application/controller layer, but the command IDs are not yet mapped to named Windows messages or menu commands.
