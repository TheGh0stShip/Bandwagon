#!/usr/bin/env python3
"""Recover high-confidence SettingsApp third-party function names."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
XREFS = ROOT / "analysis/xex/SettingsApp-string-xrefs.json"
FUNCTIONS = ROOT / "analysis/xex/SettingsApp-functions.json"
CALLGRAPH = ROOT / "analysis/xex/SettingsApp-callgraph.json"
OUTPUT = ROOT / "analysis/xex/SettingsApp-thirdparty-symbols.json"

JSONCPP = {
    0x82073430: ("Json::duplicateStringValue", "in Json::Value::duplicateStringValue(): Failed to allocate string value buffer"),
    0x820739B8: ("Json::duplicateAndPrefixStringValue", "in Json::Value::duplicateAndPrefixStringValue(): length too big for prefixing"),
    0x82073E90: ("Json::Value::asCString", "in Json::Value::asCString(): requires stringValue"),
    0x82073F28: ("Json::Value::asString", "Type is not convertible to string"),
    0x82074090: ("Json::Value::asInt", "Value is not convertible to Int."),
    0x82074290: ("Json::Value::asUInt", "Value is not convertible to UInt."),
    0x82074480: ("Json::Value::asInt64", "Value is not convertible to Int64."),
    0x82074630: ("Json::Value::asBool", "Value is not convertible to bool."),
    0x82074830: ("Json::Value::find", "in Json::Value::find(begin, end): requires objectValue or nullValue"),
    0x82075358: ("Json::Value::operator[](ArrayIndex)", "in Json::Value::operator[](ArrayIndex): requires arrayValue"),
    0x82075498: ("Json::Value::operator[](int)", "in Json::Value::operator[](int index): index cannot be negative"),
    0x82075508: ("Json::Value::resolveReference", "in Json::Value::resolveReference(key, end): requires objectValue"),
    0x820759C8: ("Json::CharReaderBuilder::setDefaults", "allowTrailingCommas"),
    0x820771D8: ("Json::OurReader::getLocationLineAndColumn", "Line %d, Column %d"),
    0x82078790: ("Json::OurReader::getFormattedErrorMessages", "See "),
    0x8207AAE8: ("Json::OurReader::decodeUnicodeEscapeSequence", "Bad unicode escape sequence in string: four digits expected."),
    0x8207B010: ("Json::OurReader::decodeUnicodeCodePoint", "additional six characters expected to parse unicode surrogate pair."),
    0x8207B430: ("Json::OurReader::decodeString", "Empty escape sequence in string"),
    0x8207B708: ("Json::OurReader::readObject", "Missing '}' or object member name"),
    0x8207BB40: ("Json::OurReader::readArray", "Missing ',' or ']' in array declaration"),
    0x8207BE68: ("Json::CharReaderBuilder::newCharReader", "collectComments"),
    0x8207C010: ("Json::OurReader::readValue", "Exceeded stackLimit in readValue()."),
    0x8207C308: ("Json::OurReader::parse", "Extra non-whitespace after JSON value."),
}

BEARSSL = {
    0x820609C8: ("br_ssl_engine_compute_master", "master secret"),
    0x82060A48: ("compute_key_block", "key expansion"),
    0x82065CC0: ("br_ssl_hs_client_run", "server finished"),
}


def main() -> None:
    refs = json.loads(XREFS.read_text(encoding="utf-8"))["references"]
    funcs = json.loads(FUNCTIONS.read_text(encoding="utf-8"))["functions"]
    edges = json.loads(CALLGRAPH.read_text(encoding="utf-8"))["edges"]
    original = {int(item["address"], 0): item["name"] for item in funcs}
    strings = {}
    for item in refs:
        strings.setdefault(int(item["function_address"], 0), set()).add(item["string"])

    symbols = []
    for ownership, mapping in (("JsonCpp 00.11.0", JSONCPP), ("BearSSL 0.6", BEARSSL)):
        for address, (name, signature) in mapping.items():
            if signature not in strings.get(address, set()):
                raise SystemExit(f"{address:#x}: missing signature {signature!r}")
            symbols.append({"address": f"0x{address:08x}", "recovered_name": original[address], "semantic_name": name, "confidence": "high", "ownership": ownership, "evidence": [f"function-local exact upstream signature: {signature}"]})

    wrapper = 0x8207C4E8
    targets = {int(edge["target_address"], 0) for edge in edges if int(edge["caller_address"], 0) == wrapper}
    if not {0x8207C308, 0x82078790}.issubset(targets):
        raise SystemExit("OurCharReader::parse call-chain evidence changed")
    symbols.append({"address": f"0x{wrapper:08x}", "recovered_name": original[wrapper], "semantic_name": "Json::OurCharReader::parse", "confidence": "high", "ownership": "JsonCpp 00.11.0", "evidence": ["calls recovered OurReader::parse and OurReader::getFormattedErrorMessages"]})

    symbols.sort(key=lambda item: int(item["address"], 0))
    result = {"schema": "bandwagon-settingsapp-thirdparty-symbols-v1", "image_sha256": "a77a2a06d71c9a9512879122187fe5541270056f54657574d5a2fe9a109e035a", "counts": {"jsoncpp": len(JSONCPP) + 1, "bearssl": len(BEARSSL)}, "symbols": symbols}
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"SettingsApp third-party symbols: {len(symbols)} recovered")


if __name__ == "__main__":
    main()
