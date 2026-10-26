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

    base = int(image_info["base_address"], 0)
    total = 0
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

    for test in manifest["tests"]:
        if not (ROOT / test).is_file():
            raise SystemExit(f"missing Kernel Expansion source test: {test}")
    range_label = "range" if data_range_count == 1 else "ranges"
    print(
        "Kernel Expansion source coverage verified: "
        f"{len(manifest['functions'])} identified functions, "
        f"{total} exact instruction bytes; {data_range_count} data {range_label}, "
        f"{data_total} exact bytes"
    )


if __name__ == "__main__":
    main()
