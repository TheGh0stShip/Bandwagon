#!/usr/bin/env python3
"""Render a raw big-endian PowerPC range as deterministic LLVM assembly."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path


def parse_int(value: str) -> int:
    return int(value, 0)


def decode_word(llvm_mc: str, word: bytes) -> str:
    encoded = " ".join(f"0x{byte:02x}" for byte in word) + "\n"
    result = subprocess.run(
        [llvm_mc, "--disassemble", "--triple=powerpc64-unknown-linux-gnu"],
        input=encoded,
        text=True,
        capture_output=True,
        check=True,
    )
    instructions = [
        line.strip()
        for line in result.stdout.splitlines()
        if line.strip() and line.strip() != ".text"
    ]
    if len(instructions) == 1:
        return instructions[0]
    return f".long 0x{int.from_bytes(word, 'big'):08x}"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("start", type=parse_int)
    parser.add_argument("end", type=parse_int)
    parser.add_argument("--label", required=True)
    parser.add_argument(
        "--data",
        action="store_true",
        help="emit literal bytes instead of decoding PowerPC instructions",
    )
    args = parser.parse_args()

    data = args.input.read_bytes()
    if args.start < 0 or args.end > len(data) or args.start >= args.end:
        raise SystemExit("invalid input range")
    if not args.data and (args.start % 4 or args.end % 4):
        raise SystemExit("PowerPC instruction ranges must be four-byte aligned")

    llvm_mc = shutil.which("llvm-mc")
    if llvm_mc is None:
        raise SystemExit("required tool is not installed: llvm-mc")

    print("/* Deterministically decoded from the reference binary; do not hand-edit. */")
    print(f"        .org 0x{args.start:04x}, 0")
    print(f"{args.label}:")
    if args.data:
        for offset in range(args.start, args.end, 16):
            chunk = data[offset : min(offset + 16, args.end)]
            values = ", ".join(f"0x{byte:02x}" for byte in chunk)
            print(f"        .byte {values:<94} /* 0x{offset:04x} */")
        return
    for offset in range(args.start, args.end, 4):
        word = data[offset : offset + 4]
        instruction = decode_word(llvm_mc, word)
        print(f"        {instruction:<36} /* 0x{offset:04x}: {word.hex()} */")


if __name__ == "__main__":
    main()
