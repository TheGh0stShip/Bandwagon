#!/usr/bin/env python3
"""Verify executable-source coverage for Falcon/Jasper/Xenon/Zephyr SMC streams."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/smc/Legacy-source-coverage.json"


def parse_records(data: bytes, board: str) -> list[tuple[int, bytes]]:
    records = []
    cursor = 0
    while cursor < len(data):
        if len(data) - cursor < 8:
            raise SystemExit(f"truncated {board} SMC record header")
        target = int.from_bytes(data[cursor:cursor + 4], "big")
        size = int.from_bytes(data[cursor + 4:cursor + 8], "big")
        cursor += 8
        if len(data) - cursor < size:
            raise SystemExit(f"truncated {board} SMC record payload")
        records.append((target, data[cursor:cursor + size]))
        cursor += size
    return records


def parse_source_profiles(source: str) -> dict[str, list[int | bool]]:
    profiles: dict[str, list[int | bool]] = {}
    pattern = re.compile(r"\{Motherboard::k(\w+),([^{}]+)\}", re.DOTALL)
    for match in pattern.finditer(source):
        values: list[int | bool] = []
        for token in match.group(2).split(","):
            token = token.strip()
            if token == "true":
                values.append(True)
            elif token == "false":
                values.append(False)
            else:
                values.append(int(token, 0))
        profiles[match.group(1)] = values
    return profiles


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    source = (ROOT / manifest["source"]).read_text(encoding="utf-8")
    if not (ROOT / manifest["tests"]).is_file():
        raise SystemExit("missing legacy-board SMC runtime tests")
    profiles = parse_source_profiles(source)
    total_executable = 0
    total_integration = 0

    for board_manifest in manifest["boards"]:
        board = board_manifest["board"]
        artifact = board_manifest["artifact"]
        data = (ROOT / artifact["path"]).read_bytes()
        if len(data) != artifact["size"]:
            raise SystemExit(f"{board} SMC size differs from source manifest")
        if hashlib.sha256(data).hexdigest() != artifact["sha256"]:
            raise SystemExit(f"{board} SMC hash differs from source manifest")
        records = parse_records(data, board)
        executable = board_manifest["executable_record"]
        integration = board_manifest["integration_records"]
        if len(records) != len(integration) + 1:
            raise SystemExit(f"{board} SMC record ownership is incomplete")

        integration_indices = set()
        for item in integration:
            index = item["record_index"]
            if index in integration_indices or index >= len(records):
                raise SystemExit(f"invalid {board} integration record {index}")
            integration_indices.add(index)
            target, payload = records[index]
            if target != int(item["target"], 0) or len(payload) != item["size"]:
                raise SystemExit(f"{board} integration record {index} changed")

        index = executable["record_index"]
        if integration_indices | {index} != set(range(len(records))):
            raise SystemExit(f"{board} record ownership overlaps or has gaps")
        target, payload = records[index]
        if target != int(executable["target"], 0) or len(payload) != executable["size"]:
            raise SystemExit(f"{board} executable record changed")
        if hashlib.sha256(payload).hexdigest() != executable["sha256"]:
            raise SystemExit(f"{board} executable payload hash changed")

        cursor = target
        for region in executable["coverage"]:
            start, end = int(region["start"], 0), int(region["end"], 0)
            if start != cursor or end <= start:
                raise SystemExit(f"non-contiguous {board} executable coverage")
            cursor = end
            for symbol in region["symbols"]:
                if symbol not in source:
                    raise SystemExit(f"missing legacy-board source symbol: {symbol}")
        if cursor != target + len(payload):
            raise SystemExit(f"incomplete {board} executable source coverage")

        if profiles.get(board) != board_manifest["profile_values"]:
            raise SystemExit(f"{board} source profile differs from coverage manifest")
        if len(profiles[board]) != len(manifest["profile_fields"]):
            raise SystemExit(f"{board} source profile field count changed")
        total_executable += len(payload)
        total_integration += len(integration)

    if set(profiles) != {board["board"] for board in manifest["boards"]}:
        raise SystemExit("unexpected or missing legacy-board source profile")
    print(
        "Legacy-board SMC source coverage verified: "
        f"{total_executable} executable bytes, "
        f"{total_integration} declarative integration records across 4 boards"
    )


if __name__ == "__main__":
    main()
