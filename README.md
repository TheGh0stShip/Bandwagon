# Bandwagon

Bandwagon is a clean-room, reproducible reverse-engineering project for the
Xbox 360 Peer Pressure softmod. Its goal is to document the implementation and
replace closed components with reviewable source code.

The repository tracks analysis, reconstruction source, and deterministic
tooling. Original Peer Pressure and Microsoft binaries are local research
inputs and are intentionally excluded from version control. See
[`ANALYSIS_SAFETY.md`](ANALYSIS_SAFETY.md) for the artifact-handling boundary
and [`analysis/README.md`](analysis/README.md) for current findings. See
[`SOURCE_RECOVERY.md`](SOURCE_RECOVERY.md) for the distinction between
binary-exact assembly evidence and maintainable source-level recovery; assembly
equivalence alone is not treated as completion of the open-source goal.

The public BadUpdate source baseline is pinned as the
`third_party/Xbox360BadUpdate` submodule. Initialize it with
`git submodule update --init --recursive`.

## Current state

- The XBPP manifest and update archive wrappers have lossless decoders.
- SMC and kernel patch streams have reproducible JSON decodes.
- All six motherboard SMC patch streams have exact assembly evidence and
  complete, tested C++ coverage of every executable replacement record.
- The retail kernel patch stream has a typed 23-record effect table whose C++
  serializer reproduces all 372 bytes exactly.
- The 24-byte HDD security trigger has a typed six-field representation and
  exact C++ serializer; unresolved kernel pointers remain conservatively named.
- The sparse 16 KiB Flash DMA ROP/data image has a complete typed C++ builder
  and semantic operation model that reproduce every byte; two private retail
  kernel globals remain conservatively unnamed.
- XEX base images have been extracted locally with recorded hashes.
- Peer Stage 4 has a byte-identical PowerPC assembly reconstruction.
- Peer Stage 3 and Stage 4 have complete, byte-identical PowerPC assembly
  reconstructions.
- Stage 3 also has complete source-level coverage: its Peer-only entry and
  resident probe are reconstructed C++, and every inherited exploit routine is
  mapped to the pinned public BadUpdate source.
- Stage 4 has complete source-level coverage of every Peer-authored code and
  data region; its final 64 KiB is independently verified as third-party clean
  hypervisor data rather than counted as Peer source.
- Updater's complete 88,640-byte PowerPC `.text` section has a labeled,
  byte-identical assembly reconstruction.
- Updater's initialized `.rdata`, `.pdata`, and `.data` sections have
  byte-identical, buildable assembly reconstructions.
- Updater has reproducible function, direct-call, string-reference, and initial
  semantic-symbol inventories; its Stage2 HDD record algorithm is documented.
- BootXell's initialized code and data sections have complete byte-identical
  assembly reconstructions; seven identified Peer routines now have tested,
  exact-byte-bound C++ lifts, including its Xell sanitizer and launch flow.
- AuroraFixer has a complete byte-identical initialized-image reconstruction
  and complete source-level recovery of all five Peer-authored routines; its
  live `DbgBreakPoint` import redirection and cache maintenance are tested.
- UpdateCheck has a complete byte-identical initialized-image reconstruction;
  both identified Peer routines (the HTTPS latest-release workflow and release
  parser) have exact-byte-bound, tested C++ lifts.
- BootEdit has a complete byte-identical initialized-image reconstruction and
  an initial semantic map of its `.xbat` theme editing workflow.
- SettingsApp has a complete byte-identical initialized-image reconstruction
  and a semantic map of update, boot, dashboard, OtherOS, and version settings.
- The bundled dashboard-9199 boot animation is provenance-recorded as Microsoft
  code and intentionally excluded from Peer-authored source publication.
- The persistent HDD Stage2 ROP/worker image has an exact whole-file assembly
  reconstruction and an initial semantic map of its partition/update loader.

`analysis/component-inventory.json` is the generated manifest-wide completion
matrix. Of 33 installed components, no Peer raw payload remains without
binary-exact source-form reconstruction. `FlashDmaPayload.bin` additionally
has complete source-level C++ recovery. `KernelExpansion.bin` now has its first
three exact-byte-bound C++ lifts, while the remainder and Stage2 still require
maintainable-source recovery beyond their byte-identical assembly evidence.

Semantic recovery of remaining functions, types, globals, indirect control
flow, raw stage payloads, and SMC firmware is still in progress.
