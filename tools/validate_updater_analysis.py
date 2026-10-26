#!/usr/bin/env python3
"""Validate cross-file invariants in the generated Updater analysis."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


REQUIRED_APPLICATION_EDGES = {
    (0x98007758, 0x98006990),  # RunUpdate -> SoftmodData_CollectFilesByPrefix
    (0x98007758, 0x98007BC0),  # RunUpdate -> FileExists
    (0x98007758, 0x98007EF0),  # RunUpdate -> EnsureParentDirectories
    (0x98007758, 0x98006740),  # RunUpdate -> SoftmodData_ExtractFileToPath
    (0x98007758, 0x98007680),  # RunUpdate -> AppendPhaseTwoPath
    (0x98007758, 0x98006F60),  # RunUpdate -> WritePhaseTwoFileList
}


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("analysis_dir", type=Path)
    parser.add_argument("image", type=Path)
    parser.add_argument("--text-address", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--text-size", type=lambda value: int(value, 0), required=True)
    args = parser.parse_args()

    digest = hashlib.sha256(args.image.read_bytes()).hexdigest()
    functions_doc = load(args.analysis_dir / "Updater-functions.json")
    xrefs = load(args.analysis_dir / "Updater-string-xrefs.json")
    calls = load(args.analysis_dir / "Updater-callgraph.json")
    symbols = load(args.analysis_dir / "Updater-symbols.json")
    attribution = load(args.analysis_dir / "Updater-source-attribution.json")

    for name, document in (("string xrefs", xrefs), ("call graph", calls), ("symbols", symbols), ("attribution", attribution)):
        if document["image_sha256"] != digest:
            raise SystemExit(f"{name} image digest mismatch")

    functions = functions_doc["functions"]
    addresses = {int(item["address"], 0) for item in functions}
    if len(addresses) != len(functions):
        raise SystemExit("duplicate function addresses")
    for symbol in symbols["symbols"]:
        if int(symbol["address"], 0) not in addresses:
            raise SystemExit(f"semantic symbol is not a recovered function: {symbol['address']}")
    if len({item["address"] for item in symbols["symbols"]}) != len(symbols["symbols"]):
        raise SystemExit("duplicate semantic symbol addresses")
    for reference in xrefs["references"]:
        if int(reference["function_address"], 0) not in addresses:
            raise SystemExit(f"string xref caller is not mapped: {reference['function_address']}")
    for edge in calls["edges"]:
        if int(edge["caller_address"], 0) not in addresses:
            raise SystemExit(f"call edge caller is not mapped: {edge['caller_address']}")
    edge_pairs = {(int(edge["caller_address"], 0), int(edge["target_address"], 0)) for edge in calls["edges"]}
    missing_edges = REQUIRED_APPLICATION_EDGES - edge_pairs
    if missing_edges:
        rendered = ", ".join(f"0x{caller:08x}->0x{target:08x}" for caller, target in sorted(missing_edges))
        raise SystemExit(f"Updater application pipeline edges changed: {rendered}")

    cursor = args.text_address
    for item in attribution["ranges"]:
        start = int(item["start"], 0)
        end = int(item["end_exclusive"], 0)
        if start != cursor or end <= start:
            raise SystemExit(f"attribution gap, overlap, or empty range at {item['start']}")
        cursor = end
    expected_end = args.text_address + args.text_size
    if cursor != expected_end:
        raise SystemExit(f"attribution ends at 0x{cursor:08x}, expected 0x{expected_end:08x}")

    print(
        f"Updater analysis valid: {len(functions)} functions, {len(calls['edges'])} calls, "
        f"{len(xrefs['references'])} string xrefs, {len(symbols['symbols'])} semantic symbols, "
        f"{len(attribution['ranges'])} contiguous attribution ranges"
    )


if __name__ == "__main__":
    main()
