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
memory. A final `xxxxxxxxxxxxxxxx` is a scan marker, not a mandatory safe
trailer: unmarked images proceed directly, while marked images invoke
`SanitizeXellImage` unless the exact bypass acknowledgement was configured.
It then constructs an `rbtr` launch request using either the configured base
address or the allocated physical address. All handles and allocations are
released on the observed exit paths.

`SanitizeDangerousXellNames` at `0x91a05190` finds `updxell.bin`,
`flashdmp.bin`, and `updflash.bin`, clears each match (including an immediately
preceding slash), and leaves sentinel byte 1. `SanitizeXellImage` at
`0x91a05410` first applies that pass directly. If no match is found, it
recognizes a gzip member at offset `0x4000`, excludes the final 16-byte marker,
decompresses up to 1 MiB, sanitizes the expanded data, and recompresses it into
the original capacity.

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

## Exhaustive source ownership

`BootXell-function-ownership.json` classifies every one of the 256 function
inventory entries and all 74,840 initialized instruction bytes:

| Ownership | Entries | Bytes | Recovery treatment |
| --- | ---: | ---: | --- |
| Peer Pressure product policy/adapters | 9 | 2,944 | Recovered as readable source |
| inih r62 | 2 | 808 | Use the pinned upstream source |
| miniz 3.1.1 | 18 | 18,776 | Use the pinned upstream source |
| MSVC/Xbox platform and toolchain | 227 | 52,312 | Reuse the target ABI/runtime; do not relift |

The miniz boundary is not based on adjacency alone. Its 5,288-byte `tinfl`
core is instruction-structure-identical after relocation normalization to the
independently attributed copies in Updater and SettingsApp. The inih line
parser is byte-identical in BootEdit and SettingsApp, and its string driver is
also structure-identical after relocation normalization.

All nine product entries now have readable source coverage. The final two were
app-local derivatives of miniz's stock compress/uncompress helpers: they select
raw DEFLATE with window bits `-15`, and the compressor fixes level 10, method
8, memory level 9, and default strategy. The adjacent seven file,
physical-memory, last-error, and handle routines are standard XDK wrappers and
are excluded as platform code. No inih, miniz engine, CRT, XDK wrapper, or
import-thunk body remains in the product queue. Run
`tools/verify_bootxell_function_ownership.py` to verify the partition and its
cross-XEX evidence.

`reconstruction/bootxell/boot_options.cpp` reconstructs the Peer INI callback,
quoted value trimming, hexadecimal 64-bit base addresses, exact bypass
acknowledgement, record finalization, marker gate, OtherOS path join, and
five-field `rbtr` request. `reconstruction/bootxell/xell_scan.cpp` implements
the direct and gzip-wrapped sanitizer. `reconstruction/bootxell/runtime.cpp`
lifts `ReadFileToBuffer`, `LoadBootIni`, `LaunchBootOption`, and `BootXellMain`
behind explicit file, memory, parser, compression, and launch interfaces.
`BootXell-peer-source-coverage.json` binds all nine product routines (2,944
bytes) to exact image hashes and source symbols. The option
lookup preserves `BootXellMain`'s first-match, 64-byte bounded comparison.
Run all host tests and the coverage check with
`tools/test_bootxell_reconstruction.sh`.
