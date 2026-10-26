#!/usr/bin/env python3
"""Compare Peer Pressure's BadUpdate payloads with official v1.3 release files."""

from __future__ import annotations

import argparse
import hashlib
import json
from difflib import SequenceMatcher
from pathlib import Path
from zipfile import ZipFile


ROOT = Path(__file__).resolve().parent.parent
PREFIX = "Rock Band Blitz/BadUpdatePayload/"
COMPONENTS = (
    ("BadUpdateExploit-2ndStage.bin", "BadUpdatePayload/BadUpdateExploit-2ndStage.bin"),
    ("BadUpdateExploit-3rdStage.bin", "BadUpdatePayload/BadUpdateExploit-3rdStage.bin"),
    ("BadUpdateExploit-4thStage.bin", "BadUpdatePayload/BadUpdateExploit-4thStage.bin"),
    ("BadUpdateExploit-Data.bin", "BadUpdatePayload/BadUpdateExploit-Data.bin"),
    ("update_data.bin", "BadUpdatePayload/update_data.bin"),
    ("xke_update.bin", "BadUpdatePayload/xke_update.bin"),
    ("bootanim.xex", "BadUpdatePayload/bootanim.xex"),
    ("default.xex", "BadUpdatePayload/default.xex"),
    (
        "../Content/0000000000000000/5841122D/00000001/DaTArrest_Xbox360BadUpdate",
        "Content/0000000000000000/5841122D/00000001/DaTArrest_Xbox360BadUpdate",
    ),
)
PROLOGUE = bytes.fromhex("7d8802a6")  # mflr r12


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def matching_blocks(old: bytes, new: bytes, minimum: int = 16) -> list[dict[str, object]]:
    matcher = SequenceMatcher(None, old, new, autojunk=False)
    return [
        {
            "upstream_offset": f"0x{block.a:08x}",
            "peer_pressure_offset": f"0x{block.b:08x}",
            "size": block.size,
        }
        for block in matcher.get_matching_blocks()
        if block.size >= minimum
    ]


def prologues(data: bytes) -> list[int]:
    result = []
    cursor = 0
    while True:
        found = data.find(PROLOGUE, cursor)
        if found < 0:
            return result
        if found % 4 == 0:
            result.append(found)
        cursor = found + 1


def normalize_words(data: bytes, start: int, end: int) -> list[int]:
    words = []
    for offset in range(start, end - ((end - start) % 4), 4):
        word = int.from_bytes(data[offset : offset + 4], "big")
        opcode = word >> 26
        if opcode == 18:  # b / bl: discard relative target, retain AA and LK
            word &= 0xFC000003
        elif opcode == 16:  # conditional branch: discard relative target
            word &= 0xFFFF0003
        words.append(word)
    return words


def function_map(old: bytes, new: bytes) -> list[dict[str, object]]:
    old_starts = prologues(old)
    new_starts = prologues(new)
    old_ranges = list(zip(old_starts, old_starts[1:] + [len(old)]))
    new_ranges = list(zip(new_starts, new_starts[1:] + [len(new)]))
    result = []
    for new_start, new_end in new_ranges:
        new_words = normalize_words(new, new_start, new_end)
        candidates = []
        for old_start, old_end in old_ranges:
            old_words = normalize_words(old, old_start, old_end)
            similarity = SequenceMatcher(None, old_words, new_words, autojunk=False).ratio()
            candidates.append((similarity, old_start, old_end))
        similarity, old_start, old_end = max(candidates)
        result.append(
            {
                "peer_pressure_range": f"0x{new_start:04x}-0x{new_end:04x}",
                "best_upstream_range": f"0x{old_start:04x}-0x{old_end:04x}",
                "normalized_instruction_similarity": round(similarity, 6),
            }
        )
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("release_zip", type=Path)
    parser.add_argument("-o", "--output", type=Path, required=True)
    args = parser.parse_args()

    release_bytes = args.release_zip.read_bytes()
    results = []
    with ZipFile(args.release_zip) as archive:
        if archive.testzip() is not None:
            raise SystemExit("release archive failed CRC validation")
        for archive_name, local_name in COMPONENTS:
            name = Path(local_name).name
            member_name = (
                "Rock Band Blitz/" + archive_name[3:]
                if archive_name.startswith("../")
                else PREFIX + archive_name
            )
            upstream = archive.read(member_name)
            local = (ROOT / local_name).read_bytes()
            component = {
                "name": name,
                "peer_pressure_path": local_name,
                "upstream_size": len(upstream),
                "peer_pressure_size": len(local),
                "upstream_sha256": sha256(upstream),
                "peer_pressure_sha256": sha256(local),
                "byte_identical": upstream == local,
            }
            if name == "BadUpdateExploit-3rdStage.bin" and upstream != local:
                blocks = matching_blocks(upstream, local)
                component["matching_blocks_min_16_bytes"] = blocks
                component["matching_block_bytes"] = sum(block["size"] for block in blocks)
                component["function_map"] = function_map(upstream, local)
            results.append(component)

    report = {
        "baseline": "grimdoomer/Xbox360BadUpdate release BadUpdate-v1.3",
        "source_commit": "94f9215a00382c218288ea230d5f81d73b7f2f9b",
        "release_archive": args.release_zip.name,
        "release_archive_sha256": sha256(release_bytes),
        "components": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
