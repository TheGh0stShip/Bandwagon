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

- 351 entry points from standard PPC prologues and direct `bl` targets.
- 1,085 direct call edges.
- 133 four-instruction absolute retail-kernel thunks; 91 names are corroborated
  against the pinned public BadUpdate 17559 symbol maps.
- 46 direct references from 24 functions to embedded ASCII strings.
- 21 high-confidence behavioral labels are emitted directly into the exact
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
- `ReadFileToAllocatedBuffer` (`0x801f14d0`, 472 bytes), represented as a typed
  file-service workflow. It preserves pending-I/O waiting, allocation of the
  file size plus one, NUL placement, failure cleanup order, and the rule that
  caller outputs change only after a successful read.
- `RecognizeGptDisk` (`0x801fa540`, 256 bytes), represented as a bounds-safe
  GPT-header recognizer and the exact recovered reflected CRC-32 loop. The
  binary includes the stored checksum field in its checksum input; the lift
  preserves and tests that unusual observed behavior rather than silently
  replacing it with the conventional GPT algorithm.
- `IsAsciiAlphaNumeric` (`0x801fa640`, 72 bytes), a direct typed predicate for
  the binary's three accepted ASCII ranges.

Their source coverage is bound to exact function hashes by
`KernelExpansion-peer-source-coverage.json`. Run the verifier and host tests
with `tools/test_kernel_expansion_reconstruction.sh`.

This source-level layer remains partial. The exact whole-file assembly is the
evidence oracle, not a claim that all 351 discovered entries are maintainable
source. The routine formerly labeled `RecognizeFat32Volume` also identifies
exFAT and returns multiple classification values; it is now conservatively
named `ClassifyFatBootSector` and still needs a typed lift before it can be
counted as recovered source.
