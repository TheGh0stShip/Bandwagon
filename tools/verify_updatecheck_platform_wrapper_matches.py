#!/usr/bin/env python3
"""Verify positive platform provenance for UpdateCheck's five wrapper entries."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/UpdateCheck-platform-wrapper-matches.json"
EXPECTED_SCHEMA = "bandwagon-updatecheck-platform-wrapper-matches-v1"


def fail(message: str) -> None:
    raise SystemExit(message)


def digest(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def read_bound_file(item: dict[str, object], label: str) -> bytes:
    path = ROOT / str(item["path"])
    if not path.is_file():
        fail(f"missing {label}: {path}")
    payload = path.read_bytes()
    if digest(payload) != item["sha256"]:
        fail(f"{label} hash differs: {path}")
    return payload


def normalized_wrapper_bytes(payload: bytes) -> bytes:
    """Mask only link-layout operands used by the matched XDK wrappers."""

    if len(payload) % 4:
        fail("platform-wrapper range is not word aligned")
    words = [
        int.from_bytes(payload[offset : offset + 4], "big")
        for offset in range(0, len(payload), 4)
    ]
    # A lis-created image pointer remains live for at most the following ten
    # instructions in these small wrappers. This keeps ordinary loads exact.
    pointer_registers: dict[int, int] = {}
    normalized: list[int] = []
    for index, word in enumerate(words):
        opcode = word >> 26
        value = word
        if opcode == 18:  # b / bl: retain opcode, AA, and LK.
            value &= 0xFC000003
        elif (
            opcode == 15
            and ((word >> 16) & 0x1F) == 0
            and 0x8000 <= (word & 0xFFFF) <= 0x9FFF
        ):  # lis rD,image_high
            pointer_registers[(word >> 21) & 0x1F] = index + 10
            value &= 0xFFFF0000
        elif opcode in (14, 32):  # addi/lwz through a live image pointer.
            source_register = (word >> 16) & 0x1F
            if pointer_registers.get(source_register, -1) >= index:
                target_register = (word >> 21) & 0x1F
                pointer_registers[target_register] = index + 10
                value &= 0xFFFF0000
        elif opcode == 24:  # ori through a live image pointer.
            source_register = (word >> 21) & 0x1F
            if pointer_registers.get(source_register, -1) >= index:
                value &= 0xFFFF0000
        normalized.append(value)
    return b"".join(word.to_bytes(4, "big") for word in normalized)


def direct_branch_target(address: int, word: int) -> int:
    if word >> 26 != 18:
        fail(f"expected direct PPC branch at {address:#x}")
    displacement = word & 0x03FFFFFC
    if displacement & 0x02000000:
        displacement -= 0x04000000
    absolute = bool(word & 0x2)
    return (displacement if absolute else address + displacement) & 0xFFFFFFFF


def slice_at(
    image: bytes,
    base: int,
    address: int,
    size: int,
    declared_offset: int,
    label: str,
) -> bytes:
    offset = address - base
    if offset != declared_offset:
        fail(f"{label} virtual/file offset mapping differs")
    payload = image[offset : offset + size]
    if len(payload) != size:
        fail(f"{label} exceeds its image")
    return payload


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if manifest.get("schema") != EXPECTED_SCHEMA:
        fail("unexpected UpdateCheck platform-wrapper schema")
    if manifest.get("status") != "positive_platform_provenance":
        fail("UpdateCheck platform-wrapper evidence is not positive")

    subject_info = manifest["subject_image"]
    subject = read_bound_file(subject_info, "UpdateCheck image")
    if len(subject) != subject_info["size"]:
        fail("UpdateCheck image size differs")
    subject_base = int(subject_info["base_address"], 0)

    reference_info = manifest["reference_image"]
    reference = read_bound_file(reference_info, "clean bootanim reference image")
    if len(reference) != reference_info["size"]:
        fail("clean bootanim reference-image size differs")
    reference_base = int(reference_info["base_address"], 0)

    inventory_info = manifest["function_inventory"]
    inventory_payload = read_bound_file(inventory_info, "UpdateCheck function inventory")
    inventory = json.loads(inventory_payload)
    functions = inventory["functions"]
    if len(functions) != inventory_info["entry_count"]:
        fail("UpdateCheck function-inventory count differs")
    if inventory.get("source_sha256") != subject_info["sha256"]:
        fail("UpdateCheck inventory is bound to a different image")
    functions_by_address = {
        int(item["address"], 0): item for item in functions
    }

    provenance = manifest["reference_provenance"]
    component_info = provenance["component_inventory"]
    component_inventory = json.loads(
        read_bound_file(component_info, "component inventory")
    )
    candidates = [
        item
        for item in component_inventory["entries"]
        if item["path"] == component_info["expected_path"]
    ]
    if len(candidates) != 1:
        fail("clean bootanim component-inventory entry differs")
    component = candidates[0]
    for field in ("size", "sha256", "category", "status"):
        if component[field] != component_info[f"expected_{field}"]:
            fail(f"clean bootanim component provenance differs: {field}")

    readme_info = provenance["public_baseline_readme"]
    readme_payload = read_bound_file(readme_info, "BadUpdate public README")
    if len(readme_payload) != readme_info["size"]:
        fail("BadUpdate public README size differs")
    if readme_info["required_text"] not in readme_payload.decode("utf-8"):
        fail("BadUpdate clean-bootanim provenance text differs")

    containers: list[tuple[int, int]] = []
    for entry in manifest["inventory_entries"]:
        address = int(entry["address"], 0)
        end = int(entry["end_exclusive"], 0)
        size = int(entry["size"])
        if end - address != size:
            fail(f"wrapper-container size differs at {address:#x}")
        payload = slice_at(
            subject,
            subject_base,
            address,
            size,
            int(entry["file_offset"], 0),
            f"wrapper container {address:#x}",
        )
        if digest(payload) != entry["sha256"]:
            fail(f"wrapper-container bytes differ at {address:#x}")
        function = functions_by_address.get(address)
        if function is None or int(function["size_to_next_symbol"]) != size:
            fail(f"wrapper container left its inventory extent at {address:#x}")
        containers.append((address, end))

    expected_containers = [
        (0x91A0D9F8, 0x91A0DA98),
        (0x91A0DA98, 0x91A0DAF8),
        (0x91A0DAF8, 0x91A0DB78),
        (0x91A0DB78, 0x91A0DBD0),
        (0x91A0DBD0, 0x91A0DC58),
    ]
    if containers != expected_containers:
        fail("UpdateCheck wrapper-container inventory differs")

    covered: list[tuple[int, int, str]] = []
    for match in manifest["normalized_matches"]:
        subject_address = int(match["subject_address"], 0)
        reference_address = int(match["reference_address"], 0)
        size = int(match["size"])
        subject_payload = slice_at(
            subject,
            subject_base,
            subject_address,
            size,
            int(match["subject_file_offset"], 0),
            f"subject match {match['id']}",
        )
        reference_payload = slice_at(
            reference,
            reference_base,
            reference_address,
            size,
            int(match["reference_file_offset"], 0),
            f"reference match {match['id']}",
        )
        if digest(subject_payload) != match["subject_sha256"]:
            fail(f"subject wrapper bytes differ: {match['id']}")
        if digest(reference_payload) != match["reference_sha256"]:
            fail(f"reference wrapper bytes differ: {match['id']}")
        subject_normalized = normalized_wrapper_bytes(subject_payload)
        reference_normalized = normalized_wrapper_bytes(reference_payload)
        if subject_normalized != reference_normalized:
            fail(f"normalized platform-wrapper match differs: {match['id']}")
        if digest(subject_normalized) != match["normalized_sha256"]:
            fail(f"normalized platform-wrapper hash differs: {match['id']}")
        covered.append((subject_address, subject_address + size, "match"))

    for thunk in manifest["direct_thunks"]:
        address = int(thunk["address"], 0)
        size = int(thunk["size"])
        if size != 4:
            fail(f"direct thunk is not one instruction at {address:#x}")
        payload = slice_at(
            subject,
            subject_base,
            address,
            size,
            int(thunk["file_offset"], 0),
            f"direct thunk {address:#x}",
        )
        if digest(payload) != thunk["sha256"]:
            fail(f"direct-thunk bytes differ at {address:#x}")
        word = int.from_bytes(payload, "big")
        if direct_branch_target(address, word) != int(thunk["target_address"], 0):
            fail(f"direct-thunk target differs at {address:#x}")
        if "iat_address" in thunk:
            iat_address = int(thunk["iat_address"], 0)
            iat_payload = slice_at(
                subject,
                subject_base,
                iat_address,
                4,
                iat_address - subject_base,
                f"IAT record for {address:#x}",
            )
            if int.from_bytes(iat_payload, "big") != int(thunk["iat_record"], 0):
                fail(f"IAT record differs for thunk at {address:#x}")
            if int(thunk["ordinal"], 0) != (int(thunk["iat_record"], 0) & 0xFFFF):
                fail(f"IAT ordinal differs for thunk at {address:#x}")
        covered.append((address, address + size, "thunk"))

    for padding in manifest["padding"]:
        address = int(padding["address"], 0)
        size = int(padding["size"])
        payload = slice_at(
            subject,
            subject_base,
            address,
            size,
            int(padding["file_offset"], 0),
            f"wrapper padding {address:#x}",
        )
        if digest(payload) != padding["sha256"] or any(payload):
            fail(f"wrapper padding differs at {address:#x}")
        covered.append((address, address + size, "padding"))

    covered.sort()
    cursor = expected_containers[0][0]
    for start, end, kind in covered:
        if start != cursor:
            fail(
                f"wrapper evidence gap/overlap before {start:#x} ({kind}); "
                f"expected {cursor:#x}"
            )
        cursor = end
    if cursor != expected_containers[-1][1]:
        fail("wrapper evidence does not cover the complete 608-byte range")

    summary = manifest["summary"]
    match_bytes = sum(int(item["size"]) for item in manifest["normalized_matches"])
    thunk_bytes = sum(int(item["size"]) for item in manifest["direct_thunks"])
    padding_bytes = sum(int(item["size"]) for item in manifest["padding"])
    if summary["inventory_entry_count"] != len(containers):
        fail("platform-wrapper entry summary differs")
    if summary["inventory_bytes"] != cursor - expected_containers[0][0]:
        fail("platform-wrapper byte summary differs")
    if summary["normalized_match_count"] != len(manifest["normalized_matches"]):
        fail("platform-wrapper match-count summary differs")
    if summary["normalized_match_bytes"] != match_bytes:
        fail("platform-wrapper match-byte summary differs")
    if summary["direct_thunk_count"] != len(manifest["direct_thunks"]):
        fail("platform-wrapper thunk-count summary differs")
    if summary["direct_thunk_bytes"] != thunk_bytes:
        fail("platform-wrapper thunk-byte summary differs")
    if summary["padding_count"] != len(manifest["padding"]):
        fail("platform-wrapper padding-count summary differs")
    if summary["padding_bytes"] != padding_bytes:
        fail("platform-wrapper padding-byte summary differs")
    explained = match_bytes + thunk_bytes + padding_bytes
    if explained != summary["inventory_bytes"]:
        fail("platform-wrapper evidence byte accounting differs")
    if summary["remaining_unexplained_bytes"] != 0:
        fail("platform-wrapper evidence still has unexplained bytes")

    print(
        "UpdateCheck platform-wrapper provenance verified: "
        f"{len(containers)} entries / {summary['inventory_bytes']} bytes; "
        f"{len(manifest['normalized_matches'])} clean-bootanim matches / "
        f"{match_bytes} bytes, {len(manifest['direct_thunks'])} thunks / "
        f"{thunk_bytes} bytes, {len(manifest['padding'])} padding words / "
        f"{padding_bytes} bytes; unexplained=0"
    )


if __name__ == "__main__":
    main()
