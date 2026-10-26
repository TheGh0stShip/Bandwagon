# AuroraFixer.xex analysis

## Provenance and exact reconstruction

- Container: `PeerPressure/SideLoading/AuroraFixer.xex`, 102,400 bytes
- Extracted image: 163,840 bytes
- Image SHA-256: `eca239f0f17b4c8c4b9e1e24bb29fad324ed6b698081bf8e2fa379707b2de168`
- Image base: `0x88000000`; entry point: `0x88010668`
- Devkit, uncompressed XEX; MSVC `16.0.11886.0`, Xbox/XAPI `2.0.21256.0`
- PDB path: `Z:\\D\\Visual Studio 2010\\AuroraFixer\\AuroraFixer\\Release\\AuroraFixer.pdb`

The complete initialized image is represented by buildable assembly:

| Source | Section | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `AuroraFixer.text.reconstructed.S` | `.text` | 18,988 | `4a670dad5ee09934ec457c0b50ae9f7ca51ba0a2e1a17974506f94341c8a7925` |
| `AuroraFixer.nontext.reconstructed.S` | `.rdata` | 3,284 | `2945b9d2c52ecd5ab9a71c21596e7b46d5a398da2ba564e6f25c14f7eebe1f85` |
| same | `.pdata` | 496 | `e177de5091701bce1c1f682fcf749ea88b4f6d82d882479b92f8b2bf73b02791` |
| same | `.data` | 1,312 | `5a604ef96dd30e88b7dd9603b67410ec27f1ec96587b4d7eccc3faeed8553254` |

The function inventory has 103 labels (62 `.pdata`, entry/text labels, and 40
IDC import names). The generated call graph contains 236 direct calls, 178
resolving exactly. Regenerate everything with
`tools/analyze_aurorafixer_xex.sh`.

## Recovered behavior

`AuroraFixerDllMain` handles `DLL_PROCESS_ATTACH`, emits its attach diagnostic,
obtains the `xboxkrnl.exe` module, and calls `PatchModuleImportByOrdinal` for
ordinal 1 with `ReturnImmediately` as the replacement. Xbox kernel ordinal 1 is
`DbgBreakPoint`; the fix therefore suppresses Aurora's imported debugger break
by redirecting its import stub to a one-instruction `blr`.

`PatchModuleImportByOrdinal` obtains the target module's XEX import descriptor,
resolves the requested export address, walks imported stubs, and patches the
matching one. Its diagnostics cover every failed lookup. `PatchAbsoluteBranch`
optionally saves the replaced 16 bytes, writes `lis r11`, `addi r11`, `mtctr
r11`, and `bctr`/`bctrl`, then performs the required data/instruction cache
maintenance. `reconstruction/aurora_fixer/absolute_branch.*` is the portable,
tested high-level reconstruction of that instruction encoder; it performs no
live memory patching.
