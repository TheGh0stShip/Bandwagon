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

The public BadUpdate source baseline and the exact declared third-party
versions are pinned under `third_party/`. Initialize them with
`git submodule update --init --recursive`; see
[`third_party/README.md`](third_party/README.md) for the revision ledger.

## Status at a glance

Status date: 2026-10-26

| Completion gate | Measured state | Result |
|---|---:|---|
| Installed-artifact inventory | 33 / 33 | Complete |
| Raw-payload evidence | Every current raw Peer payload | Binary-exact; this is an evidence milestone, not source completion |
| XEX function ownership | 4,194 / 4,194 entries and 1,387,504 / 1,387,504 code bytes | Exhaustively partitioned; 136 entries remain conservatively unknown |
| Maintainable-source targets | 8 / 14 | Six large targets remain partial |
| Source-built replacement distribution | 0 / 1 | Not yet integrated |

The XEX ownership pass is now complete. Of their 1,387,504 code bytes,
1,257,448 (90.6%) are pinned upstream or proven platform/toolchain code and are
not a Peer source-recovery task. The auditable remainder is 318 product/mixed
candidates / 92,776 bytes plus 136 unknown entries / 37,280 bytes. Only 11 of
those product entries / 3,664 bytes currently have hash-bound readable-source
coverage; existing unbound behavior models are not counted as completed
functions.

This project is therefore **not near 100% source recovery**. The unfinished
targets are the largest ones, so 8 / 14 completed targets must not be read as
57% of the remaining effort. The current backlog is:

- 50 known KernelExpansion product functions / 18,828 instruction bytes that
  still lack readable source models, plus 72 functions still needing ownership
  classification;
- 3,432 candidate product bytes in the Stage2 worker, plus its modified outer
  ROP/data;
- readable source for 307 currently uncovered XEX product/mixed entries /
  89,112 bytes, led by SettingsApp, BootEdit, and Updater, while refining the
  136 unknown XEX entries before treating any of them as product code;
- a final target-build and integration-validation phase.

The current engineering forecast is **32-65 substantive commits and roughly
3-8 weeks of sustained work to a source-built integration candidate**, not a
few more commits. Authorized target/emulator validation can extend that range.
The estimate will narrow as the 136-entry unknown bucket is resolved. See
[`STATUS.md`](STATUS.md) for the component-by-component scoreboard, remaining
queue, and exact definition of 100% completion.

## Source-scope rule

Bandwagon does not reimplement code merely because it was linked into a closed
Peer Pressure binary. A redistributable upstream range is bound to a pinned
source revision. A proprietary compiler/platform range instead requires
positive toolchain or ABI evidence and a documented replacement boundary; no
Microsoft source is claimed or pinned. Unproven platform-facing code stays
product/mixed or unknown. Only the unmatched Peer-specific delta and its
modifications, adapters, policy, and orchestration remain source-recovery
targets. "Peer-specific" describes binary provenance, not personal authorship.

## Repository guide

- [`STATUS.md`](STATUS.md): authoritative completion scoreboard, remaining
  queue, measured boundaries, and definition of 100%.
- [`SOURCE_RECOVERY.md`](SOURCE_RECOVERY.md): source-complete versus
  binary-exact status for each target.
- [`analysis/component-inventory.json`](analysis/component-inventory.json): all
  33 installed artifacts and their evidence status.
- [`analysis/README.md`](analysis/README.md): recovered formats, hashes, and
  component findings.
- [`analysis/reconstruction/`](analysis/reconstruction/): binary-exact evidence
  oracles; these are not counted as final source implementations.
- [`reconstruction/`](reconstruction/): readable source models and tests.

## Verification

The common offline rebuild verifies all raw reconstructed payloads without
executing them:

```sh
python3 -B tools/rebuild_peer_reconstructions.py
```

Component test scripts under `tools/test_*_reconstruction.sh` exercise the
readable source models and provenance verifiers. Run
`python3 tools/report_recovery_dashboard.py` for the current hash-bound
function/byte denominator. All five XEX ownership-pass targets and
KernelExpansion now have exhaustive partitions; Stage2 and Stage4 have separate
ownership/source-composition verifiers.
