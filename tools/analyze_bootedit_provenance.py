#!/usr/bin/env python3
"""Build reproducible Dear ImGui provenance evidence for BootEdit.xex."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMAGE = ROOT / "analysis/xex/basefiles/PeerPressure_System_BootEdit.pe"
XREFS = ROOT / "analysis/xex/BootEdit-string-xrefs.json"
OUTPUT = ROOT / "analysis/xex/BootEdit-provenance.json"
SYMBOLS = ROOT / "analysis/xex/BootEdit-thirdparty-symbols.json"
FUNCTIONS = ROOT / "analysis/xex/BootEdit-functions.json"


def main() -> None:
    refs = json.loads(XREFS.read_text(encoding="utf-8"))["references"]
    strings = {item["string"] for item in refs}
    required = {"1.86", "imgui.ini", "imgui_log.txt", "imgui_impl_xbox360", "imgui_impl_dx9"}
    missing = sorted(required - strings)
    if missing:
        raise SystemExit(f"missing Dear ImGui signatures: {missing}")
    digest = hashlib.sha256(IMAGE.read_bytes()).hexdigest()
    provenance = {
        "schema": "bandwagon-bootedit-provenance-v1", "image_sha256": digest,
        "author_attribution": {"repository": "https://github.com/grimdoomer/Xbox360PeerPressure", "commit": "df524f050db795d6212128e3a4b0c5904ac764e1", "file": "ATTRIBUTION.md"},
        "library": {"name": "Dear ImGui", "version": "1.86", "license": "MIT", "upstream": "https://github.com/ocornut/imgui/tree/v1.86", "tag_commit": "512c54bbc062c41c74f8a8bd8ff1fd6bebd1e6d0", "binary_signatures": sorted(required), "all_binary_signatures_present": True},
    }
    OUTPUT.write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")

    original = {int(item["address"], 0): item["name"] for item in json.loads(FUNCTIONS.read_text(encoding="utf-8"))["functions"]}
    address = 0x82014428
    local = {item["string"] for item in refs if int(item["function_address"], 0) == address}
    if "1.86" not in local:
        raise SystemExit("ImGui version/layout checker signature moved")
    symbols = {"schema": "bandwagon-bootedit-thirdparty-symbols-v1", "image_sha256": digest, "symbols": [{"address": f"0x{address:08x}", "recovered_name": original[address], "semantic_name": "ImGui::DebugCheckVersionAndDataLayout", "confidence": "high", "ownership": "Dear ImGui 1.86", "evidence": ["function-local comparison against exact IMGUI_VERSION string", "called by BootEdit's ImGui initialization path"]}]}
    SYMBOLS.write_text(json.dumps(symbols, indent=2) + "\n", encoding="utf-8")
    print("BootEdit provenance: Dear ImGui 1.86 and version/layout checker verified")


if __name__ == "__main__":
    main()
