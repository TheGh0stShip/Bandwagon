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


def analyze_xbpp_handshake(board: str, payload: bytes, base: int) -> dict[str, object]:
    marker = bytes.fromhex("e5f1b458")
    offset = payload.find(marker)
    if offset < 12 or payload.find(marker, offset + 1) >= 0:
        raise SystemExit(f"{board}: expected one XBPP comparison chain")
    expected = {
        0: bytes.fromhex("e5f1b458"),
        5: bytes.fromhex("e5f2b442"),
        10: bytes.fromhex("e5f3b450"),
        15: bytes.fromhex("e5f4b450"),
    }
    for relative, prefix in expected.items():
        if payload[offset + relative : offset + relative + 4] != prefix:
            raise SystemExit(f"{board}: malformed XBPP comparison at +0x{relative:x}")
    state_clear = payload[offset - 12 : offset - 9]
    if len(state_clear) != 3 or state_clear[0] != 0x53 or state_clear[2] != 0xFE:
        raise SystemExit(f"{board}: missing XBPP state-bit clear")
    if payload[offset - 9 : offset] != bytes.fromhex("75eb0175ea1075f500"):
        raise SystemExit(f"{board}: missing XBPP initial SFR writes")

    cursor = offset + 20
    rewrites_eb = payload[cursor : cursor + 3] == bytes.fromhex("75eb01")
    if rewrites_eb:
        cursor += 3
    if payload[cursor : cursor + 6] != bytes.fromhex("75ea1875f500"):
        raise SystemExit(f"{board}: missing XBPP transfer SFR writes")
    cursor += 6
    if payload[cursor] != 0x78:
        raise SystemExit(f"{board}: missing XBPP staging-pointer load")
    staging_ram = payload[cursor + 1]
    cursor += 2
    if payload[cursor : cursor + 8] != bytes.fromhex("a6f308a6f208a6f1"):
        raise SystemExit(f"{board}: missing XBPP reverse-byte staging")
    cursor += 8
    if payload[cursor] != 0x43 or payload[cursor + 2] != 1:
        raise SystemExit(f"{board}: missing XBPP state-bit set")
    state_set_address = payload[cursor + 1]
    cursor += 3
    corona_followup = payload[cursor : cursor + 9] == bytes.fromhex("75eb0175ea1c75f500")
    if state_set_address != state_clear[1]:
        raise SystemExit(f"{board}: XBPP state clear/set address mismatch")
    sender_marker = bytes((0xE5, state_set_address, 0x30, 0xE1))
    sender = payload.find(sender_marker, cursor)
    if sender < 0 or payload.find(sender_marker, sender + 1) >= 0:
        raise SystemExit(f"{board}: expected one XBPP staged sender")
    sender_cursor = sender + 5  # Skip MOV A,state and JNB ACC.1,relative.
    sets_p2_6 = payload[sender_cursor : sender_cursor + 2] == bytes.fromhex("d2a6")
    if sets_p2_6:
        sender_cursor += 2
    if payload[sender_cursor : sender_cursor + 4] != bytes.fromhex("e5ee20e0"):
        raise SystemExit(f"{board}: missing XBPP EE.0 sender gate")
    sender_cursor += 5  # Also skip the relative branch byte.
    sender_prefix = bytes.fromhex("e5eef5ee75ea00") + bytes((0x78, staging_ram))
    sender_suffix = bytes.fromhex(
        "86eb0886ec0886ede5e344c0f5e3e5e454fc4402f5e475f506"
    )
    if payload[sender_cursor : sender_cursor + len(sender_prefix)] != sender_prefix:
        raise SystemExit(f"{board}: missing XBPP staged sender prefix")
    sender_cursor += len(sender_prefix)
    if payload[sender_cursor : sender_cursor + len(sender_suffix)] != sender_suffix:
        raise SystemExit(f"{board}: missing XBPP staged sender body")
    sender_cursor += len(sender_suffix)
    if board == "Corona":
        if not sets_p2_6:
            raise SystemExit("Corona: XBPP sender does not assert P2.6")
    elif sets_p2_6 or payload[sender_cursor] != 0x22:
        raise SystemExit(f"{board}: unexpected XBPP sender tail")
    return {
        "comparison_address": f"0x{base + offset:04x}",
        "magic_ascii": "XBPP",
        "input_sfrs": ["0xf1", "0xf2", "0xf3", "0xf4"],
        "initial_sfr_writes": {"0xeb": "0x01", "0xea": "0x10", "0xf5": "0x00"},
        "state_ram_address": f"0x{state_clear[1]:02x}",
        "staging_ram_address": f"0x{staging_ram:02x}",
        "staged_sfr_order": ["0xf3", "0xf2", "0xf1"],
        "rewrites_eb_for_transfer": rewrites_eb,
        "transfer_ea": "0x18",
        "corona_followup_ea": "0x1c" if corona_followup else None,
        "staged_sender": {
            "address": f"0x{base + sender:04x}",
            "gates": ["state_bit_1_set", "sfr_ee_bit_0_clear"],
            "busy_behavior": "wait" if board == "Corona" else "return",
            "sets_p2_bit_6": sets_p2_6,
            "destination_sfrs": ["0xeb", "0xec", "0xed"],
            "ea": "0x00",
            "e3_or_mask": "0xc0",
            "e4_and_mask": "0xfc",
            "e4_or_mask": "0x02",
            "f5": "0x06",
        },
    }


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
    return {
        "target_address": f"0x{address:04x}",
        "size": len(payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
        "xbpp_handshake": analyze_xbpp_handshake(board, payload, address),
    }


def main() -> None:
    RECON.mkdir(parents=True, exist_ok=True)
    OUTDIR.mkdir(parents=True, exist_ok=True)
    summary = {"schema": "bandwagon-smc-patch-summary-v2", "boards": []}
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
