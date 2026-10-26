#!/usr/bin/env python3
"""Verify the recovered PeerPressure Config.ini schema against image bytes."""

from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SCHEMA = ROOT / "analysis/kernel-expansion/KernelExpansion-config-schema.json"
BASE = 0x801F0000


def config_hash(text: str) -> int:
    value = 0xFFFFFFFF
    for character in text.encode("ascii"):
        if ord("a") <= character <= ord("z"):
            character -= ord("a") - ord("A")
        value ^= character
        for _ in range(8):
            value = (value >> 1) ^ (0xEDB88320 if value & 1 else 0)
    return (~value) & 0xFFFFFFFF


def image_string(image: bytes, address: int) -> str:
    offset = address - BASE
    end = image.index(0, offset)
    return image[offset:end].decode("ascii")


def main() -> None:
    document = json.loads(SCHEMA.read_text(encoding="utf-8"))
    image = (ROOT / document["image"]).read_bytes()
    if hashlib.sha256(image).hexdigest() != document["image_sha256"]:
        raise SystemExit("Kernel Expansion image hash differs from schema evidence")

    table_address = int(document["table_address"], 0)
    table_offset = table_address - BASE
    table_size = document["table_size"]
    table = image[table_offset:table_offset + table_size]
    if len(table) != table_size:
        raise SystemExit("truncated Kernel Expansion config schema")
    if hashlib.sha256(table).hexdigest() != document["table_sha256"]:
        raise SystemExit("Kernel Expansion config schema hash differs")

    record_size = document["record_size"]
    records = document["records"]
    if len(records) * record_size != table_size:
        raise SystemExit("config schema record count does not cover table")

    for index, expected in enumerate(records):
        offset = index * record_size
        section_pointer, cached_hash, key_pointer, options, value_type, destination = (
            struct.unpack_from(">IIIHHI", table, offset)
        )
        actual_address = table_address + offset
        checks = {
            "address": actual_address,
            "section_address": section_pointer,
            "cached_key_hash": cached_hash,
            "key_address": key_pointer,
            "options": options,
            "value_type": value_type,
            "destination_offset": destination,
        }
        for key, actual in checks.items():
            expected_value = expected[key]
            if isinstance(expected_value, str):
                expected_value = int(expected_value, 0)
            if actual != expected_value:
                raise SystemExit(
                    f"config schema record {index} {key} differs: "
                    f"{actual:#x} != {expected_value:#x}"
                )
        if image_string(image, section_pointer) != expected["section"]:
            raise SystemExit(f"config schema record {index} section differs")
        if image_string(image, key_pointer) != expected["key"]:
            raise SystemExit(f"config schema record {index} key differs")
        if config_hash(expected["key"]) != int(expected["computed_key_hash"], 0):
            raise SystemExit(f"config schema record {index} computed hash differs")
        if "cached_hash_text" in expected and (
            config_hash(expected["cached_hash_text"]) != cached_hash
        ):
            raise SystemExit(f"config schema record {index} stale hash text differs")

    print(
        "Kernel Expansion config schema verified: "
        f"{len(records)} records, {table_size} exact bytes"
    )


if __name__ == "__main__":
    main()
