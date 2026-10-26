#!/usr/bin/env python3
"""Reproducibly describe the Peer Pressure Stage4-specific preamble.

This parser is deliberately static.  It never executes payload bytes.  All
integer fields are decoded explicitly as big-endian values for Xbox 360 Xenon.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_PEER = ROOT / "PeerPressure/System/PeerPressureExploit-4thStage.bin"
DEFAULT_PUBLIC = ROOT / "BadUpdatePayload/BadUpdateExploit-4thStage.bin"

PEER_PREAMBLE_SIZE = 0x780
PUBLIC_PREAMBLE_SIZE = 0x180
CLEAN_HV_SIZE = 0x10000
PATCH_STREAM_START = 0x380
PATCH_STREAM_END = 0x750


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def display_path(path: Path) -> str:
    resolved = path.resolve()
    try:
        return str(resolved.relative_to(ROOT))
    except ValueError:
        return str(resolved)


def u32be(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], "big")


def hv_effective_address(raw_address: int) -> int:
    """Reproduce Stage4 instructions 0x134-0x13c exactly."""
    return raw_address | ((raw_address & 0x00030000) << 17)


def parse_patch_stream(data: bytes) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    cursor = PATCH_STREAM_START
    while cursor < PATCH_STREAM_END:
        raw_address = u32be(data, cursor)
        if raw_address == 0xFFFFFFFF:
            if cursor != PATCH_STREAM_END - 4:
                raise ValueError(f"early patch sentinel at 0x{cursor:x}")
            records.append({"sentinel_offset": f"0x{cursor:03x}", "value": "0xffffffff"})
            cursor += 4
            break
        if cursor + 8 > PATCH_STREAM_END:
            raise ValueError(f"truncated patch header at 0x{cursor:x}")
        size = u32be(data, cursor + 4)
        payload_offset = cursor + 8
        payload_end = payload_offset + size
        if payload_end > PATCH_STREAM_END:
            raise ValueError(f"patch at 0x{cursor:x} extends beyond stream")
        payload = data[payload_offset:payload_end]
        record: dict[str, object] = {
            "header_offset": f"0x{cursor:03x}",
            "payload_range": f"0x{payload_offset:03x}-0x{payload_end:03x}",
            "raw_target": f"0x{raw_address:08x}",
            "effective_target": f"0x{hv_effective_address(raw_address):x}",
            "size": size,
            "sha256": sha256(payload),
        }
        if size <= 0x3C:
            record["data_hex"] = payload.hex()
        records.append(record)
        cursor = payload_end
    if cursor != PATCH_STREAM_END:
        raise ValueError(f"patch stream ended at 0x{cursor:x}, expected 0x{PATCH_STREAM_END:x}")
    return records


@dataclass(frozen=True)
class Region:
    name: str
    file_start: int
    file_end: int
    va_start: int
    entries: tuple[int, ...]

    def va(self, file_offset: int) -> int:
        return self.va_start + file_offset - self.file_start


REGIONS = (
    Region("stage4_entry", 0x000, 0x0EC, 0x000, (0x000,)),
    Region("apply_embedded_patch_stream", 0x0EC, 0x174, 0x0EC, (0x0EC,)),
    Region("apply_address_value_pairs_hypothesis", 0x174, 0x210, 0x174, (0x174,)),
    Region("clear_mapping_low_bits_hypothesis", 0x210, 0x294, 0x210, (0x210,)),
    Region("send_smc_message", 0x294, 0x300, 0x294, (0x294,)),
    Region("status_trampoline", 0x4E8, 0x508, 0x7698, (0x4E8,)),
    Region("copied_block_jump", 0x508, 0x524, 0x76B8, (0x508,)),
    Region("rbtr_dispatch", 0x52C, 0x5BC, 0x600031FB8, (0x52C,)),
    Region("copied_continuation_hypothesis", 0x5BC, 0x62C, 0x600032048, (0x5BC,)),
)


def sign_extend(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def branch_info(word: int, address: int) -> tuple[str, int | None, bool] | None:
    opcode = word >> 26
    if opcode == 18:
        displacement = sign_extend(word & 0x03FFFFFC, 26)
        absolute = bool(word & 2)
        link = bool(word & 1)
        target = displacement if absolute else address + displacement
        return ("call" if link else "jump", target, absolute)
    if opcode == 16:
        displacement = sign_extend(word & 0xFFFC, 16)
        absolute = bool(word & 2)
        target = displacement if absolute else address + displacement
        return ("conditional", target, absolute)
    if word == 0x4E800020:
        return ("return", None, False)
    if word == 0x4E800420:
        return ("indirect_jump", None, False)
    if word == 0x4E800421:
        return ("indirect_call", None, False)
    return None


def build_cfg(data: bytes) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    nodes: list[dict[str, object]] = []
    edges: list[dict[str, object]] = []
    for region in REGIONS:
        leaders = set(region.entries)
        decoded: list[tuple[int, int, tuple[str, int | None, bool] | None]] = []
        for offset in range(region.file_start, region.file_end, 4):
            address = region.va(offset)
            info = branch_info(u32be(data, offset), address)
            decoded.append((offset, address, info))
            if info is None:
                continue
            kind, target, _ = info
            next_offset = offset + 4
            if kind in {"conditional", "call", "indirect_call"} and next_offset < region.file_end:
                leaders.add(next_offset)
            if target is not None:
                target_offset = region.file_start + target - region.va_start
                if region.file_start <= target_offset < region.file_end:
                    leaders.add(target_offset)
            if kind in {"jump", "return", "indirect_jump"} and next_offset < region.file_end:
                leaders.add(next_offset)

        sorted_leaders = sorted(leaders)
        blocks: list[tuple[int, int]] = []
        for index, start in enumerate(sorted_leaders):
            end = sorted_leaders[index + 1] if index + 1 < len(sorted_leaders) else region.file_end
            blocks.append((start, end))
            nodes.append(
                {
                    "id": f"{region.name}_{start:x}",
                    "region": region.name,
                    "file_range": f"0x{start:03x}-0x{end:03x}",
                    "va_range": f"0x{region.va(start):x}-0x{region.va(end):x}",
                }
            )

        block_for_offset = {
            offset: start
            for start, end in blocks
            for offset in range(start, end, 4)
        }
        for start, end in blocks:
            last_offset = end - 4
            address = region.va(last_offset)
            info = branch_info(u32be(data, last_offset), address)
            source = f"{region.name}_{start:x}"
            if info is None:
                if end < region.file_end:
                    edges.append({"from": source, "to": f"{region.name}_{end:x}", "kind": "fallthrough"})
                continue
            kind, target, absolute = info
            if kind == "conditional":
                if end < region.file_end:
                    edges.append({"from": source, "to": f"{region.name}_{end:x}", "kind": "false"})
                assert target is not None
                target_offset = region.file_start + target - region.va_start
                if target_offset in block_for_offset:
                    destination = f"{region.name}_{block_for_offset[target_offset]:x}"
                else:
                    destination = f"external_0x{target:x}"
                edges.append({"from": source, "to": destination, "kind": "true"})
            elif kind in {"jump", "call"}:
                assert target is not None
                target_offset = region.file_start + target - region.va_start
                if target_offset in block_for_offset:
                    destination = f"{region.name}_{block_for_offset[target_offset]:x}"
                else:
                    destination = f"external_0x{target:x}"
                edges.append(
                    {
                        "from": source,
                        "to": destination,
                        "kind": kind + ("_absolute" if absolute else ""),
                    }
                )
                if kind == "call" and end < region.file_end:
                    edges.append({"from": source, "to": f"{region.name}_{end:x}", "kind": "return_site"})
            elif kind in {"return", "indirect_jump", "indirect_call"}:
                edges.append({"from": source, "to": kind, "kind": kind})
                if kind == "indirect_call" and end < region.file_end:
                    edges.append({"from": source, "to": f"{region.name}_{end:x}", "kind": "return_site"})
    node_by_va_start = {
        int(str(node["va_range"]).split("-", 1)[0], 16): str(node["id"])
        for node in nodes
    }
    for edge in edges:
        destination = str(edge["to"])
        if destination.startswith("external_0x"):
            address = int(destination.removeprefix("external_"), 16)
            if address in node_by_va_start:
                edge["to"] = node_by_va_start[address]
    return nodes, edges


def dot_graph(nodes: list[dict[str, object]], edges: list[dict[str, object]]) -> str:
    lines = [
        "digraph peer_stage4_preamble {",
        '  graph [label="Peer Pressure Stage4 preamble CFG (static)", labelloc=t];',
        '  node [shape=box, fontname="monospace"];',
    ]
    node_ids = {str(node["id"]) for node in nodes}
    for node in nodes:
        label = f'{node["region"]}\\nfile {node["file_range"]}\\nVA {node["va_range"]}'
        lines.append(f'  "{node["id"]}" [label="{label}"];')
    external_ids = sorted(
        {str(edge["to"]) for edge in edges if str(edge["to"]) not in node_ids}
    )
    for external in external_ids:
        lines.append(f'  "{external}" [shape=ellipse, label="{external}"];')
    for edge in edges:
        lines.append(f'  "{edge["from"]}" -> "{edge["to"]}" [label="{edge["kind"]}"];')
    lines.append("}")
    return "\n".join(lines) + "\n"


def exact_range(
    public: bytes, peer: bytes, public_start: int, peer_start: int, size: int, note: str
) -> dict[str, object]:
    if public[public_start : public_start + size] != peer[peer_start : peer_start + size]:
        raise ValueError(f"claimed exact range failed: public 0x{public_start:x}, peer 0x{peer_start:x}")
    return {
        "public_range": f"0x{public_start:03x}-0x{public_start + size:03x}",
        "peer_range": f"0x{peer_start:03x}-0x{peer_start + size:03x}",
        "size": size,
        "note": note,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--peer", type=Path, default=DEFAULT_PEER)
    parser.add_argument("--public-stage4", type=Path, default=DEFAULT_PUBLIC)
    parser.add_argument("--clean-hv", type=Path, required=True)
    parser.add_argument("--json-output", type=Path)
    parser.add_argument("--dot-output", type=Path)
    args = parser.parse_args()

    peer = args.peer.read_bytes()
    public = args.public_stage4.read_bytes()
    clean_hv = args.clean_hv.read_bytes()
    if len(peer) != PEER_PREAMBLE_SIZE + CLEAN_HV_SIZE:
        raise ValueError(f"unexpected Peer Stage4 size: {len(peer)}")
    if len(public) != PUBLIC_PREAMBLE_SIZE + CLEAN_HV_SIZE:
        raise ValueError(f"unexpected public Stage4 size: {len(public)}")
    if len(clean_hv) != CLEAN_HV_SIZE:
        raise ValueError(f"unexpected clean-HV size: {len(clean_hv)}")
    if peer[PEER_PREAMBLE_SIZE:] != clean_hv:
        raise ValueError("Peer tail does not equal the supplied clean-HV block")
    if public[PUBLIC_PREAMBLE_SIZE:] != clean_hv:
        raise ValueError("public tail does not equal the supplied clean-HV block")
    if any(peer[0x308:0x380]):
        raise ValueError("Peer reserved data 0x308-0x380 is not all zero")
    if any(peer[0x750:0x780]):
        raise ValueError("Peer preamble padding 0x750-0x780 is not all zero")

    patch_records = parse_patch_stream(peer)
    nodes, edges = build_cfg(peer)
    report = {
        "schema": "peer-pressure-stage4-preamble-v1",
        "architecture": "Xbox 360 Xenon, big-endian PowerPC",
        "sources": {
            "peer_stage4": {
                "path": display_path(args.peer),
                "size": len(peer),
                "sha256": sha256(peer),
            },
            "public_badupdate_stage4": {
                "path": display_path(args.public_stage4),
                "size": len(public),
                "sha256": sha256(public),
            },
            "clean_hv": {
                "path": display_path(args.clean_hv),
                "size": len(clean_hv),
                "sha256": sha256(clean_hv),
            },
        },
        "boundaries": {
            "peer_specific_range": "0x000-0x780",
            "peer_specific_size": PEER_PREAMBLE_SIZE,
            "peer_specific_sha256": sha256(peer[:PEER_PREAMBLE_SIZE]),
            "peer_clean_hv_range": "0x780-0x10780",
            "peer_clean_hv_exact_match": True,
            "public_preamble_range": "0x000-0x180",
            "public_preamble_sha256": sha256(public[:PUBLIC_PREAMBLE_SIZE]),
            "public_clean_hv_range": "0x180-0x10180",
            "public_clean_hv_exact_match": True,
        },
        "peer_layout": [
            {"range": "0x000-0x300", "kind": "resident executable code"},
            {"range": "0x300-0x380", "kind": "fixed data/reserved"},
            {"range": "0x380-0x750", "kind": "big-endian patch stream"},
            {"range": "0x750-0x780", "kind": "zero padding"},
            {"range": "0x780-0x10780", "kind": "exact public clean-HV block"},
        ],
        "public_comparison": {
            "exact_instruction_or_data_ranges": [
                exact_range(public, peer, 0x000, 0x000, 0x018, "entry prologue"),
                exact_range(public, peer, 0x018, 0x034, 0x028, "SMC command construction"),
                exact_range(public, peer, 0x050, 0x060, 0x004, "cache-line count"),
                exact_range(public, peer, 0x05C, 0x06C, 0x00C, "HvpRelocateCacheLines call setup/call"),
                exact_range(public, peer, 0x0C0, 0x0CC, 0x020, "entry return constant and epilogue"),
                exact_range(public, peer, 0x0E0, 0x294, 0x06C, "complete SMC helper; source-reuse eligible"),
                exact_range(public, peer, 0x14C, 0x300, 0x008, "clean-HV destination qword"),
            ],
            "relocation_adjusted_correspondences": [
                {
                    "public": "file 0x054: ld r4,0x14c(r31)",
                    "peer": "file 0x064: ld r4,0x300(r31)",
                    "meaning": "load each image's clean-HV destination qword",
                },
                {
                    "public": "file 0x058: addi r3,r31,0x180",
                    "peer": "file 0x068: addi r3,r31,0x780",
                    "meaning": "address each image's clean-HV block",
                },
                {
                    "public": "file 0x0e0-0x14c",
                    "peer": "file 0x294-0x300",
                    "delta": "+0x1b4",
                    "meaning": "byte-identical complete SMC helper relocated within the blob",
                },
                {
                    "public": "file 0x14c-0x154",
                    "peer": "file 0x300-0x308",
                    "delta": "+0x1b4",
                    "meaning": "byte-identical clean-HV destination qword relocated with the helper",
                },
            ],
        },
        "known_entry_calls": [
            {"file_offset": "0x05c", "target": "B+0x294", "kind": "direct local call"},
            {"file_offset": "0x074", "target": "0x0e14", "kind": "indirect HV call"},
            {"file_offset": "0x07c", "target": "B+0x0ec", "kind": "direct local call"},
            {"file_offset": "0x098", "target": "0x057c", "kind": "indirect HV call"},
            {"file_offset": "0x0b4", "target": "B+0x174", "kind": "direct local call"},
            {"file_offset": "0x0c4", "target": "B+0x210", "kind": "direct local call"},
        ],
        "patch_stream": {
            "range": "0x380-0x750",
            "size": PATCH_STREAM_END - PATCH_STREAM_START,
            "sha256": sha256(peer[PATCH_STREAM_START:PATCH_STREAM_END]),
            "records": patch_records,
        },
        "copied_regions": [
            {
                "file_range": "0x4e8-0x524",
                "effective_va_range": "0x7698-0x76d4",
                "sha256": sha256(peer[0x4E8:0x524]),
            },
            {
                "file_range": "0x52c-0x62c",
                "effective_va_range": "0x600031fb8-0x6000320b8",
                "kind": "executable portion of 0x220-byte record",
                "sha256": sha256(peer[0x52C:0x62C]),
            },
            {
                "file_range": "0x62c-0x73c",
                "effective_va_range": "0x6000320b8-0x6000321c8",
                "kind": "hypothesized RSA public-key structure; not source-identified",
                "sha256": sha256(peer[0x62C:0x73C]),
            },
            {
                "file_range": "0x73c-0x74c",
                "effective_va_range": "0x6000321c8-0x6000321d8",
                "kind": "zero tail within copied record",
                "sha256": sha256(peer[0x73C:0x74C]),
            },
        ],
        "cfg": {"nodes": nodes, "edges": edges},
    }

    encoded = json.dumps(report, indent=2) + "\n"
    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")
    if args.dot_output:
        args.dot_output.parent.mkdir(parents=True, exist_ok=True)
        args.dot_output.write_text(dot_graph(nodes, edges), encoding="utf-8")


if __name__ == "__main__":
    main()
