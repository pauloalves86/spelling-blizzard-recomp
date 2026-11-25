# Toolchain and Runtime Hypothesis

## Direct evidence

- `file` identifies a 16-bit Windows 3.10 NE executable.
- The NE import modules are `KERNEL`, `GDI`, `USER`, `KEYBOARD`, `MMSYSTEM`, and `SHELL`.
- Ghidra resolved Win16 exports including `INITTASK`, `DOS3CALL`, `GLOBALALLOC`, `OPENFILE`, `_HREAD`, `_HWRITE`, `INITAPP`, GDI bitmap/DC/text/palette functions, USER message/window functions, and MMSYSTEM waveOut functions.
- The binary contains Microsoft-style runtime diagnostic strings: `R6000`, `R6003`, `R6008`, `R6009`, `R6018`, `R6020`, `R6021`, `R6002`, `run-time error`, and `ABNORMAL PROGRAM TERMINATION`.
- The binary contains `_C_FILE_INFO=`, `__WINFLAGS`, `__AHSHIFT`, `__AHINCR`, and `FailNIL(%s) failed!`.
- Ghidra's decompiler identifies segmented x86 calling conventions and far functions such as `__cdecl16far` and `__stdcall16far`.
- The strings include C++-style object/framework names and a `Subclass Responsibility` diagnostic.

## Hypothesis

The executable was likely built with a Microsoft 16-bit C/C++ toolchain and Microsoft Windows 3.x runtime/framework components, probably using Microsoft C/C++ libraries or a closely related Bright Star application framework.

Confidence: medium.

## Why this is not definitive

The R600x diagnostics and Win16 import set are strong compatibility evidence, but runtime libraries can be statically linked, reused by other compilers, or incorporated into a vendor framework. The binary does not yet provide a definitive compiler banner, linker signature, or build record. Do not label the compiler as proven Microsoft C/C++ until startup byte patterns, library fingerprints, or a known-library comparison confirms it.

## Ruled out or unsupported hypotheses

- DOS/4GW or another DOS extender: unsupported. The real image is NE, not a DOS-extender LE/LX payload; the executable imports Win16 system modules.
- DJGPP: unsupported by the observed NE/Win16 format and imports.
- 32-bit Watcom/PE-style build: unsupported by the 16-bit NE format.
- Direct Sound Blaster/AdLib/OPL implementation: not supported by current evidence. The imported audio interface is Windows MMSYSTEM waveOut, although a DLL may hide additional behavior.
- Direct VGA BIOS graphics: not supported by current Ghidra instruction findings. Graphics are strongly represented by GDI APIs.

These are format/interface conclusions, not claims about every possible dynamically loaded component.

## Reconstruction compiler candidate

Open Watcom 2.0 has been installed locally as a reconstruction tool and successfully emits 16-bit Windows large-model OMF objects. The tested compiler identifies itself as Open Watcom C x86 16-bit, version 2.0 beta. The downloaded Linux x64 package was verified against SHA-256 `637c56498b8e1a283832eb4b30595dbfb817a0ee4578f2d37be0f3237cb03b3a`. This establishes a usable Win16 ABI build path; it does not identify the compiler used for the original executable. See [local setup](local-setup.md) and the [Open Watcom 2.0 C/C++ User's Guide](https://open-watcom.github.io/open-watcom-v2-wikidocs/cguide.html).
