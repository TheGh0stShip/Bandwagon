#!/usr/bin/env python3
"""Verify BootXell's exhaustive product/upstream/platform ownership split."""

from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/BootXell-function-ownership.json"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def normalized_ppc(payload: bytes) -> bytes:
    """Mask only fields known to change under PPC image relocation.

    Opcode 18 carries a relative branch displacement. Opcodes 14 and 15 are
    addi/addis, which this evidence uses for image-relative address material.
    Raw hashes remain authoritative; this signature only proves identical
    instruction/register structure across independently linked XEX images.
    """

    if len(payload) % 4:
        fail("relocation-normalized evidence is not word aligned")
    words: list[int] = []
    for (word,) in struct.iter_unpack(">I", payload):
        opcode = word >> 26
        if opcode == 18:
            word &= 0xFC000003
        elif opcode in (14, 15):
            word &= 0xFFFF0000
        words.append(word)
    return b"".join(struct.pack(">I", word) for word in words)


def image_slice(image: bytes, base: int, start: int, size: int) -> bytes:
    offset = start - base
    if offset < 0 or offset + size > len(image):
        fail(f"range {start:#x}+{size:#x} is outside its image")
    return image[offset : offset + size]


def main() -> None:
    ownership = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if ownership.get("schema") != "bandwagon-bootxell-function-ownership-v1":
        fail("unexpected BootXell ownership schema")

    image_info = ownership["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if digest(image) != image_info["sha256"]:
        fail("BootXell image hash differs")
    base = int(image_info["base_address"], 0)

    inventory_info = ownership["function_inventory"]
    inventory_bytes = (ROOT / inventory_info["path"]).read_bytes()
    if digest(inventory_bytes) != inventory_info["sha256"]:
        fail("BootXell function inventory hash differs")
    functions = json.loads(inventory_bytes)["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("BootXell function inventory entry count differs")
    function_addresses = [int(function["address"], 0) for function in functions]
    function_address_set = set(function_addresses)
    if len(function_address_set) != len(function_addresses):
        fail("BootXell function inventory contains duplicate addresses")

    region = ownership["instruction_region"]
    region_start = int(region["start"], 0)
    region_end = int(region["end_exclusive"], 0)
    region_bytes = image_slice(image, base, region_start, region_end - region_start)
    if region_end - region_start != region["size"]:
        fail("BootXell instruction-region size metadata differs")
    if len(region_bytes) != region["size"] or digest(region_bytes) != region["sha256"]:
        fail("BootXell instruction-region bytes differ")

    for source_name, source in ownership["upstream_sources"].items():
        source_root = ROOT / source["vendored_root"]
        for file_info in source["files"]:
            source_path = source_root / file_info["path"]
            if digest(source_path.read_bytes()) != file_info["sha256"]:
                fail(f"{source_name} source hash differs: {file_info['path']}")

    required_categories = {
        "peer_product_adapter_or_policy",
        "upstream_inih_r62",
        "upstream_miniz_311",
        "platform_toolchain",
    }
    categories = ownership["categories"]
    if set(categories) != required_categories:
        fail("BootXell ownership category set differs")

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
                fail(f"{category_name} range is not inventory aligned at {start:#x}")
            if start < region_start or end > region_end or end <= start:
                fail(f"invalid {category_name} range at {start:#x}")
            payload = image_slice(image, base, start, end - start)
            if len(payload) != item["size"] or end - start != item["size"]:
                fail(f"invalid {category_name} range size at {start:#x}")
            if digest(payload) != item["sha256"]:
                fail(f"{category_name} range bytes changed at {start:#x}")

            addresses = [address for address in function_addresses if start <= address < end]
            if len(addresses) != item["inventory_entry_count"]:
                fail(f"{category_name} inventory count changed at {start:#x}")
            for address in addresses:
                if address in entry_owners:
                    fail(f"function {address:#x} has multiple owners")
                entry_owners[address] = category_name
            category_ranges.append((start, end, item))
            flat_ranges.append((start, end, category_name, item))

        category_ranges.sort()
        concatenated = b"".join(
            image_slice(image, base, start, end - start)
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
                f"ownership gap/overlap before {start:#x} ({category_name}); "
                f"expected {next_address:#x}"
            )
        next_address = end
    if next_address != region_end:
        fail("BootXell ownership partition does not reach the region end")
    if set(entry_owners) != function_address_set:
        missing = sorted(function_address_set - set(entry_owners))
        fail(f"BootXell ownership partition misses entries: {missing}")

    for index, start in enumerate(function_addresses):
        end = function_addresses[index + 1] if index + 1 < len(functions) else region_end
        owner = entry_owners[start]
        containing = [
            item
            for item in flat_ranges
            if item[2] == owner and item[0] <= start and end <= item[1]
        ]
        if len(containing) != 1:
            fail(f"function extent {start:#x}-{end:#x} is split or uncovered")

    evidence = ownership["relocation_normalized_evidence"]
    reference_images: dict[str, tuple[bytes, int]] = {}
    for component, reference in evidence["references"].items():
        reference_image = (ROOT / reference["path"]).read_bytes()
        if digest(reference_image) != reference["sha256"]:
            fail(f"{component} reference image hash differs")
        reference_images[component] = (
            reference_image,
            int(reference["base_address"], 0),
        )

    for match in evidence["matches"]:
        size = int(match["size"])
        if size % 4 or match["word_count"] != size // 4:
            fail(f"invalid evidence word count for {match['name']}")
        bootxell_payload = image_slice(
            image, base, int(match["bootxell_start"], 0), size
        )
        reference_image, reference_base = reference_images[
            match["reference_component"]
        ]
        reference_payload = image_slice(
            reference_image,
            reference_base,
            int(match["reference_start"], 0),
            size,
        )
        bootxell_words = list(struct.iter_unpack(">I", bootxell_payload))
        reference_words = list(struct.iter_unpack(">I", reference_payload))
        identical_words = sum(
            left == right for left, right in zip(bootxell_words, reference_words)
        )
        if identical_words != match["identical_word_count"]:
            fail(f"raw structural match count differs for {match['name']}")
        bootxell_normalized = normalized_ppc(bootxell_payload)
        reference_normalized = normalized_ppc(reference_payload)
        if bootxell_normalized != reference_normalized:
            fail(f"relocation-normalized bodies differ for {match['name']}")
        if digest(bootxell_normalized) != match["normalized_sha256"]:
            fail(f"normalized signature differs for {match['name']}")

    recovery = ownership["source_recovery"]
    coverage_info = recovery["existing_coverage"]
    coverage_bytes = (ROOT / coverage_info["path"]).read_bytes()
    if digest(coverage_bytes) != coverage_info["sha256"]:
        fail("BootXell source-coverage manifest hash differs")
    coverage_entries = json.loads(coverage_bytes)["functions"]
    covered_addresses = {int(item["address"], 0) for item in coverage_entries}
    if len(covered_addresses) != coverage_info["inventory_entry_count"]:
        fail("BootXell covered-product entry count differs")
    if sum(int(item["size"]) for item in coverage_entries) != coverage_info["size"]:
        fail("BootXell covered-product byte count differs")

    product_addresses = {
        address
        for address, owner in entry_owners.items()
        if owner == "peer_product_adapter_or_policy"
    }
    if not covered_addresses <= product_addresses:
        fail("source coverage includes a non-product ownership entry")
    remaining_entries = recovery["remaining_product_entries"]
    remaining_addresses = {int(item["address"], 0) for item in remaining_entries}
    if remaining_addresses != product_addresses - covered_addresses:
        fail("remaining BootXell product-entry set differs")
    remaining_size = 0
    address_to_index = {address: index for index, address in enumerate(function_addresses)}
    for item in remaining_entries:
        start = int(item["address"], 0)
        index = address_to_index[start]
        end = function_addresses[index + 1] if index + 1 < len(functions) else region_end
        payload = image_slice(image, base, start, end - start)
        if len(payload) != item["size"] or digest(payload) != item["sha256"]:
            fail(f"remaining product entry differs at {start:#x}")
        remaining_size += len(payload)
    if len(remaining_entries) != recovery["remaining_product_entry_count"]:
        fail("remaining BootXell product-entry total differs")
    if remaining_size != recovery["remaining_product_size"]:
        fail("remaining BootXell product-byte total differs")

    totals = ownership["totals"]
    if total_entries != totals["inventory_entry_count"] or total_entries != len(functions):
        fail("BootXell total entry count differs")
    if total_bytes != totals["instruction_size"] or total_bytes != len(region_bytes):
        fail("BootXell total instruction size differs")
    if totals["category_count"] != len(categories):
        fail("BootXell category count differs")

    product = categories["peer_product_adapter_or_policy"]
    platform = categories["platform_toolchain"]
    upstream_entries = (
        categories["upstream_inih_r62"]["inventory_entry_count"]
        + categories["upstream_miniz_311"]["inventory_entry_count"]
    )
    upstream_size = (
        categories["upstream_inih_r62"]["size"]
        + categories["upstream_miniz_311"]["size"]
    )
    expected = {
        "product_entry_count": product["inventory_entry_count"],
        "product_size": product["size"],
        "upstream_entry_count": upstream_entries,
        "upstream_size": upstream_size,
        "platform_toolchain_entry_count": platform["inventory_entry_count"],
        "platform_toolchain_size": platform["size"],
    }
    for key, value in expected.items():
        if totals[key] != value:
            fail(f"BootXell {key} differs")
    if totals["unknown_entry_count"] != 0 or totals["unknown_size"] != 0:
        fail("BootXell ownership must not retain an unknown bucket")

    print(
        "BootXell function ownership verified: "
        f"{total_entries} entries/{total_bytes} bytes; "
        f"product={product['inventory_entry_count']}/{product['size']}, "
        f"upstream={upstream_entries}/{upstream_size}, "
        f"platform={platform['inventory_entry_count']}/{platform['size']}; "
        f"remaining_product={len(remaining_entries)}/{remaining_size}"
    )


if __name__ == "__main__":
    main()
