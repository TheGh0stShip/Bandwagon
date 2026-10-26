# Bandwagon

Bandwagon is a clean-room, reproducible reverse-engineering project for the
Xbox 360 Peer Pressure softmod. Its goal is to document the implementation and
replace closed components with reviewable source code.

The repository tracks analysis, reconstruction source, and deterministic
tooling. Original Peer Pressure and Microsoft binaries are local research
inputs and are intentionally excluded from version control. See
[`ANALYSIS_SAFETY.md`](ANALYSIS_SAFETY.md) for the artifact-handling boundary
and [`analysis/README.md`](analysis/README.md) for current findings and status.

The public BadUpdate source baseline is pinned as the
`third_party/Xbox360BadUpdate` submodule. Initialize it with
`git submodule update --init --recursive`.

## Current state

- The XBPP manifest and update archive wrappers have lossless decoders.
- SMC and kernel patch streams have reproducible JSON decodes.
- XEX base images have been extracted locally with recorded hashes.
- Peer Stage 4 has a byte-identical PowerPC assembly reconstruction.
- Peer Stage 3 and Stage 4 have complete, byte-identical PowerPC assembly
  reconstructions.
- Updater's complete 88,640-byte PowerPC `.text` section has a labeled,
  byte-identical assembly reconstruction.
- Updater's initialized `.rdata`, `.pdata`, and `.data` sections have
  byte-identical, buildable assembly reconstructions.
- Updater has reproducible function, direct-call, string-reference, and initial
  semantic-symbol inventories; its Stage2 HDD record algorithm is documented.
- BootXell's initialized code and data sections have complete byte-identical
  assembly reconstructions, with its INI option and launch flow documented.
- AuroraFixer has a complete byte-identical initialized-image reconstruction;
  its `DbgBreakPoint` import redirection and branch encoder are open and tested.
- UpdateCheck has a complete byte-identical initialized-image reconstruction;
  its HTTPS latest-release lookup and semantic-version parser are documented.
- BootEdit has a complete byte-identical initialized-image reconstruction and
  an initial semantic map of its `.xbat` theme editing workflow.
- The bundled dashboard-9199 boot animation is provenance-recorded as Microsoft
  code and intentionally excluded from Peer-authored source publication.

Semantic recovery of the remaining executable functions, types, globals,
indirect control flow, raw stage payloads, and SMC firmware is still in progress.
