#!/usr/bin/env python3
"""Verify exact Kernel Expansion ownership/provenance range claims."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
ATTRIBUTION = (
    ROOT / "analysis/kernel-expansion/KernelExpansion-source-attribution.json"
)
INVENTORY = ROOT / "analysis/kernel-expansion/KernelExpansion-functions.json"


def main() -> None:
    attribution = json.loads(ATTRIBUTION.read_text(encoding="utf-8"))
    image_info = attribution["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if hashlib.sha256(image).hexdigest() != image_info["sha256"]:
        raise SystemExit("Kernel Expansion attribution image hash differs")

    for source_name, source in attribution.get("upstream_sources", {}).items():
        source_root = ROOT / source["vendored_root"]
        for file_info in source["files"]:
            source_path = source_root / file_info["path"]
            if hashlib.sha256(source_path.read_bytes()).hexdigest() != file_info["sha256"]:
                raise SystemExit(
                    f"{source_name} source hash differs: {file_info['path']}"
                )

    inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
    function_addresses = [
        int(function["address"], 0) for function in inventory["functions"]
    ]
    base = int(image_info["base_address"], 0)
    prior_end = base
    classified_bytes = 0
    classified_entries = 0
    for item in attribution["classified_ranges"]:
        start = int(item["start"], 0)
        end = int(item["end_exclusive"], 0)
        if start < prior_end or end <= start:
            raise SystemExit("Kernel Expansion attribution ranges overlap or regress")
        payload = image[start - base : end - base]
        if len(payload) != item["size"] or end - start != item["size"]:
            raise SystemExit(f"invalid attribution size at {start:#x}")
        if hashlib.sha256(payload).hexdigest() != item["sha256"]:
            raise SystemExit(f"attribution bytes changed at {start:#x}")
        entries = sum(start <= address < end for address in function_addresses)
        if entries != item["inventory_entry_count"]:
            raise SystemExit(
                f"attribution inventory count changed at {start:#x}: "
                f"expected {item['inventory_entry_count']}, found {entries}"
            )
        prior_end = end
        classified_bytes += len(payload)
        classified_entries += entries

    print(
        "Kernel Expansion source attribution verified: "
        f"{len(attribution['classified_ranges'])} non-Peer ranges, "
        f"{classified_bytes} exact bytes, {classified_entries} inventory entries"
    )


if __name__ == "__main__":
    main()
