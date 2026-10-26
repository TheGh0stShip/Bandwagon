#!/usr/bin/env python3
"""Assemble recovered Peer Pressure code and prove byte identity statically."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
STAGE4_SOURCE = ROOT / "analysis/reconstruction/PeerPressureExploit-4thStage.reconstructed.S"
STAGE4_REFERENCE = ROOT / "PeerPressure/System/PeerPressureExploit-4thStage.bin"
STAGE3_SOURCE = ROOT / "analysis/reconstruction/PeerPressureExploit-3rdStage.reconstructed.S"
STAGE3_REFERENCE = ROOT / "BadUpdatePayload/BadUpdateExploit-3rdStage.bin"
STAGE2_SOURCE = ROOT / "analysis/reconstruction/PeerPressureExploit-Stage2.reconstructed.S"
STAGE2_REFERENCE = ROOT / "Hdd/PeerPressureExploit-Stage2.bin"
HDD_SECURITY_SOURCE = ROOT / "analysis/reconstruction/HddSecuritySectorPayload.reconstructed.S"
HDD_SECURITY_REFERENCE = ROOT / "Hdd/HddSecuritySectorPayload.bin"
FLASH_DMA_SOURCE = ROOT / "analysis/reconstruction/FlashDmaPayload.reconstructed.S"
FLASH_DMA_REFERENCE = ROOT / "Flash/FlashDmaPayload.bin"
KERNEL_EXPANSION_SOURCE = ROOT / "analysis/reconstruction/KernelExpansion.reconstructed.S"
KERNEL_EXPANSION_REFERENCE = ROOT / "PeerPressure/System/KernelExpansion.bin"
KERNEL_PATCH_SOURCE = ROOT / "analysis/reconstruction/kernel_patches.reconstructed.S"
KERNEL_PATCH_REFERENCE = ROOT / "PeerPressure/System/kernel_patches.bin"

STAGE3_RANGES = (
    (0x000, 0x0F8, "Peer Stage3 entry point and alignment"),
    (0x0F8, 0x1E8, "Peer-specific probe function and alignment"),
    (0x1E8, 0x490, "modified RunBadUpdateExploit"),
    (0x680, 0x68C, "syscall 0x0d stub"),
    (0x690, 0x69C, "syscall 0x00 stub"),
    (0x11D0, 0x11EC, "savegpr_27 fragment"),
    (0x1220, 0x1240, "restoregpr_27 return fragment"),
    (0x1240, 0x12A0, "fatal-error, physical-memory, memset, SMC, and launch stubs"),
    (0x12A0, 0x12C0, "allocation and memcpy stubs"),
    (0x15A0, 0x15D8, "embedded HV probe"),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise SystemExit(f"required tool is not installed: {name}")
    return path


def assemble(source: Path, output: Path, llvm_mc: str, llvm_objcopy: str) -> None:
    object_path = output.with_suffix(".o")
    subprocess.run(
        [
            llvm_mc,
            "-triple=powerpc64-unknown-linux-gnu",
            "-filetype=obj",
            "-I",
            str(ROOT),
            str(source),
            "-o",
            str(object_path),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run(
        [llvm_objcopy, "-O", "binary", "--only-section=.text", str(object_path), str(output)],
        cwd=ROOT,
        check=True,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true", help="emit machine-readable results")
    args = parser.parse_args()

    llvm_mc = require_tool("llvm-mc")
    llvm_objcopy = require_tool("llvm-objcopy")
    with tempfile.TemporaryDirectory(prefix="peerpressure-rebuild-") as temporary:
        temporary_path = Path(temporary)
        stage4_output = temporary_path / "stage4.bin"
        stage3_output = temporary_path / "stage3-regions.bin"
        stage2_output = temporary_path / "stage2.bin"
        hdd_security_output = temporary_path / "hdd-security.bin"
        flash_dma_output = temporary_path / "flash-dma.bin"
        kernel_expansion_output = temporary_path / "kernel-expansion.bin"
        kernel_patch_output = temporary_path / "kernel-patches.bin"
        assemble(STAGE4_SOURCE, stage4_output, llvm_mc, llvm_objcopy)
        assemble(STAGE3_SOURCE, stage3_output, llvm_mc, llvm_objcopy)
        assemble(STAGE2_SOURCE, stage2_output, llvm_mc, llvm_objcopy)
        assemble(HDD_SECURITY_SOURCE, hdd_security_output, llvm_mc, llvm_objcopy)
        assemble(FLASH_DMA_SOURCE, flash_dma_output, llvm_mc, llvm_objcopy)
        assemble(KERNEL_EXPANSION_SOURCE, kernel_expansion_output, llvm_mc, llvm_objcopy)
        assemble(KERNEL_PATCH_SOURCE, kernel_patch_output, llvm_mc, llvm_objcopy)

        kernel_patch_built = kernel_patch_output.read_bytes()
        kernel_patch_reference = KERNEL_PATCH_REFERENCE.read_bytes()
        if kernel_patch_built != kernel_patch_reference:
            raise SystemExit("Kernel patch stream reconstruction mismatch")

        kernel_expansion_built = kernel_expansion_output.read_bytes()
        kernel_expansion_reference = KERNEL_EXPANSION_REFERENCE.read_bytes()
        if kernel_expansion_built != kernel_expansion_reference:
            raise SystemExit("Kernel expansion reconstruction mismatch")

        flash_dma_built = flash_dma_output.read_bytes()
        flash_dma_reference = FLASH_DMA_REFERENCE.read_bytes()
        if flash_dma_built != flash_dma_reference:
            raise SystemExit("Flash DMA payload reconstruction mismatch")

        hdd_security_built = hdd_security_output.read_bytes()
        hdd_security_reference = HDD_SECURITY_REFERENCE.read_bytes()
        if hdd_security_built != hdd_security_reference:
            raise SystemExit("HDD security-sector payload reconstruction mismatch")

        stage2_built = stage2_output.read_bytes()
        stage2_reference = STAGE2_REFERENCE.read_bytes()
        if stage2_built != stage2_reference:
            mismatch = next(
                (
                    offset
                    for offset, (actual, expected) in enumerate(zip(stage2_built, stage2_reference))
                    if actual != expected
                ),
                min(len(stage2_built), len(stage2_reference)),
            )
            raise SystemExit(f"Stage2 reconstruction mismatch at file offset 0x{mismatch:x}")

        stage4_built = stage4_output.read_bytes()
        stage4_reference = STAGE4_REFERENCE.read_bytes()
        if stage4_built != stage4_reference:
            mismatch = next(
                (
                    offset
                    for offset, (actual, expected) in enumerate(zip(stage4_built, stage4_reference))
                    if actual != expected
                ),
                min(len(stage4_built), len(stage4_reference)),
            )
            raise SystemExit(f"Stage4 reconstruction mismatch at file offset 0x{mismatch:x}")

        stage3_built = stage3_output.read_bytes()
        stage3_reference = STAGE3_REFERENCE.read_bytes()
        stage3_results = []
        for start, end, description in STAGE3_RANGES:
            if len(stage3_built) < end:
                raise SystemExit(f"Stage3 reconstruction ends before 0x{end:x}")
            actual = stage3_built[start:end]
            expected = stage3_reference[start:end]
            if actual != expected:
                mismatch = next(
                    offset for offset, pair in enumerate(zip(actual, expected)) if pair[0] != pair[1]
                )
                raise SystemExit(
                    f"Stage3 {description} mismatch at file offset 0x{start + mismatch:x}"
                )
            stage3_results.append(
                {
                    "range": f"0x{start:04x}-0x{end:04x}",
                    "description": description,
                    "size": end - start,
                    "sha256": sha256(actual),
                    "byte_identical": True,
                }
            )

        if stage3_built != stage3_reference:
            mismatch = next(
                (
                    offset
                    for offset, (actual, expected) in enumerate(
                        zip(stage3_built, stage3_reference)
                    )
                    if actual != expected
                ),
                min(len(stage3_built), len(stage3_reference)),
            )
            raise SystemExit(
                f"Stage3 whole-file reconstruction mismatch at file offset 0x{mismatch:x}"
            )

        result = {
            "assembler_target": "powerpc64-unknown-linux-gnu (big-endian)",
            "flash_dma": {
                "source": str(FLASH_DMA_SOURCE.relative_to(ROOT)),
                "reference": str(FLASH_DMA_REFERENCE.relative_to(ROOT)),
                "size": len(flash_dma_built),
                "sha256": sha256(flash_dma_built),
                "whole_file_byte_identical": True,
            },
            "kernel_expansion": {
                "source": str(KERNEL_EXPANSION_SOURCE.relative_to(ROOT)),
                "reference": str(KERNEL_EXPANSION_REFERENCE.relative_to(ROOT)),
                "size": len(kernel_expansion_built),
                "sha256": sha256(kernel_expansion_built),
                "whole_file_byte_identical": True,
            },
            "kernel_patch_stream": {
                "source": str(KERNEL_PATCH_SOURCE.relative_to(ROOT)),
                "reference": str(KERNEL_PATCH_REFERENCE.relative_to(ROOT)),
                "size": len(kernel_patch_built),
                "sha256": sha256(kernel_patch_built),
                "whole_file_byte_identical": True,
            },
            "hdd_security_sector": {
                "source": str(HDD_SECURITY_SOURCE.relative_to(ROOT)),
                "reference": str(HDD_SECURITY_REFERENCE.relative_to(ROOT)),
                "size": len(hdd_security_built),
                "sha256": sha256(hdd_security_built),
                "whole_file_byte_identical": True,
            },
            "stage2": {
                "source": str(STAGE2_SOURCE.relative_to(ROOT)),
                "reference": str(STAGE2_REFERENCE.relative_to(ROOT)),
                "size": len(stage2_built),
                "sha256": sha256(stage2_built),
                "whole_file_byte_identical": True,
            },
            "stage4": {
                "source": str(STAGE4_SOURCE.relative_to(ROOT)),
                "reference": str(STAGE4_REFERENCE.relative_to(ROOT)),
                "size": len(stage4_built),
                "sha256": sha256(stage4_built),
                "whole_file_byte_identical": True,
            },
            "stage3": {
                "source": str(STAGE3_SOURCE.relative_to(ROOT)),
                "reference": str(STAGE3_REFERENCE.relative_to(ROOT)),
                "reference_sha256": sha256(stage3_reference),
                "size": len(stage3_built),
                "verified_ranges": stage3_results,
                "whole_file_reconstructed": True,
                "whole_file_byte_identical": True,
            },
        }

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(
            "Kernel patch stream: exact whole-file rebuild, "
            f"{result['kernel_patch_stream']['size']} bytes, "
            f"SHA-256 {result['kernel_patch_stream']['sha256']}"
        )
        print(
            "Kernel expansion: exact whole-file rebuild, "
            f"{result['kernel_expansion']['size']} bytes, "
            f"SHA-256 {result['kernel_expansion']['sha256']}"
        )
        print(
            "Flash DMA payload: exact whole-file rebuild, "
            f"{result['flash_dma']['size']} bytes, SHA-256 {result['flash_dma']['sha256']}"
        )
        print(
            "HDD security sector payload: exact whole-file rebuild, "
            f"{result['hdd_security_sector']['size']} bytes, "
            f"SHA-256 {result['hdd_security_sector']['sha256']}"
        )
        print(
            "Stage2: exact whole-file rebuild, "
            f"{result['stage2']['size']} bytes, SHA-256 {result['stage2']['sha256']}"
        )
        print(
            "Stage4: exact whole-file rebuild, "
            f"{result['stage4']['size']} bytes, SHA-256 {result['stage4']['sha256']}"
        )
        print(
            "Stage3: exact whole-file rebuild, "
            f"{result['stage3']['size']} bytes, "
            f"SHA-256 {result['stage3']['reference_sha256']}"
        )


if __name__ == "__main__":
    main()
