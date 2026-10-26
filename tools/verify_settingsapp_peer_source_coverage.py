#!/usr/bin/env python3
"""Verify hash-bound readable-source coverage for SettingsApp product code."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/SettingsApp-peer-source-coverage.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def address(value: object) -> int:
    if not isinstance(value, str):
        fail(f"expected hexadecimal address string, got {value!r}")
    return int(value, 0)


def read_bound(record: dict[str, object], label: str) -> tuple[bytes, dict[str, object]]:
    raw = (ROOT / str(record["path"])).read_bytes()
    if len(raw) != record["size"] or digest(raw) != record["sha256"]:
        fail(f"{label} size or hash differs: {record['path']}")
    document = json.loads(raw)
    if document.get("schema") != record["schema"]:
        fail(f"{label} schema differs: {record['path']}")
    return raw, document


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if manifest.get("schema") != "bandwagon-settingsapp-peer-source-coverage-v1":
        fail("unexpected SettingsApp source-coverage schema")
    if manifest.get("status") != "partial_exact_entry_coverage":
        fail("SettingsApp source coverage must remain explicitly partial")
    if "not a claim" not in manifest.get("authorship_policy", ""):
        fail("SettingsApp source coverage must disclaim personal authorship")

    image_record = manifest["image"]
    image = (ROOT / image_record["path"]).read_bytes()
    if len(image) != image_record["size"] or digest(image) != image_record["sha256"]:
        fail("SettingsApp source-coverage image size or hash differs")
    base = address(image_record["base_address"])

    evidence: dict[str, dict[str, object]] = {}
    for name, record in manifest["evidence_artifacts"].items():
        _, evidence[name] = read_bound(record, f"SettingsApp coverage evidence {name}")
    inventory_record = manifest["evidence_artifacts"]["function_inventory"]
    inventory = evidence["function_inventory"]
    functions = inventory["functions"]
    if (
        inventory.get("source_sha256") != image_record["sha256"]
        or inventory.get("function_count") != inventory_record["entry_count"]
        or len(functions) != inventory_record["entry_count"]
    ):
        fail("SettingsApp coverage inventory binding or count differs")
    ownership = evidence["function_ownership"]
    if ownership["image"]["sha256"] != image_record["sha256"]:
        fail("SettingsApp ownership evidence is bound to another image")
    for name in ("semantic_symbols", "string_xrefs"):
        if evidence[name].get("image_sha256") != image_record["sha256"]:
            fail(f"SettingsApp {name} evidence is bound to another image")

    source_bytes: dict[str, bytes] = {}
    for name, record in manifest["source_artifacts"].items():
        raw = (ROOT / record["path"]).read_bytes()
        if len(raw) != record["size"] or digest(raw) != record["sha256"]:
            fail(f"SettingsApp readable-source artifact differs: {record['path']}")
        if record["kind"] not in {"source", "header", "test"}:
            fail(f"unexpected source-artifact kind: {record['kind']}")
        source_bytes[name] = raw

    inventory_by_address = {address(item["address"]): item for item in functions}
    function_addresses = [address(item["address"]) for item in functions]
    if function_addresses != sorted(function_addresses):
        fail("SettingsApp coverage inventory is not ordered")
    next_address: dict[int, int] = {}
    code_end = address(ownership["code_region"]["end_exclusive"])
    for index, entry_address in enumerate(function_addresses):
        next_address[entry_address] = (
            function_addresses[index + 1]
            if index + 1 < len(function_addresses)
            else code_end
        )

    product_category_name = "peer_product_application_glue"
    product_category = ownership["categories"][product_category_name]
    product_addresses: set[int] = set()
    for item in product_category["ranges"]:
        start = address(item["start"])
        end = address(item["end_exclusive"])
        product_addresses.update(
            entry_address
            for entry_address in function_addresses
            if start <= entry_address < end
        )
    if (
        len(product_addresses) != product_category["inventory_entry_count"]
        or sum(int(inventory_by_address[item]["size_to_next_symbol"]) for item in product_addresses)
        != product_category["size"]
    ):
        fail("SettingsApp product ownership totals differ")

    semantic_pairs = {
        (address(item["address"]), item["semantic_name"])
        for item in evidence["semantic_symbols"]["symbols"]
    }
    string_pairs = {
        (address(item["function_address"]), item["string"])
        for item in evidence["string_xrefs"]["references"]
    }

    covered_addresses: set[int] = set()
    covered_bytes = 0
    referenced_artifacts: set[str] = set()
    for record in manifest["functions"]:
        entry_address = address(record["address"])
        if entry_address in covered_addresses:
            fail(f"duplicate SettingsApp covered entry: {entry_address:#x}")
        covered_addresses.add(entry_address)
        inventory_entry = inventory_by_address.get(entry_address)
        if inventory_entry is None:
            fail(f"covered SettingsApp address is not an inventory entry: {entry_address:#x}")
        if inventory_entry["name"].lower() != record["inventory_name"].lower():
            fail(f"SettingsApp inventory name differs at {entry_address:#x}")
        extent_end = next_address[entry_address]
        if (
            address(record["end_exclusive"]) != extent_end
            or record["size"] != extent_end - entry_address
            or record["size"] != inventory_entry["size_to_next_symbol"]
        ):
            fail(f"SettingsApp covered extent differs at {entry_address:#x}")
        payload = image[entry_address - base : extent_end - base]
        if len(payload) != record["size"] or digest(payload) != record["sha256"]:
            fail(f"SettingsApp covered bytes differ at {entry_address:#x}")
        if (
            record["ownership_category"] != product_category_name
            or entry_address not in product_addresses
        ):
            fail(f"SettingsApp covered entry is outside product ownership: {entry_address:#x}")
        if record["coverage_kind"] != "readable_behavioral_model_for_exact_function_entry":
            fail(f"unexpected SettingsApp coverage kind at {entry_address:#x}")
        if (entry_address, record["semantic_name"]) not in semantic_pairs:
            fail(f"SettingsApp semantic mapping differs at {entry_address:#x}")
        for required_string in record["required_strings"]:
            if (entry_address, required_string) not in string_pairs:
                fail(f"SettingsApp string evidence differs at {entry_address:#x}: {required_string}")

        model_artifacts = record["source_artifacts"]
        test_artifacts = record["test_artifacts"]
        if not model_artifacts or not test_artifacts:
            fail(f"SettingsApp covered entry lacks source or tests: {entry_address:#x}")
        if any(manifest["source_artifacts"].get(item, {}).get("kind") == "test" for item in model_artifacts):
            fail(f"SettingsApp model artifact is mislabeled as a test: {entry_address:#x}")
        if any(manifest["source_artifacts"].get(item, {}).get("kind") != "test" for item in test_artifacts):
            fail(f"SettingsApp test artifact kind differs: {entry_address:#x}")
        artifact_names = model_artifacts + test_artifacts
        if any(item not in source_bytes for item in artifact_names):
            fail(f"SettingsApp function references an unknown source artifact: {entry_address:#x}")
        referenced_artifacts.update(artifact_names)
        readable_model = b"\n".join(source_bytes[item] for item in model_artifacts).decode("utf-8")
        for symbol in record["symbols"]:
            if symbol not in readable_model:
                fail(f"SettingsApp readable model lacks {symbol}: {entry_address:#x}")
        covered_bytes += record["size"]

    if referenced_artifacts != set(manifest["source_artifacts"]):
        fail("SettingsApp source-coverage manifest has unreferenced artifacts")
    summary = manifest["summary"]
    expected_summary = {
        "product_mixed_function_count": product_category["inventory_entry_count"],
        "product_mixed_bytes": product_category["size"],
        "model_covered_function_count": len(covered_addresses),
        "model_covered_bytes": covered_bytes,
        "remaining_product_mixed_function_count": product_category["inventory_entry_count"] - len(covered_addresses),
        "remaining_product_mixed_bytes": product_category["size"] - covered_bytes,
    }
    if summary != expected_summary:
        fail("SettingsApp readable-source coverage summary differs")

    print(
        "SettingsApp Peer source coverage verified: "
        f"{len(covered_addresses)}/{product_category['inventory_entry_count']} product/mixed entries, "
        f"{covered_bytes}/{product_category['size']} bytes; "
        f"remaining {expected_summary['remaining_product_mixed_function_count']}/"
        f"{expected_summary['remaining_product_mixed_bytes']}"
    )


if __name__ == "__main__":
    main()
