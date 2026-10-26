#!/usr/bin/env python3
"""Recover direct PowerPC branch-and-link edges from a mapped code section."""

from __future__ import annotations

import argparse
import hashlib
import json
from bisect import bisect_right
from pathlib import Path


def parse_int(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("function_map", type=Path)
    parser.add_argument("--text-offset", type=parse_int, required=True)
    parser.add_argument("--text-address", type=parse_int, required=True)
    parser.add_argument("--text-size", type=parse_int, required=True)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    image = args.image.read_bytes()
    text = image[args.text_offset : args.text_offset + args.text_size]
    if len(text) != args.text_size:
        raise SystemExit("text range exceeds image")
    function_map = json.loads(args.function_map.read_text(encoding="utf-8"))
    functions = function_map["functions"]
    by_address = {int(item["address"], 0): item for item in functions}
    starts = sorted(by_address)

    edges: list[dict[str, object]] = []
    for function in functions:
        caller_address = int(function["address"], 0)
        start = int(function["section_offset"], 0)
        size = int(function.get("size_to_next_symbol", 0))
        for offset in range(start, min(start + size, len(text)), 4):
            word = int.from_bytes(text[offset : offset + 4], "big")
            if word >> 26 != 18 or not (word & 1):
                continue
            displacement = word & 0x03FFFFFC
            if displacement & 0x02000000:
                displacement -= 0x04000000
            instruction = args.text_address + offset
            target = displacement & 0xFFFFFFFF if word & 2 else (instruction + displacement) & 0xFFFFFFFF
            index = bisect_right(starts, target) - 1
            callee = by_address.get(target)
            containing = by_address[starts[index]] if index >= 0 else None
            edges.append(
                {
                    "caller": function["name"],
                    "caller_address": function["address"],
                    "instruction_address": f"0x{instruction:08x}",
                    "target_address": f"0x{target:08x}",
                    "callee": callee["name"] if callee else None,
                    "target_containing_function": containing["name"] if containing else None,
                }
            )

    result = {
        "schema": "bandwagon-ppc-direct-callgraph-v1",
        "image": str(args.image),
        "image_sha256": hashlib.sha256(image).hexdigest(),
        "edge_count": len(edges),
        "resolved_function_entry_edges": sum(item["callee"] is not None for item in edges),
        "edges": edges,
    }
    rendered = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8", newline="\n")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
