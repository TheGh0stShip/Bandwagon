#!/usr/bin/env python3
"""Validate static evidence for the Peer-specific Stage3 probe function."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_STAGE3 = ROOT / "BadUpdatePayload/BadUpdateExploit-3rdStage.bin"
BASE = 0x98030000


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def u32be(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], "big")


def expect_words(data: bytes, start: int, expected: tuple[int, ...], label: str) -> None:
    actual = tuple(u32be(data, start + index * 4) for index in range(len(expected)))
    if actual != expected:
        raise ValueError(f"{label} differs at file offset 0x{start:x}")


def display_path(path: Path) -> str:
    resolved = path.resolve()
    try:
        return str(resolved.relative_to(ROOT))
    except ValueError:
        return str(resolved)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage3", type=Path, default=DEFAULT_STAGE3)
    parser.add_argument("-o", "--output", type=Path)
    args = parser.parse_args()

    data = args.stage3.read_bytes()
    if len(data) != 0x15E0:
        raise ValueError(f"unexpected Peer Stage3 size: {len(data)}")

    # Allocation arguments and call to the literal-address stub at 0x12a0.
    expect_words(
        data,
        0x104,
        (
            0x39000080,
            0x38E0FFFF,
            0x38C00000,
            0x38A00204,
            0x38801000,
            0x38600001,
            0x3B600000,
            0x48001181,
        ),
        "allocation setup",
    )
    expect_words(data, 0x12A0, (0x3D608008, 0x616B01D8, 0x7D6903A6, 0x4E800420), "allocation stub")
    expect_words(data, 0x1270, (0x3D608010, 0x616BD110, 0x7D6903A6, 0x4E800420), "memset stub")
    expect_words(data, 0x12B0, (0x3D608010, 0x616BCC40, 0x7D6903A6, 0x4E800420), "memcpy stub")
    expect_words(data, 0x1250, (0x3D608008, 0x616B0048, 0x7D6903A6, 0x4E800420), "physical-address stub")
    expect_words(data, 0x1260, (0x3D608008, 0x616B07B8, 0x7D6903A6, 0x4E800420), "free stub")
    expect_words(data, 0x680, (0x3800000D, 0x44000002, 0x4E800020), "syscall-0x0d stub")
    expect_words(data, 0x690, (0x38000000, 0x44000002, 0x4E800020), "syscall-0 stub")

    # memcpy(allocation, BASE+0x15a0, 0x38), marker comparisons, and free.
    expect_words(data, 0x13C, (0x3D609803, 0x38A00038, 0x388B15A0, 0x7FA3EB78, 0x48001165), "probe copy")
    expect_words(data, 0x190, (0x3D604141, 0x617E0001, 0x7F03F000, 0x419A002C), "first marker check")
    expect_words(data, 0x1A0, (0x3C607262, 0x7F88E378, 0x38E00001, 0x7FE6FB78, 0x7FE5FB78, 0x38800004, 0x60637472, 0x480004D5), "rbtr fallback")
    expect_words(data, 0x1CC, (0x7FA4EB78, 0x38600001, 0x4800108D), "physical free")

    probe = data[0x15A0:0x15D8]
    expect_words(
        data,
        0x15A0,
        (
            0x3C608000,
            0x60630200,
            0x786307C6,
            0x64630002,
            0x3880000C,
            0x7C8903A6,
            0xE8830000,
            0xF8880000,
            0x38630200,
            0x39080008,
            0x4200FFF0,
            0x3C604141,
            0x60630001,
            0x4E800020,
        ),
        "embedded probe",
    )

    result = {
        "schema": "peer-pressure-stage3-probe-v1",
        "architecture": "Xbox 360 Xenon, big-endian PowerPC",
        "source": {
            "path": display_path(args.stage3),
            "size": len(data),
            "sha256": sha256(data),
            "mapping": "file offset 0 -> 0x98030000",
        },
        "function": {
            "analyzed_file_range": "0x0f8-0x1e8",
            "instruction_file_range": "0x0f8-0x1e4",
            "runtime_range": "0x980300f8-0x980301e4",
            "sha256_including_alignment": sha256(data[0xF8:0x1E8]),
            "behavioral_name_hypothesis": "probe_stage4_hv_dispatch",
            "verified_result_domain": [0, 1],
        },
        "allocation": {
            "size": 0x1000,
            "allocation_stub": "file 0x12a0 -> 0x800801d8",
            "zero_fill_stub": "file 0x1270 -> 0x8010d110",
            "free_stub": "file 0x1260 -> 0x800807b8",
        },
        "probe": {
            "file_range": "0x15a0-0x15d8",
            "runtime_range": "0x980315a0-0x980315d8",
            "size": len(probe),
            "sha256": sha256(probe),
            "source_address": "0x8000020000020000",
            "iterations": 12,
            "source_stride": 0x200,
            "destination_stride": 8,
            "copied_bytes": 0x60,
            "return_marker": "0x41410001",
        },
        "attempts": [
            {
                "syscall_ordinal": "0x0d",
                "stub_file_offset": "0x680",
                "marker": "0x41414141",
            },
            {
                "syscall_ordinal": "0x00",
                "stub_file_offset": "0x690",
                "magic": "0x72627472 ('rbtr')",
                "operation": 4,
            },
        ],
        "success_result": "0x41410001",
    }
    encoded = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
