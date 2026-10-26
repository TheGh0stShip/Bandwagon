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
| Stage4 | Complete whole-file assembly | Partial: executable modes 0-2 through a platform adapter; resident dispatcher, copied continuation, RSA key layout |
| Stage3 | Complete whole-file assembly | Partial: executable entry/probe/Stage4 orchestration through a platform adapter |
| Motherboard SMC patches | Complete streams and mapped 8051 listings | Partial: shared `XBPP` handshake and staged transfer |
| Updater | Complete initialized sections | Partial: manifest/wrapper parsing, parent paths, phase-two list, HDD bootstrap record |
| BootXell | Complete initialized sections | Partial: boot-option parsing and launch decisions |
| AuroraFixer | Complete initialized sections | Partial: import lookup/redirection, branch encoding, module entry flow |
| UpdateCheck | Complete initialized sections | Partial: release-version handling |
| SettingsApp | Complete initialized sections | Partial: release metadata and offline/update decisions |
| BootEdit | Complete initialized sections | Partial: theme navigation/edit decisions |
| Stage2 / Flash DMA / KernelExpansion | Complete whole-file assembly | Initial semantic maps only |

## Completion rule

Future status reports must state both layers. “Complete” without qualification
means source-level recovery is complete; “binary-exact” means only that the
evidence reconstruction reproduces the original bytes.
