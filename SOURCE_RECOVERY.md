# Source-level recovery status

Bandwagon tracks two different kinds of completion. They must not be conflated:

1. **Binary-exact evidence**: assembly or data declarations rebuild the original
   bytes. This fixes instruction boundaries and preserves evidence, but does not
   by itself constitute a maintainable open-source implementation.
2. **Source-level recovery**: readable C/C++ (and narrowly scoped target assembly
   where the ISA requires it) expresses the recovered algorithms, types, state,
   and control flow, with tests tied back to the binary evidence.

A component is not considered fully open-source reconstructed until its
Peer-authored behavior has source-level coverage. Generated `.S` files under
`analysis/reconstruction/` are the reference oracle used to validate that work,
not the final implementation.

## Current source-level coverage

| Component | Binary-exact evidence | Source-level recovery |
|---|---|---|
| Stage4 | Complete whole-file assembly | Complete: all 1,872 Peer-authored bytes have C++/typed-source coverage; the verified 64 KiB tail is third-party hypervisor data |
| Stage3 | Complete whole-file assembly | Complete: Peer entry/probe source plus function-by-function verified composition with pinned public BadUpdate C++ |
| Motherboard SMC patches | Complete streams and mapped 8051 listings | Complete: all executable records for all six boards have verified C++ coverage; integration edits have machine-readable ownership |
| Updater | Complete initialized sections | Partial: manifest/wrapper parsing, parent paths, phase-two list, HDD bootstrap record |
| BootXell | Complete initialized sections | Partial: seven identified Peer routines have verified C++ coverage; whole-image function ownership classification remains |
| AuroraFixer | Complete initialized sections | Complete: all five Peer-authored routines, including live import traversal, mutation, cache maintenance, and entry flow |
| UpdateCheck | Complete initialized sections | Partial: both identified Peer routines have verified C++ coverage; whole-image function ownership classification remains |
| SettingsApp | Complete initialized sections | Partial: release metadata and offline/update decisions |
| BootEdit | Complete initialized sections | Partial: theme navigation/edit decisions |
| Stage2 / Flash DMA / KernelExpansion | Complete whole-file assembly | Initial semantic maps only |

## Completion rule

Future status reports must state both layers. “Complete” without qualification
means source-level recovery is complete; “binary-exact” means only that the
evidence reconstruction reproduces the original bytes.
