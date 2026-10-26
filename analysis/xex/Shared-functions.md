# Cross-XEX exact function fingerprints

`shared-function-fingerprints.json` compares every initialized function body in
the six Peer executables. It hashes bytes from each memory-layout PE image's
`.text` RVA (not the stale pre-map raw pointer) and retains groups appearing in
at least two different components.

An exact match is strong evidence of the same linked implementation and safely
propagates a name only when a group has exactly one non-generic existing name.
The reverse is not true: relocation, import-stub position, or link-layout
changes can make common source compile to different bytes. The report therefore
does not classify unmatched functions as unique Peer code.

This analysis currently identifies 203 shared groups containing 506 instances.
It resolves AuroraFixer's generic kernel import slots 17 through 28 against
named Updater stubs, including `RtlInitAnsiString`, `NtAllocateVirtualMemory`,
`KeBugCheckEx`, `KeGetCurrentProcessType`, `NtFreeVirtualMemory`,
`RtlCompareMemoryUlong`, critical-section setup, virtual-memory query,
exception raising, and ANSI/Unicode string helpers.

The top-level `propagated_aliases` array is the consumable name layer. Every
entry cites the exact body hash that supports it; ambiguous groups with zero or
multiple authoritative names are excluded automatically.
