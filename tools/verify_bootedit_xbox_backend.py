#!/usr/bin/env python3
"""Verify the hash-bound semantic partition of BootEdit's Xbox backend."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ANALYSIS = ROOT / "analysis/xex"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def fail(message: str) -> None:
    raise SystemExit(message)


def main() -> None:
    model = json.loads((ANALYSIS / "BootEdit-xbox-backend.json").read_text())
    image = (ANALYSIS / "basefiles/PeerPressure_System_BootEdit.pe").read_bytes()
    inventory = json.loads((ANALYSIS / "BootEdit-functions.json").read_text())
    ownership = json.loads((ANALYSIS / "BootEdit-function-ownership.json").read_text())
    xrefs = json.loads((ANALYSIS / "BootEdit-string-xrefs.json").read_text())
    if model.get("schema") != "bandwagon-bootedit-xbox-backend-v1":
        fail("unexpected BootEdit Xbox-backend schema")
    if digest(image) != model["image_sha256"]:
        fail("BootEdit Xbox-backend image binding differs")

    base = 0x82000000
    start = int(model["range"]["start"], 0)
    end = int(model["range"]["end_exclusive"], 0)
    body = image[start - base : end - base]
    if len(body) != model["range"]["size"] or digest(body) != model["range"]["sha256"]:
        fail("BootEdit Xbox-backend range differs")

    functions = {int(f["address"], 0): f for f in inventory["functions"]}
    seen = set()
    strings = {(int(x["function_address"], 0), x["string"]) for x in xrefs["references"]}
    for entry in model["entries"]:
        address = int(entry["address"], 0)
        if address in seen:
            fail(f"duplicate Xbox-backend entry: {address:#x}")
        seen.add(address)
        function = functions.get(address)
        if not function or function["size_to_next_symbol"] != entry["size"]:
            fail(f"Xbox-backend inventory extent differs: {address:#x}")
        payload = image[address - base : address - base + entry["size"]]
        if digest(payload) != entry["sha256"]:
            fail(f"Xbox-backend function bytes differ: {address:#x}")
        if not entry.get("semantic_name") or not entry.get("evidence"):
            fail(f"Xbox-backend semantic evidence missing: {address:#x}")
        for value in entry.get("required_strings", []):
            if (address, value) not in strings:
                fail(f"Xbox-backend string evidence differs: {address:#x} {value!r}")

    expected = {int(f["address"], 0) for f in inventory["functions"]
                if start <= int(f["address"], 0) < end}
    if seen != expected or len(seen) != model["range"]["entry_count"]:
        fail("Xbox-backend semantic partition is not exhaustive")
    unknown = ownership["categories"]["unknown"]["ranges"]
    if not any(int(r["start"], 0) == start and int(r["end_exclusive"], 0) == end
               for r in unknown):
        fail("Xbox-backend provenance is no longer represented as unknown")
    print("BootEdit Xbox backend verified: 11 entries / 5128 bytes semantically partitioned")


if __name__ == "__main__":
    main()
