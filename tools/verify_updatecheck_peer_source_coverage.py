#!/usr/bin/env python3
"""Verify exact UpdateCheck product/mixed readable-source coverage."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/UpdateCheck-peer-source-coverage.json"
EXPECTED_SCHEMA = "bandwagon-updatecheck-peer-source-coverage-v3"


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
    if manifest.get("schema") != EXPECTED_SCHEMA:
        fail("unexpected UpdateCheck source-coverage schema")
    if manifest.get("status") != "exhaustive_product_readable_source_coverage":
        fail("UpdateCheck source coverage is not marked exhaustive")

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

    evidence_inputs: dict[str, dict[str, object]] = {}
    for item in manifest["evidence_inputs"]:
        path = str(item["path"])
        evidence_inputs[path] = json.loads(
            read_bound_file(item, "UpdateCheck source-coverage evidence")
        )
    required_evidence = {
        "analysis/xex/UpdateCheck-callgraph.json",
        "analysis/xex/UpdateCheck-string-xrefs.json",
    }
    if set(evidence_inputs) != required_evidence:
        fail("UpdateCheck source-coverage evidence set differs")

    source_text: dict[str, str] = {}
    for item in manifest["source_files"]:
        payload = read_bound_file(item, "UpdateCheck readable source")
        source_text[item["path"]] = payload.decode("utf-8")
    test_paths: set[str] = set()
    for item in manifest["tests"]:
        path = str(item["path"])
        if path in test_paths:
            fail(f"duplicate UpdateCheck source test: {path}")
        test_paths.add(path)
        read_bound_file(item, "UpdateCheck source test")
    required_tests = {
        "reconstruction/update_check/release_version_test.cpp",
        "reconstruction/update_check/latest_release_test.cpp",
        "reconstruction/update_check/http_response_test.cpp",
        "reconstruction/update_check/certificate_store_test.cpp",
        "reconstruction/update_check/network_lifecycle_test.cpp",
        "reconstruction/update_check/https_request_test.cpp",
    }
    if test_paths != required_tests:
        fail("UpdateCheck source-test bindings are incomplete or unexpected")

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
                paths = [record["source"], *record.get("supporting_sources", [])]
                if len(paths) != len(set(paths)):
                    fail(f"duplicate source binding at {address:#x}")
                for path in paths:
                    if path not in source_text:
                        fail(f"unbound source file for covered function {address:#x}")
                bound_text = "\n".join(source_text[path] for path in paths)
                for symbol in record["symbols"]:
                    if symbol not in bound_text:
                        fail(f"missing UpdateCheck readable-source symbol: {symbol}")
                if address in {0x91A0CD28, 0x91A0D1E8, 0x91A0D328}:
                    if record.get("fidelity") not in {
                        "exact_behavior",
                        "behavioral_reconstruction",
                    }:
                        fail(f"missing HTTPS fidelity statement at {address:#x}")
                    if not record.get("evidence"):
                        fail(f"missing HTTPS evidence statement at {address:#x}")
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
    if remaining_addresses or remaining_bytes:
        fail("UpdateCheck product source coverage still has a remainder")

    callgraph = evidence_inputs["analysis/xex/UpdateCheck-callgraph.json"]
    edges = {
        (int(edge["caller_address"], 0), int(edge["target_address"], 0))
        for edge in callgraph["edges"]
    }
    required_edges = {
        (0x91A0CD28, 0x91A40F9C),
        (0x91A0CD28, 0x91A40F4C),
        (0x91A0CD28, 0x91A40F8C),
        (0x91A0CD28, 0x91A40F7C),
        (0x91A0CD28, 0x91A40F6C),
        (0x91A0CD28, 0x91A40F5C),
        (0x91A0D328, 0x91A40FAC),
        (0x91A0D328, 0x91A0CD28),
        (0x91A0D328, 0x91A0D1E8),
        (0x91A0D328, 0x91A0D328),
    }
    if not required_edges.issubset(edges):
        fail("UpdateCheck HTTPS call-chain evidence differs")

    string_xrefs = evidence_inputs["analysis/xex/UpdateCheck-string-xrefs.json"]
    strings = {
        (int(item["function_address"], 0), item["string"])
        for item in string_xrefs["references"]
    }
    required_xrefs = {
        (0x91A0D1E8, "Accept:"),
        (0x91A0D328, "location:"),
    }
    if not required_xrefs.issubset(strings):
        fail("UpdateCheck HTTPS string-xref evidence differs")

    required_binary_strings = {
        0x91A00720: "XHttps::HttpsClient::OpenConnection: DNS time %d ms\n",
        0x91A00758: "XHttps::HttpsClient::OpenConnection: connect failed 0x%08x\n",
        0x91A00798: "XHttps::HttpsClient::OpenConnection: failed to connect to '%s'\n",
        0x91A007D8: "XHttps::HttpsClient::OpenConnection: XNetDnsLookup failed 0x%08x\n",
        0x91A0081C: "XHttps::HttpsClient::OpenConnection: setsockopt failed 0x%08x\n",
        0x91A0085C: "XHttps::HttpsClient::OpenConnection: socket failed 0x%08x\n",
        0x91A008A0: "Accept: */*\r\n",
        0x91A008B0: "Accept:",
        0x91A008BC: "Connection: close\r\n",
        0x91A008D0: "Connection: keep-alive\r\n",
        0x91A008EC: "\r\nUser-Agent: Xbox360XHttps/1.0\r\n",
        0x91A00910: " HTTP/1.1\r\nHost: ",
        0x91A00928: "location:",
        0x91A00934: "XHttps::HttpsClient::SubmitGETRequest: socket time %d ms\n",
        0x91A00970: "%s\n",
        0x91A00974: "GET",
        0x91A00978: "/",
        0x91A00980: "XHttps::HttpsClient::SubmitGETRequest: XHttpCrackUrl failed 0x%08x\n",
    }
    for address, expected in required_binary_strings.items():
        offset = address - image_base
        end = image.find(b"\0", offset)
        if end < 0 or image[offset:end].decode("ascii") != expected:
            fail(f"UpdateCheck binary string differs at {address:#x}")

    runner = (ROOT / "tools/test_updatecheck_reconstruction.sh").read_text(
        encoding="utf-8"
    )
    for token in (
        "verify_updatecheck_peer_source_coverage.py",
        "verify_updatecheck_platform_wrapper_matches.py",
        "network_lifecycle.cpp",
        "network_lifecycle_test.cpp",
        "https_request.cpp",
        "https_request_test.cpp",
    ):
        if token not in runner:
            fail(f"UpdateCheck test runner is missing {token}")

    print(
        "UpdateCheck product source coverage verified: "
        f"{len(covered_addresses)}/{category['inventory_entry_count']} entries, "
        f"{covered_bytes}/{category['size']} exact bytes covered; "
        f"{len(remaining_addresses)} entries, {remaining_bytes} bytes remain"
    )


if __name__ == "__main__":
    main()
