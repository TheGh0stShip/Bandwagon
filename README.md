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

| Layer | Status |
|---|---|
| Artifact inventory | 33 / 33 installed artifacts classified |
| Binary-exact evidence | Complete for raw Peer payloads; initialized code/data preserved for XEX applications |
| Maintainable-source targets | 7 / 14 complete; 7 partial |
| Source-built replacement distribution | Not complete |

This project is **not near 100% source recovery**. The unfinished targets are
the largest ones, so 7 / 14 completed targets must not be read as 50% of the
remaining effort. The current backlog includes:

- 50 known KernelExpansion product functions / 18,828 instruction bytes that
  still lack readable source models, plus 72 functions still needing ownership
  classification;
- 3,432 candidate product bytes in the Stage2 worker, plus its modified outer
  ROP/data;
- ownership and product-source recovery across five XEX applications containing
  4,194 discovered function entries before public libraries and platform code
  are excluded;
- a final target-build and integration-validation phase.

The honest remaining scale is **dozens of material commits, not a few more
commits**. A defensible calendar ETA requires the five XEX ownership partitions
first. See [`STATUS.md`](STATUS.md) for the component-by-component scoreboard,
remaining queue, and exact definition of 100% completion.

## Source-scope rule

Bandwagon does not reimplement code merely because it was linked into a closed
Peer Pressure binary. Once a byte range is attributed to an available upstream
project, compiler runtime, or platform library, the project pins that source
and records any binary differences. Only the unmatched Peer-specific delta and
its modifications, adapters, policy, and orchestration remain source-recovery
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
readable source models and provenance verifiers. KernelExpansion's test now
also enforces its exhaustive 372-entry ownership partition; Stage2 and Stage4
have separate ownership/source-composition verifiers.
