#!/usr/bin/env python3
"""Verify BootEdit's XPR2 readable-source reconstruction."""

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
    model = json.loads((ANALYSIS / "BootEdit-packed-resource.json").read_text())
    image = (ANALYSIS / "basefiles/PeerPressure_System_BootEdit.pe").read_bytes()
    inventory = json.loads((ANALYSIS / "BootEdit-functions.json").read_text())
    ownership = json.loads((ANALYSIS / "BootEdit-function-ownership.json").read_text())
    if model.get("schema") != "bandwagon-bootedit-packed-resource-v1":
        fail("unexpected BootEdit packed-resource schema")
    if model.get("status") != "readable_behavioral_model_complete_provenance_unknown":
        fail("BootEdit packed-resource source coverage is incomplete")
    if digest(image) != model["image_sha256"]:
        fail("BootEdit packed-resource image binding differs")

    base = 0x82000000
    start = int(model["range"]["start"], 0)
    end = int(model["range"]["end_exclusive"], 0)
    payload = image[start - base:end - base]
    if len(payload) != model["range"]["size"] or digest(payload) != model["range"]["sha256"]:
        fail("BootEdit packed-resource range differs")

    source_parts = []
    tests = 0
    for artifact in model["source_artifacts"]:
        raw = (ROOT / artifact["path"]).read_bytes()
        if len(raw) != artifact["size"] or digest(raw) != artifact["sha256"]:
            fail(f"BootEdit packed-resource artifact differs: {artifact['path']}")
        if artifact["kind"] == "test": tests += 1
        else: source_parts.append(raw)
    if tests != 1 or len(source_parts) != 2:
        fail("BootEdit packed-resource source/test set differs")
    source = b"\n".join(source_parts).decode("utf-8")

    functions = {int(f["address"], 0): f for f in inventory["functions"]}
    seen = set()
    for entry in model["entries"]:
        address = int(entry["address"], 0)
        function = functions.get(address)
        if address in seen or not function or function["size_to_next_symbol"] != entry["size"]:
            fail(f"BootEdit packed-resource extent differs: {address:#x}")
        seen.add(address)
        body = image[address - base:address - base + entry["size"]]
        if digest(body) != entry["sha256"]:
            fail(f"BootEdit packed-resource bytes differ: {address:#x}")
        if entry["semantic_name"] not in source or not entry.get("evidence"):
            fail(f"BootEdit packed-resource model lacks {entry['semantic_name']}")
    expected = {int(f["address"], 0) for f in inventory["functions"]
                if start <= int(f["address"], 0) < end}
    if seen != expected or len(seen) != model["range"]["entry_count"]:
        fail("BootEdit packed-resource entries are not exhaustive")
    if not any(int(r["start"], 0) <= start and int(r["end_exclusive"], 0) >= end
               for r in ownership["categories"]["unknown"]["ranges"]):
        fail("BootEdit packed-resource provenance is no longer unknown")

    settings_model = model["settingsapp_binding"]
    settings_image = (ANALYSIS / "basefiles/PeerPressure_System_SettingsApp.pe").read_bytes()
    settings_inventory = json.loads((ANALYSIS / "SettingsApp-functions.json").read_text())
    settings_ownership = json.loads((ANALYSIS / "SettingsApp-function-ownership.json").read_text())
    if digest(settings_image) != settings_model["image_sha256"]:
        fail("SettingsApp packed-resource image binding differs")
    settings_start = int(settings_model["range"]["start"], 0)
    settings_end = int(settings_model["range"]["end_exclusive"], 0)
    settings_body = settings_image[settings_start - base:settings_end - base]
    if (len(settings_body) != settings_model["range"]["size"] or
            digest(settings_body) != settings_model["range"]["sha256"]):
        fail("SettingsApp packed-resource range differs")
    settings_functions = {int(f["address"], 0): f for f in settings_inventory["functions"]}
    settings_seen = set()
    for entry in settings_model["entries"]:
        address = int(entry["address"], 0)
        function = settings_functions.get(address)
        if address in settings_seen or not function or function["size_to_next_symbol"] != entry["size"]:
            fail(f"SettingsApp packed-resource extent differs: {address:#x}")
        settings_seen.add(address)
        body = settings_image[address - base:address - base + entry["size"]]
        if digest(body) != entry["sha256"] or entry["semantic_name"] not in source:
            fail(f"SettingsApp packed-resource coverage differs: {address:#x}")
    expected_settings = {int(f["address"], 0) for f in settings_inventory["functions"]
                         if settings_start <= int(f["address"], 0) < settings_end}
    if settings_seen != expected_settings or len(settings_seen) != settings_model["range"]["entry_count"]:
        fail("SettingsApp packed-resource entries are not exhaustive")
    if not any(int(r["start"], 0) <= settings_start and
               int(r["end_exclusive"], 0) >= settings_end
               for r in settings_ownership["categories"]["unknown"]["ranges"]):
        fail("SettingsApp packed-resource provenance is no longer unknown")
    print("Packed resources verified: BootEdit 4/712 and SettingsApp 4/688 source-covered")


if __name__ == "__main__":
    main()
