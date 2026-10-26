#!/usr/bin/env python3
"""Bind the recovered UpdateCheck Peer functions to exact image bytes and C++."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/UpdateCheck-peer-source-coverage.json"


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    image_info = manifest["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if len(image) != image_info["size"]:
        raise SystemExit("UpdateCheck image size differs from source manifest")
    if hashlib.sha256(image).hexdigest() != image_info["sha256"]:
        raise SystemExit("UpdateCheck image hash differs from source manifest")

    inventory = json.loads(
        (ROOT / manifest["peer_symbol_inventory"]).read_text(encoding="utf-8")
    )
    peer_symbols = {
        (item["address"].lower(), item["semantic_name"])
        for item in inventory["symbols"]
        if "ownership" not in item
    }
    covered_symbols = {
        (item["address"].lower(), item["semantic_name"])
        for item in manifest["functions"]
    }
    if inventory["counts"]["peer_application"] != len(covered_symbols):
        raise SystemExit("UpdateCheck Peer symbol count changed")
    if peer_symbols != covered_symbols:
        raise SystemExit("UpdateCheck Peer symbol coverage is incomplete")

    base = int(image_info["base_address"], 0)
    header = (
        ROOT / "reconstruction/update_check/latest_release.hpp"
    ).read_text(encoding="utf-8")
    total = 0
    for function in manifest["functions"]:
        address = int(function["address"], 0)
        offset = address - base
        payload = image[offset:offset + function["size"]]
        if len(payload) != function["size"]:
            raise SystemExit(f"truncated UpdateCheck function at {address:#x}")
        if hashlib.sha256(payload).hexdigest() != function["sha256"]:
            raise SystemExit(f"UpdateCheck function changed at {address:#x}")
        source = (ROOT / function["source"]).read_text(encoding="utf-8")
        for symbol in function["symbols"]:
            if symbol not in source and symbol not in header:
                raise SystemExit(f"missing UpdateCheck source symbol: {symbol}")
        total += len(payload)

    for test in manifest["tests"]:
        if not (ROOT / test).is_file():
            raise SystemExit(f"missing UpdateCheck source test: {test}")
    print(
        "UpdateCheck Peer source coverage verified: "
        f"{len(covered_symbols)} identified functions, {total} exact bytes"
    )


if __name__ == "__main__":
    main()
