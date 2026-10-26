#!/usr/bin/env python3
"""Verify hash-bound readable-source coverage for BootEdit product code."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/BootEdit-peer-source-coverage.json"


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
    if manifest.get("schema") != "bandwagon-bootedit-peer-source-coverage-v1":
        fail("unexpected BootEdit source-coverage schema")
    if manifest.get("status") != "partial_exact_entry_coverage":
        fail("BootEdit source coverage must remain explicitly partial")
    if "not a claim" not in manifest.get("authorship_policy", ""):
        fail("BootEdit source coverage must disclaim personal authorship")

    image_record = manifest["image"]
    image = (ROOT / image_record["path"]).read_bytes()
    if len(image) != image_record["size"] or digest(image) != image_record["sha256"]:
        fail("BootEdit source-coverage image size or hash differs")
    base = address(image_record["base_address"])

    evidence: dict[str, dict[str, object]] = {}
    for name, record in manifest["evidence_artifacts"].items():
        _, evidence[name] = read_bound(record, f"BootEdit coverage evidence {name}")
    inventory_record = manifest["evidence_artifacts"]["function_inventory"]
    inventory = evidence["function_inventory"]
    functions = inventory["functions"]
    if (
        inventory.get("source_sha256") != image_record["sha256"]
        or inventory.get("function_count") != inventory_record["entry_count"]
        or len(functions) != inventory_record["entry_count"]
    ):
        fail("BootEdit coverage inventory binding or count differs")
    ownership = evidence["function_ownership"]
    if ownership["image"]["sha256"] != image_record["sha256"]:
        fail("BootEdit ownership evidence is bound to another image")
    for name in ("semantic_symbols", "string_xrefs"):
        if evidence[name].get("image_sha256") != image_record["sha256"]:
            fail(f"BootEdit {name} evidence is bound to another image")

    source_bytes: dict[str, bytes] = {}
    for name, record in manifest["source_artifacts"].items():
        raw = (ROOT / record["path"]).read_bytes()
        if len(raw) != record["size"] or digest(raw) != record["sha256"]:
            fail(f"BootEdit readable-source artifact differs: {record['path']}")
        if record["kind"] not in {"source", "header", "test"}:
            fail(f"unexpected BootEdit source-artifact kind: {record['kind']}")
        source_bytes[name] = raw

    inventory_by_address = {address(item["address"]): item for item in functions}
    function_addresses = [address(item["address"]) for item in functions]
    if function_addresses != sorted(function_addresses):
        fail("BootEdit coverage inventory is not ordered")

    product_name = "product_application"
    product = ownership["categories"][product_name]
    product_addresses: set[int] = set()
    for item in product["ranges"]:
        start, end = address(item["start"]), address(item["end_exclusive"])
        product_addresses.update(value for value in function_addresses if start <= value < end)
    if (
        len(product_addresses) != product["inventory_entry_count"]
        or sum(inventory_by_address[value]["size_to_next_symbol"] for value in product_addresses)
        != product["size"]
    ):
        fail("BootEdit product ownership totals differ")

    product_roles = {
        address(item["address"]): item["role"]
        for item in ownership["product_application_entries"]
    }
    if set(product_roles) != product_addresses:
        fail("BootEdit product role evidence differs")
    semantic_by_address = {
        address(item["address"]): item["semantic_name"]
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
            fail(f"duplicate BootEdit covered entry: {entry_address:#x}")
        covered_addresses.add(entry_address)
        inventory_entry = inventory_by_address.get(entry_address)
        if inventory_entry is None:
            fail(f"covered BootEdit address is not an inventory entry: {entry_address:#x}")
        size = inventory_entry["size_to_next_symbol"]
        end = entry_address + size
        if (
            inventory_entry["name"].lower() != record["inventory_name"].lower()
            or address(record["end_exclusive"]) != end
            or record["size"] != size
        ):
            fail(f"BootEdit covered extent differs at {entry_address:#x}")
        payload = image[entry_address - base : end - base]
        if len(payload) != size or digest(payload) != record["sha256"]:
            fail(f"BootEdit covered bytes differ at {entry_address:#x}")
        if record["ownership_category"] != product_name or entry_address not in product_addresses:
            fail(f"BootEdit covered entry is outside product ownership: {entry_address:#x}")
        if record["coverage_kind"] != "readable_behavioral_model_for_exact_function_entry":
            fail(f"unexpected BootEdit coverage kind at {entry_address:#x}")
        semantic = semantic_by_address.get(entry_address)
        if semantic is not None and semantic != record["semantic_name"]:
            fail(f"BootEdit semantic mapping differs at {entry_address:#x}")
        for required_string in record["required_strings"]:
            if (entry_address, required_string) not in string_pairs:
                fail(f"BootEdit string evidence differs at {entry_address:#x}: {required_string}")

        model_artifacts = record["source_artifacts"]
        test_artifacts = record["test_artifacts"]
        if not model_artifacts or not test_artifacts:
            fail(f"BootEdit covered entry lacks source or tests: {entry_address:#x}")
        if any(manifest["source_artifacts"].get(item, {}).get("kind") == "test" for item in model_artifacts):
            fail(f"BootEdit model artifact is mislabeled as a test: {entry_address:#x}")
        if any(manifest["source_artifacts"].get(item, {}).get("kind") != "test" for item in test_artifacts):
            fail(f"BootEdit test artifact kind differs: {entry_address:#x}")
        artifact_names = model_artifacts + test_artifacts
        if any(item not in source_bytes for item in artifact_names):
            fail(f"BootEdit function references an unknown source artifact: {entry_address:#x}")
        referenced_artifacts.update(artifact_names)
        readable_model = b"\n".join(source_bytes[item] for item in model_artifacts).decode("utf-8")
        for symbol in record["symbols"]:
            if symbol not in readable_model:
                fail(f"BootEdit readable model lacks {symbol}: {entry_address:#x}")
        covered_bytes += size

    remaining_addresses: set[int] = set()
    remaining_bytes = 0
    for record in manifest["remaining_product_entries"]:
        entry_address = address(record["address"])
        if entry_address in remaining_addresses or entry_address in covered_addresses:
            fail(f"duplicate BootEdit remaining entry: {entry_address:#x}")
        remaining_addresses.add(entry_address)
        inventory_entry = inventory_by_address.get(entry_address)
        if inventory_entry is None or entry_address not in product_addresses:
            fail(f"BootEdit remaining entry is outside product ownership: {entry_address:#x}")
        size = inventory_entry["size_to_next_symbol"]
        end = entry_address + size
        payload = image[entry_address - base : end - base]
        if (
            address(record["end_exclusive"]) != end
            or record["size"] != size
            or len(payload) != size
            or digest(payload) != record["sha256"]
        ):
            fail(f"BootEdit remaining extent differs at {entry_address:#x}")
        if record["role"] != product_roles[entry_address] or not record.get("blocker"):
            fail(f"BootEdit remaining role/blocker differs at {entry_address:#x}")
        remaining_bytes += size

    if covered_addresses | remaining_addresses != product_addresses:
        fail("BootEdit covered and remaining entries do not exhaust product ownership")
    if referenced_artifacts != set(manifest["source_artifacts"]):
        fail("BootEdit source-coverage manifest has unreferenced artifacts")
    if 0x82053238 in covered_addresses or 0x82053238 in product_addresses:
        fail("BootEdit generic shared helper was counted as product source coverage")

    summary = manifest["summary"]
    expected_summary = {
        "product_function_count": product["inventory_entry_count"],
        "product_bytes": product["size"],
        "model_covered_function_count": len(covered_addresses),
        "model_covered_bytes": covered_bytes,
        "remaining_product_function_count": len(remaining_addresses),
        "remaining_product_bytes": remaining_bytes,
    }
    if summary != expected_summary:
        fail("BootEdit readable-source coverage summary differs")
    if covered_bytes + remaining_bytes != product["size"]:
        fail("BootEdit covered and remaining byte totals differ")

    print(
        "BootEdit Peer source coverage verified: "
        f"{len(covered_addresses)}/{product['inventory_entry_count']} product entries, "
        f"{covered_bytes}/{product['size']} bytes; "
        f"remaining {len(remaining_addresses)}/{remaining_bytes}"
    )


if __name__ == "__main__":
    main()
