#!/usr/bin/env python3
"""Verify the exact strict FatFs R0.16 map in KernelExpansion.bin."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MAP_PATH = ROOT / "analysis/kernel-expansion/KernelExpansion-fatfs-r016-map.json"
INVENTORY_PATH = ROOT / "analysis/kernel-expansion/KernelExpansion-functions.json"
EXPECTED_SCHEMA = "bandwagon-kernel-expansion-fatfs-r016-map-v1"
EXPECTED_FUNCTIONS = 47
EXPECTED_BYTES = 18_728


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def verify_range(
    item: dict[str, object], image: bytes, base: int, image_end: int, label: str
) -> tuple[int, int]:
    start = int(str(item["start"]), 0)
    end = int(str(item["end_exclusive"]), 0)
    size = int(item["size"])
    if start < base or end > image_end or end <= start:
        fail(f"{label} has an invalid range: {start:#x}-{end:#x}")
    if start % 4 or end % 4:
        fail(f"{label} is not instruction-aligned: {start:#x}-{end:#x}")
    if end - start != size:
        fail(f"{label} size differs: declared {size}, range has {end - start}")
    payload = image[start - base : end - base]
    if len(payload) != size:
        fail(f"{label} falls outside the mapped image")
    actual_hash = sha256(payload)
    if actual_hash != item["sha256"]:
        fail(f"{label} bytes changed: expected {item['sha256']}, found {actual_hash}")
    return start, end


def main() -> None:
    mapping = json.loads(MAP_PATH.read_text(encoding="utf-8"))
    if mapping.get("schema") != EXPECTED_SCHEMA:
        fail("unexpected Kernel Expansion FatFs map schema")

    image_info = mapping["image"]
    image_path = ROOT / image_info["path"]
    image = image_path.read_bytes()
    actual_image_hash = sha256(image)
    if actual_image_hash != image_info["sha256"]:
        fail(
            "Kernel Expansion image hash differs: "
            f"expected {image_info['sha256']}, found {actual_image_hash}"
        )

    upstream = mapping["upstream"]
    source_root = ROOT / upstream["vendored_root"]
    source_files: set[str] = set()
    source_texts: dict[str, str] = {}
    for file_info in upstream["files"]:
        relative = file_info["path"]
        source_files.add(relative)
        source_path = source_root / relative
        if not source_path.is_file():
            fail(f"vendored FatFs source is missing: {relative}")
        actual_hash = sha256(source_path.read_bytes())
        if actual_hash != file_info["sha256"]:
            fail(
                f"vendored FatFs source hash differs for {relative}: "
                f"expected {file_info['sha256']}, found {actual_hash}"
            )
        source_texts[relative] = source_path.read_text(
            encoding="utf-8", errors="replace"
        )

    inventory = json.loads(INVENTORY_PATH.read_text(encoding="utf-8"))
    inventory_starts = {
        int(function["address"], 0) for function in inventory["functions"]
    }
    functions = mapping["functions"]
    scope = mapping["scope"]
    declared_count = int(scope["function_count"])
    declared_bytes = int(scope["instruction_bytes"])
    if declared_count != EXPECTED_FUNCTIONS or len(functions) != EXPECTED_FUNCTIONS:
        fail(
            "strict FatFs function count differs: "
            f"scope={declared_count}, entries={len(functions)}, "
            f"expected={EXPECTED_FUNCTIONS}"
        )

    base = int(image_info["base_address"], 0)
    image_end = base + len(image)
    total_bytes = 0
    prior_end = base
    seen_symbols: set[str] = set()
    for index, item in enumerate(functions):
        symbol = item.get("upstream_symbol")
        if not isinstance(symbol, str) or not symbol:
            fail(f"FatFs entry {index} has no upstream symbol")
        if symbol in seen_symbols:
            fail(f"duplicate strict FatFs symbol: {symbol}")
        seen_symbols.add(symbol)
        source_file = item.get("upstream_file")
        if source_file not in source_files:
            fail(f"{symbol} references an unpinned source file: {source_file}")
        if symbol not in source_texts[source_file]:
            fail(f"{symbol} is absent from pinned FatFs source: {source_file}")

        start, end = verify_range(item, image, base, image_end, symbol)
        if start < prior_end:
            fail(f"strict FatFs ranges overlap or regress at {symbol} ({start:#x})")
        if start not in inventory_starts:
            fail(f"strict FatFs start is absent from function inventory: {symbol} ({start:#x})")
        prior_end = end
        total_bytes += end - start

    if declared_bytes != EXPECTED_BYTES or total_bytes != EXPECTED_BYTES:
        fail(
            "strict FatFs byte total differs: "
            f"scope={declared_bytes}, ranges={total_bytes}, expected={EXPECTED_BYTES}"
        )

    strict_ranges = [
        (int(item["start"], 0), int(item["end_exclusive"], 0))
        for item in functions
    ]
    for item in mapping.get("excluded_derived_ranges", []):
        symbol = str(item.get("upstream_symbol", "excluded derived range"))
        start, end = verify_range(item, image, base, image_end, symbol)
        if any(
            start < strict_end and end > strict_start
            for strict_start, strict_end in strict_ranges
        ):
            fail(f"excluded derived range overlaps the strict map: {start:#x}-{end:#x}")

    print(
        "Kernel Expansion FatFs R0.16 map verified: "
        f"{len(functions)} strict functions, {total_bytes} instruction bytes, "
        f"{len(mapping.get('excluded_derived_ranges', []))} mixed range excluded"
    )


if __name__ == "__main__":
    main()
