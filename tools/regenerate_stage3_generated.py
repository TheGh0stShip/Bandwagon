#!/usr/bin/env python3
"""Regenerate or verify the deterministic Stage3 assembly include files."""

from __future__ import annotations

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "BadUpdatePayload" / "BadUpdateExploit-3rdStage.bin"
OUTPUT = ROOT / "analysis" / "reconstruction" / "generated"
RECOVER = ROOT / "tools" / "recover_ppc_region.py"


@dataclass(frozen=True)
class Region:
    output: str
    start: int
    end: int
    label: str
    data: bool = False


REGIONS = (
    Region("Stage3-RunBadUpdateExploit.S", 0x01E8, 0x0490, "RunBadUpdateExploit"),
    Region("Stage3-ReadFileIntoPhysicalMemory.S", 0x0490, 0x0670, "ReadFileIntoPhysicalMemory"),
    Region("Stage3-HvxPostOutputExploit.S", 0x0670, 0x0680, "HvxPostOutputExploit"),
    Region("Stage3-alignment-068c.S", 0x068C, 0x0690, "stage3_alignment_068c", True),
    Region("Stage3-core-functions.S", 0x069C, 0x11CC, "stage3_recovered_core_functions"),
    Region("Stage3-compiler-fragment-11cc.S", 0x11CC, 0x11D0, "stage3_compiler_fragment_11cc"),
    Region("Stage3-compiler-fragment-11ec.S", 0x11EC, 0x1220, "stage3_compiler_fragment_11ec"),
    Region("Stage3-remaining-import-stubs.S", 0x12C0, 0x13F0, "stage3_remaining_import_stubs"),
    Region("Stage3-strings-and-data.S", 0x13F0, 0x15A0, "stage3_strings_and_data", True),
    Region("Stage3-tail-padding.S", 0x15D8, 0x15E0, "stage3_tail_padding", True),
)


def render(region: Region) -> str:
    command = [
        sys.executable,
        "-B",
        str(RECOVER),
        str(REFERENCE),
        hex(region.start),
        hex(region.end),
        "--label",
        region.label,
    ]
    if region.data:
        command.append("--data")
    return subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True).stdout


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    OUTPUT.mkdir(parents=True, exist_ok=True)
    changed: list[str] = []
    for region in REGIONS:
        target = OUTPUT / region.output
        expected = render(region)
        if args.check:
            if not target.exists() or target.read_text(encoding="utf-8") != expected:
                changed.append(region.output)
        else:
            target.write_text(expected, encoding="utf-8", newline="\n")

    if changed:
        raise SystemExit("generated Stage3 includes differ: " + ", ".join(changed))
    if args.check:
        print(f"Stage3 generated includes are reproducible ({len(REGIONS)} files)")
    else:
        print(f"Regenerated {len(REGIONS)} Stage3 include files")


if __name__ == "__main__":
    main()
