#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IMAGE = ROOT / "analysis/xex/basefiles/PeerPressure_System_BootEdit.pe"
EXPECTED_IMAGE = "1396cf0f05e5cb07de4f2ca067975376b0b319dd52f3ca4182af8b00d1175df8"
EXPECTED_COLOR_EQUALS = "1c39100e26bf568f13260a9672759e5cd09aa4de8bca4316f4889cf0ba2c8c33"


def load(name):
    return json.loads((ROOT / "analysis/xex" / name).read_text(encoding="utf-8"))


image = IMAGE.read_bytes()
assert hashlib.sha256(image).hexdigest() == EXPECTED_IMAGE
assert hashlib.sha256(image[0x50A00:0x50A4C]).hexdigest() == EXPECTED_COLOR_EQUALS

symbols = {item["semantic_name"]: item for item in load("BootEdit-symbols.json")["symbols"]}
assert symbols["ColorEqualsExact"]["address"] == "0x82050a00"
assert symbols["ColorEqualsExact"]["recovered_name"] is None
assert symbols["DrawColorContextMenu"]["address"] == "0x82050b08"

functions = {item["name"]: item for item in load("BootEdit-functions.json")["functions"]}
assert functions["fn_82050B08"]["address"].lower() == "0x82050b08"

references = load("BootEdit-string-xrefs.json")["references"]
menu_strings = {
    item["string"] for item in references if item["function"] == "fn_82050B08"
}
assert {"Copy", "Paste", "Undo changes", "Reset to default"} <= menu_strings
theme_strings = {
    item["string"] for item in references if item["function"] == "fn_820506B8"
}
assert {"Themes:\\*.xbat", ".xbat", "Themes:\\"} <= theme_strings

edges = load("BootEdit-callgraph.json")["edges"]
assert any(
    edge["caller"] == "fn_82050B08"
    and edge["target_address"].lower() == "0x82050a00"
    for edge in edges
)

print("BootEdit semantic analysis validated")
