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

    helper = manifest["public_badupdate_helper"]
    helper_start_text, helper_end_text = helper["artifact_range"].split("-")
    helper_start, helper_end = int(helper_start_text, 0), int(helper_end_text, 0)
    helper_bytes = image[helper_start:helper_end]
    if len(helper_bytes) != helper["size"] or digest_bytes(helper_bytes) != helper["sha256"]:
        raise SystemExit("Stage4 public BadUpdate helper differs from attribution")
    upstream_artifact = ROOT / helper["upstream_artifact"]
    if upstream_artifact.is_file():
        upstream_start_text, upstream_end_text = helper[
            "upstream_binary_range"
        ].split("-")
        upstream_start = int(upstream_start_text, 0)
        upstream_end = int(upstream_end_text, 0)
        upstream_image = upstream_artifact.read_bytes()
        if upstream_image[upstream_start:upstream_end] != helper_bytes:
            raise SystemExit("Stage4 helper no longer matches public BadUpdate")
    helper_source = ROOT / helper["source_path"]
    if helper["symbol"] not in helper_source.read_text(encoding="utf-8"):
        raise SystemExit("pinned BadUpdate source no longer names the Stage4 helper")
    if helper["upstream_commit"] != commit:
        raise SystemExit("Stage4 helper and clean-HV provenance commits differ")

    cursor = 0
    source_cache: dict[str, str] = {}
    peer_bytes = 0
    upstream_bytes = 0
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
        if region["kind"].startswith("upstream_"):
            upstream_bytes += end - start
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
    if peer_bytes != clean_start - 0x30 - helper["size"]:
        # Before the clean block, exclude the 0x30 alignment bytes at 0x750
        # and the public BadUpdate SMC helper. This catches ownership expansion.
        raise SystemExit(f"unexpected Peer-owned byte count: {peer_bytes}")
    if upstream_bytes != helper["size"]:
        raise SystemExit(f"unexpected upstream-code byte count: {upstream_bytes}")
    print(
        "Stage4 source coverage verified: "
        f"{peer_bytes} Peer-specific bytes sourced; "
        f"{upstream_bytes} public BadUpdate code bytes mapped; "
        f"{len(clean_bytes)} third-party data bytes matched"
    )


if __name__ == "__main__":
    main()
