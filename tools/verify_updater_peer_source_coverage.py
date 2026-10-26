#!/usr/bin/env python3
"""Verify exhaustive, byte-bound readable-source coverage for Updater code."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/Updater-peer-source-coverage.json"
EXPECTED_SCHEMA = "bandwagon-updater-peer-source-coverage-v1"


def fail(message: str) -> None:
    raise SystemExit(message)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def checked_file(item: dict, label: str) -> bytes:
    path = ROOT / item["path"]
    if not path.is_file():
        fail(f"missing {label}: {item['path']}")
    data = path.read_bytes()
    if digest(data) != item["sha256"]:
        fail(f"{label} hash changed: {item['path']}")
    return data


def main() -> None:
    manifest = read_json(MANIFEST)
    if manifest.get("schema") != EXPECTED_SCHEMA:
        fail("unexpected Updater source-coverage schema")
    if manifest.get("status") != "exhaustive_product_readable_source_coverage":
        fail("Updater source coverage is not marked exhaustive")

    image_info = manifest["image"]
    image = checked_file(image_info, "Updater image")
    if len(image) != image_info["size"]:
        fail("Updater image size changed")
    base = int(image_info["base_address"], 0)

    inventory_info = manifest["function_inventory"]
    inventory_data = checked_file(inventory_info, "Updater function inventory")
    inventory = json.loads(inventory_data)
    if len(inventory["functions"]) != inventory_info["entry_count"]:
        fail("Updater function inventory count changed")
    inventory_by_address = {
        int(item["address"], 0): item for item in inventory["functions"]
    }
    if len(inventory_by_address) != len(inventory["functions"]):
        fail("Updater function inventory contains duplicate addresses")

    ownership_info = manifest["ownership_partition"]
    ownership_data = checked_file(ownership_info, "Updater ownership partition")
    ownership = json.loads(ownership_data)
    category_name = ownership_info["category"]
    category = ownership["categories"].get(category_name)
    if category is None:
        fail("Updater product ownership category is missing")
    for field in ("inventory_entry_count", "size", "concatenated_sha256"):
        expected_field = "entry_count" if field == "inventory_entry_count" else field
        if category[field] != ownership_info[expected_field]:
            fail(f"Updater product ownership {field} changed")
    if ownership["function_inventory"]["sha256"] != inventory_info["sha256"]:
        fail("ownership and source coverage bind different function inventories")
    if ownership["image"]["sha256"] != image_info["sha256"]:
        fail("ownership and source coverage bind different Updater images")

    for evidence in manifest["evidence_inputs"]:
        checked_file(evidence, "Updater coverage evidence")

    expected = {
        int(item["address"], 0): item
        for item in ownership["product_application_entries"]
    }
    if len(expected) != ownership_info["entry_count"]:
        fail("Updater product entry list count changed")

    functions = manifest["functions"]
    covered = {int(item["address"], 0): item for item in functions}
    if len(covered) != len(functions):
        fail("Updater source coverage contains duplicate addresses")
    if set(covered) != set(expected):
        missing = sorted(set(expected) - set(covered))
        extra = sorted(set(covered) - set(expected))
        fail(
            "Updater product source coverage is not exhaustive: "
            f"missing={[hex(x) for x in missing]}, extra={[hex(x) for x in extra]}"
        )

    source_data: dict[str, str] = {}
    for source in manifest["source_files"]:
        path = source["path"]
        if path in source_data:
            fail(f"duplicate Updater source file: {path}")
        if not path.startswith("reconstruction/updater/"):
            fail(f"source escapes Updater reconstruction scope: {path}")
        source_data[path] = checked_file(source, "Updater readable source").decode(
            "utf-8"
        )

    test_paths: set[str] = set()
    for test in manifest["tests"]:
        path = test["path"]
        if path in test_paths:
            fail(f"duplicate Updater test binding: {path}")
        test_paths.add(path)
        checked_file(test, "Updater coverage test")
    required_tests = {
        "reconstruction/updater/archive_wrapper_test.cpp",
        "reconstruction/updater/bootstrap_record_test.cpp",
        "reconstruction/updater/parent_directories_test.cpp",
        "reconstruction/updater/phase_two_list_test.cpp",
        "reconstruction/updater/softmod_manifest_test.cpp",
        "reconstruction/updater/updater_runtime_test.cpp",
        "tools/test_updater_reconstruction.sh",
    }
    if test_paths != required_tests:
        fail("Updater test bindings are incomplete or contain unexpected files")

    referenced_sources: set[str] = set()
    concatenated = bytearray()
    covered_size = 0
    for address in sorted(covered):
        function = covered[address]
        owned = expected[address]
        if function["semantic_name"] != owned["semantic_name"]:
            fail(f"semantic name drift at {address:#x}")
        inventory_function = inventory_by_address.get(address)
        if inventory_function is None:
            fail(f"covered address is absent from inventory: {address:#x}")
        size = inventory_function["size_to_next_symbol"]
        if function["size"] != size:
            fail(f"function size drift at {address:#x}")
        offset = address - base
        payload = image[offset : offset + size]
        if offset < 0 or len(payload) != size:
            fail(f"truncated Updater product function at {address:#x}")
        if digest(payload) != function["sha256"]:
            fail(f"Updater product function bytes changed at {address:#x}")
        concatenated.extend(payload)
        covered_size += size

        paths = [function["source"], *function.get("supporting_sources", [])]
        if len(paths) != len(set(paths)):
            fail(f"duplicate source binding at {address:#x}")
        for path in paths:
            if path not in source_data:
                fail(f"unregistered source binding at {address:#x}: {path}")
            referenced_sources.add(path)
        text = "\n".join(source_data[path] for path in paths)
        symbols = function.get("symbols", [])
        if not symbols:
            fail(f"no readable-source symbols at {address:#x}")
        for symbol in symbols:
            if symbol not in text:
                fail(f"missing source symbol at {address:#x}: {symbol}")
        if function.get("fidelity") not in {
            "exact_behavior",
            "exact_algorithm_reconstruction",
            "behavioral_reconstruction",
            "safe_behavioral_reconstruction",
            "idiomatic_raii_reconstruction",
            "idiomatic_container_reconstruction",
        }:
            fail(f"missing or invalid fidelity statement at {address:#x}")
        if not function.get("evidence"):
            fail(f"missing evidence statement at {address:#x}")

    if referenced_sources != set(source_data):
        fail("Updater source file inventory contains an unreferenced file")
    if digest(bytes(concatenated)) != ownership_info["concatenated_sha256"]:
        fail("concatenated Updater product bytes changed")

    summary = manifest["coverage_summary"]
    expected_summary = {
        "owned_entry_count": ownership_info["entry_count"],
        "owned_size": ownership_info["size"],
        "covered_entry_count": len(covered),
        "covered_size": covered_size,
        "remaining_entry_count": ownership_info["entry_count"] - len(covered),
        "remaining_size": ownership_info["size"] - covered_size,
    }
    if summary != expected_summary:
        fail("Updater source coverage summary is stale")
    if summary["remaining_entry_count"] or summary["remaining_size"]:
        fail("Updater product source coverage still has a remainder")

    callgraph = read_json(ROOT / "analysis/xex/Updater-callgraph.json")
    edges = {
        (int(edge["caller_address"], 0), int(edge["target_address"], 0))
        for edge in callgraph["edges"]
    }
    required_edges = {
        (0x98005140, 0x9800BEE0),
        (0x98005280, 0x9800C450),
        (0x98005EB0, 0x98005280),
        (0x980062C8, 0x98005EB0),
        (0x980064B8, 0x9801A730),
        (0x98006990, 0x98007DA8),
        (0x98006D40, 0x9801A760),
        (0x98007758, 0x980062C8),
        (0x98007758, 0x98006990),
        (0x98007758, 0x98006D40),
        (0x98007758, 0x98007BC0),
        (0x98007758, 0x98007680),
        (0x98007758, 0x98007EF0),
        (0x98007758, 0x98006740),
        (0x98007758, 0x98006F60),
    }
    if not required_edges.issubset(edges):
        fail("Updater product call-chain evidence changed")

    string_xrefs = read_json(ROOT / "analysis/xex/Updater-string-xrefs.json")
    strings = {
        (int(item["function_address"], 0), item["string"])
        for item in string_xrefs["references"]
    }
    required_strings = {
        (0x98005EB0, "softmod.manifest"),
        (0x98006D40, r"\Device\Harddisk0\PhysicalDisk"),
        (0x98006D40, "Hdd/PeerPressureExploit-Stage2.bin"),
        (0x98006F60, r"PPROOT:\update.phase2"),
        (0x98007758, "PeerPressure/"),
        (0x98007758, "PPROOT:\\"),
        (0x98007758, "Finalizing..."),
    }
    if not required_strings.issubset(strings):
        fail("Updater product string evidence changed")

    script = (ROOT / "tools/test_updater_reconstruction.sh").read_text(
        encoding="utf-8"
    )
    for token in (
        "verify_updater_peer_source_coverage.py",
        "updater_runtime.cpp",
        "updater_runtime_test.cpp",
    ):
        if token not in script:
            fail(f"Updater test runner is missing {token}")

    print(
        "Updater Peer source coverage verified: "
        f"{len(covered)}/{ownership_info['entry_count']} product entries, "
        f"{covered_size}/{ownership_info['size']} exact bytes; remaining=0/0"
    )


if __name__ == "__main__":
    main()
