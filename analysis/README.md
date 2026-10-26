# Peer Pressure reverse-engineering workspace

This directory contains reproducible analysis artifacts derived from the
original Xbox 360 files in the repository root. Original inputs are never
modified.

## Confirmed target

The extracted XEX base files identify as PE32 XBOX executables for 32-bit,
big-endian PowerPC. This is the Xbox 360 Xenon execution environment, not PS
Vita or AArch64. Image bases and entry points must be taken from each XEX
header; they are not uniform across the set.

## XEX extraction

`xex/basefiles/` contains decrypted and LZX-decompressed PE images. The output
is an in-memory PE image layout: tools which insist on an ordinary disk PE may
reject it, while `radare2`/`rabin2` load the virtual sections correctly.

`xex/idc/` contains import-thunk scripts generated from each XEX header.

`patch-records/` contains lossless JSON decodes of the six SMC exploit patch
streams and `kernel_patches.bin`. Each stream consists of big-endian target
address, big-endian byte count, and exact replacement bytes. The kernel stream
ends with the four-byte `0xffffffff` sentinel. Regenerate or validate these
files with `tools/decode_patch_records.py`.

`softmod-manifest.json` is the fully decoded `XBPP` manifest. Its parser in
`tools/decode_softmod_manifest.py` verifies the internal SHA-1, safely resolves
all paths, checks every listed file hash, and serializes back to a byte-for-byte
identical 1,980-byte file. The current manifest is version `1.0.477`, declares
SMC format/version `1.1`, and all 33 entries match their source files.

`UpdateData-wrapper.json` and `SoftmodData-wrapper.json` describe the custom
24-byte prefix on the update archives. It is a SHA-1 over the four-byte version
and ZIP data, followed by version `1.0.477`; the ordinary ZIP begins at offset
`0x18`. `tools/decode_update_archive.py` validates the wrapper, ZIP CRCs, and
member paths without extracting them.

Two original XEX files, `BadUpdatePayload/bootanim.xex` and
`Flash/bootanim.xex`, are byte-identical and therefore produce identical base
files. They are retained under both names to preserve provenance.

## BadUpdate baseline and modified stages

`badupdate-v1.3-baseline.md` records the pinned public-source/release
provenance and the exact component-identity boundary. Its machine-readable
comparison remains in `badupdate-v1.3-comparison.json`.

`peer-stage4-preamble.md` establishes that the separate Peer Stage4 file has a
Peer-specific `0x780`-byte preamble followed by the exact 65,536-byte public
clean-HV block. `peer-stage4-preamble.json` contains the hashes, lossless patch
record decode, effective 64-bit HV destinations, copied-code regions, and CFG
edges; `peer-stage4-preamble-cfg.dot` is the focused static graph. The complete
SMC helper at Peer offsets `0x294-0x2ff` is byte-identical to the rebuilt public
function. Other Peer-only functions and record payloads remain independently
reconstructed.

`peer-stage3-probe.md` proves the behavior of the new Stage3 function at file
`0x0f8-0x1e4` and its `0x38`-byte probe at `0x15a0`. The function is assigned
only a hypothesis-grade behavioral label pending resolution of both syscall
paths.

`peer-stage3-entry.md` reconstructs the complete entry point at file
`0x0000-0x00f7` and proves its boot decision: an available resident dispatcher
skips the inherited BadUpdate race, while probe failure runs the race and
Stage4 installation path before both branches launch the normal payload.

`reconstruction/` contains buildable big-endian PowerPC assembly rather than
invented C. The reconstructed Stage3 and Stage4 assembly files both rebuild
their complete inputs byte-for-byte. Stage3's entry point and new probe are
hand-labeled; deterministic generated assembly preserves the remaining code
while semantic function naming and higher-level recovery continue.

## Extraction verification

| Derived PE | Size | SHA-256 |
|---|---:|---|
| `Updater.pe` | 118784 | `2f8210528112a5cf5df514db7a3fa2c05322e845c6c2c747e8fc7c1485f4522c` |
| `BadUpdatePayload_default.pe` | 2293760 | `2444d5b04d1067c1923fdf2d1769636acabb66266bfd510fd7096e3f22ae1f4c` |
| `BadUpdatePayload_bootanim.pe` | 638976 | `b2bc842157ea7fbc6fde9adede7d55a5b6b9a4bdb88f73fbdb01f03371b1835b` |
| `Flash_bootanim.pe` | 638976 | `b2bc842157ea7fbc6fde9adede7d55a5b6b9a4bdb88f73fbdb01f03371b1835b` |
| `PeerPressure_System_SettingsApp.pe` | 2555904 | `a77a2a06d71c9a9512879122187fe5541270056f54657574d5a2fe9a109e035a` |
| `PeerPressure_System_BootEdit.pe` | 2228224 | `1396cf0f05e5cb07de4f2ca067975376b0b319dd52f3ca4182af8b00d1175df8` |
| `PeerPressure_System_UpdateCheck.pe` | 278528 | `ab6aaf5f2d3f880701269e5835cf9a16aa26c223fcf8ddccb31f8db8ffb72ece` |
| `PeerPressure_OtherOS_BootXell.pe` | 106496 | `4ef810a55390105a0756d2dc48502f8d9fad50edc08d8617fe146bc3ae496ebc` |
| `PeerPressure_SideLoading_AuroraFixer.pe` | 163840 | `eca239f0f17b4c8c4b9e1e24bb29fad324ed6b698081bf8e2fa379707b2de168` |
| `PeerPressure_Themes_BootAnim_9199_bootanim.pe` | 585728 | `d978e0fa36eb14b08bbd163338a009906435f9f4e92db11654897737d4545fed` |

## Current analysis boundary

The XEX container layer, patch-list containers, manifest, update wrapper, and
Peer Stage4 preamble patch stream are decoded exactly. Stage3 and Stage4 have
complete byte-identical assembly reconstructions. Higher-level recovery
remains in progress: executable functions, imports, types, globals, and control
flow have not yet been completely named across all components. The focused
Stage4 preamble CFG does not imply that the clean-HV data tail is executable.
Other raw stage payloads and the 8051-family SMC firmware still require
architecture-specific loaders and annotation.
