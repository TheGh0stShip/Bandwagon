# BootXell.xex analysis

## Provenance and exact reconstruction

- Container: `PeerPressure/OtherOS/BootXell.xex`, 47,104 bytes
- Extracted image: 106,496 bytes
- Image SHA-256: `4ef810a55390105a0756d2dc48502f8d9fad50edc08d8617fe146bc3ae496ebc`
- Load address: `0x91a00000`
- Entry point: `0x91a0c378`
- Toolchain: Xbox/XAPI `2.0.21256.0`, MSVC `16.0.11886.0`

[`BootXell.text.reconstructed.S`](../reconstruction/BootXell.text.reconstructed.S)
rebuilds all 74,840 initialized `.text` bytes exactly (SHA-256
`accec1f0174a197cf7fd3aa658a84d801c1fa8f660f525d8fce9d570f6cde818`).
Its 256 labels combine 207 exception-table entries, the PE entry point, the
text start, and 48 import names recovered from the XEX IDC metadata.

[`BootXell.nontext.reconstructed.S`](../reconstruction/BootXell.nontext.reconstructed.S)
exactly rebuilds the initialized application data:

| Section | Bytes | SHA-256 |
| --- | ---: | --- |
| `.rdata` | 13,356 | `2bbd65b9fe43f4f0b26f7901a658baebded9b8d7365bba6e6851ef6a4e49528c` |
| `.pdata` | 1,656 | `78157e45b133060fb8880d3e52384974eef9cca31c2b4cf33e9c289b1da6edb1` |
| `.data` | 6,392 | `c46626e04cfa9ccf3c72c3b7796001104b3c5a116bb938a33e33a725d2d106de` |

The generated call graph contains 765 direct calls, 503 resolving exactly to a
current label. The string-reference map contains 28 references in 16 labeled
functions. Regenerate all artifacts with `tools/analyze_bootxell_xex.sh`.

## Recovered application behavior

The INI callback at `0x91a06188` creates a 72-byte option record for each
`BootOption` and recognizes `Name`, `FileName`, `BaseAddress`, and
`BypassXellScanning`. The bypass is not a general Boolean: it is enabled only
when its value equals `I Understand This Can Break My Console`. `BaseAddress`
is parsed in radix 16 and stored as a 64-bit value.

`LaunchBootOption` at `0x91a064d8` prefixes the selected filename with
`PPROOT:\\OtherOS\\`, opens the file, reads it into 128-byte-aligned physical
memory, and—unless bypass was explicitly acknowledged—requires its final 16
bytes to equal `xxxxxxxxxxxxxxxx`. It then constructs an `rbtr` launch request
using either the configured base address or the allocated physical address.
All handles and allocations are released on the observed exit paths.

`BootXellMain` at `0x91a06658` loads
`PPROOT:\\OtherOS\\boot.ini`, selects the requested named option from the parsed
records, and passes it to `LaunchBootOption`. Platform import ordinals are named
only when supported by exact cross-XEX fingerprints. The provenance record
contains 39 such import aliases plus the shared `__savegprlr` runtime helper.

The same provenance record pins the bundled INI parser to the author's declared
inih r62, upstream commit `26254ee9de7681f8825433415443e7116ff24b98`.
`BootXell-thirdparty-symbols.json` recovers `ini_parse_string` from its call by
`LoadBootIni`, its section/name/value parsing flow, and its indirect callback
dispatch. This separates the upstream parser from Peer's `ParseBootIniOption`
callback without attributing diagnostic-free runtime code by address alone.
