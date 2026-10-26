#!/usr/bin/env python3
"""Generate deterministic static linear PPC disassemblies of known payload regions."""

from __future__ import annotations

import hashlib
import subprocess
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "analysis" / "disassembly"


@dataclass(frozen=True)
class Target:
    source: str
    output: str
    base: int
    start: int
    size: int | None
    note: str


TARGETS = (
    Target(
        "BadUpdatePayload/BadUpdateExploit-3rdStage.bin",
        "BadUpdateExploit-3rdStage.ppc.asm",
        0x98030000,
        0,
        0x15D8,
        "Base proven by absolute references to strings at file offsets 0x13f0 and later.",
    ),
    Target(
        "BadUpdatePayload/BadUpdateExploit-4thStage.bin",
        "BadUpdateExploit-4thStage.ppc.asm",
        0,
        0,
        0x2138,
        "Position-independent payload; runtime base is supplied by stage 3.",
    ),
    Target(
        "PeerPressure/System/PeerPressureExploit-4thStage.bin",
        "PeerPressureExploit-4thStage.ppc.asm",
        0,
        0,
        0x2738,
        "Position-independent payload; runtime base is not yet established.",
    ),
    Target(
        "PeerPressure/System/KernelExpansion.bin",
        "KernelExpansion.ppc.asm",
        0x801F0000,
        0,
        None,
        "Base proven by absolute references into the image; output includes embedded data as linear decode.",
    ),
    Target(
        "Hdd/PeerPressureExploit-Stage2.bin",
        "PeerPressureExploit-Stage2-tail.ppc.asm",
        0x98000000,
        0x32C40,
        None,
        "Known executable tail; base proven by absolute references into the same blob.",
    ),
)


def disassemble(target: Target) -> None:
    source = ROOT / target.source
    data = source.read_bytes()
    size = target.size if target.size is not None else len(data) - target.start
    if target.start < 0 or size <= 0 or target.start + size > len(data):
        raise ValueError(f"invalid region for {target.source}")
    virtual_start = target.base + target.start
    command = [
        "radare2",
        "-2",
        "-q",
        "-nn",
        "-a",
        "ppc",
        "-b",
        "32",
        "-m",
        hex(target.base),
        "-e",
        "cfg.bigendian=true",
        "-e",
        "scr.color=0",
        "-c",
        f"e asm.bytes=true; pD {size} @ {virtual_start}; q",
        str(source),
    ]
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    header = (
        f"; source: {target.source}\n"
        f"; source_sha256: {hashlib.sha256(data).hexdigest()}\n"
        f"; architecture: Xbox 360 Xenon PowerPC, big-endian\n"
        f"; mapping: file offset 0 -> 0x{target.base:08x}\n"
        f"; decoded_region: file[0x{target.start:x}:0x{target.start + size:x}]\n"
        f"; note: {target.note}\n"
        "; This is a lossless linear instruction decode, not recovered source.\n\n"
    )
    OUTPUT.mkdir(parents=True, exist_ok=True)
    (OUTPUT / target.output).write_text(header + result.stdout, encoding="utf-8")


def main() -> None:
    for target in TARGETS:
        disassemble(target)


if __name__ == "__main__":
    main()
