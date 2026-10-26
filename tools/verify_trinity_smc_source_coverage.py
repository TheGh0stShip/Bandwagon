#!/usr/bin/env python3
"""Verify complete executable-source coverage for the Trinity SMC stream."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/smc/Trinity-source-coverage.json"


def parse_records(data: bytes) -> list[tuple[int, bytes]]:
    records = []
    cursor = 0
    while True:
        if cursor == len(data):
            return records
        if len(data) - cursor < 4:
            raise SystemExit("truncated Trinity SMC record target")
        target = int.from_bytes(data[cursor:cursor + 4], "big")
        cursor += 4
        if target == 0xffffffff:
            if cursor != len(data):
                raise SystemExit("bytes follow Trinity SMC sentinel")
            return records
        if len(data) - cursor < 4:
            raise SystemExit("truncated Trinity SMC record size")
        size = int.from_bytes(data[cursor:cursor + 4], "big")
        cursor += 4
        if len(data) - cursor < size:
            raise SystemExit("truncated Trinity SMC record payload")
        records.append((target, data[cursor:cursor + size]))
        cursor += size


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    artifact = manifest["artifact"]
    data = (ROOT / artifact["path"]).read_bytes()
    if len(data) != artifact["size"]:
        raise SystemExit("Trinity SMC size differs from source manifest")
    if hashlib.sha256(data).hexdigest() != artifact["sha256"]:
        raise SystemExit("Trinity SMC hash differs from source manifest")
    records = parse_records(data)

    source = (ROOT / manifest["source"]).read_text(encoding="utf-8")
    tests = ROOT / manifest["tests"]
    if not tests.is_file():
        raise SystemExit("missing Trinity runtime tests")
    modeled = set()
    for executable in manifest["executable_records"]:
        index = executable["record_index"]
        modeled.add(index)
        if index >= len(records):
            raise SystemExit(f"missing Trinity executable record {index}")
        target, payload = records[index]
        expected_target = int(executable["target"], 0)
        if target != expected_target or len(payload) != executable["size"]:
            raise SystemExit(f"Trinity executable record {index} changed")
        cursor = target
        for region in executable["coverage"]:
            start, end = int(region["start"], 0), int(region["end"], 0)
            if start != cursor or end <= start:
                raise SystemExit(
                    f"non-contiguous Trinity source coverage in record {index}"
                )
            cursor = end
            for symbol in region["symbols"]:
                if symbol not in source:
                    raise SystemExit(f"missing Trinity source symbol: {symbol}")
        if cursor != target + len(payload):
            raise SystemExit(f"incomplete Trinity source coverage in record {index}")

    integration = {
        item["record_index"] for item in manifest["integration_records"]
    }
    for item in manifest["integration_records"]:
        target, payload = records[item["record_index"]]
        if target != int(item["target"], 0) or len(payload) != item["size"]:
            raise SystemExit(
                f"Trinity integration record {item['record_index']} changed"
            )
    if modeled & integration or modeled | integration != set(range(len(records))):
        raise SystemExit("Trinity record ownership is incomplete or overlapping")
    print(
        "Trinity SMC source coverage verified: "
        f"{sum(len(records[i][1]) for i in modeled)} executable bytes, "
        f"{len(integration)} declarative integration records"
    )


if __name__ == "__main__":
    main()
