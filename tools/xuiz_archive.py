#!/usr/bin/env python3
"""Safely unpack and deterministically rebuild Xbox 360 XUIZ/XZP archives."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path, PureWindowsPath


HEADER = struct.Struct(">4sIIIIH")
ENTRY = struct.Struct(">IIB")


@dataclass(frozen=True)
class XuizEntry:
    name: str
    data: bytes
    offset: int


@dataclass(frozen=True)
class XuizArchive:
    flags: int
    unknown: int
    entries: tuple[XuizEntry, ...]

    @classmethod
    def parse(cls, data: bytes) -> "XuizArchive":
        if len(data) < HEADER.size:
            raise ValueError("archive is smaller than its XUIZ header")
        magic, flags, declared_size, unknown, directory_size, count = (
            HEADER.unpack_from(data)
        )
        if magic != b"XUIZ":
            raise ValueError(f"invalid XUIZ magic {magic!r}")
        if declared_size != len(data):
            raise ValueError(
                f"declared size 0x{declared_size:x} != actual 0x{len(data):x}"
            )
        data_start = HEADER.size + directory_size
        if data_start > len(data):
            raise ValueError("directory extends past end of archive")

        cursor = HEADER.size
        records: list[tuple[str, int, int]] = []
        for index in range(count):
            if cursor + ENTRY.size > data_start:
                raise ValueError(f"truncated directory entry {index}")
            size, offset, name_size = ENTRY.unpack_from(data, cursor)
            cursor += ENTRY.size
            if cursor + name_size > data_start:
                raise ValueError(f"entry {index} name extends past directory")
            raw_name = data[cursor : cursor + name_size]
            cursor += name_size
            try:
                name = raw_name.decode("ascii")
            except UnicodeDecodeError as error:
                raise ValueError(f"entry {index} name is not ASCII") from error
            records.append((name, offset, size))
        if cursor != data_start:
            raise ValueError(f"0x{data_start - cursor:x} unexplained directory bytes")

        entries = []
        previous_end = 0
        for index, (name, offset, size) in enumerate(records):
            end = offset + size
            if offset < previous_end:
                raise ValueError(f"entry {index} overlaps a previous payload")
            if data_start + end > len(data):
                raise ValueError(f"entry {index} payload extends past EOF")
            if offset != previous_end:
                raise ValueError(f"entry {index} leaves an unexplained payload gap")
            entries.append(
                XuizEntry(name, data[data_start + offset : data_start + end], offset)
            )
            previous_end = end
        if previous_end != len(data) - data_start:
            raise ValueError("unreferenced bytes remain after the last payload")
        return cls(flags, unknown, tuple(entries))

    def build(self) -> bytes:
        directory = bytearray()
        payload = bytearray()
        for entry in self.entries:
            name = entry.name.encode("ascii")
            if not name or len(name) > 0xFF:
                raise ValueError(f"invalid XUIZ entry name {entry.name!r}")
            directory += ENTRY.pack(len(entry.data), len(payload), len(name))
            directory += name
            payload += entry.data
        size = HEADER.size + len(directory) + len(payload)
        return (
            HEADER.pack(
                b"XUIZ", self.flags, size, self.unknown, len(directory), len(self.entries)
            )
            + directory
            + payload
        )

    def manifest(self) -> dict[str, object]:
        return {
            "schema": "bandwagon-xuiz-archive-v1",
            "flags": self.flags,
            "unknown": self.unknown,
            "entry_count": len(self.entries),
            "entries": [
                {
                    "name": entry.name,
                    "size": len(entry.data),
                    "offset": entry.offset,
                    "sha256": hashlib.sha256(entry.data).hexdigest(),
                }
                for entry in self.entries
            ],
        }


def safe_target(root: Path, name: str) -> Path:
    path = PureWindowsPath(name)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError(f"unsafe XUIZ member path {name!r}")
    target = root.joinpath(*path.parts).resolve()
    resolved_root = root.resolve()
    if target != resolved_root and resolved_root not in target.parents:
        raise ValueError(f"XUIZ member escapes output directory: {name!r}")
    return target


def unpack(source: Path, output: Path, manifest_path: Path | None) -> None:
    archive = XuizArchive.parse(source.read_bytes())
    output.mkdir(parents=True, exist_ok=True)
    for entry in archive.entries:
        target = safe_target(output, entry.name)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(entry.data)
    manifest = archive.manifest()
    manifest["source"] = str(source)
    manifest["source_size"] = source.stat().st_size
    manifest["source_sha256"] = hashlib.sha256(source.read_bytes()).hexdigest()
    destination = manifest_path or output / "xuiz-manifest.json"
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def pack(manifest_path: Path, input_root: Path, output: Path) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = []
    offset = 0
    for item in manifest["entries"]:
        name = item["name"]
        data = safe_target(input_root, name).read_bytes()
        expected = item.get("sha256")
        actual = hashlib.sha256(data).hexdigest()
        if expected and actual != expected:
            raise ValueError(f"payload hash differs for {name!r}")
        entries.append(XuizEntry(name, data, offset))
        offset += len(data)
    archive = XuizArchive(manifest["flags"], manifest["unknown"], tuple(entries))
    encoded = archive.build()
    expected_archive = manifest.get("source_sha256")
    if expected_archive and hashlib.sha256(encoded).hexdigest() != expected_archive:
        raise ValueError("rebuilt archive does not match the source SHA-256")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(encoded)


def main() -> None:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    unpack_parser = sub.add_parser("unpack")
    unpack_parser.add_argument("source", type=Path)
    unpack_parser.add_argument("output", type=Path)
    unpack_parser.add_argument("--manifest", type=Path)
    pack_parser = sub.add_parser("pack")
    pack_parser.add_argument("manifest", type=Path)
    pack_parser.add_argument("input_root", type=Path)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "unpack":
        unpack(args.source, args.output, args.manifest)
    else:
        pack(args.manifest, args.input_root, args.output)


if __name__ == "__main__":
    main()
