#!/usr/bin/env python3
"""Validate the 24-byte Peer Pressure wrapper around an ordinary ZIP."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
from zipfile import BadZipFile, ZipFile


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    data = args.archive.read_bytes()
    if len(data) < 24:
        raise SystemExit("archive is smaller than its 24-byte wrapper")
    stored_digest = data[:20]
    version = data[20:24]
    zip_data = data[24:]
    actual_digest = hashlib.sha1(data[20:]).digest()
    if stored_digest != actual_digest:
        raise SystemExit(
            f"wrapper SHA-1 mismatch: stored {stored_digest.hex()}, "
            f"calculated {actual_digest.hex()}"
        )
    if not zip_data.startswith(b"PK\x03\x04"):
        raise SystemExit("wrapped data does not begin with a ZIP local-file header")

    members = []
    try:
        with ZipFile(args.archive) as archive:
            bad_crc = archive.testzip()
            if bad_crc is not None:
                raise SystemExit(f"ZIP CRC failure in {bad_crc!r}")
            for info in archive.infolist():
                path = PurePosixPath(info.filename.replace("\\", "/"))
                safe = not path.is_absolute() and ".." not in path.parts
                members.append(
                    {
                        "path": info.filename,
                        "safe_path": safe,
                        "uncompressed_size": info.file_size,
                        "compressed_size": info.compress_size,
                        "crc32": f"{info.CRC:08x}",
                    }
                )
    except BadZipFile as error:
        raise SystemExit(f"invalid wrapped ZIP: {error}") from error

    result = {
        "source": str(args.archive),
        "wrapper_size": 24,
        "wrapper_sha1": stored_digest.hex(),
        "hash_coverage": "bytes 0x14 through EOF (version + ZIP)",
        "softmod_version": f"{version[0]}.{version[1]}.{int.from_bytes(version[2:], 'big')}",
        "zip_offset": "0x00000018",
        "zip_member_count": len(members),
        "all_member_paths_safe": all(member["safe_path"] for member in members),
        "members": members,
    }
    encoded = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
