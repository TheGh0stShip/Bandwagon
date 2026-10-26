#!/usr/bin/env python3
"""Emit exact assembly and 8051 listings for all motherboard SMC patch streams."""

from __future__ import annotations

import hashlib
import json
import subprocess
from difflib import SequenceMatcher
from pathlib import Path

from decode_patch_records import decode

ROOT = Path(__file__).resolve().parent.parent
BOARDS = ("Corona", "Falcon", "Jasper", "Trinity", "Xenon", "Zephyr")
RECON = ROOT / "analysis/reconstruction"
OUTDIR = ROOT / "analysis/smc"


def emit_source(board: str, records: list[dict[str, object]]) -> Path:
    output = RECON / f"{board}_SmcExploit.reconstructed.S"
    lines = [
        f"/* Lossless source for Flash/{board}_SmcExploit.bin.",
        " * Record format: SMC address, byte count, replacement bytes; big-endian header.",
        " */", ".section .text", ".balign 4", f"{board.lower()}_smc_patch_stream:",
    ]
    for index, record in enumerate(records):
        if "sentinel" in record:
            lines.append("    .long 0xffffffff")
            continue
        payload = bytes.fromhex(str(record["data_hex"]))
        address = int(str(record["target_address"]), 0)
        kind = "ljmp_redirect" if len(payload) == 3 and payload[0] == 0x02 else "replacement"
        if len(payload) >= 0x100:
            kind = "embedded_8051_payload"
        lines += [
            f"patch_{index:02d}_{kind}:",
            f"    .long 0x{address:08x}",
            f"    .long {len(payload)}",
        ]
        for offset in range(0, len(payload), 16):
            chunk = payload[offset : offset + 16]
            lines.append("    .byte " + ", ".join(f"0x{x:02x}" for x in chunk))
    lines += [f"{board.lower()}_smc_patch_stream_end:", ""]
    output.write_text("\n".join(lines), encoding="utf-8")
    return output


def emit_8051_listing(board: str, records: list[dict[str, object]]) -> dict[str, object]:
    candidates = [r for r in records if "data_hex" in r and int(r["size"]) >= 0x100]
    if len(candidates) != 1:
        raise SystemExit(f"{board}: expected one embedded 8051 payload, found {len(candidates)}")
    record = candidates[0]
    address = int(str(record["target_address"]), 0)
    payload = bytes.fromhex(str(record["data_hex"]))
    command = ["wsl", "rasm2", "-a", "8051", "-o", hex(address), "-D", payload.hex()]
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    listing = OUTDIR / f"{board}_SmcExploit.8051.asm"
    header = (
        f"; source: Flash/{board}_SmcExploit.bin embedded replacement record\n"
        f"; target_address: 0x{address:04x}\n"
        f"; payload_size: {len(payload)}\n"
        f"; payload_sha256: {hashlib.sha256(payload).hexdigest()}\n"
        "; architecture: Intel 8051-compatible SMC\n\n"
    )
    listing.write_text(header + result.stdout, encoding="utf-8")
    return {"target_address": f"0x{address:04x}", "size": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}


def main() -> None:
    RECON.mkdir(parents=True, exist_ok=True)
    OUTDIR.mkdir(parents=True, exist_ok=True)
    summary = {"schema": "bandwagon-smc-patch-summary-v1", "boards": []}
    embedded_payloads: dict[str, bytes] = {}
    for board in BOARDS:
        source = ROOT / "Flash" / f"{board}_SmcExploit.bin"
        data = source.read_bytes()
        records = decode(data)
        reconstructed = emit_source(board, records)
        payload = emit_8051_listing(board, records)
        embedded_record = next(r for r in records if "data_hex" in r and int(r["size"]) >= 0x100)
        embedded_payloads[board] = bytes.fromhex(str(embedded_record["data_hex"]))
        summary["boards"].append({
            "board": board,
            "source": str(source.relative_to(ROOT)).replace("\\", "/"),
            "size": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "record_count": sum("sentinel" not in r for r in records),
            "ljmp_redirect_count": sum(
                "data_hex" in r and len(bytes.fromhex(str(r["data_hex"]))) == 3
                and bytes.fromhex(str(r["data_hex"]))[0] == 0x02
                for r in records
            ),
            "embedded_8051": payload,
            "reconstruction": str(reconstructed.relative_to(ROOT)).replace("\\", "/"),
        })
        print(reconstructed.relative_to(ROOT))
    summary["embedded_payload_similarity"] = []
    for index, left in enumerate(BOARDS):
        for right in BOARDS[index + 1:]:
            summary["embedded_payload_similarity"].append({
                "left": left,
                "right": right,
                "sequence_match_ratio": round(
                    SequenceMatcher(None, embedded_payloads[left], embedded_payloads[right], autojunk=False).ratio(), 6
                ),
            })
    (OUTDIR / "SmcExploit-summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
