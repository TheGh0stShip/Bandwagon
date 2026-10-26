#!/usr/bin/env python3
"""Recover high-confidence JsonCpp symbols from exact diagnostic xrefs."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
XREFS = ROOT / "analysis/xex/UpdateCheck-string-xrefs.json"
FUNCTIONS = ROOT / "analysis/xex/UpdateCheck-functions.json"
CALLGRAPH = ROOT / "analysis/xex/UpdateCheck-callgraph.json"
OUTPUT = ROOT / "analysis/xex/UpdateCheck-symbols.json"

# Address: (semantic name, binary-resident source signature).
JSONCPP = {
    0x91A22F88: ("Json::CharReaderBuilder::setDefaults", "allowTrailingCommas"),
    0x91A250A8: ("Json::OurReader::getLocationLineAndColumn", "Line %d, Column %d"),
    0x91A26730: ("Json::OurReader::getFormattedErrorMessages", "See "),
    0x91A28A88: ("Json::OurReader::decodeUnicodeEscapeSequence", "Bad unicode escape sequence in string: four digits expected."),
    0x91A28FB0: ("Json::OurReader::decodeUnicodeCodePoint", "additional six characters expected to parse unicode surrogate pair."),
    0x91A293D0: ("Json::OurReader::decodeString", "Empty escape sequence in string"),
    0x91A296A8: ("Json::OurReader::readObject", "Missing '}' or object member name"),
    0x91A29AE0: ("Json::OurReader::readArray", "Missing ',' or ']' in array declaration"),
    0x91A29E08: ("Json::CharReaderBuilder::newCharReader", "collectComments"),
    0x91A29FB0: ("Json::OurReader::readValue", "Exceeded stackLimit in readValue()."),
    0x91A2A2A8: ("Json::OurReader::parse", "Extra non-whitespace after JSON value."),
    0x91A2B2F8: ("Json::duplicateStringValue", "in Json::Value::duplicateStringValue(): Failed to allocate string value buffer"),
    0x91A2B880: ("Json::duplicateAndPrefixStringValue", "in Json::Value::duplicateAndPrefixStringValue(): length too big for prefixing"),
    0x91A2BC88: ("Json::Value::asCString", "in Json::Value::asCString(): requires stringValue"),
    0x91A2BD20: ("Json::Value::asString", "Type is not convertible to string"),
    0x91A2BE88: ("Json::Value::asUInt", "Value is not convertible to UInt."),
    0x91A2C078: ("Json::Value::asBool", "Value is not convertible to bool."),
    0x91A2C278: ("Json::Value::find", "in Json::Value::find(begin, end): requires objectValue or nullValue"),
    0x91A2CCE0: ("Json::Value::operator[](ArrayIndex)", "in Json::Value::operator[](ArrayIndex): requires arrayValue"),
    0x91A2CE20: ("Json::Value::operator[](int)", "in Json::Value::operator[](int index): index cannot be negative"),
    0x91A2CE90: ("Json::Value::resolveReference", "in Json::Value::resolveReference(key, end): requires objectValue"),
}

DERIVED = {
    0x91A2A488: (
        "Json::OurCharReader::parse",
        {0x91A2A2A8, 0x91A26730},
        "calls recovered OurReader::parse and OurReader::getFormattedErrorMessages",
    ),
}

APPLICATION = [
    {
        "address": "0x91a0c3e8", "recovered_name": "fn_91A0C3E8",
        "semantic_name": "ParseReleaseVersion", "confidence": "high",
        "evidence": ["looks up JSON member tag_name", "parses %*c%d.%d.%d and requires three conversions", "packs the three parsed components into the caller output"],
    },
    {
        "address": "0x91a0d860", "recovered_name": "fn_91A0D860",
        "semantic_name": "CheckLatestReleaseVersion", "confidence": "high",
        "evidence": ["loads PPROOT:\\System\\IncludedRootsPEM.txt", "requests the GitHub latest-release API URL", "requires HTTP status 200 and invokes ParseReleaseVersion", "returns success only when the parsed output fits the caller buffer"],
    },
]


def main() -> None:
    xrefs = json.loads(XREFS.read_text(encoding="utf-8"))["references"]
    functions = json.loads(FUNCTIONS.read_text(encoding="utf-8"))["functions"]
    edges = json.loads(CALLGRAPH.read_text(encoding="utf-8"))["edges"]
    original = {int(item["address"], 0): item["name"] for item in functions}
    by_function = {}
    for item in xrefs:
        by_function.setdefault(int(item["function_address"], 0), set()).add(item["string"])

    recovered = []
    for address, (name, signature) in JSONCPP.items():
        if signature not in by_function.get(address, set()):
            raise SystemExit(f"{address:#x}: missing expected JsonCpp signature {signature!r}")
        recovered.append({
            "address": f"0x{address:08x}", "recovered_name": original[address],
            "semantic_name": name, "confidence": "high", "ownership": "JsonCpp 00.11.0",
            "evidence": [f"references exact upstream diagnostic/setting: {signature}", "function role agrees with upstream source context"],
        })

    targets_by_caller = {}
    for edge in edges:
        targets_by_caller.setdefault(int(edge["caller_address"], 0), set()).add(int(edge["target_address"], 0))
    for address, (name, required_targets, evidence) in DERIVED.items():
        if not required_targets.issubset(targets_by_caller.get(address, set())):
            raise SystemExit(f"{address:#x}: missing expected JsonCpp call-chain evidence")
        recovered.append({
            "address": f"0x{address:08x}", "recovered_name": original[address],
            "semantic_name": name, "confidence": "high", "ownership": "JsonCpp 00.11.0",
            "evidence": [evidence, "control flow agrees with the upstream virtual parse wrapper"],
        })

    result = {
        "schema": "bandwagon-updatecheck-semantic-symbols-v2",
        "image_sha256": "ab6aaf5f2d3f880701269e5835cf9a16aa26c223fcf8ddccb31f8db8ffb72ece",
        "symbols": APPLICATION + recovered,
        "counts": {"peer_application": len(APPLICATION), "jsoncpp": len(recovered)},
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"UpdateCheck symbols: {len(APPLICATION)} Peer and {len(recovered)} JsonCpp routines")


if __name__ == "__main__":
    main()
