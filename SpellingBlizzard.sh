#!/usr/bin/env bash
# Spelling Blizzard: the program players (and launchers) start.
#
#   SpellingBlizzard.sh --prepare --game=PATH [--data DIR]
#       Build the game from your own copy, without a window. PATH is the folder
#       with BLIZZARD.EXE and SB_CDRES.DLL (or one of those files). Prints
#       "progress <0..1> <text>" lines. Without --game it rebuilds from the
#       copy kept by the last run.
#   SpellingBlizzard.sh [--data DIR] [sb_recomp options...]
#       Play. Options such as --scale 3, --fullscreen or --no-audio are passed on.
#
# Everything lives in one data folder: --data DIR, else $RECOMPLAUNCHER_DATA,
# else ~/.local/share/spelling-blizzard-recomp. Inside it: game/ (your copy of
# the two game files), build/ (the recompiled game), saves/ (players, high
# scores, settings) and logs/.
#
# Exit codes: 64 not set up yet, 65 wrong or missing game files, 66 build tools
# missing, 67 build failed, 70 the game hit something not supported yet.
set -uo pipefail

HERE=$(cd "$(dirname "$(readlink -f "$0")")" && pwd)
EXE_SHA=ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2
DLL_SHA=1c6c36bde51b8c9ad3b3a0eaba0283d0468d81326b3dbe4dbfbf7305863b70e1

DATA=${RECOMPLAUNCHER_DATA:-${XDG_DATA_HOME:-$HOME/.local/share}/spelling-blizzard-recomp}
PREPARE=0
GAME=
PASS=()
while [ $# -gt 0 ]; do
    case $1 in
        --prepare) PREPARE=1 ;;
        --game=*) GAME=${1#--game=} ;;
        --game) GAME=${2:-}; shift ;;
        --data=*) DATA=${1#--data=} ;;
        --data) DATA=${2:-}; shift ;;
        *) PASS+=("$1") ;;
    esac
    shift
done

fail() { # code, message lines (the last line is what players read first)
    local code=$1; shift
    for line in "$@"; do echo "$line" >&2; done
    exit "$code"
}

sha256() {
    if command -v sha256sum >/dev/null; then sha256sum "$1" | cut -d' ' -f1
    else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

# The Liberation fonts stand in for the Windows fonts; distributions put them in different places.
find_fonts() {
    [ -n "${SB_FONT_DIR:-}" ] && return 0
    local f
    f=$(fc-match -f '%{file}' 'Liberation Sans' 2>/dev/null || true)
    if [ -n "$f" ] && [ "$(basename "$f")" = LiberationSans-Regular.ttf ]; then
        export SB_FONT_DIR=$(dirname "$f")
        return 0
    fi
    for d in /usr/share/fonts/liberation /usr/share/fonts/truetype/liberation /usr/share/fonts/TTF \
             /usr/share/fonts/liberation-sans /usr/local/share/fonts; do
        if [ -f "$d/LiberationSans-Regular.ttf" ]; then export SB_FONT_DIR=$d; return 0; fi
    done
    return 1
}

BIN=$DATA/build/sb_recomp
STAMP=$DATA/build/.sb-build-version

prepare() {
    local version src exe dll missing=()
    version="$(cat "$HERE/VERSION" 2>/dev/null || echo dev) $EXE_SHA"
    mkdir -p "$DATA/game" "$DATA/saves" "$DATA/logs" || fail 65 "Can't write to $DATA."

    echo "progress 0.01 Checking your copy"
    if [ -n "$GAME" ]; then
        src=$GAME
        [ -f "$src" ] && src=$(dirname "$src")
        [ -d "$src" ] || fail 65 "$GAME isn't a folder or a file in one."
        exe=$(find "$src" -maxdepth 3 -iname BLIZZARD.EXE -type f -print -quit)
        dll=$(find "$src" -maxdepth 3 -iname SB_CDRES.DLL -type f -print -quit)
        [ -n "$exe" ] || fail 65 "BLIZZARD.EXE isn't in $GAME. Pick the folder that has both BLIZZARD.EXE and SB_CDRES.DLL."
        [ -n "$dll" ] || fail 65 "SB_CDRES.DLL isn't in $GAME. Pick the folder that has both BLIZZARD.EXE and SB_CDRES.DLL."
        [ "$(sha256 "$exe")" = "$EXE_SHA" ] ||
            fail 65 "This BLIZZARD.EXE is a different version. The recomp needs Spelling Blizzard 1.10.CD for Windows."
        [ "$(sha256 "$dll")" = "$DLL_SHA" ] ||
            fail 65 "This SB_CDRES.DLL is a different version. The recomp needs Spelling Blizzard 1.10.CD for Windows."
        cp "$exe" "$DATA/game/BLIZZARD.EXE" && cp "$dll" "$DATA/game/SB_CDRES.DLL" ||
            fail 65 "Couldn't copy the game files into $DATA/game."
    else
        [ "$(sha256 "$DATA/game/BLIZZARD.EXE" 2>/dev/null)" = "$EXE_SHA" ] &&
            [ "$(sha256 "$DATA/game/SB_CDRES.DLL" 2>/dev/null)" = "$DLL_SHA" ] ||
            fail 65 "Pick your copy of the game: the folder with BLIZZARD.EXE and SB_CDRES.DLL."
    fi

    if [ -x "$BIN" ] && [ "$(cat "$STAMP" 2>/dev/null)" = "$version" ] && [ "${version%% *}" != dev ]; then
        echo "progress 1 Already built"
        return 0
    fi

    echo "progress 0.03 Checking the build tools"
    for tool in cmake cc pkg-config; do command -v "$tool" >/dev/null || missing+=("$tool"); done
    if command -v pkg-config >/dev/null; then
        pkg-config --exists sdl2 || missing+=("SDL2 (development files)")
        pkg-config --exists freetype2 || missing+=("FreeType (development files)")
    fi
    find_fonts || missing+=("the Liberation fonts")
    if [ ${#missing[@]} -gt 0 ]; then
        fail 66 "Missing: $(IFS=,; echo "${missing[*]}" | sed 's/,/, /g')." \
            "Spelling Blizzard is recompiled on your PC from your copy. Install them (Debian/Ubuntu: sudo apt install build-essential cmake pkg-config libsdl2-dev libfreetype-dev fonts-liberation; Arch: sudo pacman -S base-devel cmake sdl2 freetype2 ttf-liberation), then set it up again."
    fi

    rm -f "$STAMP"
    echo "progress 0.05 Recompiling BLIZZARD.EXE to C"
    cmake -S "$HERE" -B "$DATA/build" -DCMAKE_BUILD_TYPE=Release -DSB_GAME_DIR="$DATA/game" ||
        fail 67 "The build couldn't be configured (see the lines above)."
    local jobs
    jobs=$( (nproc || sysctl -n hw.ncpu) 2>/dev/null || echo 4)
    echo "progress 0.1 Compiling the game"
    cmake --build "$DATA/build" -j "$jobs" 2>&1 | while IFS= read -r line; do
        if [[ $line =~ ^\[\ *([0-9]+)%\] ]]; then
            printf 'progress %s Compiling the game\n' "$(awk -v p="${BASH_REMATCH[1]}" 'BEGIN { printf "%.2f", 0.1 + 0.89 * p / 100 }')"
        fi
        echo "$line"
    done
    [ "${PIPESTATUS[0]}" -eq 0 ] && [ -x "$BIN" ] || fail 67 "The build failed (see the lines above)."
    echo "$version" > "$STAMP"
    echo "progress 1 Ready to play"
}

play() {
    [ -x "$BIN" ] || fail 64 "Spelling Blizzard isn't built yet. Run: $0 --prepare --game=/path/to/your/game"
    find_fonts || fail 66 "The Liberation fonts aren't installed (fonts-liberation, ttf-liberation)."
    mkdir -p "$DATA/saves" "$DATA/logs"
    local extra=() log status
    [ "${RECOMPLAUNCHER_MODE:-}" = bigpicture ] && extra+=(--fullscreen)
    # keep the last five logs
    ls -1t "$DATA/logs"/sb_recomp-*.log 2>/dev/null | tail -n +5 | xargs -r rm -f
    log=$DATA/logs/sb_recomp-$(date +%Y%m%d-%H%M%S).log
    "$BIN" --game-dir "$DATA/game" --data-dir "$DATA/saves" ${extra[@]+"${extra[@]}"} ${PASS[@]+"${PASS[@]}"} 2> >(tee "$log" >&2)
    status=$?
    if [ $status -ge 128 ]; then
        fail 70 "Spelling Blizzard stopped on something the recomp can't do yet (see $log)."
    fi
    exit $status
}

if [ $PREPARE = 1 ]; then prepare; else play; fi
