# Peer Pressure UI assets

`HudResources.xzp` is now losslessly unpacked instead of being treated as an
opaque binary asset. `tools/xuiz_archive.py` validates the big-endian `XUIZ`
header and directory, rejects unsafe paths, extracts all four members, and
rebuilds the original 2,871-byte archive byte-for-byte. The archive inventory
and member hashes are in `HudResources-xuiz.json`.

Each member is a compiled `XUIB`/`.xur` resource. `tools/decode_xur.py` validates
its four contiguous `STRN`, `VECT`, `FLOT`, and `DATA` sections, exposes the
string table and big-endian floating-point values in readable JSON, preserves
all section bytes, and rebuilds every member byte-for-byte. The adjacent
`.xur.json` files are the lossless source forms.

This closes preservation and unpack/repack coverage for the only manifest UI
archive. It is not yet a claim that the proprietary `DATA` instruction stream
has been translated back into original Xbox UI XML; the JSON deliberately
marks that remaining semantic-decompilation boundary instead of inventing XUI.

Run:

```sh
python3 tools/test_asset_unpackers.py
```

The test performs an exact XZP unpack/repack, exact decode/rebuild of all four
XUR members, and a safe extraction of all 35 wrapped update-archive members.
