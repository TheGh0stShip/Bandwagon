#!/usr/bin/env python3
"""Verify BootEdit's exhaustive, provenance-first function ownership map."""

from __future__ import annotations

import hashlib
import json
import struct
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/BootEdit-function-ownership.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def address(value: object) -> int:
    if not isinstance(value, str):
        fail(f"expected hexadecimal address string, got {value!r}")
    return int(value, 0)


def read_bound_json(record: dict[str, Any], label: str) -> tuple[bytes, dict[str, Any]]:
    path = ROOT / record["path"]
    raw = path.read_bytes()
    if digest(raw) != record["sha256"]:
        fail(f"{label} hash differs: {record['path']}")
    document = json.loads(raw)
    if document.get("schema") != record["schema"]:
        fail(f"{label} schema differs: {record['path']}")
    return raw, document


def git_output(arguments: list[str], cwd: Path, purpose: str) -> str:
    result = subprocess.run(
        ["git", *arguments], cwd=cwd, capture_output=True, text=True, check=False
    )
    if result.returncode:
        fail(f"git failed while checking {purpose}: {result.stderr.strip()}")
    return result.stdout.strip()


def parse_memory_layout_pe(image: bytes) -> tuple[int, int, dict[str, dict[str, int]]]:
    if len(image) < 0x40 or image[:2] != b"MZ":
        fail("BootEdit image has no valid DOS header")
    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    if pe_offset + 24 > len(image) or image[pe_offset : pe_offset + 4] != b"PE\0\0":
        fail("BootEdit image has no valid PE header")
    coff = pe_offset + 4
    section_count = struct.unpack_from("<H", image, coff + 2)[0]
    optional_size = struct.unpack_from("<H", image, coff + 16)[0]
    optional = coff + 20
    if optional + optional_size > len(image):
        fail("BootEdit optional header is truncated")
    magic = struct.unpack_from("<H", image, optional)[0]
    entry_rva = struct.unpack_from("<I", image, optional + 16)[0]
    if magic == 0x10B:
        image_base = struct.unpack_from("<I", image, optional + 28)[0]
    elif magic == 0x20B:
        image_base = struct.unpack_from("<Q", image, optional + 24)[0]
    else:
        fail("BootEdit optional-header format is unsupported")
    sections: dict[str, dict[str, int]] = {}
    section_table = optional + optional_size
    for index in range(section_count):
        header = section_table + index * 40
        if header + 40 > len(image):
            fail("BootEdit section table is truncated")
        name = image[header : header + 8].rstrip(b"\0").decode("ascii")
        virtual_size, rva, raw_size, raw_pointer = struct.unpack_from(
            "<IIII", image, header + 8
        )
        sections[name] = {
            "virtual_size": virtual_size,
            "rva": rva,
            "raw_size": raw_size,
            "raw_pointer": raw_pointer,
        }
    return image_base, entry_rva, sections


def normalized_ppc_bytes(body: bytes) -> bytes:
    """Mask direct branches and image-pointer relocation operands only."""
    if len(body) % 4:
        fail("PowerPC function extent is not four-byte aligned")
    words = [int.from_bytes(body[offset : offset + 4], "big") for offset in range(0, len(body), 4)]
    pointer_registers: dict[int, int] = {}
    normalized: list[int] = []
    for index, word in enumerate(words):
        opcode = word >> 26
        value = word
        if opcode == 18:
            value &= 0xFC000003
        elif opcode == 16:
            value &= 0xFFFF0003
        elif (
            opcode == 15
            and ((word >> 16) & 0x1F) == 0
            and 0x8000 <= (word & 0xFFFF) <= 0x9FFF
        ):
            pointer_registers[(word >> 21) & 0x1F] = index + 10
            value &= 0xFFFF0000
        elif opcode == 14:
            source = (word >> 16) & 0x1F
            if pointer_registers.get(source, -1) >= index:
                value &= 0xFFFF0000
        elif opcode == 24:
            source = (word >> 21) & 0x1F
            if pointer_registers.get(source, -1) >= index:
                value &= 0xFFFF0000
        normalized.append(value)
    return b"".join(word.to_bytes(4, "big") for word in normalized)


def main() -> None:
    ownership = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if ownership.get("schema") != "bandwagon-bootedit-function-ownership-v1":
        fail("unexpected BootEdit ownership schema")
    if ownership.get("status") != "exhaustive_provenance_first_partition":
        fail("BootEdit ownership status is not exhaustive")

    image_info = ownership["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if len(image) != image_info["size"] or digest(image) != image_info["sha256"]:
        fail("BootEdit image size or hash differs")
    image_base, entry_rva, sections = parse_memory_layout_pe(image)
    if image_base != address(image_info["load_address"]):
        fail("BootEdit image base differs")
    if image_base + entry_rva != address(image_info["entry_point"]):
        fail("BootEdit PE entry point differs")
    if ".text" not in sections:
        fail("BootEdit image has no .text section")

    region = ownership["instruction_region"]
    text = sections[".text"]
    region_start = address(region["start"])
    region_end = address(region["end_exclusive"])
    if text["rva"] != address(region["rva"]):
        fail("BootEdit .text RVA differs")
    if image_base + text["rva"] != region_start:
        fail("BootEdit instruction region does not begin at .text")
    if text["virtual_size"] != region["size"] or region_end - region_start != region["size"]:
        fail("BootEdit .text size metadata differs")
    # This extracted PE is in memory layout, so RVA is the initialized-byte offset.
    region_bytes = image[text["rva"] : text["rva"] + text["virtual_size"]]
    if len(region_bytes) != region["size"] or digest(region_bytes) != region["sha256"]:
        fail("BootEdit .text bytes differ")

    inventory_info = ownership["function_inventory"]
    _, inventory = read_bound_json(inventory_info, "BootEdit function inventory")
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("BootEdit inventory entry count differs")
    if inventory.get("function_count") != len(functions):
        fail("BootEdit inventory function_count differs")
    if inventory.get("pdata_record_count") != inventory_info["pdata_record_count"]:
        fail("BootEdit inventory pdata count differs")
    if inventory.get("idc_name_count") != inventory_info["idc_name_count"]:
        fail("BootEdit inventory IDC-name count differs")
    if inventory.get("source_sha256") != image_info["sha256"]:
        fail("BootEdit inventory targets a different image")
    if address(inventory["base_address"]) != region_start:
        fail("BootEdit inventory base differs")
    if address(inventory["section_end_offset"]) != region["size"]:
        fail("BootEdit inventory section end differs")

    evidence: dict[str, dict[str, Any]] = {}
    expected_evidence = {
        "analysis/xex/BootEdit-callgraph.json",
        "analysis/xex/BootEdit-string-xrefs.json",
        "analysis/xex/BootEdit-symbols.json",
        "analysis/xex/BootEdit-thirdparty-symbols.json",
        "analysis/xex/BootEdit-provenance.json",
    }
    for record in ownership["evidence_inputs"]:
        _, document = read_bound_json(record, "BootEdit ownership evidence")
        if document.get("image_sha256") != image_info["sha256"]:
            fail(f"BootEdit evidence targets a different image: {record['path']}")
        evidence[record["path"]] = document
    if set(evidence) != expected_evidence:
        fail("BootEdit evidence-input set differs")

    imgui = ownership["upstream_sources"]["dear_imgui_1_86"]
    imgui_root = ROOT / imgui["vendored_root"]
    for item in imgui["files"]:
        if digest((imgui_root / item["path"]).read_bytes()) != item["sha256"]:
            fail(f"Dear ImGui source hash differs: {item['path']}")
    tree_line = git_output(["ls-tree", "HEAD", imgui["vendored_root"]], ROOT, "ImGui gitlink")
    fields = tree_line.split(None, 3)
    if (
        len(fields) != 4
        or fields[0] != "160000"
        or fields[1] != "commit"
        or fields[2] != imgui["gitlink_commit"]
    ):
        fail("repository Dear ImGui gitlink differs")
    if git_output(["rev-parse", "HEAD"], imgui_root, "ImGui checkout") != imgui["gitlink_commit"]:
        fail("checked-out Dear ImGui revision differs")
    imgui_header = (imgui_root / "imgui.h").read_text(encoding="utf-8")
    if '#define IMGUI_VERSION               "1.86"' not in imgui_header:
        fail("Dear ImGui 1.86 version marker is absent")
    xbox_paths = git_output(["ls-files", "*xbox360*"], imgui_root, "Xbox 360 backend absence")
    if xbox_paths:
        fail("pinned upstream unexpectedly contains an Xbox 360 backend")

    function_addresses = [address(item["address"]) for item in functions]
    if function_addresses != sorted(function_addresses) or len(set(function_addresses)) != len(function_addresses):
        fail("BootEdit inventory addresses are not strictly ordered")
    if not function_addresses or function_addresses[0] != region_start:
        fail("BootEdit inventory does not begin at .text")
    for index, function in enumerate(functions):
        function_address = function_addresses[index]
        if address(function["section_offset"]) != function_address - region_start:
            fail(f"BootEdit section offset differs at {function_address:#x}")
        extent_end = function_addresses[index + 1] if index + 1 < len(functions) else region_end
        if function["size_to_next_symbol"] != extent_end - function_address:
            fail(f"BootEdit function extent differs at {function_address:#x}")
    if sum(item["size_to_next_symbol"] for item in functions) != len(region_bytes):
        fail("BootEdit inventory extents do not total .text")

    required_categories = {
        "product_application",
        "upstream_imgui_1_86",
        "platform_toolchain",
        "unknown",
    }
    categories = ownership["categories"]
    if set(categories) != required_categories:
        fail("BootEdit category set differs")
    inventory_set = set(function_addresses)
    allowed_boundaries = inventory_set | {region_end}
    flat_ranges: list[tuple[int, int, str, dict[str, Any]]] = []
    entry_owners: dict[int, str] = {}
    total_entries = 0
    total_bytes = 0
    for category_name, category in categories.items():
        parsed_ranges: list[tuple[int, int, dict[str, Any]]] = []
        for item in category["ranges"]:
            start, end = address(item["start"]), address(item["end_exclusive"])
            if start not in inventory_set or end not in allowed_boundaries:
                fail(f"{category_name} range is not inventory-aligned at {start:#x}")
            if start < region_start or end > region_end or end <= start:
                fail(f"invalid {category_name} range at {start:#x}")
            body = image[start - image_base : end - image_base]
            if len(body) != item["size"] or end - start != item["size"]:
                fail(f"{category_name} range size differs at {start:#x}")
            if digest(body) != item["sha256"]:
                fail(f"{category_name} range bytes differ at {start:#x}")
            owned = [value for value in function_addresses if start <= value < end]
            if len(owned) != item["inventory_entry_count"]:
                fail(f"{category_name} entry count differs at {start:#x}")
            for value in owned:
                if value in entry_owners:
                    fail(f"BootEdit entry {value:#x} has multiple owners")
                entry_owners[value] = category_name
            parsed_ranges.append((start, end, item))
            flat_ranges.append((start, end, category_name, item))
        if parsed_ranges != sorted(parsed_ranges):
            fail(f"{category_name} ranges are not ordered")
        category_entries = sum(item["inventory_entry_count"] for _, _, item in parsed_ranges)
        category_bytes = sum(item["size"] for _, _, item in parsed_ranges)
        concatenated = b"".join(
            image[start - image_base : end - image_base]
            for start, end, _ in parsed_ranges
        )
        if category_entries != category["inventory_entry_count"]:
            fail(f"{category_name} aggregate entry count differs")
        if category_bytes != category["size"] or len(concatenated) != category["size"]:
            fail(f"{category_name} aggregate byte count differs")
        if digest(concatenated) != category["concatenated_sha256"]:
            fail(f"{category_name} aggregate hash differs")
        total_entries += category_entries
        total_bytes += category_bytes

    flat_ranges.sort()
    cursor = region_start
    for start, end, category_name, _ in flat_ranges:
        if start != cursor:
            fail(f"BootEdit ownership gap/overlap before {start:#x} ({category_name})")
        cursor = end
    if cursor != region_end:
        fail("BootEdit ownership partition does not reach .text end")
    if set(entry_owners) != inventory_set:
        fail("BootEdit ownership partition misses inventory entries")
    for index, function_address in enumerate(function_addresses):
        extent_end = function_addresses[index + 1] if index + 1 < len(function_addresses) else region_end
        owner = entry_owners[function_address]
        containers = [
            item for item in flat_ranges
            if item[2] == owner and item[0] <= function_address and extent_end <= item[1]
        ]
        if len(containers) != 1:
            fail(f"BootEdit inventory extent {function_address:#x}-{extent_end:#x} is split")

    cross = ownership["cross_component_evidence"]
    other_images: dict[str, tuple[bytes, int]] = {}
    other_inventories: dict[str, dict[str, Any]] = {}
    for component, image_key, inventory_key in (
        ("SettingsApp", "settings_image", "settings_inventory"),
        ("Updater", "updater_image", "updater_inventory"),
    ):
        image_record = cross[image_key]
        other_image = (ROOT / image_record["path"]).read_bytes()
        if len(other_image) != image_record["size"] or digest(other_image) != image_record["sha256"]:
            fail(f"{component} cross-component image differs")
        other_base, _, _ = parse_memory_layout_pe(other_image)
        _, other_inventory = read_bound_json(cross[inventory_key], f"{component} cross-component inventory")
        if len(other_inventory["functions"]) != cross[inventory_key]["entry_count"]:
            fail(f"{component} cross-component inventory count differs")
        other_images[component] = (other_image, other_base)
        other_inventories[component] = other_inventory

    _, updater_ownership = read_bound_json(
        cross["updater_ownership"], "Updater cross-component ownership"
    )
    updater_platform_ranges = [
        (address(item["start"]), address(item["end_exclusive"]))
        for item in updater_ownership["categories"]["platform_toolchain"]["ranges"]
    ]
    boot_functions = {address(item["address"]): item for item in functions}
    other_functions = {
        component: {address(item["address"]): item for item in document["functions"]}
        for component, document in other_inventories.items()
    }
    for item in cross["relocation_normalized_exact_pairs"]:
        boot_address = address(item["bootedit_address"])
        component = item["other_component"]
        other_address = address(item["other_address"])
        if boot_address not in boot_functions or other_address not in other_functions[component]:
            fail(f"cross-component pair entry is absent at {boot_address:#x}")
        if (
            boot_functions[boot_address]["size_to_next_symbol"] != item["size"]
            or other_functions[component][other_address]["size_to_next_symbol"] != item["size"]
        ):
            fail(f"cross-component pair size differs at {boot_address:#x}")
        boot_body = image[boot_address - image_base : boot_address - image_base + item["size"]]
        other_image, other_base = other_images[component]
        other_body = other_image[other_address - other_base : other_address - other_base + item["size"]]
        boot_hash = digest(normalized_ppc_bytes(boot_body))
        other_hash = digest(normalized_ppc_bytes(other_body))
        if boot_hash != item["normalized_sha256"] or other_hash != boot_hash:
            fail(f"cross-component normalized match differs at {boot_address:#x}")
        if entry_owners[boot_address] != item["classification"]:
            fail(f"cross-component pair ownership differs at {boot_address:#x}")
        if component == "Updater" and not any(
            start <= other_address < end for start, end in updater_platform_ranges
        ):
            fail(f"Updater cross-match is no longer platform-owned at {other_address:#x}")

    for boot_address, settings_address, size in (
        (0x82011ED8, 0x82034830, 136),
        (0x82011F60, 0x820348B8, 320),
        (0x820120A0, 0x820349F8, 624),
    ):
        if (
            boot_functions[boot_address]["size_to_next_symbol"] != size
            or other_functions["SettingsApp"][settings_address]["size_to_next_symbol"] != size
            or entry_owners[boot_address] != "upstream_imgui_1_86"
        ):
            fail(f"BootEdit/SettingsApp ImGui opening sequence differs at {boot_address:#x}")

    product_addresses = [address(item["address"]) for item in ownership["product_application_entries"]]
    if len(product_addresses) != len(set(product_addresses)):
        fail("BootEdit product evidence has duplicate entries")
    expected_product = {value for value, owner in entry_owners.items() if owner == "product_application"}
    if set(product_addresses) != expected_product:
        fail("BootEdit product evidence does not exactly cover product-owned entries")
    if categories["product_application"].get("personal_authorship_claim") is not False:
        fail("BootEdit product classification became a personal-authorship claim")

    semantic = evidence["analysis/xex/BootEdit-symbols.json"]
    semantic_by_address = {address(item["address"]): item for item in semantic["symbols"]}
    supplemental = ownership["supplemental_discovered_functions"]
    if len(supplemental) != 1:
        fail("BootEdit supplemental-function set differs")
    leaf = supplemental[0]
    leaf_start, leaf_end = address(leaf["address"]), address(leaf["end_exclusive"])
    leaf_bytes = image[leaf_start - image_base : leaf_end - image_base]
    if leaf_start != 0x82050A00 or leaf_end != 0x82050A4C:
        fail("BootEdit ColorEqualsExact extent differs")
    if len(leaf_bytes) != leaf["size"] or digest(leaf_bytes) != leaf["sha256"]:
        fail("BootEdit ColorEqualsExact bytes differ")
    container = address(leaf["inventory_container"])
    if not (container <= leaf_start < leaf_end <= function_addresses[function_addresses.index(container) + 1]):
        fail("BootEdit ColorEqualsExact container differs")
    if entry_owners[container] != leaf["classification"]:
        fail("BootEdit ColorEqualsExact owner differs")
    if semantic_by_address.get(leaf_start, {}).get("semantic_name") != "ColorEqualsExact":
        fail("BootEdit semantic overlay lost ColorEqualsExact")
    for semantic_address, item in semantic_by_address.items():
        if semantic_address == leaf_start:
            continue
        if entry_owners.get(semantic_address) != "product_application":
            fail(f"BootEdit semantic product symbol is not product-owned: {semantic_address:#x}")

    thirdparty = evidence["analysis/xex/BootEdit-thirdparty-symbols.json"]
    if len(thirdparty["symbols"]) != 1:
        fail("BootEdit Dear ImGui semantic-anchor count differs")
    imgui_anchor = thirdparty["symbols"][0]
    if (
        address(imgui_anchor["address"]) != 0x82014428
        or imgui_anchor["semantic_name"] != "ImGui::DebugCheckVersionAndDataLayout"
        or entry_owners[0x82014428] != "upstream_imgui_1_86"
    ):
        fail("BootEdit Dear ImGui version/layout anchor differs")
    provenance = evidence["analysis/xex/BootEdit-provenance.json"]["library"]
    if (
        provenance.get("name") != "Dear ImGui"
        or provenance.get("version") != "1.86"
        or provenance.get("tag_commit") != imgui["gitlink_commit"]
    ):
        fail("BootEdit Dear ImGui provenance differs")

    expected_boundaries = [
        (0x82011ED8, 0x82038AB8, "upstream_imgui_1_86"),
        (0x82038AB8, 0x82039EC0, "unknown"),
        (0x82039EC0, 0x8204B998, "upstream_imgui_1_86"),
    ]
    actual_boundaries = [
        (address(item["start"]), address(item["end_exclusive"]), item["classification"])
        for item in ownership["boundary_evidence"]
    ]
    if actual_boundaries != expected_boundaries:
        fail("BootEdit ImGui/backend boundary evidence differs")
    category_pairs = {
        name: {(address(item["start"]), address(item["end_exclusive"])) for item in category["ranges"]}
        for name, category in categories.items()
    }
    for start, end, owner in expected_boundaries:
        if (start, end) not in category_pairs[owner]:
            fail(f"BootEdit boundary span is not {owner}: {start:#x}-{end:#x}")

    xrefs = evidence["analysis/xex/BootEdit-string-xrefs.json"]
    xref_pairs = {(address(item["function_address"]), item["string"]) for item in xrefs["references"]}
    for required in (
        (0x82038E78, "1.86"),
        (0x82038E78, "imgui_impl_xbox360"),
        (0x82038E78, "imgui_impl_dx9"),
        (0x82039EC0, "RefScale=%f"),
        (0x8204B768, "Alpha Bar"),
        (0x8204BDA0, "Cancel"),
        (0x8204D818, "vector<T> too long"),
        (0x82051BA8, "invalid vector<T> subscript"),
        (0x82050F60, "Fonts\\NotoSans17.ttf"),
        (0x820512E8, "Editor"),
    ):
        if required not in xref_pairs:
            fail(f"BootEdit ownership string anchor is absent: {required!r}")
    if entry_owners[0x82038E78] != "unknown":
        fail("BootEdit custom backend identifier owner differs")

    templates = ownership["compiler_emitted_template_partition"]
    template_entries: set[int] = set()
    template_bytes = 0
    for item in templates["ranges"]:
        start, end = address(item["start"]), address(item["end_exclusive"])
        owned = {value for value in function_addresses if start <= value < end}
        if (start, end) not in category_pairs["platform_toolchain"]:
            fail(f"BootEdit template range is not platform-owned at {start:#x}")
        if len(owned) != item["inventory_entry_count"] or end - start != item["size"]:
            fail(f"BootEdit template range totals differ at {start:#x}")
        if template_entries & owned:
            fail(f"BootEdit template ranges overlap at {start:#x}")
        template_entries |= owned
        template_bytes += end - start
    if (
        templates["classification"] != "platform_toolchain"
        or len(template_entries) != templates["inventory_entry_count"]
        or template_bytes != templates["size"]
    ):
        fail("BootEdit compiler-template partition totals differ")

    callgraph = evidence["analysis/xex/BootEdit-callgraph.json"]
    edges = {
        (address(item["caller_address"]), address(item["target_address"]))
        for item in callgraph["edges"]
    }
    for edge in (
        (0x82050F60, 0x82052CC8),
        (0x82050F60, 0x82052CF8),
        (0x82052CC8, 0x82052C48),
        (0x82052CF8, 0x82052C48),
    ):
        if edge not in edges:
            fail(f"BootEdit font lookup evidence edge is absent: {edge[0]:#x}->{edge[1]:#x}")
    for value in (0x82052A88, 0x82052C48, 0x82052CC8, 0x82052CF8):
        if entry_owners[value] != "unknown":
            fail(f"BootEdit unresolved XPR packed-resource helper was over-attributed: {value:#x}")
    xpr_words = struct.unpack_from(">III", image, 0x82052AE8 - image_base)
    if xpr_words != (0x3D405850, 0x614A5232, 0x812B0000):
        fail("BootEdit XPR2 packed-resource magic check differs")
    if struct.unpack_from(">I", image, 0x82052BD0 - image_base)[0] != 0x2B0A5458:
        fail("BootEdit TX resource-record check differs")

    refinement = ownership["bounded_refinement"]
    before, after = refinement["before"], refinement["after"]
    if before != {
        "unknown_entry_count": 144,
        "unknown_size": 41248,
        "product_application_entry_count": 24,
        "product_application_size": 14072,
    }:
        fail("BootEdit refinement baseline differs")
    if (
        after["unknown_entry_count"] != categories["unknown"]["inventory_entry_count"]
        or after["unknown_size"] != categories["unknown"]["size"]
        or after["product_application_entry_count"] != categories["product_application"]["inventory_entry_count"]
        or after["product_application_size"] != categories["product_application"]["size"]
    ):
        fail("BootEdit refinement after-totals differ")
    moved_from_unknown_entries = sum(
        item["inventory_entry_count"]
        for item in refinement["reassignments"]
        if item["from"] == "unknown"
    )
    moved_from_unknown_bytes = sum(
        item["size"]
        for item in refinement["reassignments"]
        if item["from"] == "unknown"
    )
    if (
        moved_from_unknown_entries != before["unknown_entry_count"] - after["unknown_entry_count"]
        or moved_from_unknown_bytes != before["unknown_size"] - after["unknown_size"]
    ):
        fail("BootEdit refinement reassignment totals differ")

    entry_point = address(image_info["entry_point"])
    if entry_point != 0x82053A40 or entry_owners[entry_point] != "platform_toolchain":
        fail("BootEdit CRT entry ownership differs")
    names = {address(item["address"]): item["name"] for item in functions}
    if names[entry_point] != "_start":
        fail("BootEdit _start inventory name differs")
    import_addresses = [value for value in function_addresses if 0x82069EB0 <= value < region_end]
    if len(import_addresses) != 69:
        fail("BootEdit import-thunk entry count differs")
    allowed_prefixes = ("xam_xex_", "xboxkrnl_exe_", "__imp_xboxkrnl_exe_")
    for value in import_addresses:
        if entry_owners[value] != "platform_toolchain" or not names[value].startswith(allowed_prefixes):
            fail(f"BootEdit import-thunk owner/name differs at {value:#x}")

    totals = ownership["totals"]
    if total_entries != len(functions) or total_entries != totals["inventory_entry_count"]:
        fail("BootEdit total inventory-entry count differs")
    if total_bytes != len(region_bytes) or total_bytes != totals["instruction_size"]:
        fail("BootEdit total instruction-byte count differs")
    product = categories["product_application"]
    unknown = categories["unknown"]
    if totals["category_count"] != len(categories):
        fail("BootEdit total category count differs")
    if (
        totals["product_application_entry_count"] != product["inventory_entry_count"]
        or totals["product_application_size"] != product["size"]
        or totals["non_product_entry_count"] != total_entries - product["inventory_entry_count"]
        or totals["non_product_size"] != total_bytes - product["size"]
        or totals["unknown_entry_count"] != unknown["inventory_entry_count"]
        or totals["unknown_size"] != unknown["size"]
    ):
        fail("BootEdit ownership summary totals differ")

    summary = ", ".join(
        f"{name}={item['inventory_entry_count']}/{item['size']}"
        for name, item in categories.items()
    )
    print(
        "BootEdit function ownership verified: "
        f"{total_entries} entries, {total_bytes} .text bytes; {summary}; "
        f"compiler_emitted_templates={len(template_entries)}/{template_bytes}; "
        f"supplemental_non_pdata={len(supplemental)}"
    )


if __name__ == "__main__":
    main()
