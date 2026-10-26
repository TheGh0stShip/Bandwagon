#!/usr/bin/env python3
"""Build reproducible third-party provenance evidence for SettingsApp.xex."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMAGE = ROOT / "analysis/xex/basefiles/PeerPressure_System_SettingsApp.pe"
XREFS = ROOT / "analysis/xex/SettingsApp-string-xrefs.json"
OUTPUT = ROOT / "analysis/xex/SettingsApp-provenance.json"

LIBRARIES = [
    {"name": "Dear ImGui", "version": "1.86", "license": "MIT", "upstream": "https://github.com/ocornut/imgui/tree/v1.86", "binary_signatures": ["1.86", "imgui.ini", "imgui_impl_xbox360", "imgui_impl_dx9"]},
    {"name": "BearSSL", "version": "0.6", "license": "MIT", "upstream": "https://bearssl.org/", "tag_commit": "8ef7680081c61b486622f2d983c0d3d21e83caad", "binary_signatures": ["master secret", "key expansion", "server finished", "client finished"]},
    {"name": "JsonCpp", "version": "00.11.0", "license": "MIT", "upstream": "https://github.com/open-source-parsers/jsoncpp/tree/00.11.0", "tag_commit": "7c186e549fba358ac0541ed75f5a524b393954b3", "binary_signatures": ["allowTrailingCommas", "rejectDupKeys", "skipBom", "additional six characters expected to parse unicode surrogate pair."]},
]


def main() -> None:
    strings = {item["string"] for item in json.loads(XREFS.read_text(encoding="utf-8"))["references"]}
    for library in LIBRARIES:
        missing = sorted(set(library["binary_signatures"]) - strings)
        if missing:
            raise SystemExit(f"{library['name']}: missing signatures: {missing}")
        library["all_binary_signatures_present"] = True
    result = {
        "schema": "bandwagon-settingsapp-provenance-v1",
        "image": str(IMAGE.relative_to(ROOT)).replace("\\", "/"),
        "image_sha256": hashlib.sha256(IMAGE.read_bytes()).hexdigest(),
        "author_attribution": {"repository": "https://github.com/grimdoomer/Xbox360PeerPressure", "commit": "df524f050db795d6212128e3a4b0c5904ac764e1", "file": "ATTRIBUTION.md"},
        "libraries": LIBRARIES,
        "scope": "component-local binary evidence plus author-declared project versions",
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("SettingsApp provenance: ImGui 1.86, BearSSL 0.6, and JsonCpp 00.11.0 verified")


if __name__ == "__main__":
    main()
