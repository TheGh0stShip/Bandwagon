#!/usr/bin/env python3
"""Emit readable assembly for the Peer Pressure retail-kernel patch stream."""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "analysis/reconstruction/kernel_patches.reconstructed.S"

PATCHES = [
    (0x8007B920, "386000004e800020", "return_zero"),
    (0x8007C4B8, "38600001", "force_true_r3"),
    (0x8007C5E8, "38600000", "force_false_r3"),
    (0x8007C634, "39600000", "clear_r11"),
    (0x8007C684, "39600000", "clear_r11"),
    (0x8007AF08, "386000004e800020", "return_zero"),
    (0x800998D0, "2b0b00ff", "compare_r11_with_255"),
    (0x800992B4, "3860000060000000600000006000000060000000", "force_zero_and_nop_four"),
    (0x80109C90, "386000014e800020", "return_one"),
    (0x8010A788, "386000004e800020", "return_zero"),
    (0x8010AA68, "386000004e800020", "return_zero"),
    (0x8010AF30, "386000014e800020", "return_one"),
    (0x8010B138, "386000004e800020", "return_zero"),
    (0x8010B0E8, "386000004e800020", "return_zero"),
    (0x8010B278, "386000004e800020", "return_zero"),
    (0x8010B3F8, "386000004e800020", "return_zero"),
    (0x8010BF20, "280500003860000141820008906500004e800020", "return_one_and_optionally_store_one"),
    (0x80111990, "386000014e800020", "return_one"),
    (0x800E1754, "48000018", "skip_forward_0x18"),
    (0x800E17CC, "835c00986000000060000000", "load_r26_and_nop_two"),
    (0x8015D9D8, "386000014e800020", "return_one"),
    (0x8009D660, "48000068", "skip_forward_0x68"),
    (0x800D8748, "386000014e800020", "return_one"),
]

def main() -> None:
    lines = [
        "/* Lossless source for PeerPressure/System/kernel_patches.bin.",
        " * Record format: target address, byte count, replacement bytes; big-endian.",
        " */", ".section .text", ".balign 4", "kernel_patch_stream:",
    ]
    for index, (address, hex_data, effect) in enumerate(PATCHES):
        data = bytes.fromhex(hex_data)
        lines += [f"patch_{index:02d}_{effect}:", f"    .long 0x{address:08x}", f"    .long {len(data)}"]
        for offset in range(0, len(data), 4):
            lines.append(f"    .long 0x{int.from_bytes(data[offset:offset+4], 'big'):08x}")
    lines += ["    .long 0xffffffff", "kernel_patch_stream_end:", ""]
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(OUTPUT.relative_to(ROOT))

if __name__ == "__main__":
    main()
