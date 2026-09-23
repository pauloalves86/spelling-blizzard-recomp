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

The shared class-data helper `1010:5bd8` writes the `WNDCLASS`-like fields directly. In particular, it stores `0x5db2` at `object + 0x10c` and `0x1010` at `object + 0x10e`, so `lpfnWndProc` is the explicit far pointer `1010:5db2`. `1010:5348` and `1010:5824` call this helper; `1008:6fa0` calls `1010:5348`. This proves the callback target for classes using these initializer paths, not necessarily every class in the executable.

Ghidra has no function boundary at `1010:5db2`. Raw 16-bit disassembly of that target shows a window-procedure-shaped handler that:

- obtains the per-window object through `GETWINDOWLONG(hwnd, 0)` and falls back to `DEFWINDOWPROC` when absent;
- handles `WM_CREATE` (`0x0001`) by taking the object pointer from the creation data, storing it with `SETWINDOWLONG(hwnd, 0, object)`, storing the HWND at object offset `+0x04`, and returning zero;
- handles `WM_PAINT` (`0x000e`) with `BEGINPAINT`, the object's virtual slot `+0x10`, and `ENDPAINT`;
- routes messages in `0x0200`-`0x0209` through the object's slot `+0x0c`, with small event values; and
- routes other messages with an attached object through slot `+0x04`; message `0x0010` also calls `POSTQUITMESSAGE(0)`.

The known constructor tables identify `1010:527c` or `1010:553e` at slot `+0x0c`; these handlers translate coordinates and update cursor-related state. Paint slots include `1008:6dfc`, `1010:559e`, and the `RETF 8` stub at `1010:5d62`. The common `+0x18` post-create callback is `1010:5d66`, whose raw bytes call `SHOWWINDOW`.

The procedure address and its message-dispatch behavior are high-confidence facts for the paths above. The runtime object stored at window offset `+0x154`, and whether it is the startup-created `DAT_1060:07c8` object, remain unresolved. Therefore the complete paint-to-application-owner chain is not yet proven.

```text
window object constructor
    -> virtual class-data initializer (+0x14 slot)
    -> WNDCLASS-like data at object+0x10a
    -> lpfnWndProc = 1010:5db2
    -> REGISTERCLASS
    -> USER window dispatch
    -> GETWINDOWLONG(hwnd, 0) -> object vtable slots +0x04/+0x0c/+0x10
```

See `docs/message-loop-trace.md` for the caller graph, the startup polling loop, static paint-table edges, and remaining ownership limits.

## Application-level dispatcher

`1008:ac80` is a stronger application-facing event dispatcher than the generic constructors. It handles command-like values `0x2af9`, `0x2afa`, `0x2afb`, `0x32c9`, and `0x32ca`. For `0x2af9` it initializes the global object at `DAT_1060_01e4`, calls `1018:a9a0`, reads `1018:aeaa`, sets `DAT_1060_01de`, clears `DAT_1060_0a36`, calls `1010:d0a6`, and then calls an object callback. For `0x32c9` and `0x32ca` it repeatedly invokes object methods at slots corresponding to `+0x110` and `+0x114` while polling `1010:08bc`.

This function is likely in the application/controller layer, but the command IDs are not yet mapped to named Windows messages or menu commands.
