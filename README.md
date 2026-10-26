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
| XEX function ownership | 4,194 / 4,194 entries and 1,387,504 / 1,387,504 code bytes | Exhaustively partitioned; 137 entries remain conservatively unknown |
| Maintainable-source targets | 9 / 14 | Five targets remain partial |
| Source-built replacement distribution | 0 / 1 | Not yet integrated |

### Source-target ledger

The 14 targets are not equal-sized. “Complete” means the Peer-specific behavior
inside that target has readable source and the non-Peer code has a documented
source or ABI boundary; it does not mean the final replacement distribution has
already been built.

| Target | State | Verified source coverage | Concrete remainder |
|---|---|---:|---|
| Stage3 | Complete | Product entry/probe covered | None in the current boundary |
| Stage4 | Complete | 1,764 product bytes covered | None in the current boundary |
| Kernel patch stream | Complete | 23 / 23 typed records | None |
| HDD security trigger | Complete | Complete typed serializer | No algorithm missing |
| Flash DMA payload | Complete | Complete typed 16 KiB builder | No algorithm missing |
| Motherboard SMC patches | Complete | All executable records for six boards | None in the replacement records |
| AuroraFixer | Complete | 5 / 5 product routines | None |
| BootXell | Complete | 9 / 9 product entries, 2,944 / 2,944 B | None |
| Updater | Complete | 20 / 20 product entries, 6,728 / 6,728 B | None |
| KernelExpansion | Partial | 46 / 96 known product entries, 6,480 / 25,308 B | 50 entries / 18,828 B, plus 72 unknown entries / 16,024 B |
| HDD Stage2 | Partial | Ownership split and exact binary reconstruction | 3,432 candidate product worker bytes, modified ROP/data, and integration |
| UpdateCheck | Partial | 16 / 19 product entries, 4,400 / 6,768 B | 3 entries / 2,368 B, plus 5 unknown entries / 608 B |
| SettingsApp | Partial | 3 / 246 product/mixed entries, 3,112 / 62,264 B | 243 entries / 59,152 B, plus 13 unknown entries / 5,336 B |
| BootEdit | Partial | 8 / 23 product entries, 2,424 / 13,696 B | 15 entries / 11,272 B, plus 119 unknown entries / 31,712 B |

The five-XEX readable-source counter is therefore **56 / 317 product entries
(17.7%) and 19,608 / 92,400 product bytes (21.2%)**. These percentages describe
that measured XEX queue only; they deliberately exclude pinned public source,
proven platform/toolchain code, KernelExpansion, Stage2, and final integration.

The XEX ownership pass is now complete. Of their 1,387,504 code bytes,
1,257,448 (90.6%) are pinned upstream or proven platform/toolchain code and are
not a Peer source-recovery task. The current denominator is 317 product/mixed
candidates / 92,400 bytes plus 137 unknown entries / 37,656 bytes. Of the
product bucket, 56 entries / 19,608 bytes now have hash-bound readable-source
coverage; 261 / 72,792 bytes remain. Existing unbound behavior models are not
counted as completed functions.

Within that XEX denominator, BootXell is 9/9 and Updater is 20/20 source
covered. UpdateCheck is 16/19, SettingsApp is 3/246, and BootEdit is 8/23.
That is the shortest current answer to "what is done versus left."

This project is therefore **not near 100% source recovery**. The unfinished
targets are the largest ones, so 9 / 14 completed targets must not be read as
64% of the remaining effort. The current backlog is:

- 50 known KernelExpansion product functions / 18,828 instruction bytes that
  still lack readable source models, plus 72 functions still needing ownership
  classification;
- 3,432 candidate product bytes in the Stage2 worker, plus its modified outer
  ROP/data;
- readable source for 261 currently uncovered XEX product/mixed entries /
  72,792 bytes: UpdateCheck 3 / 2,368, SettingsApp 243 / 59,152, and BootEdit
  15 / 11,272;
- provenance work on 137 unknown XEX entries / 37,656 bytes before treating
  any of them as product code;
- a final target-build and integration-validation phase.

The current engineering forecast is **27-55 substantive commits and roughly
3-7 weeks of sustained, active engineering work to a source-built integration
candidate**, not a few more commits. The estimate is approximately 2-5 commits
for provenance refinement, 20-40 for the five partial source targets, and 5-10
for build/integration closure. It is an active-work estimate, not elapsed time
while no analysis session is running. Authorized target/emulator validation
can extend that range. The estimate will narrow as the 137-entry unknown bucket
is resolved. See [`STATUS.md`](STATUS.md) for the component-by-component
scoreboard, remaining queue, and exact definition of 100% completion.

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
