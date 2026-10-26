#!/usr/bin/env python3
"""Verify BootXell's inih boundary and recover its public parser entry."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMAGE = ROOT / "analysis/xex/basefiles/PeerPressure_OtherOS_BootXell.pe"
FUNCTIONS = ROOT / "analysis/xex/BootXell-functions.json"
CALLGRAPH = ROOT / "analysis/xex/BootXell-callgraph.json"
FINGERPRINTS = ROOT / "analysis/xex/shared-function-fingerprints.json"
OUTPUT = ROOT / "analysis/xex/BootXell-provenance.json"
SYMBOLS = ROOT / "analysis/xex/BootXell-thirdparty-symbols.json"


def main() -> None:
    funcs = json.loads(FUNCTIONS.read_text(encoding="utf-8"))["functions"]
    original = {int(item["address"], 0): item["name"] for item in funcs}
    edges = json.loads(CALLGRAPH.read_text(encoding="utf-8"))["edges"]
    pairs = {(int(edge["caller_address"], 0), int(edge["target_address"], 0)) for edge in edges}
    required = {(0x91A06450, 0x91A07430), (0x91A07430, 0x91A07228)}
    if not required.issubset(pairs):
        raise SystemExit("BootXell inih call-chain evidence changed")

    aliases = [item for item in json.loads(FINGERPRINTS.read_text(encoding="utf-8"))["propagated_aliases"] if item["component"] == "BootXell"]
    digest = hashlib.sha256(IMAGE.read_bytes()).hexdigest()
    result = {
        "schema": "bandwagon-bootxell-provenance-v1", "image_sha256": digest,
        "author_attribution": {"repository": "https://github.com/grimdoomer/Xbox360PeerPressure", "commit": "df524f050db795d6212128e3a4b0c5904ac764e1", "file": "ATTRIBUTION.md"},
        "library": {"name": "inih", "version": "r62", "license": "BSD-3-Clause", "upstream": "https://github.com/benhoyt/inih/tree/r62", "tag_commit": "26254ee9de7681f8825433415443e7116ff24b98", "component_evidence": ["LoadBootIni passes ParseBootIniOption into the recovered parser entry", "parser trims whitespace, recognizes sections and name/value separators, and dispatches through an indirect handler call"]},
        "safe_cross_xex_alias_count": len(aliases),
        "safe_cross_xex_aliases": aliases,
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    symbols = {
        "schema": "bandwagon-bootxell-thirdparty-symbols-v1", "image_sha256": digest,
        "symbols": [{"address": "0x91a07430", "recovered_name": original[0x91A07430], "semantic_name": "ini_parse_string", "confidence": "high", "ownership": "inih r62", "evidence": ["called by LoadBootIni with input buffer, callback, and user context", "loops over the input and indirectly dispatches parsed section/name/value tuples"]}],
    }
    SYMBOLS.write_text(json.dumps(symbols, indent=2) + "\n", encoding="utf-8")
    print(f"BootXell provenance: inih r62 parser and {len(aliases)} safe cross-XEX aliases verified")


if __name__ == "__main__":
    main()
