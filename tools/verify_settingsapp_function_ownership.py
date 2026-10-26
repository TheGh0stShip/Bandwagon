#!/usr/bin/env python3
"""Verify the exhaustive, provenance-first SettingsApp .text partition."""

from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/SettingsApp-function-ownership.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def parse_address(value: object) -> int:
    if not isinstance(value, str):
        fail(f"expected hexadecimal address string, got {value!r}")
    return int(value, 0)


def read_bound_json(record: dict[str, object], label: str) -> tuple[bytes, dict[str, object]]:
    path = ROOT / str(record["path"])
    raw = path.read_bytes()
    if digest(raw) != record["sha256"]:
        fail(f"{label} hash differs: {record['path']}")
    document = json.loads(raw)
    expected_schema = record.get("schema")
    if expected_schema is not None and document.get("schema") != expected_schema:
        fail(f"{label} schema differs: {record['path']}")
    return raw, document


def git_output(*arguments: str) -> str:
    result = subprocess.run(
        ["git", *arguments],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        fail(f"git {' '.join(arguments)} failed: {result.stderr.strip()}")
    return result.stdout.strip()


def git_has_tracked_content_changes(source_root: Path) -> bool:
    """Ignore checkout-only CRLF differences while detecting real edits."""

    result = subprocess.run(
        [
            "git",
            "-C",
            str(source_root),
            "diff",
            "--quiet",
            "--ignore-cr-at-eol",
            "HEAD",
            "--",
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode not in (0, 1):
        fail(f"git diff failed for {source_root}: {result.stderr.strip()}")
    return result.returncode == 1


def normalized_ppc_words(body: bytes) -> list[int]:
    """Mask link-layout operands while retaining opcode/register structure.

    This is deliberately narrow: direct branch displacements and image-pointer
    materialization are ignored, while all other instruction bits remain.
    """

    if len(body) % 4:
        fail("PowerPC function extent is not four-byte aligned")
    words = [int.from_bytes(body[offset : offset + 4], "big") for offset in range(0, len(body), 4)]
    pointer_registers: dict[int, int] = {}
    normalized: list[int] = []
    for index, word in enumerate(words):
        opcode = word >> 26
        value = word
        if opcode == 18:  # b / bl: retain opcode, AA, and LK.
            value &= 0xFC000003
        elif opcode == 16:  # bc / bcl: retain BO, BI, AA, and LK.
            value &= 0xFFFF0003
        elif (
            opcode == 15
            and ((word >> 16) & 0x1F) == 0
            and 0x8000 <= (word & 0xFFFF) <= 0x9FFF
        ):  # lis rD, image_high
            pointer_registers[(word >> 21) & 0x1F] = index + 10
            value &= 0xFFFF0000
        elif opcode == 14:  # addi rD,rA,image_low
            source_register = (word >> 16) & 0x1F
            if pointer_registers.get(source_register, -1) >= index:
                value &= 0xFFFF0000
        elif opcode == 24:  # ori rA,rS,image_low
            source_register = (word >> 21) & 0x1F
            if pointer_registers.get(source_register, -1) >= index:
                value &= 0xFFFF0000
        normalized.append(value)
    return normalized


def normalized_ppc_bytes(body: bytes) -> bytes:
    return b"".join(word.to_bytes(4, "big") for word in normalized_ppc_words(body))


def function_bodies(
    image: bytes,
    inventory: dict[str, object],
    text_file_offset: int,
) -> dict[int, bytes]:
    result: dict[int, bytes] = {}
    for function in inventory["functions"]:
        address = parse_address(function["address"])
        offset = int(function["section_offset"], 0)
        size = int(function["size_to_next_symbol"])
        body = image[text_file_offset + offset : text_file_offset + offset + size]
        if len(body) != size:
            fail(f"function body exceeds image at {address:#x}")
        result[address] = body
    return result


def main() -> None:
    ownership = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if ownership.get("schema") != "bandwagon-settingsapp-function-ownership-v1":
        fail("unexpected SettingsApp ownership schema")
    if ownership.get("status") != "exhaustive_conservative_partition":
        fail("SettingsApp ownership status is not exhaustive")

    image_info = ownership["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if len(image) != image_info["size"] or digest(image) != image_info["sha256"]:
        fail("SettingsApp image size or hash differs")

    inventory_info = ownership["function_inventory"]
    _, inventory = read_bound_json(inventory_info, "SettingsApp function inventory")
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("SettingsApp inventory entry count differs")
    if inventory.get("function_count") != len(functions):
        fail("SettingsApp inventory function_count differs")
    if inventory.get("pdata_record_count") != inventory_info["pdata_record_count"]:
        fail("SettingsApp .pdata record count differs")
    if inventory.get("idc_name_count") != inventory_info["idc_name_count"]:
        fail("SettingsApp IDC name count differs")
    if inventory.get("source_sha256") != image_info["sha256"]:
        fail("SettingsApp inventory is bound to a different image")

    evidence_documents: dict[str, dict[str, object]] = {}
    for name, record in ownership["evidence_artifacts"].items():
        _, evidence_documents[name] = read_bound_json(record, f"evidence artifact {name}")
    for name in ("semantic_symbols", "third_party_symbols", "provenance", "string_xrefs", "callgraph"):
        if evidence_documents[name].get("image_sha256") != image_info["sha256"]:
            fail(f"SettingsApp evidence artifact {name} is bound to a different image")

    declared_libraries = {
        (item["name"], item["version"])
        for item in evidence_documents["provenance"]["libraries"]
    }
    if declared_libraries != {
        ("Dear ImGui", "1.86"),
        ("BearSSL", "0.6"),
        ("JsonCpp", "00.11.0"),
    }:
        fail("SettingsApp declared-library provenance differs")
    updater_library = evidence_documents["updater_provenance"]["library"]
    if (
        updater_library.get("name") != "Miniz"
        or updater_library.get("version") != "3.1.1"
        or updater_library.get("tag_commit")
        != ownership["upstream_sources"]["miniz_3_1_1"]["commit"]
    ):
        fail("Updater Miniz provenance differs")

    for name, source in ownership["upstream_sources"].items():
        source_root = ROOT / source["vendored_root"]
        actual_commit = git_output("-C", str(source_root), "rev-parse", "HEAD")
        if actual_commit != source["commit"]:
            fail(f"{name} pinned commit differs")
        if git_has_tracked_content_changes(source_root):
            fail(f"{name} pinned source has tracked modifications")

    region = ownership["code_region"]
    region_start = parse_address(region["start"])
    region_end = parse_address(region["end_exclusive"])
    text_file_offset = int(region["file_offset"], 0)
    if int(region["rva"], 0) != text_file_offset:
        fail("SettingsApp memory-layout .text RVA/file offset differs")
    if parse_address(image_info["load_address"]) + int(region["rva"], 0) != region_start:
        fail("SettingsApp load address and .text RVA do not produce the code start")
    region_bytes = image[text_file_offset : text_file_offset + region["size"]]
    if region_end - region_start != region["size"]:
        fail("SettingsApp code-region address size differs")
    if len(region_bytes) != region["size"] or digest(region_bytes) != region["sha256"]:
        fail("SettingsApp code-region bytes differ")
    if parse_address(inventory["base_address"]) != region_start:
        fail("SettingsApp inventory base differs from code-region start")
    if int(inventory["section_end_offset"], 0) != region["size"]:
        fail("SettingsApp inventory section end differs")

    function_addresses = [parse_address(function["address"]) for function in functions]
    if function_addresses != sorted(function_addresses) or len(set(function_addresses)) != len(function_addresses):
        fail("SettingsApp inventory addresses are not strictly ordered")
    for index, function in enumerate(functions):
        address = function_addresses[index]
        offset = int(function["section_offset"], 0)
        if address != region_start + offset:
            fail(f"SettingsApp section offset differs at {address:#x}")
        extent_end = (
            function_addresses[index + 1]
            if index + 1 < len(functions)
            else region_end
        )
        if int(function["size_to_next_symbol"]) != extent_end - address:
            fail(f"SettingsApp function extent differs at {address:#x}")

    required_categories = {
        "peer_product_application_glue",
        "platform_toolchain_graphics",
        "platform_import_thunks",
        "upstream_dear_imgui_1_86",
        "upstream_bearssl_0_6",
        "upstream_jsoncpp_00_11_0",
        "upstream_miniz_3_1_1",
        "unknown",
    }
    categories = ownership["categories"]
    if set(categories) != required_categories:
        fail("SettingsApp ownership category set differs")
    product = categories["peer_product_application_glue"]
    if product.get("personal_authorship_claim") is not False:
        fail("product classification must explicitly disclaim personal authorship")
    if product.get("product_specific") is not True:
        fail("product category must be marked product-specific")
    for category_name in required_categories - {"peer_product_application_glue", "unknown"}:
        if categories[category_name].get("product_specific") is not False:
            fail(f"non-product category is not explicitly excluded: {category_name}")
    if categories["unknown"].get("product_specific") is not None:
        fail("unknown category must not assert product provenance")

    allowed_boundaries = set(function_addresses) | {region_end}
    owners: dict[int, str] = {}
    flat_ranges: list[tuple[int, int, str, dict[str, object]]] = []
    total_entries = 0
    total_bytes = 0
    category_summary: dict[str, dict[str, int]] = {}
    for category_name, category in categories.items():
        category_ranges: list[tuple[int, int, dict[str, object]]] = []
        for item in category["ranges"]:
            start = parse_address(item["start"])
            end = parse_address(item["end_exclusive"])
            if start not in function_addresses or end not in allowed_boundaries:
                fail(f"{category_name} range is not inventory-aligned at {start:#x}")
            if start < region_start or end > region_end or end <= start:
                fail(f"invalid {category_name} range at {start:#x}")
            relative_start = start - region_start
            payload = region_bytes[relative_start : relative_start + (end - start)]
            if len(payload) != item["size"] or end - start != item["size"]:
                fail(f"{category_name} range size differs at {start:#x}")
            if digest(payload) != item["sha256"]:
                fail(f"{category_name} range bytes differ at {start:#x}")
            addresses = [address for address in function_addresses if start <= address < end]
            if len(addresses) != item["inventory_entry_count"]:
                fail(f"{category_name} range entry count differs at {start:#x}")
            for address in addresses:
                if address in owners:
                    fail(f"function {address:#x} has multiple ownership categories")
                owners[address] = category_name
            category_ranges.append((start, end, item))
            flat_ranges.append((start, end, category_name, item))

        category_ranges.sort()
        concatenated = b"".join(
            region_bytes[start - region_start : end - region_start]
            for start, end, _ in category_ranges
        )
        entry_count = sum(int(item["inventory_entry_count"]) for _, _, item in category_ranges)
        byte_count = sum(int(item["size"]) for _, _, item in category_ranges)
        if entry_count != category["inventory_entry_count"]:
            fail(f"{category_name} aggregate entry count differs")
        if byte_count != category["size"] or len(concatenated) != category["size"]:
            fail(f"{category_name} aggregate byte count differs")
        if digest(concatenated) != category["concatenated_sha256"]:
            fail(f"{category_name} concatenated hash differs")
        category_summary[category_name] = {
            "inventory_entry_count": entry_count,
            "size": byte_count,
        }
        total_entries += entry_count
        total_bytes += byte_count

    flat_ranges.sort()
    expected_next = region_start
    for start, end, category_name, _ in flat_ranges:
        if start != expected_next:
            fail(
                f"ownership gap or overlap before {start:#x} ({category_name}); "
                f"expected {expected_next:#x}"
            )
        expected_next = end
    if expected_next != region_end:
        fail("ownership partition does not reach the code-region end")
    if set(owners) != set(function_addresses):
        fail("ownership partition does not assign every inventory entry")

    for index, address in enumerate(function_addresses):
        extent_end = function_addresses[index + 1] if index + 1 < len(functions) else region_end
        containing = [
            item
            for item in flat_ranges
            if item[2] == owners[address] and item[0] <= address and extent_end <= item[1]
        ]
        if len(containing) != 1:
            fail(f"function extent {address:#x}-{extent_end:#x} is split or uncovered")

    anchors = ownership["classification_anchors"]
    semantic_symbols = evidence_documents[anchors["product_semantic_symbols"]["artifact"]]["symbols"]
    if len(semantic_symbols) != anchors["product_semantic_symbols"]["expected_count"]:
        fail("SettingsApp product semantic-symbol count differs")
    for symbol in semantic_symbols:
        address = parse_address(symbol["address"])
        if owners.get(address) != anchors["product_semantic_symbols"]["required_category"]:
            fail(f"product semantic symbol is outside product ownership: {address:#x}")

    third_party_symbols = evidence_documents[anchors["third_party_symbols"]["artifact"]]["symbols"]
    if len(third_party_symbols) != anchors["third_party_symbols"]["expected_count"]:
        fail("SettingsApp third-party symbol count differs")
    ownership_map = anchors["third_party_symbols"]["ownership_to_category"]
    for symbol in third_party_symbols:
        expected_category = ownership_map.get(symbol["ownership"])
        if expected_category is None:
            fail(f"unmapped third-party ownership label: {symbol['ownership']}")
        address = parse_address(symbol["address"])
        if owners.get(address) != expected_category:
            fail(f"third-party symbol has wrong ownership category: {address:#x}")

    entry_point = next((item for item in functions if item["name"] == "_start"), None)
    if entry_point is None:
        fail("SettingsApp inventory has no _start entry")
    entry_point_address = parse_address(entry_point["address"])
    if entry_point_address != parse_address(anchors["entry_point"]["address"]):
        fail("SettingsApp _start address differs")
    if owners[entry_point_address] != anchors["entry_point"]["required_category"]:
        fail("SettingsApp _start is outside platform/toolchain ownership")

    import_rule = anchors["import_thunks"]
    import_start = parse_address(import_rule["start"])
    if parse_address(import_rule["end_exclusive"]) != region_end:
        fail("SettingsApp import-thunk end differs from the code-region end")
    imports = [item for item in functions if parse_address(item["address"]) >= import_start]
    if len(imports) != import_rule["expected_count"]:
        fail("SettingsApp import-thunk count differs")
    allowed_prefixes = tuple(import_rule["allowed_name_prefixes"])
    for function in imports:
        address = parse_address(function["address"])
        if not str(function["name"]).startswith(allowed_prefixes):
            fail(f"unexpected terminal import name: {function['name']}")
        if owners[address] != import_rule["required_category"]:
            fail(f"import thunk has wrong ownership category: {address:#x}")

    settings_bodies = function_bodies(image, inventory, text_file_offset)
    refinement = anchors["unknown_bucket_refinement"]
    updatecheck_image_info = refinement["updatecheck_image"]
    updatecheck_image = (ROOT / updatecheck_image_info["path"]).read_bytes()
    if (
        len(updatecheck_image) != updatecheck_image_info["size"]
        or digest(updatecheck_image) != updatecheck_image_info["sha256"]
    ):
        fail("UpdateCheck refinement image size or hash differs")
    _, updatecheck_inventory = read_bound_json(
        refinement["updatecheck_function_inventory"],
        "UpdateCheck refinement function inventory",
    )
    expected_updatecheck_count = refinement["updatecheck_function_inventory"]["entry_count"]
    if (
        updatecheck_inventory.get("function_count") != expected_updatecheck_count
        or len(updatecheck_inventory["functions"]) != expected_updatecheck_count
        or updatecheck_inventory.get("source_sha256") != updatecheck_image_info["sha256"]
    ):
        fail("UpdateCheck refinement inventory binding or count differs")
    updatecheck_bodies = function_bodies(
        updatecheck_image,
        updatecheck_inventory,
        int(refinement["updatecheck_text_file_offset"], 0),
    )

    call_edges = {
        (parse_address(edge["caller_address"]), parse_address(edge["target_address"]))
        for edge in evidence_documents["callgraph"]["edges"]
    }
    string_owners = {
        (parse_address(reference["function_address"]), reference["string"])
        for reference in evidence_documents["string_xrefs"]["references"]
    }

    packed = anchors["packed_resource_cross_component"]
    bootedit_image_info = packed["bootedit_image"]
    bootedit_image = (ROOT / bootedit_image_info["path"]).read_bytes()
    if (
        len(bootedit_image) != bootedit_image_info["size"]
        or digest(bootedit_image) != bootedit_image_info["sha256"]
    ):
        fail("BootEdit packed-resource evidence image size or hash differs")
    _, bootedit_inventory = read_bound_json(
        packed["bootedit_function_inventory"],
        "BootEdit packed-resource function inventory",
    )
    expected_bootedit_count = packed["bootedit_function_inventory"]["entry_count"]
    if (
        bootedit_inventory.get("function_count") != expected_bootedit_count
        or len(bootedit_inventory["functions"]) != expected_bootedit_count
        or bootedit_inventory.get("source_sha256") != bootedit_image_info["sha256"]
    ):
        fail("BootEdit packed-resource inventory binding or count differs")
    bootedit_bodies = function_bodies(
        bootedit_image,
        bootedit_inventory,
        int(packed["bootedit_text_file_offset"], 0),
    )
    for range_name in (
        "settings_packed_resource_range",
        "settings_graphics_backend_prelude",
    ):
        record = packed[range_name]
        start = parse_address(record["start"])
        end = parse_address(record["end_exclusive"])
        expected_category = record["required_category"]
        addresses = [address for address in function_addresses if start <= address < end]
        if not addresses or any(owners[address] != expected_category for address in addresses):
            fail(f"{range_name} ownership differs")
    for edge in packed["required_call_edges"]:
        required_edge = (parse_address(edge["caller"]), parse_address(edge["target"]))
        if required_edge not in call_edges:
            fail(f"packed-resource/backend call edge differs: {required_edge!r}")
    for marker in packed["required_magic_bytes"]:
        settings_signature = bytes.fromhex(marker["settings_hex"])
        bootedit_signature = bytes.fromhex(marker["bootedit_hex"])
        settings_address = parse_address(marker["settings_address"])
        bootedit_address = parse_address(marker["bootedit_address"])
        if (
            settings_signature not in settings_bodies[settings_address]
            or bootedit_signature not in bootedit_bodies[bootedit_address]
        ):
            fail(f"packed-resource {marker['meaning']} signature differs")
    for pair in packed["normalized_function_comparisons"]:
        settings_address = parse_address(pair["settings_address"])
        bootedit_address = parse_address(pair["bootedit_address"])
        settings_body = settings_bodies[settings_address]
        bootedit_body = bootedit_bodies[bootedit_address]
        if (
            len(settings_body) != pair["settings_extent_size"]
            or len(bootedit_body) != pair["bootedit_extent_size"]
        ):
            fail(f"packed-resource function extent differs at {settings_address:#x}")
        comparison_size = pair["comparison_size"]
        settings_normalized = normalized_ppc_bytes(settings_body[:comparison_size])
        bootedit_normalized = normalized_ppc_bytes(bootedit_body[:comparison_size])
        if settings_normalized != bootedit_normalized:
            fail(f"packed-resource normalized comparison differs at {settings_address:#x}")
        if digest(settings_normalized) != pair["normalized_sha256"]:
            fail(f"packed-resource normalized hash differs at {settings_address:#x}")
        if owners[settings_address] != packed["settings_packed_resource_range"]["required_category"]:
            fail(f"packed-resource function has wrong ownership at {settings_address:#x}")

    refinement_blocks = (
        "product_http_integration",
        "platform_vector_support",
        "bearssl_certificate_loader",
        "jsoncpp_exception_support",
    )
    for block_name in refinement_blocks:
        block = refinement[block_name]
        start = parse_address(block["start"])
        end = parse_address(block["end_exclusive"])
        required_category = block["required_category"]
        block_addresses = [address for address in function_addresses if start <= address < end]
        if not block_addresses or any(owners[address] != required_category for address in block_addresses):
            fail(f"{block_name} ownership boundary differs")
        if not any(
            range_start <= start and end <= range_end and category == required_category
            for range_start, range_end, category, _ in flat_ranges
        ):
            fail(f"{block_name} is not contained by its required category range")

        for edge in block.get("required_call_edges", []):
            required_edge = (parse_address(edge["caller"]), parse_address(edge["target"]))
            if required_edge not in call_edges:
                fail(f"{block_name} required call edge differs: {required_edge!r}")
        for record in block.get("required_string_owners", []):
            required_owner = (parse_address(record["address"]), record["string"])
            if required_owner not in string_owners:
                fail(f"{block_name} required string owner differs: {required_owner!r}")

        source = block.get("source")
        if source is not None:
            source_raw = (ROOT / source["path"]).read_bytes()
            if digest(source_raw) != source["sha256"]:
                fail(f"{block_name} pinned source hash differs")
            for needle in source["contains"]:
                if needle.encode("utf-8") not in source_raw:
                    fail(f"{block_name} pinned source signature differs: {needle}")

        for pair in block["exact_normalized_function_pairs"]:
            settings_address = parse_address(pair["settings_address"])
            updatecheck_address = parse_address(pair["updatecheck_address"])
            settings_normalized = normalized_ppc_bytes(settings_bodies[settings_address])
            updatecheck_normalized = normalized_ppc_bytes(updatecheck_bodies[updatecheck_address])
            if (
                len(settings_normalized) != pair["size"]
                or len(updatecheck_normalized) != pair["size"]
            ):
                fail(f"{block_name} normalized-pair size differs at {settings_address:#x}")
            if settings_normalized != updatecheck_normalized:
                fail(f"{block_name} normalized pair no longer matches at {settings_address:#x}")
            if digest(settings_normalized) != pair["normalized_sha256"]:
                fail(f"{block_name} normalized-pair hash differs at {settings_address:#x}")
            if owners[settings_address] != required_category:
                fail(f"{block_name} matched function has wrong ownership at {settings_address:#x}")

    exception_block = refinement["jsoncpp_exception_support"]
    exception_start = parse_address(exception_block["start"])
    exception_end = parse_address(exception_block["end_exclusive"])
    exact_exception_addresses = {
        parse_address(pair["settings_address"])
        for pair in exception_block["exact_normalized_function_pairs"]
    }
    compiler_variants = {
        parse_address(address)
        for address in exception_block["compiler_generated_variant_addresses"]
    }
    if len(compiler_variants) != 2 or compiler_variants & exact_exception_addresses:
        fail("JsonCpp compiler-generated exception variant set differs")
    if any(
        address not in function_addresses
        or not exception_start <= address < exception_end
        or owners[address] != exception_block["required_category"]
        for address in compiler_variants
    ):
        fail("JsonCpp compiler-generated exception variant ownership differs")

    miniz = anchors["miniz_cross_component"]
    updater_image_info = miniz["updater_image"]
    updater_image = (ROOT / updater_image_info["path"]).read_bytes()
    if len(updater_image) != updater_image_info["size"] or digest(updater_image) != updater_image_info["sha256"]:
        fail("Updater corroborating image size or hash differs")
    _, updater_inventory = read_bound_json(
        miniz["updater_function_inventory"], "Updater corroborating function inventory"
    )
    if (
        updater_inventory.get("function_count")
        != miniz["updater_function_inventory"]["entry_count"]
        or len(updater_inventory["functions"])
        != miniz["updater_function_inventory"]["entry_count"]
    ):
        fail("Updater corroborating function-inventory count differs")
    updater_bodies = function_bodies(
        updater_image,
        updater_inventory,
        int(miniz["updater_text_file_offset"], 0),
    )
    updater_attribution = evidence_documents[miniz["updater_source_attribution_artifact"]]
    matching_updater_ranges = [
        item
        for item in updater_attribution["ranges"]
        if item["start"] == miniz["updater_proven_range"]["start"]
        and item["end_exclusive"] == miniz["updater_proven_range"]["end_exclusive"]
        and item["classification"] == miniz["updater_proven_range"]["classification"]
    ]
    if len(matching_updater_ranges) != 1:
        fail("Updater's proven Miniz range differs")

    inflater = miniz["first_inflater_shape"]
    settings_inflater = settings_bodies[parse_address(inflater["settings_address"])]
    updater_inflater = updater_bodies[parse_address(inflater["updater_address"])]
    if len(settings_inflater) != inflater["size"] or len(updater_inflater) != inflater["size"]:
        fail("Miniz inflater extent differs")
    settings_words = normalized_ppc_words(settings_inflater)
    updater_words = normalized_ppc_words(updater_inflater)
    equal_words = sum(left == right for left, right in zip(settings_words, updater_words))
    if len(settings_words) != inflater["word_count"] or equal_words != inflater["equal_word_count"]:
        fail("Miniz inflater normalized structural match differs")
    if owners[parse_address(inflater["settings_address"])] != "upstream_miniz_3_1_1":
        fail("Miniz inflater has wrong SettingsApp ownership category")

    for pair in miniz["exact_normalized_function_pairs"]:
        settings_address = parse_address(pair["settings_address"])
        updater_address = parse_address(pair["updater_address"])
        settings_normalized = normalized_ppc_bytes(settings_bodies[settings_address])
        updater_normalized = normalized_ppc_bytes(updater_bodies[updater_address])
        if len(settings_normalized) != pair["size"] or len(updater_normalized) != pair["size"]:
            fail(f"Miniz normalized-pair size differs at {settings_address:#x}")
        if settings_normalized != updater_normalized:
            fail(f"Miniz normalized pair no longer matches at {settings_address:#x}")
        if digest(settings_normalized) != pair["normalized_sha256"]:
            fail(f"Miniz normalized-pair hash differs at {settings_address:#x}")
        if owners[settings_address] != "upstream_miniz_3_1_1":
            fail(f"Miniz corroborating function has wrong ownership at {settings_address:#x}")

    totals = ownership["totals"]
    if total_entries != totals["inventory_entry_count"] or total_entries != len(functions):
        fail("SettingsApp total inventory-entry count differs")
    if total_bytes != totals["code_size"] or total_bytes != len(region_bytes):
        fail("SettingsApp total code-byte count differs")
    if totals["category_count"] != len(categories):
        fail("SettingsApp category count differs")

    ownership_map_bytes = "".join(
        f"{category_name}\0{start:08x}\0{end:08x}\n"
        for start, end, category_name, _ in flat_ranges
    ).encode("ascii")
    if digest(ownership_map_bytes) != totals["ownership_map_sha256"]:
        fail("SettingsApp ownership-map hash differs")
    totals_document = {
        "categories": dict(sorted(category_summary.items())),
        "inventory_entry_count": total_entries,
        "code_size": total_bytes,
        "category_count": len(categories),
    }
    totals_bytes = json.dumps(
        totals_document, sort_keys=True, separators=(",", ":")
    ).encode("utf-8")
    if digest(totals_bytes) != totals["category_totals_sha256"]:
        fail("SettingsApp category-totals hash differs")

    summary = ", ".join(
        f"{name}={category['inventory_entry_count']}/{category['size']}"
        for name, category in categories.items()
    )
    print(
        "SettingsApp function ownership verified: "
        f"{total_entries} entries, {total_bytes} code bytes; {summary}"
    )


if __name__ == "__main__":
    main()
