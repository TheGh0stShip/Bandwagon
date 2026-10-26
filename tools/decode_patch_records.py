#!/usr/bin/env python3
"""Decode Peer Pressure big-endian address/length/data patch streams."""

import argparse
import json
from pathlib import Path


def decode(data: bytes) -> list[dict[str, object]]:
    records = []
    cursor = 0
    while cursor < len(data):
        if len(data) - cursor == 4 and data[cursor:] == b"\xff\xff\xff\xff":
            records.append({"sentinel": "0xffffffff"})
            cursor += 4
            break
        if len(data) - cursor < 8:
            raise ValueError(f"truncated record header at file offset 0x{cursor:x}")
        address = int.from_bytes(data[cursor : cursor + 4], "big")
        size = int.from_bytes(data[cursor + 4 : cursor + 8], "big")
        record_offset = cursor
        cursor += 8
        if size > len(data) - cursor:
            raise ValueError(
                f"record at 0x{record_offset:x} requests 0x{size:x} bytes; "
                f"only 0x{len(data) - cursor:x} remain"
            )
        payload = data[cursor : cursor + size]
        records.append(
            {
                "file_offset": f"0x{record_offset:08x}",
                "target_address": f"0x{address:08x}",
                "size": size,
                "data_hex": payload.hex(),
            }
        )
        cursor += size
    if cursor != len(data):
        raise ValueError(f"unparsed data begins at 0x{cursor:x}")
    return records


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()
    result = {
        "source": str(args.input),
        "size": args.input.stat().st_size,
        "byte_order": "big",
        "records": decode(args.input.read_bytes()),
    }
    encoded = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
