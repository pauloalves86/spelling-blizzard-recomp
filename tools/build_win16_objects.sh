#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
watcom_root=${SB_WATCOM_ROOT:-"$root/.local/tools/openwatcom"}
out_dir=${SB_WIN16_OUT_DIR:-"$root/build/win16"}
compiler="$watcom_root/binl64/wcc"
archiver="$watcom_root/binl64/wlib"
disassembler="$watcom_root/binl64/wdis"

if [[ ! -x "$compiler" || ! -x "$archiver" ]]; then
    echo "Open Watcom 2.0 Linux tools not found under $watcom_root" >&2
    exit 2
fi

mkdir -p "$out_dir"
sources=(
    src/recovered/framework/message_pump.c
    src/recovered/application/object_model.c
    src/recovered/application/application_state.c
    src/recovered/win16/application_primitives.c
)
objects=()

for source in "${sources[@]}"; do
    name=$(basename "$source" .c)
    object="$out_dir/$name.obj"
    WATCOM="$watcom_root" INCLUDE="$watcom_root/h" \
        "$compiler" -bt=windows -ml -za99 -od -fo="$object" "$root/$source"
    objects+=("+$object")
done

WATCOM="$watcom_root" "$archiver" -q -n "$out_dir/recovered_win16.lib" "${objects[@]}"
WATCOM="$watcom_root" "$disassembler" "$out_dir/application_primitives.obj" \
    > "$out_dir/application_primitives.dis"
python3 "$root/tools/check_win16_abi.py" "$out_dir/application_primitives.dis"
echo "Created Win16 OMF library: $out_dir/recovered_win16.lib"
