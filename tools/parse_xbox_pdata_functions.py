#!/usr/bin/env python3
"""Build a function-label inventory from Xbox PPC .pdata and an optional IDC map."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


IDC_NAME = re.compile(r'MakeName\(0x([0-9A-Fa-f]+),\s*"([^"]+)"\)')


def parse_int(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("--pdata-offset", type=parse_int, required=True)
    parser.add_argument("--pdata-size", type=parse_int, required=True)
    parser.add_argument("--text-address", type=parse_int, required=True)
    parser.add_argument("--text-size", type=parse_int, required=True)
    parser.add_argument("--entry", type=parse_int)
    parser.add_argument("--idc", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    image = args.image.read_bytes()
    pdata = image[args.pdata_offset : args.pdata_offset + args.pdata_size]
    if len(pdata) != args.pdata_size or len(pdata) % 8:
        raise SystemExit("invalid .pdata range")
    text_end = args.text_address + args.text_size
    names: dict[int, str] = {args.text_address: f"fn_{args.text_address:08X}"}
    records = 0
    for offset in range(0, len(pdata), 8):
        address = int.from_bytes(pdata[offset : offset + 4], "big")
        if args.text_address <= address < text_end:
            names.setdefault(address, f"fn_{address:08X}")
            records += 1
    if args.entry is not None:
        if not args.text_address <= args.entry < text_end:
            raise SystemExit("entry point is outside .text")
        names[args.entry] = "_start"
    idc_names = 0
    if args.idc:
        for match in IDC_NAME.finditer(args.idc.read_text(encoding="utf-8")):
            address = int(match.group(1), 16)
            if args.text_address <= address < text_end:
                names[address] = match.group(2)
                idc_names += 1

    addresses = sorted(names)
    functions: list[dict[str, object]] = []
    for index, address in enumerate(addresses):
        following = addresses[index + 1] if index + 1 < len(addresses) else text_end
        functions.append(
            {
                "name": names[address],
                "section": 1,
                "section_offset": f"0x{address - args.text_address:08x}",
                "address": f"0x{address:08x}",
                "storage_class": 2,
                "size_to_next_symbol": following - address,
            }
        )
    result = {
        "schema": "bandwagon-xbox-pdata-functions-v1",
        "source": str(args.image),
        "source_sha256": hashlib.sha256(image).hexdigest(),
        "pdata_record_count": records,
        "idc_name_count": idc_names,
        "function_count": len(functions),
        "base_address": f"0x{args.text_address:08x}",
        "section_end_offset": f"0x{args.text_size:08x}",
        "functions": functions,
    }
    rendered = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8", newline="\n")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
