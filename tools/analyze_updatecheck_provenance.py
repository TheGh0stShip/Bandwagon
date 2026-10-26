#!/usr/bin/env python3
"""Build a reproducible third-party provenance record for UpdateCheck.xex."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMAGE = ROOT / "analysis/xex/basefiles/PeerPressure_System_UpdateCheck.pe"
XREFS = ROOT / "analysis/xex/UpdateCheck-string-xrefs.json"
OUTPUT = ROOT / "analysis/xex/UpdateCheck-provenance.json"

LIBRARIES = [
    {
        "name": "BearSSL",
        "version": "0.6",
        "license": "MIT",
        "upstream": "https://bearssl.org/",
        "binary_signatures": ["X509 CERTIFICATE", "master secret", "key expansion", "server finished", "client finished"],
    },
    {
        "name": "JsonCpp",
        "version": "00.11.0",
        "license": "MIT",
        "upstream": "https://github.com/open-source-parsers/jsoncpp/tree/00.11.0",
        "tag_commit": "7c186e549fba358ac0541ed75f5a524b393954b3",
        "binary_signatures": [
            "allowTrailingCommas", "rejectDupKeys", "skipBom",
            "additional six characters expected to parse unicode surrogate pair.",
            "in Json::Value::duplicateStringValue(): Failed to allocate string value buffer",
        ],
    },
]


def main() -> None:
    xrefs = json.loads(XREFS.read_text(encoding="utf-8"))
    strings = {item["string"] for item in xrefs["references"]}
    for library in LIBRARIES:
        missing = sorted(set(library["binary_signatures"]) - strings)
        if missing:
            raise SystemExit(f"{library['name']}: missing signatures: {missing}")
        library["all_binary_signatures_present"] = True

    result = {
        "schema": "bandwagon-updatecheck-provenance-v1",
        "image": str(IMAGE.relative_to(ROOT)).replace("\\", "/"),
        "image_sha256": hashlib.sha256(IMAGE.read_bytes()).hexdigest(),
        "author_attribution": {
            "repository": "https://github.com/grimdoomer/Xbox360PeerPressure",
            "commit": "df524f050db795d6212128e3a4b0c5904ac764e1",
            "file": "ATTRIBUTION.md",
        },
        "libraries": LIBRARIES,
        "scope": "third-party ownership boundary; not proof that every library function has been named",
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("UpdateCheck provenance: BearSSL 0.6 and JsonCpp 00.11.0 signatures verified")


if __name__ == "__main__":
    main()
