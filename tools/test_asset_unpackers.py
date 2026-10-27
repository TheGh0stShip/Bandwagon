#!/usr/bin/env python3
"""Host tests for lossless Peer Pressure asset/container unpackers."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from xuiz_archive import XuizArchive
from decode_xur import decode as decode_xur, rebuild as rebuild_xur


ROOT = Path(__file__).resolve().parents[1]


def test_xuiz() -> None:
    source = ROOT / "PeerPressure/System/HudResources.xzp"
    raw = source.read_bytes()
    archive = XuizArchive.parse(raw)
    assert archive.build() == raw
    assert [entry.name for entry in archive.entries] == [
        "HomeTabSignedIn.xur",
        "HomeTabSignedOut.xur",
        "SettingsTabSignedIn.xur",
        "SettingsTabSignedOut.xur",
    ]
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        subprocess.run(
            [sys.executable, str(ROOT / "tools/xuiz_archive.py"), "unpack", str(source), str(work / "out")],
            check=True,
        )
        manifest = json.loads((work / "out/xuiz-manifest.json").read_text())
        assert manifest["entry_count"] == 4
        subprocess.run(
            [sys.executable, str(ROOT / "tools/xuiz_archive.py"), "pack", str(work / "out/xuiz-manifest.json"), str(work / "out"), str(work / "rebuilt.xzp")],
            check=True,
        )
        assert (work / "rebuilt.xzp").read_bytes() == raw
    for entry in archive.entries:
        assert rebuild_xur(decode_xur(entry.data)) == entry.data


def test_wrapped_zip_extraction() -> None:
    source = ROOT / "UpdateData.zip"
    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        subprocess.run(
            [sys.executable, str(ROOT / "tools/decode_update_archive.py"), str(source), "--extract", str(work / "out"), "-o", str(work / "manifest.json")],
            check=True,
        )
        decoded = json.loads((work / "manifest.json").read_text())
        assert decoded["zip_member_count"] == 35
        extracted = work / "out/PeerPressure/System/HudResources.xzp"
        assert hashlib.sha256(extracted.read_bytes()).hexdigest() == hashlib.sha256(
            (ROOT / "PeerPressure/System/HudResources.xzp").read_bytes()
        ).hexdigest()


if __name__ == "__main__":
    test_xuiz()
    test_wrapped_zip_extraction()
    print("Asset unpacker tests passed")
