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

Semantic recovery of the remaining executable functions, types, globals,
indirect control flow, raw stage payloads, and SMC firmware is still in progress.
