#!/usr/bin/env python3
"""Generate the typed C++ form of Stage3's copied resident probe."""

from __future__ import annotations

import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_INPUT = ROOT / "BadUpdatePayload/BadUpdateExploit-3rdStage.bin"
DEFAULT_OUTPUT = ROOT / "reconstruction/stage3/probe_payload.hpp"
START = 0x15A0
END = 0x15D8


def render(data: bytes) -> str:
    probe = data[START:END]
    if len(probe) != 0x38:
        raise SystemExit("invalid Stage3 probe boundary")
    values = []
    for offset in range(0, len(probe), 12):
        values.append("    " + ", ".join(f"0x{x:02x}" for x in probe[offset : offset + 12]) + ",")
    return "\n".join([
        "#pragma once", "", "#include <array>", "#include <cstdint>", "",
        "namespace bandwagon::stage3 {", "",
        "// Exact PowerPC probe copied by Stage3 to executable physical memory.",
        "// Its readable behavior is implemented by BuildProbeSampleAddresses().",
        "inline constexpr std::array<std::uint8_t, 0x38> kResidentProbePayload{{",
        *values, "}};", "", "}  // namespace bandwagon::stage3", "",
    ])


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = render(args.input.read_bytes())
    if args.check:
        if not args.output.exists() or args.output.read_text(encoding="utf-8") != expected:
            raise SystemExit(f"stale generated file: {args.output}")
        print(f"Stage3 probe C++ data is current: {args.output.relative_to(ROOT)}")
        return
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(expected, encoding="utf-8", newline="\n")
    print(args.output.relative_to(ROOT))


if __name__ == "__main__":
    main()
