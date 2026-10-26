# Kernel expansion reconstruction

`PeerPressure/System/KernelExpansion.bin` is a raw, big-endian Xbox 360 PPC
image mapped at `0x801f0000`. Its 83,792 bytes rebuild exactly from
`analysis/reconstruction/KernelExpansion.reconstructed.S` with SHA-256
`e0bea139ce9f48d0f0bd94f65fae0fd26316985f0a1b6625809893f29f028ad8`.

The base is proved by both halves of the image: signed `lis`/`addi` references
resolve strings beginning at runtime `0x801ff4b0`, while positive low-half
references resolve globals above `0x80200000`. The code and kernel-thunk region
is `0x00000-0x0f4af`; embedded strings, tables, mutable globals, and other data
occupy `0x0f4b0-0x1474f`.

## Recovered static structure

- 354 entry points from standard PPC prologues, direct `bl` targets, and three
  independently bounded indirect callbacks.
- 1,085 direct call edges.
- 133 four-instruction absolute retail-kernel thunks; 91 names are corroborated
  against the pinned public BadUpdate 17559 symbol maps.
- 46 direct references from 24 functions to embedded ASCII strings.
- 31 high-confidence behavioral labels are emitted directly into the exact
  assembly, including the config parser, file loader, XEX-section lookup,
  side-loading, boot, update, and storage-recognition paths.
- Full initialized code and data are preserved without embedding or executing
  the original binary.

The machine-readable evidence is in `KernelExpansion-functions.json`,
`KernelExpansion-callgraph.json`, and `KernelExpansion-string-xrefs.json`.
`KernelExpansion-strings.txt` is the deterministic address inventory for every
four-byte-or-longer printable run in the data region; it is intentionally kept
separate from confirmed xrefs so random printable table bytes are not promoted
to strings without evidence.
Regenerate the source and function inventory with
`python tools/reconstruct_kernel_expansion.py`; the common reconstruction
verifier proves whole-file identity.

## Initial semantic recovery

High-confidence behaviors visible from path and format strings include loading
`Config.ini`, applying `kernel_patches.bin`, selecting BootXell and custom boot
animation paths, Aurora side-loading, parsing `DllConfig` records, locating
`UpdateData.zip`, launching `SettingsApp.xex`, recognizing FAT32/GPT media, and
starting the phase-two `UpdateCheck.xex` path. These labels are behavioral
descriptions, not claims that original private symbol names were recovered.

The reconstruction closes the byte/source gap for the final manifest-listed
raw Peer payload. Higher-level naming and type recovery remains an ongoing
reverse-engineering layer and is tracked separately from byte completeness.

## Maintainable source recovery

`reconstruction/kernel_expansion/runtime.{hpp,cpp}` begins the source-level
lift. The first exact-byte-bound routines are:

- `LaunchBootXell` (`0x801f09d0`, 128 bytes), represented as a typed service
  workflow that loads `PPROOT:\OtherOS\BootXell.xex`, resolves ordinal one,
  invokes it on success, and unloads every non-null returned module handle.
- `DeleteObjectManagerSymbolicLink` and
  `CreateObjectManagerSymbolicLink` (`0x801f0480` and `0x801f05f0`, 144 bytes
  together), represented as typed wrappers around the resolved kernel object
  manager calls. Tests bind the `PAYLOAD:` link and target ordering and both
  zero/nonzero status conversions.
- `GetPeerPressureConfig` and `InitializeDefaultConfig` (104 bytes together),
  represented with an explicit 0x170-byte config type. Only the recovered
  byte-0x129 default flag is named; unclassified fields remain indexed storage.
  Tests preserve the initializer's unusual always-false return after it zeros
  the singleton, enables that flag, and enters the locked config path.
- `ReadFileToAllocatedBuffer` (`0x801f14d0`, 472 bytes), represented as a typed
  file-service workflow. It preserves pending-I/O waiting, allocation of the
  file size plus one, NUL placement, failure cleanup order, and the rule that
  caller outputs change only after a successful read.
- `LoadStructuredConfigFile` (`0x801f1700`, 144 bytes), represented as the
  generic schema-parser wrapper layered on that loader. It always releases a
  successfully loaded buffer after the parser returns.
- `InstallKernelXexDataHandlers` (`0x801f1790`, 352 bytes), represented as a
  bounds-checked mapped-PE walker. It finds `.xedata`, installs both relocated
  handler words, updates the table count, and performs the recovered CPU and
  hypervisor cache-maintenance sequence. The earlier `FindXexSection` label
  described only the first part of the routine and has been corrected.
- `RecognizeGptDisk` (`0x801fa540`, 256 bytes), represented as a bounds-safe
  GPT-header recognizer and the exact recovered reflected CRC-32 loop. The
  binary includes the stored checksum field in its checksum input; the lift
  preserves and tests that unusual observed behavior rather than silently
  replacing it with the conventional GPT algorithm.
- `IsAsciiAlphaNumeric` (`0x801fa640`, 72 bytes), a direct typed predicate for
  the binary's three accepted ASCII ranges.
- `ClassifyFatBootSector` (`0x801fa320`, 440 bytes), represented as a typed
  five-result classifier. Tests cover its exact exFAT prefix, FAT32 label,
  structural FAT fallback, MBR signature, unknown-sector, and read-error paths.
- `LaunchUpdateCheck` (`0x801fa798`, 320 bytes), represented as a synchronized
  state transition around `UpdateCheck.xex`: it detaches stale NIC state,
  dereferences the prior object under the spin lock, invokes ordinal one,
  records only a newer returned version, prepares the module, and unloads it.
- Three leaf callbacks immediately following `LaunchSettingsApp` are now
  separated from that function and represented in
  `package_callbacks.{hpp,cpp}`: `InitializeCallbackOutputs` (`0x801f8210`, 40
  bytes), `MapCallbackValue` (`0x801f8238`, 56 bytes), and
  `ClassifyStfsPackageMagic` (`0x801f8270`, 104 bytes). The classifier preserves
  the exact `CON `, `LIVE`, and `PIRS` mappings, its one-byte output, the
  unchanged-output failure path, and status `0x80070570`.
- `ParseDriveNumberPrefix` (`0x801f6af0`, 120 bytes), represented as a bounded
  UTF-16 cursor parser for the recovered one-decimal-digit drive syntax. Tests
  cover valid zero and nine prefixes, absent prefixes, malformed prefixes,
  control-character termination, cursor preservation, and null strings.
- `CopyPathAndParentDirectory` (`0x801faac0`, 144 bytes), represented with the
  recovered object field offsets and 256-byte bound. It preserves the exact
  final-backslash split, leading-backslash behavior, low-byte flag comparison,
  and the contiguous terminator write at the full-capacity boundary.
- `CanOpenExistingPath` (`0x801faa08`, 184 bytes), represented as the exact
  `NtCreateFile` option set behind the path validator. Its typed service model
  proves that only a successful open is closed and accepted.

Their source coverage is bound to exact function hashes by
`KernelExpansion-peer-source-coverage.json`. Run the verifier and host tests
with `tools/test_kernel_expansion_reconstruction.sh`. The current layer covers
18 independently bounded routines totaling 3,080 original instruction bytes.

This source-level layer remains partial. The exact whole-file assembly is the
evidence oracle, not a claim that all 354 discovered entries are maintainable
source. The routine formerly labeled `RecognizeFat32Volume` also identifies
exFAT and returns multiple classification values; its corrected
`ClassifyFatBootSector` lift now makes those outcomes explicit. The former
`ParsePeerPressureConfig` label was also corrected to
`InitializeDefaultConfig`: the routine is reached on a config-read failure and
establishes the default singleton before entering the locked config path; that
bounded behavior now has a typed lift while the deeper common config path is
still being classified.
