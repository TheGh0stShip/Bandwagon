#!/usr/bin/env python3
"""Verify whole-file source and ownership coverage for Peer Stage4."""

from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/stage4/source-coverage.json"


def digest_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    artifact = manifest["artifact"]
    image = (ROOT / artifact["path"]).read_bytes()
    if len(image) != artifact["size"] or digest_bytes(image) != artifact["sha256"]:
        raise SystemExit("Stage4 artifact differs from source-coverage manifest")

    clean = manifest["clean_hv_provenance"]
    clean_bytes = (ROOT / clean["source_path"]).read_bytes()
    if len(clean_bytes) != clean["size"] or digest_bytes(clean_bytes) != clean["sha256"]:
        raise SystemExit("pinned clean-HV input differs from source-coverage manifest")
    start_text, end_text = clean["artifact_range"].split("-")
    clean_start, clean_end = int(start_text, 0), int(end_text, 0)
    if image[clean_start:clean_end] != clean_bytes:
        raise SystemExit("Stage4 tail is not the pinned public clean-HV data")
    commit = subprocess.run(
        ["git", "-C", str(ROOT / "third_party/Xbox360BadUpdate"),
         "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if commit != clean["upstream_commit"]:
        raise SystemExit(f"unexpected BadUpdate source commit: {commit}")

    cursor = 0
    source_cache: dict[str, str] = {}
    peer_bytes = 0
    for region in manifest["coverage"]:
        start, end = int(region["start"], 0), int(region["end"], 0)
        if start != cursor or end <= start:
            raise SystemExit(
                f"non-contiguous Stage4 coverage at 0x{cursor:x}: "
                f"{region['start']}-{region['end']}"
            )
        cursor = end
        if region["kind"].startswith("peer_"):
            peer_bytes += end - start
        texts = []
        for source_name in region["sources"]:
            path = ROOT / source_name
            if not path.is_file():
                raise SystemExit(f"missing Stage4 source owner: {source_name}")
            if path.suffix.lower() not in {".bin", ".xex"}:
                if source_name not in source_cache:
                    source_cache[source_name] = path.read_text(
                        encoding="utf-8", errors="replace"
                    )
                texts.append(source_cache[source_name])
        combined = "\n".join(texts)
        for symbol in region["symbols"]:
            if symbol not in combined:
                raise SystemExit(
                    f"source owner does not name {symbol} for "
                    f"{region['start']}-{region['end']}"
                )
    if cursor != len(image):
        raise SystemExit(
            f"Stage4 coverage ends at 0x{cursor:x}, expected 0x{len(image):x}"
        )
    if peer_bytes != clean_start - 0x30:
        # The only non-Peer bytes before the clean block are 0x30 alignment
        # bytes at 0x750. This catches accidental ownership expansion.
        raise SystemExit(f"unexpected Peer-owned byte count: {peer_bytes}")
    print(
        "Stage4 source coverage verified: "
        f"{peer_bytes} Peer bytes sourced; {len(clean_bytes)} third-party data bytes matched"
    )


if __name__ == "__main__":
    main()
