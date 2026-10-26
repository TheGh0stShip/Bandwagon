#!/usr/bin/env python3
"""Recover the mapped layout and emit a lossless KernelExpansion reconstruction."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / "PeerPressure/System/KernelExpansion.bin"
OUTDIR = ROOT / "analysis/kernel-expansion"
SOURCE = ROOT / "analysis/reconstruction/KernelExpansion.reconstructed.S"
BASE = 0x801F0000
TEXT_END = 0xF4B0

# Leaf callbacks reached indirectly are not discoverable from direct BL targets
# and do not use the standard non-leaf prologue.  Their boundaries are proved
# by the return instructions and the surrounding independently discovered
# entry points.
EXPLICIT_STARTS = {
    0x8210,
    0x8238,
    0x8270,
}

# Behavioral names are deliberately distinct from claims about original private
# symbols.  Each is backed by direct paths, diagnostics, or a simple call role.
SEMANTIC_NAMES = {
    0x0000: "KernelExpansionInitialize",
    0x0150: "ReportExceptionContext",
    0x0648: "LoadPeerPressureConfig",
    0x0748: "LoadKernelPatchStream",
    0x09C0: "GetPeerPressureConfig",
    0x09D0: "LaunchBootXell",
    0x0A50: "ConfigureBootAnimation",
    0x14D0: "ReadFileToAllocatedBuffer",
    0x16A8: "InitializeDefaultConfig",
    0x1700: "LoadStructuredConfigFile",
    0x1790: "InstallKernelXexDataHandlers",
    0x1A60: "ConfigureAuroraSideLoad",
    0x1CF0: "LoadBootImage",
    0x1FC0: "LoadBootAnimationTheme",
    0x3378: "ResolveSideLoadConfiguration",
    0x4938: "ParseDllConfigEntry",
    0x6AF0: "ParseDriveNumberPrefix",
    0x7E68: "FindSoftmodUpdateArchive",
    0x8080: "LaunchSettingsApp",
    0x8210: "InitializeCallbackOutputs",
    0x8238: "MapCallbackValue",
    0x8270: "ClassifyStfsPackageMagic",
    0xA320: "ClassifyFatBootSector",
    0xA540: "RecognizeGptDisk",
    0xA640: "IsAsciiAlphaNumeric",
    0xA688: "CheckPhaseTwoMarker",
    0xA798: "LaunchUpdateCheck",
    0xAA08: "CanOpenExistingPath",
    0xAAC0: "CopyPathAndParentDirectory",
}


def direct_target(offset: int, word: int) -> int | None:
    if word >> 26 != 18 or not (word & 1):
        return None
    displacement = word & 0x03FFFFFC
    if displacement & 0x02000000:
        displacement -= 0x04000000
    return displacement if word & 2 else offset + displacement


def kernel_symbols() -> dict[int, str]:
    symbols: dict[int, str] = {}
    for name in ("KernelExports_Retail_17559.asm", "KernelConfig_Retail_17559.asm"):
        path = ROOT / "third_party/Xbox360BadUpdate/Common" / name
        for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
            match = re.match(r"\s*\.set\s+([A-Za-z0-9_]+)\s*,\s*(0x[0-9A-Fa-f]+)", line)
            if match:
                symbols.setdefault(int(match.group(2), 0), match.group(1))
    return symbols


def main() -> None:
    data = INPUT.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    words = [int.from_bytes(data[i : i + 4], "big") for i in range(0, TEXT_END, 4)]
    starts = {0, *EXPLICIT_STARTS}
    for offset, word in enumerate(words):
        byte_offset = offset * 4
        if word in (0x7D8802A6, 0x7C0802A6):
            starts.add(byte_offset)
        target = direct_target(byte_offset, word)
        if target is not None and 0 <= target < TEXT_END and target % 4 == 0:
            starts.add(target)

    known = kernel_symbols()
    thunks: dict[int, tuple[int, str | None]] = {}
    for offset in sorted(starts):
        if offset + 16 > TEXT_END:
            continue
        a, b, c, d = words[offset // 4 : offset // 4 + 4]
        if a >> 26 == 15 and b >> 26 == 24 and c == 0x7D6903A6 and d == 0x4E800420:
            address = ((a & 0xFFFF) << 16) | (b & 0xFFFF)
            thunks[offset] = (address, known.get(address))

    names = {}
    for offset in starts:
        if offset in thunks and thunks[offset][1]:
            names[offset] = thunks[offset][1]
        elif offset in SEMANTIC_NAMES:
            names[offset] = SEMANTIC_NAMES[offset]
        else:
            names[offset] = f"fn_{BASE + offset:08X}"

    ordered = sorted(starts)
    functions = []
    for index, offset in enumerate(ordered):
        end = ordered[index + 1] if index + 1 < len(ordered) else TEXT_END
        if offset in EXPLICIT_STARTS:
            evidence = ["indirect leaf callback boundary and decoded behavior"]
        elif offset in SEMANTIC_NAMES:
            evidence = ["behavioral name corroborated by strings and call role"]
        else:
            evidence = ["direct BL target or standard PPC prologue"]
        item = {
            "address": f"0x{BASE + offset:08x}",
            "section_offset": f"0x{offset:x}",
            "size_to_next_symbol": end - offset,
            "name": names[offset],
            "evidence": evidence,
        }
        if offset in thunks:
            address, symbol = thunks[offset]
            item["kind"] = "retail_kernel_thunk"
            item["target_address"] = f"0x{address:08x}"
            item["target_name"] = symbol
        else:
            item["kind"] = "local_code"
        functions.append(item)

    OUTDIR.mkdir(parents=True, exist_ok=True)
    function_map = {
        "schema": "bandwagon-kernel-expansion-functions-v1",
        "input": str(INPUT.relative_to(ROOT)).replace("\\", "/"),
        "sha256": digest,
        "image_base": f"0x{BASE:08x}",
        "text_end": f"0x{TEXT_END:x}",
        "function_count": len(functions),
        "kernel_thunk_count": len(thunks),
        "resolved_kernel_thunk_count": sum(symbol is not None for _, symbol in thunks.values()),
        "functions": functions,
    }
    (OUTDIR / "KernelExpansion-functions.json").write_text(json.dumps(function_map, indent=2) + "\n", encoding="utf-8")

    string_lines = []
    offset = TEXT_END
    while offset < len(data):
        end = offset
        while end < len(data) and 0x20 <= data[end] < 0x7F:
            end += 1
        if end - offset >= 4:
            string_lines.append(
                f"{offset:05x} 0x{BASE + offset:08x} {data[offset:end].decode('ascii')}"
            )
        offset = max(end + 1, offset + 1)
    (OUTDIR / "KernelExpansion-strings.txt").write_text("\n".join(string_lines) + "\n", encoding="utf-8")

    lines = [
        "/* Lossless clean-room reconstruction of KernelExpansion.bin.",
        f" * Runtime mapping: file offset 0 -> 0x{BASE:08x}; code/thunks end at 0x{TEXT_END:x}.",
        " * Instruction words remain literal so rebuilding does not depend on assembler aliases.",
        " */", ".section .text", ".balign 4", ".global KernelExpansionInitialize", "",
    ]
    for offset in range(0, TEXT_END, 4):
        if offset in names:
            lines.append(f"{names[offset]}:")
        lines.append(f"    .long 0x{int.from_bytes(data[offset:offset + 4], 'big'):08x} /* 0x{offset:05x} */")
    lines += ["", "embedded_data:"]
    for offset in range(TEXT_END, len(data), 16):
        chunk = data[offset : offset + 16]
        lines.append("    .byte " + ", ".join(f"0x{x:02x}" for x in chunk) + f" /* 0x{offset:05x} */")
    lines += ["payload_end:", ""]
    SOURCE.write_text("\n".join(lines), encoding="utf-8")
    print(f"functions={len(functions)} thunks={len(thunks)} resolved={sum(s is not None for _, s in thunks.values())}")
    print(SOURCE.relative_to(ROOT))


if __name__ == "__main__":
    main()
