#!/usr/bin/env python3
"""Decode the symbol table from a big-endian PowerPC Xbox COFF object."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


COFF_HEADER = struct.Struct("<HHLLLHH")
SYMBOL = struct.Struct("<LhHBB")
IMAGE_SYM_DTYPE_FUNCTION = 0x20


def parse_int(value: str) -> int:
    return int(value, 0)


def symbol_name(data: bytes, offset: int, string_base: int, string_size: int) -> str:
    raw = data[offset : offset + 8]
    if raw[:4] == b"\0\0\0\0":
        string_offset = struct.unpack_from("<L", raw, 4)[0]
        if string_offset < 4 or string_offset >= string_size:
            raise ValueError(f"invalid COFF string offset 0x{string_offset:x}")
        start = string_base + string_offset
        end = data.find(b"\0", start, string_base + string_size)
        if end < 0:
            raise ValueError("unterminated COFF symbol name")
        return data[start:end].decode("utf-8", errors="replace")
    return raw.rstrip(b"\0").decode("utf-8", errors="replace")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("object", type=Path)
    parser.add_argument("--base", type=parse_int, required=True)
    parser.add_argument("--section-end", type=parse_int, required=True)
    parser.add_argument("--source-label")
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    data = args.object.read_bytes()
    if len(data) < COFF_HEADER.size:
        raise SystemExit("COFF object is too small")
    machine, section_count, _, symbol_offset, symbol_count, _, _ = COFF_HEADER.unpack_from(data)
    if machine != 0x01F2:
        raise SystemExit(f"unexpected COFF machine 0x{machine:04x}; expected Xbox PowerPC 0x01f2")
    string_base = symbol_offset + symbol_count * 18
    if string_base + 4 > len(data):
        raise SystemExit("COFF symbol table exceeds file")
    string_size = struct.unpack_from("<L", data, string_base)[0]
    if string_base + string_size > len(data):
        raise SystemExit("COFF string table exceeds file")

    functions: list[dict[str, object]] = []
    index = 0
    while index < symbol_count:
        offset = symbol_offset + index * 18
        name = symbol_name(data, offset, string_base, string_size)
        value, section, symbol_type, storage_class, aux_count = SYMBOL.unpack_from(data, offset + 8)
        if section > 0 and symbol_type == IMAGE_SYM_DTYPE_FUNCTION:
            functions.append(
                {
                    "name": name,
                    "section": section,
                    "section_offset": f"0x{value:08x}",
                    "address": f"0x{args.base + value:08x}",
                    "storage_class": storage_class,
                }
            )
        index += 1 + aux_count

    functions.sort(key=lambda item: (item["section"], int(str(item["section_offset"]), 16)))
    for current, following in zip(functions, functions[1:]):
        if current["section"] == following["section"]:
            start = int(str(current["section_offset"]), 16)
            end = int(str(following["section_offset"]), 16)
            current["size_to_next_symbol"] = end - start
    if functions:
        last = functions[-1]
        start = int(str(last["section_offset"]), 16)
        if start <= args.section_end:
            last["size_to_next_symbol"] = args.section_end - start

    result = {
        "schema": "bandwagon-xbox-coff-functions-v1",
        "source": args.source_label or str(args.object),
        "source_size": len(data),
        "source_sha256": hashlib.sha256(data).hexdigest(),
        "machine": "0x01f2 (Xbox PowerPC)",
        "section_count": section_count,
        "symbol_count": symbol_count,
        "function_count": len(functions),
        "base_address": f"0x{args.base:08x}",
        "section_end_offset": f"0x{args.section_end:08x}",
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
