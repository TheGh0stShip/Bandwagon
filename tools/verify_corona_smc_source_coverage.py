#!/usr/bin/env python3
"""Verify complete executable-source coverage for the Corona SMC stream."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/smc/Corona-source-coverage.json"


def parse_records(data: bytes) -> list[tuple[int, bytes]]:
    records = []
    cursor = 0
    while cursor < len(data):
        if len(data) - cursor < 8:
            raise SystemExit("truncated Corona SMC record header")
        target = int.from_bytes(data[cursor:cursor + 4], "big")
        size = int.from_bytes(data[cursor + 4:cursor + 8], "big")
        cursor += 8
        if len(data) - cursor < size:
            raise SystemExit("truncated Corona SMC record payload")
        records.append((target, data[cursor:cursor + size]))
        cursor += size
    return records


def parse_profile(source: str) -> list[int]:
    match = re.search(
        r"constexpr\s+CoronaSmcProfile\s+kCoronaProfile\s*\{([^{}]+)\};",
        source,
        re.DOTALL,
    )
    if not match:
        raise SystemExit("missing Corona source profile")
    return [int(token.strip(), 0) for token in match.group(1).split(",")]


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    artifact = manifest["artifact"]
    data = (ROOT / artifact["path"]).read_bytes()
    if len(data) != artifact["size"]:
        raise SystemExit("Corona SMC size differs from source manifest")
    if hashlib.sha256(data).hexdigest() != artifact["sha256"]:
        raise SystemExit("Corona SMC hash differs from source manifest")
    records = parse_records(data)

    source = (ROOT / manifest["source"]).read_text(encoding="utf-8")
    if not (ROOT / manifest["tests"]).is_file():
        raise SystemExit("missing Corona SMC runtime tests")
    profile = parse_profile(source)
    if profile != manifest["profile_values"]:
        raise SystemExit("Corona source profile differs from coverage manifest")
    if len(profile) != len(manifest["profile_fields"]):
        raise SystemExit("Corona source profile field count changed")

    integration_indices = set()
    for item in manifest["integration_records"]:
        index = item["record_index"]
        if index in integration_indices or index >= len(records):
            raise SystemExit(f"invalid Corona integration record {index}")
        integration_indices.add(index)
        target, payload = records[index]
        if target != int(item["target"], 0) or len(payload) != item["size"]:
            raise SystemExit(f"Corona integration record {index} changed")

    executable = manifest["executable_record"]
    index = executable["record_index"]
    if integration_indices | {index} != set(range(len(records))):
        raise SystemExit("Corona record ownership overlaps or has gaps")
    target, payload = records[index]
    if target != int(executable["target"], 0) or len(payload) != executable["size"]:
        raise SystemExit("Corona executable record changed")
    if hashlib.sha256(payload).hexdigest() != executable["sha256"]:
        raise SystemExit("Corona executable payload hash changed")

    cursor = target
    for region in executable["coverage"]:
        start, end = int(region["start"], 0), int(region["end"], 0)
        if start != cursor or end <= start:
            raise SystemExit("non-contiguous Corona executable coverage")
        cursor = end
        for symbol in region["symbols"]:
            if symbol not in source:
                raise SystemExit(f"missing Corona source symbol: {symbol}")
    if cursor != target + len(payload):
        raise SystemExit("incomplete Corona executable source coverage")
    print(
        "Corona SMC source coverage verified: "
        f"{len(payload)} executable bytes, "
        f"{len(integration_indices)} declarative integration records"
    )


if __name__ == "__main__":
    main()
