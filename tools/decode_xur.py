#!/usr/bin/env python3
"""Losslessly decode/rebuild the sectioned XUIB form used by Peer XUR assets."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path


SECTION_COUNT = 4
SECTION_TABLE = 0x20
SECTION_RECORD = struct.Struct(">4sII")


def parse_strings(raw: bytes) -> dict[str, object] | None:
    if len(raw) < 6:
        return None
    payload_size = int.from_bytes(raw[:4], "big")
    count = int.from_bytes(raw[4:6], "big")
    if payload_size + 6 != len(raw):
        return None
    parts = raw[6:].split(b"\0")
    if parts and parts[-1] == b"":
        parts.pop()
    if len(parts) != count:
        return None
    try:
        strings = [part.decode("utf-8") for part in parts]
    except UnicodeDecodeError:
        return None
    return {"declared_payload_size": payload_size, "count": count, "strings": strings}


def decode(data: bytes) -> dict[str, object]:
    if len(data) < SECTION_TABLE + SECTION_COUNT * SECTION_RECORD.size:
        raise ValueError("XUR is smaller than its fixed XUIB section table")
    if data[:4] != b"XUIB":
        raise ValueError(f"invalid XUIB magic {data[:4]!r}")
    records = []
    prior_end = SECTION_TABLE + SECTION_COUNT * SECTION_RECORD.size
    for index in range(SECTION_COUNT):
        tag, offset, size = SECTION_RECORD.unpack_from(
            data, SECTION_TABLE + index * SECTION_RECORD.size
        )
        if offset != prior_end:
            raise ValueError(f"section {index} is not contiguous")
        end = offset + size
        if end > len(data):
            raise ValueError(f"section {index} extends past EOF")
        raw = data[offset:end]
        name = tag.decode("ascii")
        record: dict[str, object] = {
            "tag": name,
            "offset": offset,
            "size": size,
            "sha256": hashlib.sha256(raw).hexdigest(),
            "data_hex": raw.hex(),
        }
        if name == "STRN":
            parsed = parse_strings(raw)
            if parsed:
                record["decoded"] = parsed
        if name in ("VECT", "FLOT") and size % 4 == 0:
            record["big_endian_floats"] = [
                struct.unpack_from(">f", raw, offset)[0]
                for offset in range(0, size, 4)
            ]
        records.append(record)
        prior_end = end
    if prior_end != len(data):
        raise ValueError("unreferenced bytes follow the last XUIB section")
    return {
        "schema": "bandwagon-xuib-resource-v1",
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "header_hex": data[:SECTION_TABLE].hex(),
        "sections": records,
    }


def rebuild(document: dict[str, object]) -> bytes:
    header = bytearray.fromhex(document["header_hex"])
    if len(header) != SECTION_TABLE or header[:4] != b"XUIB":
        raise ValueError("invalid preserved XUIB header")
    sections = document["sections"]
    if len(sections) != SECTION_COUNT:
        raise ValueError("unexpected XUIB section count")
    table = bytearray()
    payload = bytearray()
    offset = SECTION_TABLE + SECTION_COUNT * SECTION_RECORD.size
    for section in sections:
        tag = section["tag"].encode("ascii")
        raw = bytes.fromhex(section["data_hex"])
        if len(tag) != 4 or len(raw) != section["size"]:
            raise ValueError("invalid XUIB section metadata")
        if hashlib.sha256(raw).hexdigest() != section["sha256"]:
            raise ValueError(f"section hash differs for {section['tag']}")
        table += SECTION_RECORD.pack(tag, offset, len(raw))
        payload += raw
        offset += len(raw)
    result = bytes(header + table + payload)
    if len(result) != document["size"]:
        raise ValueError("rebuilt XUIB size differs")
    if hashlib.sha256(result).hexdigest() != document["sha256"]:
        raise ValueError("rebuilt XUIB hash differs")
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    decode_parser = sub.add_parser("decode")
    decode_parser.add_argument("source", type=Path)
    decode_parser.add_argument("output", type=Path)
    rebuild_parser = sub.add_parser("rebuild")
    rebuild_parser.add_argument("source", type=Path)
    rebuild_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "decode":
        document = decode(args.source.read_bytes())
        args.output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
    else:
        document = json.loads(args.source.read_text(encoding="utf-8"))
        args.output.write_bytes(rebuild(document))


if __name__ == "__main__":
    main()
