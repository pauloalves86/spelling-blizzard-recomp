#!/usr/bin/env bash
# Build Spelling Blizzard (sb_recomp) from your own copy of the game.
#
#   ./build.sh                 use the files already in original/
#   ./build.sh /path/to/game   copy BLIZZARD.EXE and SB_CDRES.DLL from there first
#
# The game's code is recompiled to C on your machine; nothing from the game is
# downloaded or distributed by this project.
set -euo pipefail
cd "$(dirname "$0")"

EXE_SHA=ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2
DLL_SHA=1c6c36bde51b8c9ad3b3a0eaba0283d0468d81326b3dbe4dbfbf7305863b70e1
BUILD_DIR=${BUILD_DIR:-build/release}

die() { echo "error: $*" >&2; exit 1; }

sha256() {
    if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1
    else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

# Find a file in a directory regardless of case (CD copies are often lowercase).
find_ci() { find "$1" -maxdepth 1 -iname "$2" -print -quit 2>/dev/null; }

if [ $# -ge 1 ]; then
    src=$1
    [ -d "$src" ] || die "$src is not a directory"
    mkdir -p original
    for f in BLIZZARD.EXE SB_CDRES.DLL; do
        p=$(find_ci "$src" "$f")
        [ -n "$p" ] || die "$f not found in $src"
        cp "$p" "original/$f"
    done
fi

for f in BLIZZARD.EXE SB_CDRES.DLL; do
    [ -f "original/$f" ] || die "original/$f is missing. Run ./build.sh /path/to/your/game, or copy it there yourself."
done
[ "$(sha256 original/BLIZZARD.EXE)" = "$EXE_SHA" ] ||
    die "original/BLIZZARD.EXE is not the supported version (1.10.CD for Windows)."
[ "$(sha256 original/SB_CDRES.DLL)" = "$DLL_SHA" ] ||
    die "original/SB_CDRES.DLL is not the supported version."

for tool in cmake pkg-config cc; do
    command -v "$tool" >/dev/null || die "$tool not found; see Requirements in README.md"
done

if [ ! -f third_party/tinycc/libtcc.c ]; then
    [ -e .git ] || die "third_party/tinycc is empty: get the source with its TinyCC submodule (git clone --recursive)."
    git submodule update --init third_party/tinycc || die "fetching third_party/tinycc failed"
fi

jobs=$( (nproc || sysctl -n hw.ncpu) 2>/dev/null || echo 4)
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j "$jobs"

echo
echo "Built $BUILD_DIR/sb_recomp. Play with:"
echo "  $BUILD_DIR/sb_recomp"
echo "(run it from this directory: it plays original/ with saves in run/, unless"
echo " you kept a copy with --prepare; see README.md)"
