# Local reverse-engineering setup

This checkout has a local Ghidra 12.2 installation at
`.local/tools/ghidra_12.2_PUBLIC/`. The official release archive was checked
against SHA-256
`2c2cd776db3c1f6a097ba418922378ac3649cadd5e844a6f5f088ae628a7be67` before
extraction. Java 25, Python 3, CMake, Make, and GCC are available on this
machine.

Open Watcom 2.0 is also unpacked under `.local/tools/openwatcom/`. The official
Linux x64 C package was verified against SHA-256
`637c56498b8e1a283832eb4b30595dbfb817a0ee4578f2d37be0f3237cb03b3a`. This
compiler supports 16-bit Windows output, making it a usable ABI/build
candidate; it is not evidence that Open Watcom produced the original game.

The Ghidra project is `ghidra/local/SpellingBlizzard.gpr`; its database is
stored beside it in `ghidra/local/SpellingBlizzard.rep/`. Both the local tool
installation and this generated project are ignored by Git. The tracked
`ghidra/SpellingBlizzard.gpr` remains an empty placeholder, so do not open it
as the working project.

Launch the GUI from the repository root:

```sh
./.local/tools/ghidra_12.2_PUBLIC/ghidraRun
```

Open the `ghidra/local/SpellingBlizzard` project in the project chooser. To
run analysis headlessly against a newly imported copy of the canonical
executable, use a separate project name or location:

```sh
./.local/tools/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  ghidra/local SpellingBlizzard -import original/BLIZZARD.EXE \
  -processor 'x86:LE:16:Protected Mode' -cspec default
```

The current local project was freshly imported and auto-analyzed with the
same NE loader and language. It does not contain the prior computer's saved
Ghidra analysis database; the tracked Java analysis scripts and Markdown
reports remain available to continue the documented investigations.

Build the recovered C units as a 16-bit Windows OMF library:

```sh
cmake -S . -B build/native
cmake --build build/native --target win16_objects
```

The output is `build/win16/recovered_win16.lib`; the build also emits a
disassembly and checks `RETF 4` / `RETF 6` for the three current direct Win16
entrypoints. This is a callable object library, not a reconstructed NE game
executable. The host CMake libraries remain available for behavioral-model
work on this machine.
