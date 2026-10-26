#!/usr/bin/env python3
"""Bind lifted Kernel Expansion functions to exact immutable image bytes."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = (
    ROOT / "analysis/kernel-expansion/KernelExpansion-peer-source-coverage.json"
)
OWNERSHIP = ROOT / "analysis/kernel-expansion/KernelExpansion-function-ownership.json"


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    image_info = manifest["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if len(image) != image_info["size"]:
        raise SystemExit("Kernel Expansion image size differs from source manifest")
    if hashlib.sha256(image).hexdigest() != image_info["sha256"]:
        raise SystemExit("Kernel Expansion image hash differs from source manifest")

    inventory = json.loads(
        (ROOT / manifest["function_inventory"]).read_text(encoding="utf-8")
    )
    inventory_functions = {
        (item["address"].lower(), item["name"]): item
        for item in inventory["functions"]
    }
    ownership = json.loads(OWNERSHIP.read_text(encoding="utf-8"))
    ownership_ranges = [
        (int(item["start"], 0), int(item["end_exclusive"], 0), category)
        for category, category_info in ownership["categories"].items()
        for item in category_info["ranges"]
    ]

    base = int(image_info["base_address"], 0)
    total = 0
    upstream_count = 0
    upstream_total = 0
    ownership_totals: dict[str, list[int]] = {}
    data_range_count = 0
    data_total = 0
    for function in manifest["functions"]:
        key = (function["address"].lower(), function["semantic_name"])
        if key not in inventory_functions:
            raise SystemExit(f"missing Kernel Expansion inventory entry: {key}")
        inventory_function = inventory_functions[key]
        if function["size"] != inventory_function["size_to_next_symbol"]:
            raise SystemExit(
                "Kernel Expansion source boundary differs from inventory at "
                f"{function['address']}: manifest {function['size']} bytes, "
                f"inventory {inventory_function['size_to_next_symbol']} bytes"
            )
        address = int(function["address"], 0)
        categories = [
            category
            for start, end, category in ownership_ranges
            if start <= address < end
        ]
        if len(categories) != 1:
            raise SystemExit(
                f"Kernel Expansion lift has ambiguous ownership at {address:#x}"
            )
        ownership_totals.setdefault(categories[0], [0, 0])
        ownership_totals[categories[0]][0] += 1
        ownership_totals[categories[0]][1] += function["size"]
        offset = address - base
        payload = image[offset:offset + function["size"]]
        if len(payload) != function["size"]:
            raise SystemExit(f"truncated Kernel Expansion function at {address:#x}")
        if hashlib.sha256(payload).hexdigest() != function["sha256"]:
            raise SystemExit(f"Kernel Expansion function changed at {address:#x}")
        source = (ROOT / function["source"]).read_text(encoding="utf-8")
        header = (
            ROOT
            / function.get(
                "header", "reconstruction/kernel_expansion/runtime.hpp"
            )
        ).read_text(encoding="utf-8")
        for symbol in function["symbols"]:
            if symbol not in source and symbol not in header:
                raise SystemExit(f"missing Kernel Expansion source symbol: {symbol}")
        if function.get("provenance") == "upstream_fatfs_r016":
            upstream = function["upstream_source"]
            upstream_text = (ROOT / upstream["path"]).read_text(
                encoding="utf-8", errors="replace"
            )
            if upstream["symbol"] not in upstream_text:
                raise SystemExit(
                    "missing FatFs source symbol for Kernel Expansion lift: "
                    f"{upstream['symbol']}"
                )
            upstream_count += 1
            upstream_total += len(payload)
        for dependency in function.get("data_dependencies", []):
            dependency_address = int(dependency["address"], 0)
            dependency_offset = dependency_address - base
            data = image[
                dependency_offset : dependency_offset + dependency["size"]
            ]
            if len(data) != dependency["size"]:
                raise SystemExit(
                    "truncated Kernel Expansion data dependency at "
                    f"{dependency_address:#x}"
                )
            if hashlib.sha256(data).hexdigest() != dependency["sha256"]:
                raise SystemExit(
                    "Kernel Expansion data dependency changed at "
                    f"{dependency_address:#x}"
                )
            for symbol in dependency.get("symbols", []):
                if symbol not in source and symbol not in header:
                    raise SystemExit(
                        f"missing Kernel Expansion data symbol: {symbol}"
                    )
            data_range_count += 1
            data_total += len(data)
        total += len(payload)

    summary = manifest["summary"]
    if summary["lifted_function_count"] != len(manifest["functions"]):
        raise SystemExit("Kernel Expansion lifted-function total differs")
    if summary["lifted_instruction_bytes"] != total:
        raise SystemExit("Kernel Expansion lifted-byte total differs")
    if summary["upstream_fatfs_function_count"] != upstream_count:
        raise SystemExit("Kernel Expansion lifted FatFs-function total differs")
    if summary["upstream_fatfs_instruction_bytes"] != upstream_total:
        raise SystemExit("Kernel Expansion lifted FatFs-byte total differs")
    ownership_summary_fields = {
        "peer_product_adapter_or_policy": (
            "peer_product_adapter_or_policy_function_count",
            "peer_product_adapter_or_policy_instruction_bytes",
        ),
        "platform_toolchain": (
            "generic_platform_function_count",
            "generic_platform_instruction_bytes",
        ),
        "unknown": (
            "unattributed_function_count",
            "unattributed_instruction_bytes",
        ),
        "upstream_fatfs_r016": (
            "upstream_fatfs_function_count",
            "upstream_fatfs_instruction_bytes",
        ),
    }
    for category, (count_field, byte_field) in ownership_summary_fields.items():
        actual_count, actual_bytes = ownership_totals.get(category, [0, 0])
        if summary[count_field] != actual_count or summary[byte_field] != actual_bytes:
            raise SystemExit(
                f"Kernel Expansion lifted {category} ownership totals differ"
            )
    partition_count = sum(
        summary[name]
        for name in (
            "peer_product_adapter_or_policy_function_count",
            "generic_platform_function_count",
            "unattributed_function_count",
            "upstream_fatfs_function_count",
        )
    )
    partition_bytes = sum(
        summary[name]
        for name in (
            "peer_product_adapter_or_policy_instruction_bytes",
            "generic_platform_instruction_bytes",
            "unattributed_instruction_bytes",
            "upstream_fatfs_instruction_bytes",
        )
    )
    if partition_count != len(manifest["functions"]) or partition_bytes != total:
        raise SystemExit("Kernel Expansion lifted provenance partition differs")

    for test in manifest["tests"]:
        if not (ROOT / test).is_file():
            raise SystemExit(f"missing Kernel Expansion source test: {test}")
    range_label = "range" if data_range_count == 1 else "ranges"
    print(
        "Kernel Expansion source coverage verified: "
        f"{summary['peer_product_adapter_or_policy_function_count']} product "
        "adapter/policy functions / "
        f"{summary['peer_product_adapter_or_policy_instruction_bytes']} bytes; "
        f"{upstream_count} FatFs, {summary['generic_platform_function_count']} "
        f"platform/toolchain, {summary['unattributed_function_count']} unattributed; "
        f"{data_range_count} data {range_label}, "
        f"{data_total} exact bytes"
    )


if __name__ == "__main__":
    main()
