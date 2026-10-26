#!/usr/bin/env python3
"""Bind identified BootXell Peer functions to exact image bytes and C++."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "analysis/xex/BootXell-peer-source-coverage.json"


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    image_info = manifest["image"]
    image = (ROOT / image_info["path"]).read_bytes()
    if len(image) != image_info["size"]:
        raise SystemExit("BootXell image size differs from source manifest")
    if hashlib.sha256(image).hexdigest() != image_info["sha256"]:
        raise SystemExit("BootXell image hash differs from source manifest")

    inventory = json.loads(
        (ROOT / manifest["peer_symbol_inventory"]).read_text(encoding="utf-8")
    )
    expected = {
        (item["address"].lower(), item["semantic_name"])
        for item in inventory["symbols"]
    }
    covered = {
        (item["address"].lower(), item["semantic_name"])
        for item in manifest["functions"]
    }
    if expected != covered:
        raise SystemExit("BootXell identified Peer symbol coverage is incomplete")

    base = int(image_info["base_address"], 0)
    headers = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / "reconstruction/bootxell").glob("*.hpp")
    )
    total = 0
    for function in manifest["functions"]:
        address = int(function["address"], 0)
        offset = address - base
        payload = image[offset:offset + function["size"]]
        if len(payload) != function["size"]:
            raise SystemExit(f"truncated BootXell function at {address:#x}")
        if hashlib.sha256(payload).hexdigest() != function["sha256"]:
            raise SystemExit(f"BootXell function changed at {address:#x}")
        source = (ROOT / function["source"]).read_text(encoding="utf-8")
        for symbol in function["symbols"]:
            if symbol not in source and symbol not in headers:
                raise SystemExit(f"missing BootXell source symbol: {symbol}")
        total += len(payload)

    for test in manifest["tests"]:
        if not (ROOT / test).is_file():
            raise SystemExit(f"missing BootXell source test: {test}")
    print(
        "BootXell Peer source coverage verified: "
        f"{len(covered)} identified functions, {total} exact bytes"
    )


if __name__ == "__main__":
    main()
