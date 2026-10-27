# Source-level recovery status

Bandwagon tracks two different kinds of completion. They must not be conflated:

1. **Binary-exact evidence**: assembly or data declarations rebuild the original
   bytes. This fixes instruction boundaries and preserves evidence, but does not
   by itself constitute a maintainable open-source implementation.
2. **Source-level recovery**: readable C/C++ (and narrowly scoped target assembly
   where the ISA requires it) expresses the recovered algorithms, types, state,
   and control flow, with tests tied back to the binary evidence.

A component is not considered fully open-source reconstructed until its
Peer-specific closed-source behavior has source-level coverage, redistributable
upstream ranges are bound to pinned source, and proprietary platform/toolchain
ranges have positive provenance plus an explicit ABI replacement boundary.
Generated `.S` files under `analysis/reconstruction/` are the reference oracle
used to validate that work, not the final implementation.

## Current source-level coverage

| Component | Binary-exact evidence | Source-level recovery |
|---|---|---|
| Stage4 | Complete whole-file assembly | Complete source composition: 1,764 Peer-specific bytes have C++/typed-source coverage, the 108-byte SMC helper maps to public BadUpdate source, and the verified 64 KiB tail is third-party hypervisor data |
| Stage3 | Complete whole-file assembly | Complete: Peer entry/probe source plus function-by-function verified composition with pinned public BadUpdate C++ |
| Kernel patch stream | Complete whole-file assembly | Complete: all 23 records have a typed effect table and byte-identical C++ serializer |
| HDD security trigger | Complete whole-file assembly | Complete: typed six-field structure and byte-identical C++ serializer |
| Flash DMA | Complete whole-file assembly | Complete: typed retail-symbol set, semantic operation model, and byte-identical 16 KiB C++ builder |
| Motherboard SMC patches | Complete streams and mapped 8051 listings | Complete: all executable records for all six boards have verified C++ coverage; integration edits have machine-readable ownership |
| Updater | Complete initialized sections; all 376 code entries classified | Complete: all 20 product entries / 6,728 bytes have exact-entry readable models; miniz is pinned and platform/toolchain bodies are excluded |
| BootXell | Complete initialized sections; all 256 code entries classified | Complete: all 9 product routines / 2,944 bytes have verified readable source; inih/miniz are pinned and platform code is excluded |
| AuroraFixer | Complete initialized sections | Complete: all five Peer-specific routines, including live import traversal, mutation, cache maintenance, and entry flow |
| UpdateCheck | Complete initialized sections; all 804 entries exhaustively partitioned | Complete: all 19 product/mixed routines / 6,768 bytes have hash-bound readable models; the five former unknown wrappers / 608 bytes have positive Microsoft-platform provenance |
| SettingsApp | Complete initialized sections; all 1,824 entries conservatively partitioned | Partial: 3 of 246 product/mixed candidates are covered (3,112 of 62,264 bytes); 243 / 59,152 remain; 4 / 688 unknown-provenance XPR2 bytes are source-covered and 9 / 4,648 backend entries still need lifting |
| BootEdit | Complete initialized sections; all 934 entries conservatively partitioned | All 56 known product entries / 22,064 bytes are covered; 15 / 5,840 unknown-provenance backend/XPR2 entries are also source-covered; 17 / 2,808 unknown entries still lack readable source |
| KernelExpansion | Complete whole-file assembly | Partial: 83 resident-code functions have exact-byte-bound, tested C++ models. Of these, 5 / 1,040 bytes are upstream FatFs, 22 / 5,112 bytes remain unattributed, and 56 / 7,836 bytes are product/local adapter or policy behavior. Nine Xbox-facing helpers formerly excluded by role are correctly retained as product adapters. This is component provenance, not personal authorship. |
| Stage2 | Complete whole-file assembly | Partial: worker provenance is partitioned into 3,432 candidate Peer-specific bytes, 4,092 public BadUpdate-derived bytes, and 932 compiler/platform bytes; the Peer-specific worker and modified ROP still need source recovery |

## Completion rule

Future status reports must state both layers. “Complete” without qualification
means source-level recovery is complete; “binary-exact” means only that the
evidence reconstruction reproduces the original bytes.

Source-level completion does not require rewriting exact upstream code. It
requires a reproducible attribution to a pinned revision, recovery of every
Peer-specific change to it, and readable source for every remaining unmatched
closed range. It also does not require republishing proprietary XDK/MSVC
runtime bodies, but excluding them requires positive provenance rather than
merely observing platform-facing behavior.
