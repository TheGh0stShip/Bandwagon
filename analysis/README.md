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

`assets/` contains a lossless unpacking of `HudResources.xzp`. The native
`XUIZ` unpacker/repacker and section-aware `XUIB` decoder/rebuilder reproduce
the archive and all four compiled XUR members byte-for-byte. The XUR JSON
exposes its string and floating-point pools while retaining the proprietary
`DATA` stream losslessly; translation of that stream into original XUI XML is
still a semantic-decompilation task.

Two original XEX files, `BadUpdatePayload/bootanim.xex` and
`Flash/bootanim.xex`, are byte-identical and therefore produce identical base
files. They are retained under both names to preserve provenance.

`xex/BootAnim-9199-provenance.md` classifies the bundled themed boot animation
as a 2010 Xbox dashboard 9199 component rather than Peer-specific code. Its hash
and internal version evidence are retained, but proprietary Microsoft code is
not republished as a source-form disassembly.

`xex/Updater.md` records the first full XEX function-boundary pass. The pinned
decompilation pipeline recovers 376 Updater code symbols, including 48 named
kernel import thunks, and emits a machine-readable address inventory. The
entire 88,640-byte Updater `.text` section now has a buildable, byte-identical
assembly reconstruction. Its initialized `.rdata`, `.pdata`, and `.data`
sections are also exactly reconstructable. Reproducible call/string graphs,
semantic symbols, and a portable tested reconstruction of the Stage2
bootstrap-record builder are tracked alongside it. Exhaustive ownership
reduces the readable-source target to 20 product entries / 6,728 bytes; 356
miniz/platform/toolchain entries are excluded. All 20 product entries now have
hash-bound readable models, so Updater is source-complete at this boundary.

`xex/BootXell.md` documents the OtherOS launcher. Its complete initialized
`.text`, `.rdata`, `.pdata`, and `.data` ranges now have byte-identical assembly
source, plus reproducible function, call, string-reference, and semantic-symbol
inventories. Every one of its 256 code entries is classified and all 9 product
routines / 2,944 bytes now have tested readable source.

`xex/AuroraFixer.md` reconstructs the SideLoading compatibility DLL. Its full
initialized code/data image is byte-identical assembly, and its process-attach
hook, import lookup, and absolute-branch patch encoder are semantically named.
The pure branch encoder also has a portable C++ reconstruction and host test.

`xex/UpdateCheck.md` reconstructs the network update checker, including the
trusted-root path, GitHub latest-release endpoint, HTTP status gate, JSON
`tag_name` lookup, and three-component release version parser. All initialized
sections have exact assembly source and reproducible static-analysis maps. Its
804-entry ownership partition identifies 19 product/mixed integration entries
/ 6,768 bytes and excludes 785 upstream/platform entries. All 19 product
routines / 6,768 bytes are source-covered. The five former unknown networking
wrapper containers / 608 bytes are now positively bound to clean Microsoft
2.0.17559 bootanim code, leaving no UpdateCheck ownership unknowns.

`xex/BootEdit.md` covers the ImGui boot-theme editor: exact initialized
sections, function/call/string inventories, and semantic names for theme
enumeration, editing, save/load feedback, settings application, and selection.
Its exhaustive partition identifies 23 product entries / 14,680 bytes, excludes
799 pinned-ImGui or proven-platform entries, and conservatively retains 112
entries / 28,600 bytes as unknown. All 23 currently classified product entries /
14,680 bytes have hash-bound readable models. The recovered set includes the
exact 632-byte theme payload layout and all 39 default color vectors rather
than an opaque settings tail.

`xex/SettingsApp.md` covers the main settings UI. Its entire initialized image
is exact assembly, with reproducible maps and initial semantic names spanning
online/offline updates, boot themes, dashboard and OtherOS configuration,
settings persistence, and displayed Peer/kernel/SMC version information.
All 1,824 code entries are now partitioned: 246 product/mixed candidates /
62,264 bytes remain the conservative source-recovery upper bound, 1,565
upstream/platform entries are excluded, and 13 XPR2/graphics-backend entries
remain provenance-unknown. Four of those entries / 688 bytes now share a
readable XPR2 reconstruction. Three product/mixed entries / 3,112 bytes have
exact-entry readable models.

`stage2/PeerPressureExploit-Stage2.md` documents the persistent HDD Stage2 ROP
image and its relocated PowerPC worker. The complete 217,796-byte file now has
an exact assembly reconstruction, 87 worker labels, a fully resolved 212-edge
direct-call graph, and initial names for its partition and payload-loading path.

`component-inventory.json` is regenerated from the signed manifest by
`tools/generate_component_inventory.py`. It verifies every file hash and keeps
source status, provenance boundary, evidence, and the unresolved Peer payload
list explicit rather than inferring completion from file presence.

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

`peer-stage3-probe.md` proves the behavior of
`ProbeResidentPeerDispatcher` at file `0x0f8-0x1e4` and its `0x38`-byte probe
at `0x15a0`. Both invocation paths are resolved: the Stage4-installed
`HvxPostOutput` trampoline protocol and syscall-zero `rbtr` operation 4.

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
The persistent HDD Stage2, sparse Flash DMA ROP/data image, and kernel expansion
are now exactly reconstructed, leaving no manifest-listed Peer raw payload
without lossless source form. `HddSecuritySectorPayload.bin` is now a lossless
six-word reconstruction with its retail-17559 stack-pivot gadget identified.
The six
platform SMC firmware images remain reference binaries requiring
architecture-specific provenance/annotation rather than being treated as
Peer-specific source.
