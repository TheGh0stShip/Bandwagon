#!/usr/bin/env python3
"""Verify HDD Stage2 provenance boundaries against exact local evidence."""

from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = (
    ROOT / "analysis/stage2/PeerPressureExploit-Stage2-source-attribution.json"
)
INVENTORY = ROOT / "analysis/stage2/PeerPressureExploit-Stage2-functions.json"
PUBLIC_ARTIFACTS = {
    "BadUpdateExploit-2ndStage.bin":
        ROOT / "BadUpdatePayload/BadUpdateExploit-2ndStage.bin",
    "BadUpdateExploit-3rdStage.bin":
        ROOT / "BadUpdatePayload/BadUpdateExploit-3rdStage.bin",
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_range(value: str) -> tuple[int, int]:
    start, end = value.split("-")
    return int(start, 0), int(end, 0)


def checked_slice(data: bytes, range_text: str, size: int, sha256: str) -> bytes:
    start, end = parse_range(range_text)
    payload = data[start:end]
    if end - start != size or len(payload) != size or digest(payload) != sha256:
        raise SystemExit(f"Stage2 attribution mismatch at {range_text}")
    return payload


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    artifact = manifest["artifact"]
    image = (ROOT / artifact["path"]).read_bytes()
    if len(image) != artifact["size"] or digest(image) != artifact["sha256"]:
        raise SystemExit("HDD Stage2 artifact differs from attribution manifest")

    upstream = manifest["upstream"]
    source_root = ROOT / upstream["source_root"]
    commit = subprocess.run(
        ["git", "-C", str(source_root), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if commit != upstream["commit"]:
        raise SystemExit(f"unexpected BadUpdate source commit: {commit}")

    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    function_offsets = [int(item["file_offset"], 0) for item in inventory["functions"]]
    worker_start, worker_end = parse_range(manifest["worker"]["file_range"])
    cursor = worker_start
    totals: dict[str, int] = {}
    total_functions = 0
    for region in manifest["worker"]["ranges"]:
        start, end = int(region["start"], 0), int(region["end"], 0)
        if start != cursor or end <= start:
            raise SystemExit(f"non-contiguous Stage2 worker attribution at {cursor:#x}")
        checked_slice(image, f"{region['start']}-{region['end']}",
                      region["size"], region["sha256"])
        function_count = sum(start <= offset < end for offset in function_offsets)
        if function_count != region["function_count"]:
            raise SystemExit(f"Stage2 function count differs at {start:#x}")
        totals[region["classification"]] = (
            totals.get(region["classification"], 0) + region["size"]
        )
        total_functions += function_count
        cursor = end
    if cursor != worker_end or worker_end - worker_start != manifest["worker"]["size"]:
        raise SystemExit("Stage2 worker attribution does not cover the worker")

    summary = manifest["worker"]["summary"]
    if totals.get("candidate_peer_specific") != summary["candidate_peer_specific_bytes"]:
        raise SystemExit("Stage2 candidate Peer-specific total differs")
    if totals.get("public_badupdate_derived") != summary["public_badupdate_derived_bytes"]:
        raise SystemExit("Stage2 public BadUpdate total differs")
    support_total = totals.get("compiler_support", 0) + totals.get(
        "platform_import_thunks", 0
    )
    if support_total != summary["compiler_and_platform_bytes"]:
        raise SystemExit("Stage2 compiler/platform total differs")
    if total_functions != summary["function_count"]:
        raise SystemExit("Stage2 attributed function count differs")

    exact_public_bytes = 0
    for item in manifest["exact_public_blocks"] + manifest["exact_public_data"]:
        peer = checked_slice(image, item["peer_range"], item["size"], item["sha256"])
        public_path = PUBLIC_ARTIFACTS[item["upstream_artifact"]]
        if public_path.is_file():
            public_image = public_path.read_bytes()
            upstream_start, upstream_end = parse_range(item["upstream_range"])
            if public_image[upstream_start:upstream_end] != peer:
                raise SystemExit(
                    f"Stage2 public byte match differs at {item['peer_range']}"
                )
        exact_public_bytes += item["size"]

    known_addresses = {int(item["address"], 0) for item in inventory["functions"]}
    source_cache: dict[str, str] = {}
    for mapping in manifest["public_function_mappings"]:
        if int(mapping["address"], 0) not in known_addresses:
            raise SystemExit(f"unknown Stage2 function mapping: {mapping['address']}")
        source_name = mapping["source"]
        if source_name not in source_cache:
            source_cache[source_name] = (source_root / source_name).read_text(
                encoding="utf-8", errors="replace"
            )
        if mapping["symbol"] not in source_cache[source_name]:
            raise SystemExit(
                f"BadUpdate source no longer names {mapping['symbol']}"
            )

    print(
        "HDD Stage2 source attribution verified: "
        f"{summary['candidate_peer_specific_bytes']} candidate Peer-specific, "
        f"{summary['public_badupdate_derived_bytes']} public BadUpdate-derived, "
        f"{summary['compiler_and_platform_bytes']} compiler/platform worker bytes; "
        f"{exact_public_bytes} additional exact public ROP/data bytes"
    )


if __name__ == "__main__":
    main()
