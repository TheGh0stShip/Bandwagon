#!/usr/bin/env python3
"""Verify the exhaustive KernelExpansion function-ownership partition."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = (
    ROOT / "analysis/kernel-expansion/KernelExpansion-function-ownership.json"
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def main() -> None:
    ownership = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if ownership.get("schema") != "bandwagon-kernel-expansion-function-ownership-v1":
        fail("unexpected KernelExpansion ownership schema")

    image_info = ownership["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if digest(image) != image_info["sha256"]:
        fail("KernelExpansion image hash differs")

    inventory_info = ownership["function_inventory"]
    inventory_path = ROOT / inventory_info["path"]
    inventory_bytes = inventory_path.read_bytes()
    if digest(inventory_bytes) != inventory_info["sha256"]:
        fail("KernelExpansion function inventory hash differs")
    inventory = json.loads(inventory_bytes)
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("KernelExpansion inventory entry count differs")

    fatfs_map_info = ownership["fatfs_function_map"]
    fatfs_map_path = ROOT / fatfs_map_info["path"]
    fatfs_map_bytes = fatfs_map_path.read_bytes()
    if digest(fatfs_map_bytes) != fatfs_map_info["sha256"]:
        fail("KernelExpansion FatFs function-map hash differs")
    fatfs_map = json.loads(fatfs_map_bytes)

    for source_name, source in ownership.get("upstream_sources", {}).items():
        source_root = ROOT / source["vendored_root"]
        for file_info in source["files"]:
            source_path = source_root / file_info["path"]
            if digest(source_path.read_bytes()) != file_info["sha256"]:
                fail(f"{source_name} source hash differs: {file_info['path']}")

    base = int(image_info["base_address"], 0)
    region = ownership["instruction_region"]
    region_start = int(region["start"], 0)
    region_end = int(region["end_exclusive"], 0)
    region_bytes = image[region_start - base : region_end - base]
    if region_end - region_start != region["size"]:
        fail("KernelExpansion instruction-region size metadata differs")
    if len(region_bytes) != region["size"] or digest(region_bytes) != region["sha256"]:
        fail("KernelExpansion instruction-region bytes differ")

    tail = ownership["excluded_alignment_tail"]
    tail_start = int(tail["start"], 0)
    tail_end = int(tail["end_exclusive"], 0)
    tail_bytes = image[tail_start - base : tail_end - base]
    if tail_start != region_end:
        fail("alignment tail does not begin at the instruction-region end")
    if tail_end - tail_start != tail["size"] or len(tail_bytes) != tail["size"]:
        fail("alignment-tail size metadata differs")
    if digest(tail_bytes) != tail["sha256"]:
        fail("KernelExpansion alignment-tail bytes differ")

    required_categories = {
        "platform_toolchain",
        "upstream_fatfs_r016",
        "peer_product_adapter_or_policy",
        "unknown",
    }
    categories = ownership["categories"]
    if set(categories) != required_categories:
        fail("KernelExpansion ownership category set differs")

    flat_ranges: list[tuple[int, int, str, dict[str, object]]] = []
    entry_owners: dict[int, str] = {}
    function_addresses = [int(function["address"], 0) for function in functions]
    function_address_set = set(function_addresses)
    allowed_boundaries = function_address_set | {region_end}

    total_entries = 0
    total_bytes = 0
    for category_name, category in categories.items():
        category_ranges: list[tuple[int, int, dict[str, object]]] = []
        for item in category["ranges"]:
            start = int(item["start"], 0)
            end = int(item["end_exclusive"], 0)
            if start not in function_address_set or end not in allowed_boundaries:
                fail(f"{category_name} range is not inventory-aligned at {start:#x}")
            if start < region_start or end > region_end or end <= start:
                fail(f"invalid {category_name} range at {start:#x}")
            payload = image[start - base : end - base]
            if end - start != item["size"] or len(payload) != item["size"]:
                fail(f"invalid {category_name} range size at {start:#x}")
            if digest(payload) != item["sha256"]:
                fail(f"{category_name} range bytes changed at {start:#x}")

            addresses = [address for address in function_addresses if start <= address < end]
            if len(addresses) != item["inventory_entry_count"]:
                fail(
                    f"{category_name} inventory count changed at {start:#x}: "
                    f"expected {item['inventory_entry_count']}, found {len(addresses)}"
                )
            for address in addresses:
                if address in entry_owners:
                    fail(f"function {address:#x} has multiple ownership categories")
                entry_owners[address] = category_name
            category_ranges.append((start, end, item))
            flat_ranges.append((start, end, category_name, item))

        category_ranges.sort()
        concatenated = b"".join(
            image[start - base : end - base] for start, end, _ in category_ranges
        )
        category_entries = sum(
            int(item["inventory_entry_count"]) for _, _, item in category_ranges
        )
        category_bytes = sum(int(item["size"]) for _, _, item in category_ranges)
        if category_entries != category["inventory_entry_count"]:
            fail(f"{category_name} aggregate entry count differs")
        if category_bytes != category["size"] or len(concatenated) != category["size"]:
            fail(f"{category_name} aggregate byte count differs")
        if digest(concatenated) != category["concatenated_sha256"]:
            fail(f"{category_name} concatenated hash differs")
        if "subclassification_summary" in category:
            summary_entries = sum(
                int(item["inventory_entry_count"])
                for item in category["subclassification_summary"]
            )
            summary_bytes = sum(
                int(item["size"]) for item in category["subclassification_summary"]
            )
            if summary_entries != category_entries or summary_bytes != category_bytes:
                fail(f"{category_name} subclassification totals differ")
        total_entries += category_entries
        total_bytes += category_bytes

    flat_ranges.sort()
    next_address = region_start
    for start, end, category_name, _ in flat_ranges:
        if start != next_address:
            fail(
                f"ownership partition gap/overlap before {start:#x} "
                f"({category_name}); expected {next_address:#x}"
            )
        next_address = end
    if next_address != region_end:
        fail("ownership partition does not reach the instruction-region end")

    if set(entry_owners) != function_address_set:
        missing = sorted(function_address_set - set(entry_owners))
        fail(f"ownership partition misses function entries: {missing}")

    strict_fatfs_addresses = {
        int(item["start"], 0) for item in fatfs_map["functions"]
    }
    owned_strict_fatfs_addresses = {
        address
        for address, owner in entry_owners.items()
        if owner == "upstream_fatfs_r016"
    }
    if strict_fatfs_addresses != owned_strict_fatfs_addresses:
        fail("strict FatFs ownership differs from the authoritative function map")
    if len(strict_fatfs_addresses) != fatfs_map_info["strict_entry_count"]:
        fail("strict FatFs function-map count differs")
    mixed_fatfs_addresses = {
        int(item["start"], 0) for item in fatfs_map["excluded_derived_ranges"]
    }
    if len(mixed_fatfs_addresses) != fatfs_map_info["mixed_derived_entry_count"]:
        fail("mixed FatFs-derived function-map count differs")
    for address in mixed_fatfs_addresses:
        if entry_owners.get(address) != "peer_product_adapter_or_policy":
            fail(f"mixed FatFs-derived routine {address:#x} left the adapter bucket")

    # Calling an XDK service or implementing a generic-looking hook does not
    # establish compiler/platform ownership. These nine app-local bodies have
    # readable replacement source and must remain in the product/local-adapter
    # boundary rather than being excluded by role alone.
    required_local_adapters = {
        0x801F03C0: 192,
        0x801F0480: 56,
        0x801F04B8: 176,
        0x801F05F0: 88,
        0x801F14D0: 472,
        0x801F18E8: 8,
        0x801F18F0: 216,
        0x801F2D70: 72,
        0x801F2DB8: 32,
    }
    function_sizes = {
        int(function["address"], 0): int(function["size_to_next_symbol"])
        for function in functions
    }
    for address, size in required_local_adapters.items():
        if function_sizes.get(address) != size:
            fail(f"app-local adapter boundary changed at {address:#x}")
        if entry_owners.get(address) != "peer_product_adapter_or_policy":
            fail(f"app-local adapter {address:#x} left the product boundary")

    # Every complete inventory function extent must remain in one category. The
    # final inventory span includes eight alignment bytes, so cap it at f4a8.
    for index, function in enumerate(functions):
        start = int(function["address"], 0)
        end = (
            int(functions[index + 1]["address"], 0)
            if index + 1 < len(functions)
            else region_end
        )
        owner = entry_owners[start]
        containing = [
            item
            for item in flat_ranges
            if item[2] == owner and item[0] <= start and end <= item[1]
        ]
        if len(containing) != 1:
            fail(f"function extent {start:#x}-{end:#x} is split or uncovered")

    totals = ownership["totals"]
    if total_entries != totals["inventory_entry_count"] or total_entries != len(functions):
        fail("KernelExpansion ownership total entry count differs")
    if total_bytes != totals["instruction_size"] or total_bytes != len(region_bytes):
        fail("KernelExpansion ownership total byte count differs")
    if totals["category_count"] != len(categories):
        fail("KernelExpansion ownership category count differs")

    badupdate = ownership["public_badupdate_overlap"]
    if badupdate["inventory_entry_count"] != 0 or badupdate["size"] != 0:
        fail("public BadUpdate overlap must remain a zero-body attribution")

    summary = ", ".join(
        f"{name}={category['inventory_entry_count']}/{category['size']}"
        for name, category in categories.items()
    )
    print(
        "KernelExpansion function ownership verified: "
        f"{total_entries} entries, {total_bytes} instruction bytes; {summary}"
    )


if __name__ == "__main__":
    main()
