#!/usr/bin/env python3
"""Recover direct lis/addi-or-ori references to ASCII strings in Xbox PPC code."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def parse_int(value: str) -> int:
    return int(value, 0)


def read_ascii_string(image: bytes, offset: int, minimum: int = 4) -> str | None:
    if offset < 0 or offset >= len(image):
        return None
    end = offset
    while end < len(image) and 0x20 <= image[end] < 0x7F:
        end += 1
    if end - offset < minimum or end >= len(image) or image[end] != 0:
        return None
    return image[offset:end].decode("ascii")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("function_map", type=Path)
    parser.add_argument("--image-base", type=parse_int, required=True)
    parser.add_argument("--text-offset", type=parse_int, required=True)
    parser.add_argument("--text-address", type=parse_int, required=True)
    parser.add_argument("--text-size", type=parse_int, required=True)
    parser.add_argument("--string-start", type=parse_int, required=True)
    parser.add_argument("--string-end", type=parse_int, required=True)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    image = args.image.read_bytes()
    text = image[args.text_offset : args.text_offset + args.text_size]
    if len(text) != args.text_size:
        raise SystemExit("text range exceeds image")
    function_map = json.loads(args.function_map.read_text(encoding="utf-8"))
    functions = function_map["functions"]

    references: list[dict[str, object]] = []
    for function in functions:
        start = int(function["section_offset"], 0)
        size = int(function.get("size_to_next_symbol", 0))
        end = min(start + size, len(text))
        if start < 0 or start >= end:
            continue
        words = [int.from_bytes(text[offset : offset + 4], "big") for offset in range(start, end, 4)]
        for index, word in enumerate(words):
            opcode = word >> 26
            target_reg = (word >> 21) & 0x1F
            base_reg = (word >> 16) & 0x1F
            if opcode != 15 or base_reg != 0:  # lis == addis rD,r0,imm
                continue
            high = (word & 0xFFFF) << 16
            for following_index in range(index + 1, min(index + 9, len(words))):
                following = words[following_index]
                following_opcode = following >> 26
                candidate: int | None = None
                if following_opcode == 14:  # addi rD,rA,simm
                    source_reg = (following >> 16) & 0x1F
                    if source_reg == target_reg:
                        low = following & 0xFFFF
                        if low & 0x8000:
                            low -= 0x10000
                        candidate = (high + low) & 0xFFFFFFFF
                elif following_opcode == 24:  # ori rA,rS,uimm
                    source_reg = (following >> 21) & 0x1F
                    if source_reg == target_reg:
                        candidate = high | (following & 0xFFFF)
                if candidate is None:
                    continue
                if not (args.string_start <= candidate < args.string_end):
                    continue
                string = read_ascii_string(image, candidate - args.image_base)
                if string is None:
                    continue
                instruction_offset = start + following_index * 4
                references.append(
                    {
                        "function": function["name"],
                        "function_address": function["address"],
                        "instruction_address": f"0x{args.text_address + instruction_offset:08x}",
                        "string_address": f"0x{candidate:08x}",
                        "string": string,
                    }
                )

    unique = {
        (item["function_address"], item["instruction_address"], item["string_address"]): item
        for item in references
    }
    references = sorted(
        unique.values(),
        key=lambda item: (int(str(item["function_address"]), 0), int(str(item["instruction_address"]), 0)),
    )
    result = {
        "schema": "bandwagon-ppc-string-xrefs-v1",
        "image": str(args.image),
        "image_sha256": hashlib.sha256(image).hexdigest(),
        "reference_count": len(references),
        "functions_with_references": len({item["function_address"] for item in references}),
        "references": references,
    }
    rendered = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8", newline="\n")
    else:
        print(rendered, end="")


if __name__ == "__main__":
    main()
