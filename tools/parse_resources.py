#!/usr/bin/env python3
"""NE Resource Table parser and extractor for original/SB_CDRES.DLL and original/BLIZZARD.EXE.

Extracts all TYPEINFO and NAMEINFO records from 16-bit Windows NE executables.
Supports cataloging, inspecting headers, and dumping individual resource payloads.
"""

import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

STANDARD_TYPES = {
    1: "RT_CURSOR",
    2: "RT_BITMAP",
    3: "RT_ICON",
    4: "RT_MENU",
    5: "RT_DIALOG",
    6: "RT_STRING",
    7: "RT_FONTDIR",
    8: "RT_FONT",
    9: "RT_ACCELERATOR",
    10: "RT_RCDATA",
    11: "RT_MESSAGETABLE",
    12: "RT_GROUP_CURSOR",
    14: "RT_GROUP_ICON",
    16: "RT_VERSION",
}


def parse_resources(file_path):
    data = Path(file_path).read_bytes()
    if data[:2] != b"MZ":
        raise ValueError(f"{file_path} is not an MZ executable")
    ne = struct.unpack_from("<H", data, 0x3C)[0]
    if data[ne : ne + 2] != b"NE":
        raise ValueError(f"{file_path} is not an NE executable")

    restab = ne + struct.unpack_from("<H", data, ne + 0x24)[0]
    rsc_align_shift = struct.unpack_from("<H", data, restab)[0]
    align_factor = 1 << rsc_align_shift

    def get_name(off_or_id):
        if off_or_id & 0x8000:
            return f"#{off_or_id & 0x7FFF}"
        name_off = restab + off_or_id
        if name_off < len(data):
            nlen = data[name_off]
            return data[name_off + 1 : name_off + 1 + nlen].decode(
                "latin1", errors="replace"
            )
        return f"str@{off_or_id:#x}"

    def detect_format(payload):
        if len(payload) >= 4 and payload[:4] == b"BSW\0":
            freq = struct.unpack_from("<H", payload, 8)[0] if len(payload) >= 10 else 0
            return f"BSW_AUDIO_{freq}Hz"
        if len(payload) >= 4 and payload[:4] == b"RIFF":
            return "RIFF"
        if len(payload) >= 4 and payload[:2] == b"BM":
            return "BMP_FILE"
        if len(payload) >= 4 and struct.unpack_from("<I", payload, 0)[0] == 40:
            w, h, planes, bpp = struct.unpack_from("<iiHH", payload, 4)
            return f"DIB_{w}x{h}_{bpp}bpp"
        # Check printable text
        if len(payload) >= 8 and sum(1 for b in payload[:64] if 32 <= b <= 126 or b in (10, 13, 0)) >= min(len(payload), 64) * 0.85:
            return "TEXT_OR_NAMES"
        return "BINARY_RECORD"

    p = restab + 2
    catalog = []
    by_type = defaultdict(list)

    while True:
        type_id = struct.unpack_from("<H", data, p)[0]
        if type_id == 0:
            break
        count, reserved = struct.unpack_from("<HI", data, p + 2)
        raw_type_num = type_id & 0x7FFF if (type_id & 0x8000) else type_id
        type_name = (
            STANDARD_TYPES.get(raw_type_num, f"TYPE_{type_id:#06x}")
            if (type_id & 0x8000)
            else get_name(type_id)
        )
        p += 8

        for _ in range(count):
            rn_off, rn_len, rn_flags, rn_id, rn_handle, rn_usage = (
                struct.unpack_from("<HHHHHH", data, p)
            )
            file_off = rn_off * align_factor
            file_len = rn_len * align_factor
            res_id_str = get_name(rn_id)
            raw_id_num = rn_id & 0x7FFF if (rn_id & 0x8000) else None
            payload = data[file_off : file_off + file_len]
            fmt = detect_format(payload)

            item = {
                "type": type_name,
                "type_id": raw_type_num,
                "id": res_id_str,
                "raw_id": raw_id_num,
                "file_offset": file_off,
                "length": file_len,
                "flags": rn_flags,
                "format": fmt,
            }
            catalog.append(item)
            by_type[type_name].append(item)
            p += 12

    return {
        "file": str(file_path),
        "total_resources": len(catalog),
        "total_bytes": sum(c["length"] for c in catalog),
        "align_shift": rsc_align_shift,
        "types_count": len(by_type),
        "resources": catalog,
        "by_type": {t: len(l) for t, l in by_type.items()},
    }


def main():
    if len(sys.argv) < 2:
        print("Usage: parse_resources.py <path_to_NE_file> [--json OUT] [--dump-type TYPE_NAME OUTDIR]")
        sys.exit(1)

    file_path = Path(sys.argv[1])
    res = parse_resources(file_path)

    print(f"File: {res['file']}")
    print(f"Total Resources: {res['total_resources']}")
    print(f"Total Resource Bytes: {res['total_bytes']} ({res['total_bytes'] / (1024*1024):.2f} MB)")
    print(f"Distinct Types: {res['types_count']}")
    print("\nResources by Type:")
    for tname, count in sorted(res["by_type"].items(), key=lambda x: x[1], reverse=True):
        print(f"  {tname:20s}: {count:5d}")

    if "--json" in sys.argv:
        out_idx = sys.argv.index("--json") + 1
        out_path = sys.argv[out_idx]
        with open(out_path, "w") as f:
            json.dump(res, f, indent=1)
        print(f"\nWrote full catalog to {out_path}")


if __name__ == "__main__":
    main()
