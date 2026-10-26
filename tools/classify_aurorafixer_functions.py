#!/usr/bin/env python3
"""Classify every AuroraFixer code entry as Peer logic, runtime, or import."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FUNCTIONS = ROOT / "analysis/xex/AuroraFixer-functions.json"
FINGERPRINTS = ROOT / "analysis/xex/shared-function-fingerprints.json"
IMAGE = ROOT / "analysis/xex/basefiles/PeerPressure_SideLoading_AuroraFixer.pe"
OUTPUT = ROOT / "analysis/xex/AuroraFixer-completeness.json"

PEER_NAMES = {
    0x88010000: "FlushInstructionCacheSyscall",
    0x88010018: "PatchAbsoluteBranch",
    0x880100F0: "PatchModuleImportByOrdinal",
    0x88010300: "AuroraFixerDllMain",
}
RUNTIME_NAMES = {
    0x88010370: "CrtDllInitDispatch",
    0x8801049C: "CrtDllInitUnwindHelper",
    0x88010510: "DllMainCrtStartup",
    0x88010638: "CrtObjectCleanupThunk",
    0x88010668: "DllEntryPoint",
    0x88010670: "memcpy",
    0x88010BA0: "__savegprlr",
}

def main() -> None:
    function_map = json.loads(FUNCTIONS.read_text(encoding="utf-8"))
    fingerprint_map = json.loads(FINGERPRINTS.read_text(encoding="utf-8"))
    aliases = {
        int(item["address"], 0): item
        for item in fingerprint_map["propagated_aliases"]
        if item["component"] == "AuroraFixer"
    }
    entries = []
    counts = {"peer_authored": 0, "compiler_runtime": 0, "import_thunk": 0}
    for function in function_map["functions"]:
        address = int(function["address"], 0)
        if address in PEER_NAMES:
            category, name, evidence = "peer_authored", PEER_NAMES[address], ["application-specific control flow before CRT boundary"]
        elif address >= 0x880147BC:
            category = "import_thunk"
            alias = aliases.get(address)
            name = alias["propagated_name"] if alias else function["name"]
            evidence = (["byte-identical to named thunk", alias["evidence_sha256"]] if alias else ["IDC import-thunk range"])
        else:
            category = "compiler_runtime"
            alias = aliases.get(address)
            name = RUNTIME_NAMES.get(address, alias["propagated_name"] if alias else function["name"])
            evidence = ["MSVC/XDK CRT region reached through DllMainCrtStartup"]
            if alias:
                evidence += ["byte-identical shared implementation", alias["evidence_sha256"]]
        counts[category] += 1
        entries.append({
            "address": function["address"], "size": function["size_to_next_symbol"],
            "original_name": function["name"], "recovered_name": name,
            "category": category, "evidence": evidence,
        })
    result = {
        "schema": "bandwagon-aurorafixer-completeness-v1",
        "image": str(IMAGE.relative_to(ROOT)).replace("\\", "/"),
        "image_sha256": hashlib.sha256(IMAGE.read_bytes()).hexdigest(),
        "pdata_and_idc_entry_count": len(entries),
        "non_pdata_code_islands": [{
            "address": "0x880102f8", "size": 4, "recovered_name": "ReturnImmediately",
            "category": "peer_authored", "evidence": ["single blr", "installed over Aurora's DbgBreakPoint import"],
        }],
        "category_counts": counts,
        "all_entries_classified": sum(counts.values()) == len(entries),
        "entries": entries,
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(counts)

if __name__ == "__main__":
    main()
