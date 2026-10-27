#!/usr/bin/env python3
"""Generate the authoritative Peer Pressure manifest component status matrix."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


CLASSIFICATIONS = {
    "Updater.xex": ("peer_executable", "exact_initialized_source", "analysis/xex/Updater.md"),
    "Flash\\bootanim.xex": ("bundled_baseline_binary", "public_baseline_identical", "analysis/badupdate-v1.3-comparison.json"),
    "Flash\\Corona_SmcCode.bin": ("platform_firmware", "reference_binary_only", None),
    "Flash\\Falcon_SmcCode.bin": ("platform_firmware", "reference_binary_only", None),
    "Flash\\Jasper_SmcCode.bin": ("platform_firmware", "reference_binary_only", None),
    "Flash\\Trinity_SmcCode.bin": ("platform_firmware", "reference_binary_only", None),
    "Flash\\Xenon_SmcCode.bin": ("platform_firmware", "reference_binary_only", None),
    "Flash\\Zephyr_SmcCode.bin": ("platform_firmware", "reference_binary_only", None),
    "Flash\\Corona_SmcExploit.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/smc/README.md"),
    "Flash\\Falcon_SmcExploit.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/smc/README.md"),
    "Flash\\Jasper_SmcExploit.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/smc/README.md"),
    "Flash\\Trinity_SmcExploit.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/smc/README.md"),
    "Flash\\Xenon_SmcExploit.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/smc/README.md"),
    "Flash\\Zephyr_SmcExploit.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/smc/README.md"),
    "Flash\\FlashDmaPayload.bin": ("peer_rop_and_data", "exact_whole_file_source", "analysis/flash-dma/FlashDmaPayload.md"),
    "Hdd\\HddSecuritySectorPayload.bin": ("peer_structured_payload", "exact_whole_file_source", "analysis/hdd-security-sector-payload.json"),
    "Hdd\\PeerPressureExploit-Stage2.bin": ("peer_rop_and_code", "exact_whole_file_source", "analysis/stage2/PeerPressureExploit-Stage2.md"),
    "PeerPressure\\OtherOS\\boot.ini": ("peer_configuration", "human_readable_source", None),
    "PeerPressure\\OtherOS\\BootXell.xex": ("peer_executable", "exact_initialized_source", "analysis/xex/BootXell.md"),
    "PeerPressure\\SideLoading\\AuroraFixer.xex": ("peer_executable", "exact_initialized_source", "analysis/xex/AuroraFixer.md"),
    "PeerPressure\\SideLoading\\SideLoad.ini": ("peer_configuration", "human_readable_source", None),
    "PeerPressure\\System\\BootEdit.xex": ("peer_executable", "exact_initialized_source", "analysis/xex/BootEdit.md"),
    "PeerPressure\\System\\Config.ini": ("peer_configuration", "human_readable_source", None),
    "PeerPressure\\System\\HudResources.xzp": ("ui_asset", "exact_whole_file_source", "analysis/assets/README.md"),
    "PeerPressure\\System\\IncludedRootsPEM.txt": ("trust_store_data", "human_readable_data", None),
    "PeerPressure\\System\\KernelExpansion.bin": ("peer_code_and_data", "exact_whole_file_source", "analysis/kernel-expansion/KernelExpansion.md"),
    "PeerPressure\\System\\kernel_patches.bin": ("peer_patch_stream", "exact_whole_file_source", "analysis/kernel-patches.md"),
    "PeerPressure\\System\\PeerPressureExploit-4thStage.bin": ("peer_code_and_data", "exact_whole_file_source", "analysis/reconstruction/PeerPressureExploit-4thStage.reconstructed.S"),
    "PeerPressure\\System\\SettingsApp.xex": ("peer_executable", "exact_initialized_source", "analysis/xex/SettingsApp.md"),
    "PeerPressure\\System\\UpdateCheck.xex": ("peer_executable", "exact_initialized_source", "analysis/xex/UpdateCheck.md"),
    "PeerPressure\\System\\update_data.bin": ("public_badupdate_data", "public_baseline_identical", "analysis/badupdate-v1.3-comparison.json"),
    "PeerPressure\\System\\xke_update.bin": ("public_badupdate_data", "public_baseline_identical", "analysis/badupdate-v1.3-comparison.json"),
    "PeerPressure\\Themes\\BootAnim\\9199\\bootanim.xex": ("bundled_microsoft_component", "provenance_only", "analysis/xex/BootAnim-9199-provenance.md"),
}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    entries = []
    for manifest_entry in manifest["entries"]:
        path = manifest_entry["path"]
        if path not in CLASSIFICATIONS:
            raise SystemExit(f"unclassified manifest entry: {path}")
        category, status, evidence = CLASSIFICATIONS[path]
        local = args.root.joinpath(*path.split("\\"))
        data = local.read_bytes()
        sha1 = hashlib.sha1(data).hexdigest()
        if sha1 != manifest_entry["sha1"]:
            raise SystemExit(f"manifest SHA-1 mismatch: {path}")
        entries.append(
            {
                "path": path,
                "size": len(data),
                "sha1": sha1,
                "sha256": hashlib.sha256(data).hexdigest(),
                "category": category,
                "status": status,
                "evidence": evidence,
            }
        )
    counts: dict[str, int] = {}
    for entry in entries:
        counts[entry["status"]] = counts.get(entry["status"], 0) + 1
    unresolved = [entry["path"] for entry in entries if entry["status"] == "unresolved"]
    result = {
        "schema": "bandwagon-component-inventory-v1",
        "softmod_version": manifest["softmod_version"],
        "entry_count": len(entries),
        "status_counts": dict(sorted(counts.items())),
        "unresolved_peer_payloads": unresolved,
        "entries": entries,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"Inventory: {len(entries)} entries; unresolved Peer payloads: {len(unresolved)}")


if __name__ == "__main__":
    main()
