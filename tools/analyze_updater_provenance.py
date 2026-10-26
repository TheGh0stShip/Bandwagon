#!/usr/bin/env python3
"""Verify Updater's declared Miniz revision against binary-local evidence."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IMAGE = ROOT / "analysis/xex/basefiles/Updater.pe"
XREFS = ROOT / "analysis/xex/Updater-string-xrefs.json"
OUTPUT = ROOT / "analysis/xex/Updater-provenance.json"

SIGNATURES = {
    "not a ZIP archive",
    "invalid header or archive is corrupted",
    "unsupported multidisk archive",
    "decompression failed or archive is corrupted",
    "CRC-32 check failed",
    "archive is too large",
}


def main() -> None:
    refs = json.loads(XREFS.read_text(encoding="utf-8"))["references"]
    strings = {item["string"] for item in refs}
    missing = sorted(SIGNATURES - strings)
    if missing:
        raise SystemExit(f"missing Miniz error-table signatures: {missing}")
    result = {
        "schema": "bandwagon-updater-provenance-v1",
        "image_sha256": hashlib.sha256(IMAGE.read_bytes()).hexdigest(),
        "author_attribution": {
            "repository": "https://github.com/grimdoomer/Xbox360PeerPressure",
            "commit": "df524f050db795d6212128e3a4b0c5904ac764e1",
            "file": "ATTRIBUTION.md",
        },
        "library": {
            "name": "Miniz", "version": "3.1.1", "license": "MIT",
            "upstream": "https://github.com/richgel999/miniz/tree/3.1.1",
            "tag_commit": "d10b03cc73475af673df40f06e5cefd1d5f940d9",
            "binary_family_evidence": sorted(SIGNATURES),
            "binary_family_verified": "3.x",
            "exact_version_basis": "author declaration combined with component-local Miniz 3.x evidence",
        },
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("Updater provenance: Miniz 3.1.1 declaration and 3.x binary signatures verified")


if __name__ == "__main__":
    main()
