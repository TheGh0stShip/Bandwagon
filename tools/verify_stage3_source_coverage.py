#!/usr/bin/env python3
"""Verify that every Stage3 byte has an explicit source/provenance owner."""

from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/stage3/source-coverage.json"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_offset(value: str) -> int:
    return int(value, 0)


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    artifact = manifest["artifact"]
    image = ROOT / artifact["path"]
    image_bytes = image.read_bytes()
    if len(image_bytes) != artifact["size"]:
        raise SystemExit("Stage3 size differs from source-coverage manifest")
    if digest(image) != artifact["sha256"]:
        raise SystemExit("Stage3 hash differs from source-coverage manifest")

    upstream = manifest["upstream"]
    repository = ROOT / upstream["repository"]
    commit = subprocess.run(
        ["git", "-C", str(repository), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    if commit != upstream["commit"]:
        raise SystemExit(f"unexpected BadUpdate source commit: {commit}")
    for source in upstream["sources"]:
        path = ROOT / source["path"]
        if digest(path) != source["sha256"]:
            raise SystemExit(f"upstream source hash mismatch: {source['path']}")

    comparison = json.loads(
        (ROOT / "analysis/badupdate-v1.3-comparison.json").read_text(
            encoding="utf-8"
        )
    )
    stage3_comparison = next(
        item
        for item in comparison["components"]
        if item["name"] == "BadUpdateExploit-3rdStage.bin"
    )
    comparison_functions = {
        item["peer_pressure_range"]: item
        for item in stage3_comparison["function_map"]
    }
    for evidence in manifest["upstream_function_evidence"]:
        actual = comparison_functions.get(evidence["peer_range"])
        if actual is None:
            raise SystemExit(
                f"missing upstream comparison for {evidence['symbol']}"
            )
        if actual["best_upstream_range"] != evidence["upstream_range"]:
            raise SystemExit(
                f"upstream range changed for {evidence['symbol']}"
            )
        if actual["normalized_instruction_similarity"] != evidence[
            "normalized_instruction_similarity"
        ]:
            raise SystemExit(
                f"instruction similarity changed for {evidence['symbol']}"
            )

    cursor = 0
    source_cache: dict[str, str] = {}
    kinds: dict[str, int] = {}
    inherited_cpp_symbols: set[str] = set()
    for region in manifest["coverage"]:
        start = parse_offset(region["start"])
        end = parse_offset(region["end"])
        if start != cursor or end <= start:
            raise SystemExit(
                f"non-contiguous Stage3 coverage at 0x{cursor:04x}: "
                f"{region['start']}-{region['end']}"
            )
        cursor = end
        kinds[region["kind"]] = kinds.get(region["kind"], 0) + (end - start)
        if region["kind"] == "upstream_cpp":
            inherited_cpp_symbols.update(region["symbols"])
        texts = []
        for source_name in region["sources"]:
            source_path = ROOT / source_name
            if not source_path.is_file():
                raise SystemExit(f"missing Stage3 source owner: {source_name}")
            if source_name not in source_cache:
                source_cache[source_name] = source_path.read_text(
                    encoding="utf-8", errors="replace"
                )
            texts.append(source_cache[source_name])
        combined = "\n".join(texts)
        for symbol in region["symbols"]:
            if symbol not in combined:
                raise SystemExit(
                    f"source owner does not name {symbol} for "
                    f"{region['start']}-{region['end']}"
                )
    if cursor != len(image_bytes):
        raise SystemExit(
            f"Stage3 coverage ends at 0x{cursor:x}, expected 0x{len(image_bytes):x}"
        )
    evidence_symbols = {
        item["symbol"] for item in manifest["upstream_function_evidence"]
    }
    if inherited_cpp_symbols != evidence_symbols:
        raise SystemExit(
            "upstream C++ coverage and instruction-match evidence disagree: "
            f"coverage={sorted(inherited_cpp_symbols)}, "
            f"evidence={sorted(evidence_symbols)}"
        )

    source_bytes = sum(
        size
        for kind, size in kinds.items()
        if kind in {"peer_cpp", "upstream_cpp", "upstream_target_assembly",
                    "peer_target_assembly"}
    )
    print(
        "Stage3 source coverage verified: "
        f"{len(image_bytes)} bytes partitioned, {source_bytes} executable/source bytes"
    )


if __name__ == "__main__":
    main()
