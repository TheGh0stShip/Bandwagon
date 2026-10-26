#!/usr/bin/env python3
"""Verify Updater's exhaustive, provenance-first function ownership map."""

from __future__ import annotations

import hashlib
import json
import struct
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/Updater-function-ownership.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def parse_address(value: str) -> int:
    return int(value, 0)


def read_json(path: Path) -> tuple[bytes, dict[str, Any]]:
    raw = path.read_bytes()
    return raw, json.loads(raw)


def parse_memory_layout_pe(image: bytes) -> tuple[int, int, dict[str, dict[str, int]]]:
    """Read only the PE fields needed to locate a memory-layout image section."""
    if len(image) < 0x40 or image[:2] != b"MZ":
        fail("Updater image has no valid DOS header")
    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    if pe_offset + 24 > len(image) or image[pe_offset : pe_offset + 4] != b"PE\0\0":
        fail("Updater image has no valid PE header")

    coff = pe_offset + 4
    section_count = struct.unpack_from("<H", image, coff + 2)[0]
    optional_size = struct.unpack_from("<H", image, coff + 16)[0]
    optional = coff + 20
    if optional + optional_size > len(image):
        fail("Updater optional header is truncated")
    optional_magic = struct.unpack_from("<H", image, optional)[0]
    entry_rva = struct.unpack_from("<I", image, optional + 16)[0]
    if optional_magic == 0x10B:
        image_base = struct.unpack_from("<I", image, optional + 28)[0]
    elif optional_magic == 0x20B:
        image_base = struct.unpack_from("<Q", image, optional + 24)[0]
    else:
        fail("Updater image has an unsupported optional-header format")
    section_table = optional + optional_size
    sections: dict[str, dict[str, int]] = {}
    for index in range(section_count):
        header = section_table + index * 40
        if header + 40 > len(image):
            fail("Updater section table is truncated")
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


def checked_git_output(arguments: list[str], cwd: Path, purpose: str) -> str:
    try:
        result = subprocess.run(
            ["git", *arguments],
            cwd=cwd,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as error:
        fail(f"cannot invoke git while checking {purpose}: {error}")
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        fail(f"git failed while checking {purpose}: {detail}")
    return result.stdout.strip()


def main() -> None:
    ownership = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if ownership.get("schema") != "bandwagon-updater-function-ownership-v1":
        fail("unexpected Updater ownership schema")

    image_info = ownership["image"]
    image_path = ROOT / image_info["path"]
    image = image_path.read_bytes()
    if len(image) != image_info["size"]:
        fail("Updater image size differs")
    if digest(image) != image_info["sha256"]:
        fail("Updater image hash differs")

    image_base, entry_rva, sections = parse_memory_layout_pe(image)
    if image_base != parse_address(image_info["load_address"]):
        fail("Updater PE image base differs")
    if image_base + entry_rva != parse_address(image_info["entry_point"]):
        fail("Updater PE entry point differs")
    if ".text" not in sections:
        fail("Updater PE has no .text section")

    region = ownership["instruction_region"]
    text = sections[".text"]
    region_start = parse_address(region["start"])
    region_end = parse_address(region["end_exclusive"])
    if text["rva"] != parse_address(region["rva"]):
        fail("Updater .text RVA differs")
    if text["virtual_size"] != region["size"]:
        fail("Updater .text virtual size differs")
    if region_start != image_base + text["rva"]:
        fail("Updater ownership region does not begin at .text")
    if region_end - region_start != region["size"]:
        fail("Updater instruction-region size metadata differs")

    # These PE files are extracted in memory layout. PointerToRawData describes
    # the stale pre-map layout and must not be used to hash initialized .text.
    region_bytes = image[text["rva"] : text["rva"] + text["virtual_size"]]
    if len(region_bytes) != region["size"]:
        fail("Updater initialized .text bytes are truncated")
    if digest(region_bytes) != region["sha256"]:
        fail("Updater instruction-region hash differs")

    inventory_info = ownership["function_inventory"]
    inventory_path = ROOT / inventory_info["path"]
    inventory_bytes, inventory = read_json(inventory_path)
    if digest(inventory_bytes) != inventory_info["sha256"]:
        fail("Updater function-inventory hash differs")
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("Updater function-inventory entry count differs")
    if inventory.get("function_count") != len(functions):
        fail("Updater function_count field differs")
    if parse_address(inventory["base_address"]) != region_start:
        fail("Updater function inventory base differs")
    if parse_address(inventory["section_end_offset"]) != region["size"]:
        fail("Updater function inventory section end differs")

    recovery = inventory_info["recovery_source"]
    recovery_path = ROOT / recovery["path"]
    recovery_bytes = recovery_path.read_bytes()
    if len(recovery_bytes) != recovery["size"]:
        fail("Updater recovery-object size differs")
    if digest(recovery_bytes) != recovery["sha256"]:
        fail("Updater recovery-object hash differs")
    if inventory.get("source") != recovery["path"]:
        fail("Updater inventory recovery-object path differs")
    if inventory.get("source_size") != recovery["size"]:
        fail("Updater inventory recovery-object size field differs")
    if inventory.get("source_sha256") != recovery["sha256"]:
        fail("Updater inventory recovery-object hash field differs")

    evidence_schemas = {
        "analysis/xex/Updater-callgraph.json": "bandwagon-ppc-direct-callgraph-v1",
        "analysis/xex/Updater-string-xrefs.json": "bandwagon-ppc-string-xrefs-v1",
        "analysis/xex/Updater-symbols.json": "bandwagon-updater-semantic-symbols-v1",
        "analysis/xex/Updater-provenance.json": "bandwagon-updater-provenance-v1",
        "analysis/xex/Updater-source-attribution.json": "bandwagon-updater-source-attribution-v1",
    }
    evidence_documents: dict[str, dict[str, Any]] = {}
    for item in ownership["evidence_inputs"]:
        path_string = item["path"]
        path = ROOT / path_string
        raw, document = read_json(path)
        if digest(raw) != item["sha256"]:
            fail(f"Updater ownership evidence hash differs: {path_string}")
        expected_schema = evidence_schemas.get(path_string)
        if expected_schema is None or document.get("schema") != expected_schema:
            fail(f"unexpected Updater evidence schema: {path_string}")
        evidence_image_hash = document.get("image_sha256")
        if evidence_image_hash is not None and evidence_image_hash != image_info["sha256"]:
            fail(f"Updater evidence targets a different image: {path_string}")
        evidence_documents[path_string] = document
    if set(evidence_documents) != set(evidence_schemas):
        fail("Updater ownership evidence-input set differs")

    miniz = ownership["upstream_sources"]["miniz_3_1_1"]
    miniz_root = ROOT / miniz["vendored_root"]
    for file_info in miniz["files"]:
        source_path = miniz_root / file_info["path"]
        if digest(source_path.read_bytes()) != file_info["sha256"]:
            fail(f"miniz source hash differs: {file_info['path']}")

    gitlink = checked_git_output(
        ["ls-tree", "HEAD", miniz["vendored_root"]], ROOT, "miniz gitlink"
    )
    gitlink_fields = gitlink.split(None, 3)
    if (
        len(gitlink_fields) != 4
        or gitlink_fields[0] != "160000"
        or gitlink_fields[1] != "commit"
        or gitlink_fields[2] != miniz["gitlink_commit"]
    ):
        fail("repository miniz gitlink differs from the ownership pin")
    checkout_commit = checked_git_output(
        ["rev-parse", "HEAD"], miniz_root, "miniz checkout revision"
    )
    if checkout_commit != miniz["gitlink_commit"]:
        fail("checked-out miniz revision differs from the ownership pin")

    cmake = (miniz_root / "CMakeLists.txt").read_text(encoding="utf-8")
    for marker in (
        "set(MINIZ_API_VERSION 3)",
        "set(MINIZ_MINOR_VERSION 1)",
        "set(MINIZ_PATCH_VERSION 1)",
    ):
        if marker not in cmake:
            fail(f"miniz 3.1.1 version marker is absent: {marker}")
    miniz_zip_header = (miniz_root / "miniz_zip.h").read_text(encoding="utf-8")
    if "MZ_ZIP_WRITE_CALLBACK_FAILED" not in miniz_zip_header:
        fail("miniz WRITE_CALLBACK_FAILED enum marker is absent")
    if "MZ_ZIP_TOTAL_ERRORS" not in miniz_zip_header:
        fail("miniz TOTAL_ERRORS enum marker is absent")

    addresses = [parse_address(function["address"]) for function in functions]
    if len(addresses) != len(set(addresses)):
        fail("Updater function inventory has duplicate addresses")
    if addresses != sorted(addresses):
        fail("Updater function inventory is not address-sorted")
    if not addresses or addresses[0] != region_start:
        fail("Updater function inventory does not begin at .text")
    for index, function in enumerate(functions):
        address = addresses[index]
        if parse_address(function["section_offset"]) != address - region_start:
            fail(f"Updater function section offset differs at {address:#x}")
        next_address = addresses[index + 1] if index + 1 < len(addresses) else region_end
        expected_size = next_address - address
        if function["size_to_next_symbol"] != expected_size or expected_size <= 0:
            fail(f"Updater function extent differs at {address:#x}")
    if sum(int(function["size_to_next_symbol"]) for function in functions) != len(
        region_bytes
    ):
        fail("Updater function extents do not total the .text size")

    required_categories = {
        "product_application",
        "upstream_miniz",
        "platform_toolchain",
        "unknown",
    }
    categories = ownership["categories"]
    if set(categories) != required_categories:
        fail("Updater ownership category set differs")

    address_set = set(addresses)
    allowed_boundaries = address_set | {region_end}
    flat_ranges: list[tuple[int, int, str, dict[str, Any]]] = []
    entry_owners: dict[int, str] = {}
    total_entries = 0
    total_bytes = 0
    for category_name, category in categories.items():
        category_ranges: list[tuple[int, int, dict[str, Any]]] = []
        for item in category["ranges"]:
            start = parse_address(item["start"])
            end = parse_address(item["end_exclusive"])
            if start not in address_set or end not in allowed_boundaries:
                fail(f"{category_name} range is not inventory-aligned at {start:#x}")
            if start < region_start or end > region_end or end <= start:
                fail(f"invalid {category_name} range at {start:#x}")
            payload = image[start - image_base : end - image_base]
            if end - start != item["size"] or len(payload) != item["size"]:
                fail(f"invalid {category_name} range size at {start:#x}")
            if digest(payload) != item["sha256"]:
                fail(f"{category_name} range bytes changed at {start:#x}")

            owned_addresses = [address for address in addresses if start <= address < end]
            if len(owned_addresses) != item["inventory_entry_count"]:
                fail(
                    f"{category_name} inventory count changed at {start:#x}: "
                    f"expected {item['inventory_entry_count']}, found {len(owned_addresses)}"
                )
            for address in owned_addresses:
                if address in entry_owners:
                    fail(f"Updater function {address:#x} has multiple owners")
                entry_owners[address] = category_name
            category_ranges.append((start, end, item))
            flat_ranges.append((start, end, category_name, item))

        if category_ranges != sorted(category_ranges):
            fail(f"{category_name} ranges are not sorted")
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
        if "subclassification_summary" in category:
            summary_entries = sum(
                int(item["inventory_entry_count"])
                for item in category["subclassification_summary"]
            )
            summary_bytes = sum(
                int(item["size"]) for item in category["subclassification_summary"]
            )
            if summary_entries != category_entries or summary_bytes != category_bytes:
                fail(f"{category_name} subclassification totals differ")
        total_entries += category_entries
        total_bytes += category_bytes

    flat_ranges.sort()
    next_address = region_start
    for start, end, category_name, _ in flat_ranges:
        if start != next_address:
            fail(
                f"Updater ownership gap/overlap before {start:#x} "
                f"({category_name}); expected {next_address:#x}"
            )
        next_address = end
    if next_address != region_end:
        fail("Updater ownership partition does not reach the .text end")
    if set(entry_owners) != address_set:
        missing = sorted(address_set - set(entry_owners))
        fail(f"Updater ownership partition misses entries: {missing}")

    # An ownership boundary may never cut a recovered function extent. This is
    # also the proof that every byte represented by the inventory is assigned
    # to exactly one owner, not merely that every entry address is listed.
    for index, address in enumerate(addresses):
        extent_end = addresses[index + 1] if index + 1 < len(addresses) else region_end
        owner = entry_owners[address]
        containers = [
            item
            for item in flat_ranges
            if item[2] == owner and item[0] <= address and extent_end <= item[1]
        ]
        if len(containers) != 1:
            fail(f"Updater function extent {address:#x}-{extent_end:#x} is split")

    product_evidence = ownership["product_application_entries"]
    product_evidence_addresses = [
        parse_address(item["address"]) for item in product_evidence
    ]
    if len(product_evidence_addresses) != len(set(product_evidence_addresses)):
        fail("Updater product evidence contains duplicate entries")
    product_owned_addresses = {
        address
        for address, owner in entry_owners.items()
        if owner == "product_application"
    }
    if set(product_evidence_addresses) != product_owned_addresses:
        fail("Updater product evidence does not exactly cover product-owned entries")

    templates = ownership["compiler_emitted_template_partition"]
    template_entries = 0
    template_bytes = 0
    template_addresses: set[int] = set()
    platform_range_pairs = {
        (start, end)
        for start, end, owner, _ in flat_ranges
        if owner == "platform_toolchain"
    }
    for item in templates["ranges"]:
        start = parse_address(item["start"])
        end = parse_address(item["end_exclusive"])
        if (start, end) not in platform_range_pairs:
            fail(f"compiler-emitted template range is not platform-owned at {start:#x}")
        entries = {address for address in addresses if start <= address < end}
        if len(entries) != item["inventory_entry_count"]:
            fail(f"compiler-emitted template entry count differs at {start:#x}")
        if end - start != item["size"]:
            fail(f"compiler-emitted template size differs at {start:#x}")
        if template_addresses & entries:
            fail(f"compiler-emitted template ranges overlap at {start:#x}")
        template_addresses |= entries
        template_entries += len(entries)
        template_bytes += end - start
    if template_entries != templates["inventory_entry_count"]:
        fail("compiler-emitted template aggregate entry count differs")
    if template_bytes != templates["size"]:
        fail("compiler-emitted template aggregate byte count differs")
    if templates["classification"] != "platform_toolchain":
        fail("compiler-emitted templates are not classified as platform/toolchain")

    for item in ownership["miniz_key_matches"]:
        address = parse_address(item["address"])
        if address not in address_set:
            fail(f"miniz key match is not a recovered entry: {address:#x}")
        if entry_owners[address] != "upstream_miniz":
            fail(f"miniz key match is not upstream-owned: {address:#x}")

    miniz_ranges = categories["upstream_miniz"]["ranges"]
    if len(miniz_ranges) != 1:
        fail("Updater miniz ownership must remain one contiguous range")
    miniz_start = parse_address(miniz_ranges[0]["start"])
    miniz_end = parse_address(miniz_ranges[0]["end_exclusive"])
    if (miniz_start, miniz_end) != (0x98008D68, 0x9800CE38):
        fail("Updater refined miniz boundary differs")

    refinements = ownership["boundary_refinements"]
    if len(refinements) != 1:
        fail("Updater miniz/CRT boundary refinement record differs")
    refinement = refinements[0]
    coarse = refinement["coarse_input_range"]
    source_attribution = evidence_documents[
        "analysis/xex/Updater-source-attribution.json"
    ]
    if (
        parse_address(coarse["start"]) != miniz_start
        or parse_address(coarse["end_exclusive"]) != 0x9800D130
        or coarse["classification"] != "miniz compression and ZIP library"
    ):
        fail("Updater historical coarse miniz boundary record differs")
    if not any(
        parse_address(item["start"]) == miniz_start
        and parse_address(item["end_exclusive"]) == miniz_end
        and item["classification"] == "miniz compression and ZIP library"
        for item in source_attribution["ranges"]
    ):
        fail("Updater source attribution does not use the refined Miniz boundary")
    if parse_address(refinement["refined_upstream_end_exclusive"]) != miniz_end:
        fail("Updater refined miniz end metadata differs")
    moved = refinement["moved_to_platform_toolchain"]
    moved_start = parse_address(moved["start"])
    moved_end = parse_address(moved["end_exclusive"])
    moved_addresses = [address for address in addresses if moved_start <= address < moved_end]
    if (moved_start, moved_end) != (miniz_end, parse_address(image_info["entry_point"])):
        fail("Updater miniz-to-entry refinement span differs")
    if moved_end - moved_start != moved["size"]:
        fail("Updater miniz-to-entry refinement byte count differs")
    if len(moved_addresses) != moved["inventory_entry_count"]:
        fail("Updater miniz-to-entry refinement entry count differs")
    if any(entry_owners[address] != "platform_toolchain" for address in moved_addresses):
        fail("Updater miniz-to-entry refinement is not wholly platform-owned")

    entry_address = parse_address(image_info["entry_point"])
    entry_word = struct.unpack_from(">I", image, entry_address - image_base)[0]
    if entry_word >> 26 != 18 or entry_word & 1:
        fail("Updater PE entry is no longer the expected unconditional PPC branch")
    displacement = entry_word & 0x03FFFFFC
    if displacement & 0x02000000:
        displacement -= 0x04000000
    branch_target = displacement if entry_word & 2 else entry_address + displacement
    if entry_word != 0x4BFFFEA8 or branch_target != 0x9800CFD8:
        fail("Updater PE entry no longer branches to the refined CRT lifecycle block")

    callgraph = evidence_documents["analysis/xex/Updater-callgraph.json"]
    edge_pairs = {
        (parse_address(edge["caller_address"]), parse_address(edge["target_address"]))
        for edge in callgraph["edges"]
    }
    for edge in (
        (0x9800CDD8, 0x9800BEE0),
        (0x9800CDD8, 0x9800C8D8),
        (0x9800CFD8, 0x9800CE38),
        (0x9800CFD8, 0x98006C70),
    ):
        if edge not in edge_pairs:
            fail(f"Updater boundary-evidence call edge is absent: {edge[0]:#x}->{edge[1]:#x}")

    symbols = evidence_documents["analysis/xex/Updater-symbols.json"]
    for symbol in symbols["symbols"]:
        address = parse_address(symbol["address"])
        expected_owner = (
            "upstream_miniz"
            if symbol["semantic_name"].startswith("mz_")
            else "product_application"
        )
        if entry_owners.get(address) != expected_owner:
            fail(
                f"semantic symbol ownership differs at {address:#x}: "
                f"expected {expected_owner}"
            )

    import_start, import_end = 0x9801A720, 0x9801AA10
    import_addresses = [address for address in addresses if import_start <= address < import_end]
    if len(import_addresses) != 47:
        fail("Updater import-thunk entry count differs")
    for address in import_addresses:
        index = addresses.index(address)
        extent_end = addresses[index + 1]
        if extent_end - address != 16 or entry_owners[address] != "platform_toolchain":
            fail(f"Updater import thunk shape/owner differs at {address:#x}")

    metadata_start = 0x9801AA10
    metadata_addresses = {address for address in addresses if address >= metadata_start}
    if metadata_addresses != {0x9801AA10, 0x9801AA30}:
        fail("Updater terminal metadata inventory entries differ")
    if any(entry_owners[address] != "platform_toolchain" for address in metadata_addresses):
        fail("Updater terminal linker metadata is not platform-owned")
    terminal_words = struct.unpack_from(">IIII", image, 0x9801AA30 - image_base)
    if terminal_words != (0x00009800, 0x00000001, 0x00000001, 0x00007758):
        fail("Updater terminal linker metadata changed")

    totals = ownership["totals"]
    if total_entries != totals["inventory_entry_count"] or total_entries != len(functions):
        fail("Updater ownership total entry count differs")
    if total_bytes != totals["instruction_size"] or total_bytes != len(region_bytes):
        fail("Updater ownership total byte count differs")
    if totals["category_count"] != len(categories):
        fail("Updater ownership category count differs")
    product_category = categories["product_application"]
    if totals["product_application_entry_count"] != product_category["inventory_entry_count"]:
        fail("Updater product entry total differs")
    if totals["product_application_size"] != product_category["size"]:
        fail("Updater product byte total differs")
    if totals["non_product_entry_count"] != total_entries - product_category["inventory_entry_count"]:
        fail("Updater non-product entry total differs")
    if totals["non_product_size"] != total_bytes - product_category["size"]:
        fail("Updater non-product byte total differs")
    if categories["unknown"]["inventory_entry_count"] != 0 or categories["unknown"]["size"] != 0:
        fail("Updater unknown bucket is no longer empty")

    summary = ", ".join(
        f"{name}={category['inventory_entry_count']}/{category['size']}"
        for name, category in categories.items()
    )
    print(
        "Updater function ownership verified: "
        f"{total_entries} entries, {total_bytes} .text bytes; {summary}; "
        f"compiler_emitted_templates={template_entries}/{template_bytes}; "
        f"miniz_end={miniz_end:#x}"
    )


if __name__ == "__main__":
    main()
