#!/usr/bin/env python3
"""Find byte-identical functions across the reconstructed Peer XEX images."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

import pefile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "analysis/xex/shared-function-fingerprints.json"
TARGETS = {
    "AuroraFixer": "PeerPressure_SideLoading_AuroraFixer.pe",
    "BootEdit": "PeerPressure_System_BootEdit.pe",
    "BootXell": "PeerPressure_OtherOS_BootXell.pe",
    "SettingsApp": "PeerPressure_System_SettingsApp.pe",
    "UpdateCheck": "PeerPressure_System_UpdateCheck.pe",
    "Updater": "Updater.pe",
}
GENERIC = re.compile(r"^(fn_[0-9A-Fa-f]+|__imp_.+_[0-9]+|.+_(?:exe|xex)_[0-9]+|_start)$")


def main() -> None:
    groups: dict[str, list[dict[str, object]]] = {}
    for component, filename in TARGETS.items():
        image_path = ROOT / "analysis/xex/basefiles" / filename
        image = image_path.read_bytes()
        pe = pefile.PE(data=image)
        text = next(section for section in pe.sections if section.Name.rstrip(b"\0") == b".text")
        # The extracted files are memory-layout images: RVA, not PointerToRawData,
        # locates initialized text (the PE raw pointers describe the pre-map file).
        text_offset = text.VirtualAddress
        function_map = json.loads((ROOT / f"analysis/xex/{component}-functions.json").read_text(encoding="utf-8"))
        for function in function_map["functions"]:
            offset = int(function["section_offset"], 0)
            size = int(function["size_to_next_symbol"])
            body = image[text_offset + offset : text_offset + offset + size]
            if len(body) != size or not body or not any(body):
                continue
            digest = hashlib.sha256(body).hexdigest()
            groups.setdefault(digest, []).append({
                "component": component,
                "name": function["name"],
                "address": function["address"],
                "size": size,
            })

    shared = []
    for digest, instances in groups.items():
        components = sorted({str(item["component"]) for item in instances})
        if len(components) < 2:
            continue
        authoritative = sorted({str(item["name"]) for item in instances if not GENERIC.match(str(item["name"]))})
        shared.append({
            "sha256": digest,
            "size": instances[0]["size"],
            "component_count": len(components),
            "components": components,
            "authoritative_names": authoritative,
            "name_propagation_safe": len(authoritative) == 1,
            "instances": instances,
        })
    shared.sort(key=lambda item: (-int(item["component_count"]), int(item["size"]), str(item["sha256"])))
    propagated_aliases = []
    for group in shared:
        if not group["name_propagation_safe"]:
            continue
        name = group["authoritative_names"][0]
        for instance in group["instances"]:
            if GENERIC.match(str(instance["name"])):
                propagated_aliases.append({
                    "component": instance["component"],
                    "address": instance["address"],
                    "old_name": instance["name"],
                    "propagated_name": name,
                    "evidence_sha256": group["sha256"],
                })
    propagated_aliases.sort(key=lambda item: (str(item["component"]), int(str(item["address"]), 0)))
    result = {
        "schema": "bandwagon-shared-xex-functions-v1",
        "method": "SHA-256 of exact initialized function bytes in memory-layout PE images",
        "limitations": "Only exact bodies match; relocation or link-layout changes can hide shared source.",
        "group_count": len(shared),
        "instance_count": sum(len(item["instances"]) for item in shared),
        "safe_name_propagation_group_count": sum(bool(item["name_propagation_safe"]) for item in shared),
        "propagated_alias_count": len(propagated_aliases),
        "propagated_aliases": propagated_aliases,
        "groups": shared,
    }
    OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"shared_groups={result['group_count']} instances={result['instance_count']} safe_names={result['safe_name_propagation_group_count']}")


if __name__ == "__main__":
    main()
