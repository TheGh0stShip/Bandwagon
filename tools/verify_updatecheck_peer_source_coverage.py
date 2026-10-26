#!/usr/bin/env python3
"""Verify exact UpdateCheck product/mixed readable-source coverage."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/UpdateCheck-peer-source-coverage.json"


def digest(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def read_bound_file(item: dict[str, object], label: str) -> bytes:
    path = ROOT / str(item["path"])
    if not path.is_file():
        fail(f"missing {label}: {path}")
    payload = path.read_bytes()
    if digest(payload) != item["sha256"]:
        fail(f"{label} hash differs: {path}")
    return payload


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if manifest.get("schema") != "bandwagon-updatecheck-peer-source-coverage-v2":
        fail("unexpected UpdateCheck source-coverage schema")

    image_info = manifest["image"]
    image_path = ROOT / image_info["path"]
    image = image_path.read_bytes()
    if len(image) != image_info["size"]:
        fail("UpdateCheck image size differs from source-coverage manifest")
    if digest(image) != image_info["sha256"]:
        fail("UpdateCheck image hash differs from source-coverage manifest")
    image_base = int(image_info["base_address"], 0)

    inventory_info = manifest["function_inventory"]
    inventory = json.loads(
        read_bound_file(inventory_info, "UpdateCheck function inventory")
    )
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("UpdateCheck function-inventory count differs")
    if inventory.get("source_sha256") != image_info["sha256"]:
        fail("UpdateCheck function inventory is bound to a different image")
    inventory_by_address = {
        int(item["address"], 0): item for item in functions
    }

    ownership_info = manifest["ownership_partition"]
    ownership_path = ROOT / ownership_info["path"]
    ownership = json.loads(ownership_path.read_text(encoding="utf-8"))
    if ownership.get("schema") != ownership_info["schema"]:
        fail("UpdateCheck ownership schema differs")
    category_name = ownership_info["category"]
    category = ownership["categories"][category_name]
    if category["inventory_entry_count"] != ownership_info["inventory_entry_count"]:
        fail("UpdateCheck product ownership entry count differs")
    if category["size"] != ownership_info["size"]:
        fail("UpdateCheck product ownership byte count differs")
    if category["concatenated_sha256"] != ownership_info[
        "category_concatenated_sha256"
    ]:
        fail("UpdateCheck product ownership byte hash differs")

    product_addresses: set[int] = set()
    for item in category["ranges"]:
        start = int(item["start"], 0)
        end = int(item["end_exclusive"], 0)
        product_addresses.update(
            address for address in inventory_by_address if start <= address < end
        )
    if len(product_addresses) != category["inventory_entry_count"]:
        fail("UpdateCheck product ownership ranges differ from inventory")

    symbols_info = manifest["peer_symbol_inventory"]
    symbol_inventory = json.loads(
        read_bound_file(symbols_info, "UpdateCheck semantic-symbol inventory")
    )
    identified_product_symbols = {
        (int(item["address"], 0), item["semantic_name"])
        for item in symbol_inventory["symbols"]
        if "ownership" not in item
    }

    source_text: dict[str, str] = {}
    for item in manifest["source_files"]:
        payload = read_bound_file(item, "UpdateCheck readable source")
        source_text[item["path"]] = payload.decode("utf-8")
    all_source_text = "\n".join(source_text.values())
    for item in manifest["tests"]:
        read_bound_file(item, "UpdateCheck source test")

    covered_addresses: set[int] = set()
    remaining_addresses: set[int] = set()
    covered_bytes = 0
    remaining_bytes = 0
    covered_names: set[tuple[int, str]] = set()

    for state, records in (
        ("covered", manifest["functions"]),
        ("remaining", manifest["remaining_functions"]),
    ):
        destination = covered_addresses if state == "covered" else remaining_addresses
        for record in records:
            address = int(record["address"], 0)
            if address in destination:
                fail(f"duplicate {state} function at {address:#x}")
            function = inventory_by_address.get(address)
            if function is None:
                fail(f"{state} function is absent from inventory at {address:#x}")
            size = int(function["size_to_next_symbol"])
            if size != record["size"]:
                fail(f"{state} function size differs at {address:#x}")
            payload = image[address - image_base : address - image_base + size]
            if len(payload) != size or digest(payload) != record["sha256"]:
                fail(f"{state} function bytes differ at {address:#x}")
            destination.add(address)

            if state == "covered":
                covered_bytes += size
                if record["source"] not in source_text:
                    fail(f"unbound source file for covered function {address:#x}")
                for symbol in record["symbols"]:
                    if symbol not in all_source_text:
                        fail(f"missing UpdateCheck readable-source symbol: {symbol}")
                covered_names.add((address, record["semantic_name"]))
            else:
                remaining_bytes += size
                if not record.get("reason"):
                    fail(f"remaining function lacks a reason at {address:#x}")

    if covered_addresses & remaining_addresses:
        fail("UpdateCheck function is both covered and remaining")
    if covered_addresses | remaining_addresses != product_addresses:
        missing = product_addresses - covered_addresses - remaining_addresses
        extra = covered_addresses | remaining_addresses - product_addresses
        fail(
            "UpdateCheck product coverage is not exhaustive: "
            f"missing={sorted(hex(item) for item in missing)}, "
            f"extra={sorted(hex(item) for item in extra)}"
        )
    if not identified_product_symbols.issubset(covered_names):
        fail("identified UpdateCheck product symbols are not source-covered")

    coverage = manifest["coverage"]
    actual_covered = (len(covered_addresses), covered_bytes)
    expected_covered = (
        coverage["covered_inventory_entry_count"],
        coverage["covered_size"],
    )
    if actual_covered != expected_covered:
        fail("UpdateCheck covered function totals differ")
    actual_remaining = (len(remaining_addresses), remaining_bytes)
    expected_remaining = (
        coverage["remaining_inventory_entry_count"],
        coverage["remaining_size"],
    )
    if actual_remaining != expected_remaining:
        fail("UpdateCheck remaining function totals differ")
    if len(covered_addresses) + len(remaining_addresses) != category[
        "inventory_entry_count"
    ]:
        fail("UpdateCheck product coverage entry sum differs")
    if covered_bytes + remaining_bytes != category["size"]:
        fail("UpdateCheck product coverage byte sum differs")

    print(
        "UpdateCheck product source coverage verified: "
        f"{len(covered_addresses)}/{category['inventory_entry_count']} entries, "
        f"{covered_bytes}/{category['size']} exact bytes covered; "
        f"{len(remaining_addresses)} entries, {remaining_bytes} bytes remain"
    )


if __name__ == "__main__":
    main()
