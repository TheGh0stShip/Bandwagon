#!/usr/bin/env python3
"""Parse, verify, and exactly rebuild the Peer Pressure XBPP manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path, PureWindowsPath


HEADER = struct.Struct(">4sI20sIBBHBBHII")


@dataclass(frozen=True)
class Entry:
    digest: bytes
    flags: int
    path: str


@dataclass(frozen=True)
class Manifest:
    manifest_flags: int
    smc_major: int
    smc_minor: int
    smc_reserved: int
    softmod_major: int
    softmod_minor: int
    softmod_revision: int
    entries_offset: int
    entries: tuple[Entry, ...]

    @classmethod
    def parse(cls, data: bytes) -> "Manifest":
        if len(data) < HEADER.size:
            raise ValueError("manifest is smaller than its fixed header")
        (
            magic,
            declared_size,
            stored_digest,
            manifest_flags,
            smc_major,
            smc_minor,
            smc_reserved,
            softmod_major,
            softmod_minor,
            softmod_revision,
            entries_offset,
            entry_count,
        ) = HEADER.unpack_from(data)
        if magic != b"XBPP":
            raise ValueError(f"invalid magic {magic!r}")
        if declared_size != len(data):
            raise ValueError(
                f"declared size 0x{declared_size:x} != actual 0x{len(data):x}"
            )
        actual_digest = hashlib.sha1(data[0x1C:]).digest()
        if stored_digest != actual_digest:
            raise ValueError(
                f"manifest SHA-1 mismatch: stored {stored_digest.hex()}, "
                f"calculated {actual_digest.hex()}"
            )
        if entries_offset < HEADER.size or entries_offset > len(data):
            raise ValueError(f"invalid entries offset 0x{entries_offset:x}")
        if any(data[HEADER.size:entries_offset]):
            raise ValueError("nonzero bytes between header and entries")

        cursor = entries_offset
        entries = []
        for index in range(entry_count):
            if len(data) - cursor < 25:
                raise ValueError(f"truncated entry {index} at 0x{cursor:x}")
            digest = data[cursor : cursor + 20]
            flags = int.from_bytes(data[cursor + 20 : cursor + 24], "big")
            padded_path_size = data[cursor + 24]
            cursor += 25
            if padded_path_size > len(data) - cursor:
                raise ValueError(f"entry {index} path extends past EOF")
            raw_path = data[cursor : cursor + padded_path_size]
            cursor += padded_path_size
            path_bytes, separator, padding = raw_path.partition(b"\0")
            if not separator:
                raise ValueError(f"entry {index} path is not NUL-terminated")
            if any(padding):
                raise ValueError(f"entry {index} has nonzero path padding")
            try:
                path = path_bytes.decode("ascii")
            except UnicodeDecodeError as error:
                raise ValueError(f"entry {index} path is not ASCII") from error
            entries.append(Entry(digest=digest, flags=flags, path=path))

        if cursor != len(data):
            raise ValueError(f"0x{len(data) - cursor:x} trailing bytes after entries")
        return cls(
            manifest_flags,
            smc_major,
            smc_minor,
            smc_reserved,
            softmod_major,
            softmod_minor,
            softmod_revision,
            entries_offset,
            tuple(entries),
        )

    def build(self) -> bytes:
        if self.entries_offset < HEADER.size:
            raise ValueError("entries offset overlaps fixed header")
        records = bytearray()
        for entry in self.entries:
            path = entry.path.encode("ascii")
            # The record is padded to a four-byte boundary after its 25-byte
            # fixed portion. At least one NUL terminator is always present.
            padded_size = (-((25 + len(path) + 1))) % 4 + len(path) + 1
            if padded_size > 0xFF:
                raise ValueError(f"path record is too large: {entry.path!r}")
            records += entry.digest
            records += entry.flags.to_bytes(4, "big")
            records.append(padded_size)
            records += path
            records += bytes(padded_size - len(path))

        body = bytearray(self.entries_offset - 0x1C)
        body[0:4] = self.manifest_flags.to_bytes(4, "big")
        body[4:8] = bytes((self.smc_major, self.smc_minor)) + self.smc_reserved.to_bytes(2, "big")
        body[8:12] = bytes((self.softmod_major, self.softmod_minor)) + self.softmod_revision.to_bytes(2, "big")
        body[12:16] = self.entries_offset.to_bytes(4, "big")
        body[16:20] = len(self.entries).to_bytes(4, "big")
        body += records
        digest = hashlib.sha1(body).digest()
        size = 0x1C + len(body)
        return b"XBPP" + size.to_bytes(4, "big") + digest + body

    def verify_files(self, root: Path) -> list[dict[str, object]]:
        root = root.resolve()
        results = []
        for entry in self.entries:
            win_path = PureWindowsPath(entry.path)
            safe_parts = [part for part in win_path.parts if part not in ("", ".")]
            unsafe = win_path.is_absolute() or ".." in safe_parts
            target = root.joinpath(*safe_parts).resolve() if not unsafe else root
            unsafe = unsafe or (target != root and root not in target.parents)
            exists = not unsafe and target.is_file()
            actual = hashlib.sha1(target.read_bytes()).digest() if exists else None
            results.append(
                {
                    "path": entry.path,
                    "flags": entry.flags,
                    "safe_path": not unsafe,
                    "exists": exists,
                    "stored_sha1": entry.digest.hex(),
                    "actual_sha1": actual.hex() if actual else None,
                    "matches": actual == entry.digest if actual else False,
                }
            )
        return results

    def as_dict(self) -> dict[str, object]:
        return {
            "magic": "XBPP",
            "manifest_flags": self.manifest_flags,
            "smc_version": f"{self.smc_major}.{self.smc_minor}",
            "smc_reserved": self.smc_reserved,
            "softmod_version": (
                f"{self.softmod_major}.{self.softmod_minor}.{self.softmod_revision}"
            ),
            "entries_offset": f"0x{self.entries_offset:08x}",
            "entry_count": len(self.entries),
            "entries": [
                {"path": entry.path, "flags": entry.flags, "sha1": entry.digest.hex()}
                for entry in self.entries
            ],
        }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    parser.add_argument("--verify-root", type=Path)
    parser.add_argument("--roundtrip", action="store_true")
    args = parser.parse_args()

    original = args.manifest.read_bytes()
    manifest = Manifest.parse(original)
    result = manifest.as_dict()
    if args.verify_root:
        result["file_verification"] = manifest.verify_files(args.verify_root)
    if args.roundtrip:
        rebuilt = manifest.build()
        result["roundtrip_identical"] = rebuilt == original
        result["rebuilt_sha256"] = hashlib.sha256(rebuilt).hexdigest()
        if rebuilt != original:
            raise SystemExit("round-trip serialization differs from input")

    encoded = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
