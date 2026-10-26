#!/usr/bin/env python3
"""Verify the exhaustive, provenance-first UpdateCheck ownership partition."""

from __future__ import annotations

import hashlib
import json
import subprocess
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/UpdateCheck-function-ownership.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def read_bound_file(item: dict[str, object], label: str) -> bytes:
    path = ROOT / str(item["path"])
    if not path.is_file():
        fail(f"missing {label}: {path}")
    data = path.read_bytes()
    if digest(data) != item["sha256"]:
        fail(f"{label} hash differs: {path}")
    return data


def source_tree_digest(root: Path) -> tuple[int, int, str]:
    """Hash a source tree without depending on filesystem metadata."""

    files = sorted(
        (path for path in root.rglob("*") if path.is_file()),
        key=lambda path: path.relative_to(root).as_posix(),
    )
    hasher = hashlib.sha256()
    total_size = 0
    for path in files:
        relative = path.relative_to(root).as_posix().encode("utf-8")
        payload = path.read_bytes()
        hasher.update(len(relative).to_bytes(8, "big"))
        hasher.update(relative)
        hasher.update(len(payload).to_bytes(8, "big"))
        hasher.update(payload)
        total_size += len(payload)
    return len(files), total_size, hasher.hexdigest()


def normalized_ppc_fingerprint(payload: bytes) -> str:
    """Return the documented relocation-tolerant comparison fingerprint."""

    if len(payload) % 4:
        fail("PPC function extent is not word-aligned")
    normalized = bytearray()
    for offset in range(0, len(payload), 4):
        word = int.from_bytes(payload[offset : offset + 4], "big")
        opcode = word >> 26
        if opcode == 18:
            # Keep opcode, absolute-address bit, and link bit; clear LI.
            word &= 0xFC000003
        elif opcode in (14, 15):
            # Clear address-bearing ADDI/ADDIS immediates.
            word &= 0xFFFF0000
        normalized.extend(word.to_bytes(4, "big"))
    return digest(bytes(normalized))


def inventory_rows(
    inventory: dict[str, object], image: bytes, image_base: int
) -> list[tuple[int, int, str]]:
    rows: list[tuple[int, int, str]] = []
    for function in inventory["functions"]:
        address = int(function["address"], 0)
        size = int(function["size_to_next_symbol"])
        payload = image[address - image_base : address - image_base + size]
        if len(payload) != size:
            fail(f"function extent exceeds image at {address:#x}")
        rows.append((address, size, normalized_ppc_fingerprint(payload)))
    return rows


def verify_upstream_sources(ownership: dict[str, object]) -> None:
    for source_name, source in ownership["upstream_sources"].items():
        source_root = ROOT / source["submodule_path"]
        if not source_root.is_dir():
            fail(f"missing {source_name} source checkout")
        result = subprocess.run(
            ["git", "-C", str(source_root), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        )
        if result.stdout.strip().lower() != source["commit"]:
            fail(f"{source_name} checkout commit differs")

        for tree in source["trees"]:
            tree_root = source_root / tree["path"]
            actual = source_tree_digest(tree_root)
            expected = (
                int(tree["file_count"]),
                int(tree["size"]),
                tree["sha256"],
            )
            if actual != expected:
                fail(f"{source_name} source-tree hash differs: {tree['path']}")

        for file_info in source["signature_files"]:
            source_path = source_root / file_info["path"]
            payload = source_path.read_bytes()
            if digest(payload) != file_info["sha256"]:
                fail(f"{source_name} source hash differs: {file_info['path']}")
            text = payload.decode("utf-8")
            for needle in file_info["contains"]:
                if needle not in text:
                    fail(
                        f"{source_name} source signature missing from "
                        f"{file_info['path']}: {needle!r}"
                    )


def main() -> None:
    ownership = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if ownership.get("schema") != "bandwagon-updatecheck-function-ownership-v1":
        fail("unexpected UpdateCheck ownership schema")

    image_info = ownership["image"]
    image = read_bound_file(image_info, "UpdateCheck image")
    if len(image) != image_info["size"]:
        fail("UpdateCheck image size differs")
    image_base = int(image_info["base_address"], 0)

    inventory_info = ownership["function_inventory"]
    inventory_bytes = read_bound_file(inventory_info, "UpdateCheck function inventory")
    inventory = json.loads(inventory_bytes)
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("UpdateCheck inventory entry count differs")
    if inventory.get("source_sha256") != image_info["sha256"]:
        fail("UpdateCheck inventory is bound to a different image")

    evidence: dict[str, dict[str, object]] = ownership["evidence_files"]
    evidence_json: dict[str, dict[str, object]] = {}
    for role, item in evidence.items():
        evidence_json[role] = json.loads(read_bound_file(item, f"{role} evidence"))
    for role in ("semantic_symbols", "string_xrefs", "direct_callgraph"):
        if evidence_json[role].get("image_sha256") != image_info["sha256"]:
            fail(f"{role} evidence is bound to a different image")
    if evidence_json["declared_provenance"].get("image_sha256") != image_info["sha256"]:
        fail("declared provenance is bound to a different image")

    verify_upstream_sources(ownership)

    region = ownership["instruction_region"]
    region_start = int(region["start"], 0)
    region_end = int(region["end_exclusive"], 0)
    region_bytes = image[region_start - image_base : region_end - image_base]
    if region_start != int(inventory["base_address"], 0):
        fail("instruction-region start differs from inventory")
    if region_end != region_start + int(inventory["section_end_offset"], 0):
        fail("instruction-region end differs from inventory")
    if region_end - region_start != region["size"]:
        fail("instruction-region size metadata differs")
    if len(region_bytes) != region["size"] or digest(region_bytes) != region["sha256"]:
        fail("UpdateCheck instruction-region bytes differ")

    required_categories = {
        "product_application_or_glue",
        "bearssl_0_6",
        "jsoncpp_00_11_0",
        "platform_toolchain",
        "unknown",
    }
    categories = ownership["categories"]
    if set(categories) != required_categories:
        fail("UpdateCheck ownership category set differs")

    function_addresses = [int(function["address"], 0) for function in functions]
    if function_addresses != sorted(set(function_addresses)):
        fail("UpdateCheck function entries are not unique and sorted")
    function_address_set = set(function_addresses)
    allowed_boundaries = function_address_set | {region_end}

    flat_ranges: list[tuple[int, int, str, dict[str, object]]] = []
    entry_owners: dict[int, str] = {}
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
            payload = image[start - image_base : end - image_base]
            if len(payload) != item["size"] or end - start != item["size"]:
                fail(f"invalid {category_name} range size at {start:#x}")
            if digest(payload) != item["sha256"]:
                fail(f"{category_name} range bytes changed at {start:#x}")

            addresses = [address for address in function_addresses if start <= address < end]
            if len(addresses) != item["inventory_entry_count"]:
                fail(f"{category_name} inventory count changed at {start:#x}")
            for address in addresses:
                if address in entry_owners:
                    fail(f"function {address:#x} has multiple ownership categories")
                entry_owners[address] = category_name
            category_ranges.append((start, end, item))
            flat_ranges.append((start, end, category_name, item))

        category_ranges.sort()
        concatenated = b"".join(
            image[start - image_base : end - image_base]
            for start, end, _ in category_ranges
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
        fail("ownership partition does not own every function entry exactly once")

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

    symbols = evidence_json["semantic_symbols"]["symbols"]
    symbols_by_address = {int(item["address"], 0): item for item in symbols}
    anchor_requirements = ownership["semantic_anchor_requirements"]
    for category_name, requirement in anchor_requirements.items():
        if "ownership_label" in requirement:
            addresses = {
                int(item["address"], 0)
                for item in symbols
                if item.get("ownership") == requirement["ownership_label"]
            }
        else:
            addresses = {int(address, 0) for address in requirement["addresses"]}
        if len(addresses) != requirement["expected_count"]:
            fail(f"{category_name} semantic-anchor count differs")
        for address in addresses:
            if address not in symbols_by_address:
                fail(f"missing semantic anchor at {address:#x}")
            if entry_owners.get(address) != category_name:
                fail(f"semantic anchor {address:#x} left {category_name}")

    coverage = evidence_json["peer_source_coverage"]
    if coverage["image"]["sha256"] != image_info["sha256"]:
        fail("Peer source coverage is bound to a different image")
    covered_addresses: set[int] = set()
    for item in coverage["functions"]:
        address = int(item["address"], 0)
        size = int(item["size"])
        payload = image[address - image_base : address - image_base + size]
        if digest(payload) != item["sha256"]:
            fail(f"Peer source-coverage bytes changed at {address:#x}")
        if entry_owners.get(address) != "product_application_or_glue":
            fail(f"covered product routine {address:#x} left the product category")
        covered_addresses.add(address)
    product_addresses = {
        address
        for address, owner in entry_owners.items()
        if owner == "product_application_or_glue"
    }
    if not covered_addresses.issubset(product_addresses):
        fail("strict Peer source coverage left the product category")

    platform = ownership["platform_requirements"]
    entry_address = int(platform["entry_point"]["address"], 0)
    entry = next(
        (function for function in functions if int(function["address"], 0) == entry_address),
        None,
    )
    if entry is None or entry["name"] != platform["entry_point"]["name"]:
        fail("UpdateCheck entry-point identity differs")
    if entry_owners[entry_address] != "platform_toolchain":
        fail("UpdateCheck entry point left platform/toolchain")
    named_imports = [
        function
        for function in functions
        if function["name"] != "_start" and not function["name"].startswith("fn_")
    ]
    if len(named_imports) != platform["named_import_thunk_count"]:
        fail("named import-thunk count differs")
    for function in named_imports:
        if entry_owners[int(function["address"], 0)] != "platform_toolchain":
            fail(f"import thunk left platform/toolchain: {function['name']}")
    thunk_range = platform["import_thunk_range"]
    thunk_start = int(thunk_range["start"], 0)
    thunk_end = int(thunk_range["end_exclusive"], 0)
    named_import_addresses = [int(function["address"], 0) for function in named_imports]
    if min(named_import_addresses) != thunk_start:
        fail("named import-thunk range start differs")
    final_named_import = named_imports[-1]
    if (
        int(final_named_import["address"], 0)
        + int(final_named_import["size_to_next_symbol"])
        != thunk_end
    ):
        fail("named import-thunk range end differs")

    comparison = ownership["cross_component_comparison"]
    comparison_image = read_bound_file(comparison["image"], "SettingsApp comparison image")
    if len(comparison_image) != comparison["image"]["size"]:
        fail("SettingsApp comparison image size differs")
    comparison_inventory = json.loads(
        read_bound_file(comparison["function_inventory"], "SettingsApp function inventory")
    )
    if (
        len(comparison_inventory["functions"])
        != comparison["function_inventory"]["entry_count"]
    ):
        fail("SettingsApp comparison inventory entry count differs")
    if comparison_inventory.get("source_sha256") != comparison["image"]["sha256"]:
        fail("SettingsApp inventory is bound to a different image")
    comparison_ownership = json.loads(
        read_bound_file(comparison["ownership_partition"], "SettingsApp ownership partition")
    )
    bridge_boundaries = comparison["refined_bridge_boundaries"]
    for name, boundary in bridge_boundaries.items():
        start = int(boundary["start"], 0)
        end = int(boundary["end_exclusive"], 0)
        if name.startswith("settings_"):
            range_list = comparison_ownership["categories"][boundary["category"]][
                "ranges"
            ]
            if not any(
                int(item["start"], 0) == start
                and int(item["end_exclusive"], 0) == end
                for item in range_list
            ):
                fail(f"SettingsApp refined bridge boundary differs: {name}")
        else:
            if not any(
                range_start == start
                and range_end == end
                and category_name == boundary["category"]
                for range_start, range_end, category_name, _ in flat_ranges
            ):
                fail(f"UpdateCheck refined bridge boundary differs: {name}")
    comparison_rows = inventory_rows(
        comparison_inventory,
        comparison_image,
        int(comparison["image"]["base_address"], 0),
    )
    comparison_index: dict[tuple[int, str], list[int]] = defaultdict(list)
    for address, size, fingerprint in comparison_rows:
        comparison_index[(size, fingerprint)].append(address)

    update_rows = inventory_rows(inventory, image, image_base)
    update_rows_by_address = {row[0]: row for row in update_rows}
    comparison_rows_by_address = {row[0]: row for row in comparison_rows}
    comparison_entry_owners: dict[int, str] = {}
    for category_name, category in comparison_ownership["categories"].items():
        for item in category["ranges"]:
            start = int(item["start"], 0)
            end = int(item["end_exclusive"], 0)
            for address, _, _ in comparison_rows:
                if start <= address < end:
                    comparison_entry_owners[address] = category_name

    comparison_totals = {
        "matching_entry_count": 0,
        "matching_size": 0,
        "unique_matching_entry_count": 0,
        "unique_matching_size": 0,
    }
    for category_name, expected in comparison["category_summary"].items():
        rows = [
            row for row in update_rows if entry_owners[row[0]] == category_name
        ]
        matches = [row for row in rows if comparison_index[(row[1], row[2])]]
        unique = [
            row for row in rows if len(comparison_index[(row[1], row[2])]) == 1
        ]
        actual = {
            "matching_entry_count": len(matches),
            "matching_size": sum(row[1] for row in matches),
            "unique_matching_entry_count": len(unique),
            "unique_matching_size": sum(row[1] for row in unique),
        }
        if actual != expected:
            fail(f"SettingsApp normalized comparison differs for {category_name}")
        for key, value in actual.items():
            comparison_totals[key] += value
    if comparison_totals != comparison["totals"]:
        fail("SettingsApp normalized comparison totals differ")

    adapter = comparison["certificate_adapter"]
    adapter_start = int(adapter["update_start"], 0)
    adapter_end = int(adapter["update_end_exclusive"], 0)
    settings_adapter_start = int(adapter["settings_start"], 0)
    settings_adapter_end = int(adapter["settings_end_exclusive"], 0)
    adapter_rows = [
        row for row in update_rows if adapter_start <= row[0] < adapter_end
    ]
    settings_adapter_rows = [
        row
        for row in comparison_rows
        if settings_adapter_start <= row[0] < settings_adapter_end
    ]
    if len(adapter_rows) != adapter["inventory_entry_count"]:
        fail("UpdateCheck certificate-adapter entry count differs")
    if len(settings_adapter_rows) != adapter["inventory_entry_count"]:
        fail("SettingsApp certificate-adapter entry count differs")
    if sum(row[1] for row in adapter_rows) != adapter["size"]:
        fail("UpdateCheck certificate-adapter byte count differs")
    if sum(row[1] for row in settings_adapter_rows) != adapter["size"]:
        fail("SettingsApp certificate-adapter byte count differs")
    for address, _, _ in adapter_rows:
        if entry_owners[address] != "product_application_or_glue":
            fail(f"certificate adapter {address:#x} left product/mixed")
    for address, _, _ in settings_adapter_rows:
        if comparison_entry_owners.get(address) != "peer_product_application_glue":
            fail(f"SettingsApp certificate adapter {address:#x} left product/mixed")
    for pair in adapter["exact_normalized_pairs"]:
        update_address = int(pair["update_address"], 0)
        settings_address = int(pair["settings_address"], 0)
        update_row = update_rows_by_address.get(update_address)
        settings_row = comparison_rows_by_address.get(settings_address)
        if update_row is None or settings_row is None:
            fail("certificate-adapter comparison entry is missing")
        if update_row[1] != pair["size"] or settings_row[1] != pair["size"]:
            fail(f"certificate-adapter size differs at {update_address:#x}")
        if update_row[2] != pair["normalized_sha256"]:
            fail(f"certificate-adapter fingerprint differs at {update_address:#x}")
        if settings_row[2] != pair["normalized_sha256"]:
            fail(
                f"SettingsApp certificate-adapter fingerprint differs at "
                f"{settings_address:#x}"
            )

    runtime = comparison["msvc_runtime_island"]
    runtime_start = int(runtime["update_start"], 0)
    runtime_end = int(runtime["update_end_exclusive"], 0)
    settings_runtime_start = int(runtime["settings_start"], 0)
    settings_runtime_end = int(runtime["settings_end_exclusive"], 0)
    runtime_rows = [
        row for row in update_rows if runtime_start <= row[0] < runtime_end
    ]
    settings_runtime_rows = [
        row
        for row in comparison_rows
        if settings_runtime_start <= row[0] < settings_runtime_end
    ]
    if len(runtime_rows) != runtime["inventory_entry_count"]:
        fail("UpdateCheck MSVC runtime-island entry count differs")
    if len(settings_runtime_rows) != runtime["inventory_entry_count"]:
        fail("SettingsApp MSVC runtime-island entry count differs")
    if sum(row[1] for row in runtime_rows) != runtime["size"]:
        fail("UpdateCheck MSVC runtime-island byte count differs")
    if sum(row[1] for row in settings_runtime_rows) != runtime["size"]:
        fail("SettingsApp MSVC runtime-island byte count differs")
    for address, _, _ in runtime_rows:
        if entry_owners[address] != "platform_toolchain":
            fail(f"MSVC runtime function {address:#x} left platform/toolchain")
    for address, _, _ in settings_runtime_rows:
        if comparison_entry_owners.get(address) != "platform_toolchain_graphics":
            fail(f"SettingsApp MSVC runtime function {address:#x} left platform")
    comparison_base = int(comparison["image"]["base_address"], 0)
    settings_runtime_payload = comparison_image[
        settings_runtime_start - comparison_base : settings_runtime_end - comparison_base
    ]
    if digest(settings_runtime_payload) != runtime["settings_sha256"]:
        fail("SettingsApp MSVC runtime-island bytes differ")
    position_matches = [
        update_row
        for update_row, settings_row in zip(runtime_rows, settings_runtime_rows)
        if update_row[1:] == settings_row[1:]
    ]
    if len(position_matches) != runtime["position_matched_entry_count"]:
        fail("MSVC runtime-island positional match count differs")
    if sum(row[1] for row in position_matches) != runtime["position_matched_size"]:
        fail("MSVC runtime-island positional match bytes differ")
    string_references = {
        (int(item["function_address"], 0), item["string"])
        for item in evidence_json["string_xrefs"]["references"]
    }
    for anchor in runtime["diagnostic_anchors"]:
        expected = (int(anchor["update_function"], 0), anchor["string"])
        if expected not in string_references:
            fail(f"MSVC runtime diagnostic anchor differs: {anchor['string']}")

    bearssl_comparison = comparison["bearssl_block"]
    settings_addresses = [row[0] for row in comparison_rows]
    settings_start = int(bearssl_comparison["start"], 0)
    settings_end = int(bearssl_comparison["end_exclusive"], 0)
    settings_payload = comparison_image[
        settings_start - int(comparison["image"]["base_address"], 0) :
        settings_end - int(comparison["image"]["base_address"], 0)
    ]
    if len(settings_payload) != bearssl_comparison["size"]:
        fail("SettingsApp BearSSL comparison-block size differs")
    if digest(settings_payload) != bearssl_comparison["sha256"]:
        fail("SettingsApp BearSSL comparison-block hash differs")
    settings_count = sum(
        settings_start <= address < settings_end for address in settings_addresses
    )
    if settings_count != bearssl_comparison["inventory_entry_count"]:
        fail("SettingsApp BearSSL comparison-block function count differs")
    if settings_count != categories["bearssl_0_6"]["inventory_entry_count"]:
        fail("cross-component BearSSL block entry counts no longer agree")

    json_helpers = comparison["jsoncpp_exception_helpers"]
    helper_start = int(json_helpers["update_start"], 0)
    helper_end = int(json_helpers["update_end_exclusive"], 0)
    helper_rows = [row for row in update_rows if helper_start <= row[0] < helper_end]
    if len(helper_rows) != json_helpers["inventory_entry_count"]:
        fail("JsonCpp exception-helper entry count differs")
    if sum(row[1] for row in helper_rows) != json_helpers["size"]:
        fail("JsonCpp exception-helper byte count differs")
    for address, _, _ in helper_rows:
        if entry_owners[address] != "jsoncpp_00_11_0":
            fail(f"JsonCpp exception helper {address:#x} left JsonCpp")
    for pair in json_helpers["exact_normalized_pairs"]:
        update_address = int(pair["update_address"], 0)
        settings_address = int(pair["settings_address"], 0)
        update_row = update_rows_by_address.get(update_address)
        settings_row = comparison_rows_by_address.get(settings_address)
        if update_row is None or settings_row is None:
            fail("JsonCpp exception-helper comparison entry is missing")
        if update_row[1] != pair["size"] or settings_row[1] != pair["size"]:
            fail(f"JsonCpp exception-helper size differs at {update_address:#x}")
        if update_row[2] != pair["normalized_sha256"]:
            fail(f"JsonCpp exception-helper fingerprint differs at {update_address:#x}")
        if settings_row[2] != pair["normalized_sha256"]:
            fail(
                f"SettingsApp JsonCpp exception-helper fingerprint differs at "
                f"{settings_address:#x}"
            )

    totals = ownership["totals"]
    if total_entries != totals["inventory_entry_count"] or total_entries != len(functions):
        fail("UpdateCheck ownership total entry count differs")
    if total_bytes != totals["instruction_size"] or total_bytes != len(region_bytes):
        fail("UpdateCheck ownership total byte count differs")
    if totals["category_count"] != len(categories):
        fail("UpdateCheck ownership category count differs")
    known_entries = total_entries - categories["unknown"]["inventory_entry_count"]
    known_bytes = total_bytes - categories["unknown"]["size"]
    if known_entries != totals["classified_entry_count"]:
        fail("UpdateCheck classified entry total differs")
    if known_bytes != totals["classified_size"]:
        fail("UpdateCheck classified byte total differs")
    upstream_entries = (
        categories["bearssl_0_6"]["inventory_entry_count"]
        + categories["jsoncpp_00_11_0"]["inventory_entry_count"]
    )
    upstream_bytes = categories["bearssl_0_6"]["size"] + categories[
        "jsoncpp_00_11_0"
    ]["size"]
    if upstream_entries != totals["upstream_library_entry_count"]:
        fail("UpdateCheck upstream-library entry total differs")
    if upstream_bytes != totals["upstream_library_size"]:
        fail("UpdateCheck upstream-library byte total differs")

    refinement = ownership["unknown_refinement"]
    if refinement["after"]["inventory_entry_count"] != categories["unknown"][
        "inventory_entry_count"
    ]:
        fail("unknown-refinement final entry count differs")
    if refinement["after"]["size"] != categories["unknown"]["size"]:
        fail("unknown-refinement final byte count differs")
    moved_entries = sum(
        refinement[key]["inventory_entry_count"]
        for key in (
            "reassigned_to_platform_toolchain",
            "reassigned_to_jsoncpp_00_11_0",
            "reassigned_to_product_application_or_glue",
            "reassigned_to_bearssl_0_6",
        )
    )
    moved_bytes = sum(
        refinement[key]["size"]
        for key in (
            "reassigned_to_platform_toolchain",
            "reassigned_to_jsoncpp_00_11_0",
            "reassigned_to_product_application_or_glue",
            "reassigned_to_bearssl_0_6",
        )
    )
    if (
        refinement["before"]["inventory_entry_count"]
        != refinement["after"]["inventory_entry_count"] + moved_entries
    ):
        fail("unknown-refinement entry delta differs")
    if refinement["before"]["size"] != refinement["after"]["size"] + moved_bytes:
        fail("unknown-refinement byte delta differs")

    summary = ", ".join(
        f"{name}={category['inventory_entry_count']}/{category['size']}"
        for name, category in categories.items()
    )
    print(
        "UpdateCheck function ownership verified: "
        f"{total_entries} entries, {total_bytes} instruction bytes; {summary}"
    )


if __name__ == "__main__":
    main()
