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

## Project state

**Bandwagon is not yet a complete replacement for Peer Pressure.** The binary
evidence inventory is complete, but readable source recovery and final
integration are still in progress. Assembly is retained as evidence; it does
not count as open-source completion.

Status date: 2026-10-26

| Gate | State | What that means |
|---|---:|---|
| Preserve and map the inputs | Complete | 33 / 33 installed artifacts recorded; all 4,194 XEX code entries are accounted for |
| Recover maintainable Peer-specific source | In progress | 10 of 14 targets complete; in the measured five-XEX queue, 92 / 335 product candidates and 36,984 / 96,136 bytes have verified readable source |
| Build and validate a replacement | Not complete | No single source-built replacement distribution exists yet |

There is no honest single percentage for the whole project: the 14 targets
differ greatly in size, and KernelExpansion, HDD Stage2, and final integration
use different units. The best comparable meter is the five-XEX product queue:
**23.1% of functions and 35.6% of bytes currently have verified readable
source**. It excludes code positively identified as public upstream code or
Xbox/compiler infrastructure.

### Working now

**UpdateCheck is now source-complete:** all 19 product functions / 6,768 bytes
have verified readable source, and all five former unknown wrapper containers /
608 bytes have positive Microsoft-platform provenance. The next focused queue
is BootEdit's provenance backlog: all 41 known product functions / 17,432 bytes
now have verified readable source, while its 61 unknown entries must be
fingerprinted before any further lifting decision. The first 11 unknown entries /
5,128 bytes (the mixed Xbox 360/DX9 backend) now have a complete hash-bound,
tested readable-source model, while their original-source provenance remains
deliberately unresolved. The four-entry XPR2 loader family is also now
source-covered in both BootEdit (712 bytes) and SettingsApp (688 bytes).

### Already source-complete

Readable product-specific source is complete for Stage3, Stage4, the kernel
patch stream, HDD security trigger, flash DMA payload, motherboard SMC patches,
AuroraFixer, BootXell, Updater, and UpdateCheck. Their binary/assembly
reconstructions remain as test oracles, not as substitutes for readable source.

### What remains after UpdateCheck

| Workstream | Known source work still missing | Unresolved provenance |
|---|---:|---:|
| KernelExpansion | 50 functions / 18,828 bytes | 72 functions / 16,024 bytes |
| HDD Stage2 | 3,432 candidate worker bytes, plus modified ROP/data | Included in its remaining analysis |
| SettingsApp | 243 functions / 59,152 bytes | 13 functions / 5,336 bytes |
| BootEdit | None among currently classified product code | 61 functions / 15,824 bytes |
| Final integration | Source-built components, reproducible distribution, and host plus authorized target/emulator validation | - |

### Forecast

The current estimate is **24-50 substantive commits, or about 3-7 weeks of
sustained engineering work**, to reach a source-built integration candidate.
That is active engineering time, not calendar time between work sessions.
Hardware/emulator validation can extend it.

The exact per-component evidence and definition of 100% are in
[`STATUS.md`](STATUS.md). [`SOURCE_RECOVERY.md`](SOURCE_RECOVERY.md) explains
what qualifies as source-complete.

## Detailed status ledger

Status date: 2026-10-26

| Completion gate | Measured state | Result |
|---|---:|---|
| Installed-artifact inventory | 33 / 33 | Complete |
| Raw-payload evidence | Every current raw Peer payload | Binary-exact; this is an evidence milestone, not source completion |
| XEX function ownership | 4,194 / 4,194 entries and 1,387,504 / 1,387,504 code bytes | Exhaustively partitioned; 89 entries remain conservatively unknown |
| Maintainable-source targets | 10 / 14 | Four targets remain partial |
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
| UpdateCheck | Complete | 19 / 19 product entries, 6,768 / 6,768 B | None in the current source-composition boundary |
| SettingsApp | Partial | 3 / 246 product/mixed entries, 3,112 / 62,264 B | 243 entries / 59,152 B, plus 13 unknown entries / 5,336 B |
| BootEdit | Partial | 41 / 41 known product entries, 17,432 / 17,432 B | 61 unknown entries / 15,824 B require provenance resolution |

The five-XEX readable-source counter is therefore **92 / 335 product entries
(27.5%) and 36,984 / 96,136 product bytes (38.5%)**. These percentages describe
that measured XEX queue only; they deliberately exclude pinned public source,
proven platform/toolchain code, KernelExpansion, Stage2, and final integration.

The XEX ownership pass is now complete. Of their 1,387,504 code bytes,
1,270,208 (91.5%) are pinned upstream or proven platform/toolchain code and are
not a Peer source-recovery task. The current denominator is 335 product/mixed
candidates / 96,136 bytes plus 74 unknown entries / 21,160 bytes. Of the
product bucket, 92 entries / 36,984 bytes now have hash-bound readable-source
coverage; 243 / 59,152 bytes remain. Existing unbound behavior models are not
counted as completed functions.

Within that XEX denominator, BootXell is 9/9 and Updater is 20/20 source
covered. UpdateCheck is 19/19, SettingsApp is 3/246, and BootEdit is 41/41
among currently classified product entries.
That is the shortest current answer to "what is done versus left."

This project is therefore **not near 100% source recovery**. The unfinished
targets are the largest ones, so 10 / 14 completed targets must not be read as
71% of the remaining effort. The current backlog is:

- 50 known KernelExpansion product functions / 18,828 instruction bytes that
  still lack readable source models, plus 72 functions still needing ownership
  classification;
- 3,432 candidate product bytes in the Stage2 worker, plus its modified outer
  ROP/data;
- readable source for 243 currently uncovered XEX product/mixed entries /
  59,152 bytes, all in SettingsApp;
- provenance work on 74 unknown XEX entries / 21,160 bytes before treating
  any of them as product code;
- a final target-build and integration-validation phase.

The current engineering forecast is **24-50 substantive commits and roughly
3-7 weeks of sustained, active engineering work to a source-built integration
candidate**, not a few more commits. The estimate is approximately 1-4 commits
for provenance refinement, 18-36 for the four partial source targets, and 5-10
for build/integration closure. It is an active-work estimate, not elapsed time
while no analysis session is running. Authorized target/emulator validation
can extend that range. The estimate will narrow as the 126-entry unknown bucket
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

The asset/container gate is exercised by `python3 tools/test_asset_unpackers.py`.
It safely extracts both wrapped update ZIPs, byte-exactly unpacks and rebuilds
the XUIZ/XZP archive, and byte-exactly decodes/rebuilds its four XUIB/XUR
members. Full XUR `DATA`-stream translation back to editable XUI XML remains
explicitly unfinished.
