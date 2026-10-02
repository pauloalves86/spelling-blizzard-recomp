# Root Desktop View Lifecycle and Routing (`DAT_1060:0810`)

## Question

Who allocates and populates `DAT_1060:0810`, what is its concrete method table and class hierarchy, and how does it route mouse input into the view tree? (Settles Open Edge 1 from `docs/input-to-command.md` and Open Item 1 from `docs/method-table-inventory.md`).

## Method

- Raw disassembly: `objdump -D -b binary -m i8086 -M intel` on segment slices of `original/BLIZZARD.EXE`.
- Far-pointer and relocation resolution: `tools/ne_farptr.py`, `tools/ne_vtables.py`, and `ghidra/Step15Refs.java` (read-only headless execution).
- Executable hash unchanged: `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`.

---

## 1. Identity and Class Hierarchy (fact)

`DAT_1060:0810` (16:16 pointer stored at `1060:0810` / `1060:0812` in DGROUP) is the application's **Desktop / Screen Root View** (`TDesktopView` / `TScreenView`).

- **Allocation size**: `0x30` bytes (allocated via `1008:ef2c`).
- **Installed method table**: **`1008:7d10`** (60 entries).
- **Class constructor chain**:
  ```text
  1008:0bc8 (base object)
    -> 1008:07f4 (view base, 12 slots)
      -> 1008:0838 (view / controller base, 51 slots)
        -> 1008:0958 (desktop screen view base, 60 slots)
          -> 1008:7d10 (Spelling Blizzard specialized desktop view, 60 slots)
  ```
  Note: The common UI controls, windows, and buttons derive from `1008:0838` via `1008:1aac` (the 68-slot view family). Hence `DAT_1060:0810` and UI views share common method slots `+0x00` through `+0xc8`.

---

## 2. Allocation and Construction Path (fact)

The desktop view is instantiated during application startup as part of root object initialization:

```text
NE entry 1000:001a -> 1000:00d8 -> 1008:2bf4
1008:2bf4:
  - Allocates 0x28 bytes for application root object DAT_1060:07c4 (table 1008:2e32)
  - Calls root->+0x98 (1008:72b6)
1008:72b6:
  - Calls root base initializer 1008:0000
1008:0000:
  - Initializes root fields (byte +0x1b = 1, byte +0x1a = 0)
  - At 1008:0134: calls root virtual slot +0x6c: self->+0x6c()
```

### Virtual Slot `+0x6c` on Root Object:
- In base table `1008:2f5a`, slot `+0x6c` is `1008:02c6` (installs base desktop view table `1008:0958`).
- In `Spelling Blizzard`'s derived table `1008:2e32`, slot `+0x6c` is overridden by **`1008:7194`**:

```text
1008:7194 (RETF 0x4):
  1. Calls 1008:ef2c(0x30) to allocate the 0x30-byte object.
  2. Executes constructor table chain:
       1008:0bc8 -> 1008:07f4 -> 1008:0838 -> 1008:0958 -> 1008:7d10
  3. Stores the allocated far pointer in DAT_1060:0810 (and DAT_1060:0812).
  4. Calls 1008:3180(this, [bp+6]):
       - USER!LOADCURSOR(hInstance=ds:0x7d0, 0x80) -> stores in DAT_1060:07d6
       - USER!LOADCURSOR(hInstance=ds:0x7d0, 0x81) -> stores in DAT_1060:07d4
       - Calls base view initializer 1008:d1d8(this, [bp+6])
```

### Base Desktop View Initializer `1008:d1d8`:
1. Calls `1010:4576` (ancestor view initializer).
2. Measures display resolution:
   - `USER!GETDC(0)`
   - `GDI!GETDEVICECAPS(hdc, HORZRES=8)`
   - `GDI!GETDEVICECAPS(hdc, VERTRES=10)`
   - `1010:0fb6(&this->rect_20, 0, 0, cx, cy)`: sets desktop bounding rectangle `+0x20` to full screen dimensions.
   - `USER!RELEASEDC(0, hdc)`
3. Sets state bytes `this->byte_10 = 1; this->byte_11 = 1; this->byte_12 = 1`.
4. Allocates and constructs child view collection list (`0x1c` bytes, table `1008:0a48`) at `this->+0x28`.
5. Clears `this->+0x2c = 0`.
6. Invokes virtual method `this->+0xc4()`:
   - In table `1008:7d10`, slot `+0xc4` is **`1008:3218`**.
   - `1008:3218` allocates `0x158` bytes via `1008:eed8` and invokes window constructor `1008:670a` -> `1010:5722` (table `1008:681a`), passing `this` (`DAT_1060:0810`) as argument.
   - Window constructor stores `DAT_1060:0810` into window field `+0x154` (resolving creator for constructor `1010:5722`).
   - Stores the resulting window object pointer in `this->+0x1c` (`DAT_1060:0810->+0x1c`).

---

## 3. Object Layout of `DAT_1060:0810` (fact)

| Offset | Size | Type / Description | Initialized by |
|---|---|---|---|
| `+0x00` | 4 | Far pointer to method table (`1008:7d10`) | `1008:71af` |
| `+0x04` | 4 | Parent/owner link (NULL for root desktop view) | `1010:4576` |
| `+0x10` | 1 | Byte flag (1 = active/visible) | `1008:d233` |
| `+0x11` | 1 | Byte flag (1 = enabled) | `1008:d237` |
| `+0x12` | 1 | Byte flag | `1008:d23b` |
| `+0x1c` | 4 | Far pointer to owned native window object (`1008:681a`) | `1008:324e` |
| `+0x20` | 8 | Screen bounding `Rect` `(left=0, top=0, right=HORZRES, bottom=VERTRES)` | `1008:d21c` |
| `+0x28` | 4 | Far pointer to child view collection (`1008:0a48`) | `1008:d292` |
| `+0x2c` | 4 | Focus / modal view link (initialized to NULL) | `1008:d2b8` |

---

## 4. Teardown / Destruction (fact)

On application exit, root destructor `1008:0762` tests `DAT_1060:0810`:
```assembly
767: cmp DWORD PTR ds:0x810, 0
76d: je 0x77b
76f: les bx, DWORD PTR ds:0x810
775: les bx, DWORD PTR es:[bx]
778: call DWORD PTR es:[bx]  ; slot +0x00: 1008:31b8
```

Method `1008:31b8` (slot `+0x00` of `1008:7d10`):
1. `USER!DESTROYCURSOR(ds:0x7d6)`
2. `USER!DESTROYCURSOR(ds:0x7d4)`
3. Calls base destructor `1008:d2d2`:
   - Clears `DAT_1060:0808` if equal to `this`.
   - Destroys child view list in `this->+0x28` via slot `+0x54`.
   - Calls ancestor destructor `1010:46e0`.

---

## 5. Input Routing via Active View `DAT_1060:01ca` (fact)

In Step 15, mouse-down was traced from `1010:3bfe` to `DAT_1060:0810->+0x50(&rec)`.

`DAT_1060:0810`'s slot `+0x50` is **`1008:d410`** (overriding base `1010:491e`):
```text
1008:d410:
  target = DAT_1060:01ca
  if target != NULL and target->byte_12 != 0:
      this->+0xec()
      return target->+0x50(&rec)
  else:
      DAT_1060:0a22 = this
      return this->+0x58(&rec)
```

### Who sets `DAT_1060:01ca`?
`DAT_1060:01ca` is the **active front view/window**:
- On window activation / mouse message in window procedure (`1010:553e`, slot `+0x0c` of window):
  `DAT_1060:01ca` is set to `window->+0x154` (or `window->+0x154->+0x18`) via `1010:5561` / `1010:5594`.
- On window deactivation (`1010:40d1`):
  `DAT_1060:01ca` is cleared to `0:0`.

When `target` is active, `target->+0x50(&rec)` executes `1010:491e` on the active view, initiating the child hit-test traversal down to the clicked button (`1008:b0c8`), which triggers command dispatcher `1008:ac80`.
