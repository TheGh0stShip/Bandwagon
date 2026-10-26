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
- The complete Peer Stage 3 entry point, Peer-specific probe, and their support
  stubs have a partial, byte-identical PowerPC assembly reconstruction.

Exact reconstruction of the remaining executable functions, imports, types,
globals, control flow, raw stage payloads, and SMC firmware is still in
progress.
