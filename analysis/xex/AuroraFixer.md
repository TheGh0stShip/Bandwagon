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

`AuroraFixer-completeness.json` classifies every one of those 103 entries: four
are Peer-specific application functions, 60 are MSVC/XDK compiler-runtime
entries, and 39 are import thunks. The non-`.pdata` four-byte
`ReturnImmediately` island is recorded separately as the fifth Peer-specific
routine. Exact cross-XEX fingerprints provide 35 safe runtime/import aliases.
This closes the function-ownership inventory for AuroraFixer; it does not treat
the linked CRT as original Peer logic.

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

`reconstruction/aurora_fixer/import_stub.cpp` reconstructs the remainder of
the import-stub mutation logic offline. It recognizes a direct big-endian IAT
word or decodes the `lis r11`/`addi r11` target (including sign extension of
the low immediate), changes only the matching resolved export, and emits the
non-linking replacement thunk. Live cache maintenance and XEX-loader access
remain outside this portable layer. Run both source tests with
`tools/test_aurorafixer_reconstruction.sh`.

`reconstruction/aurora_fixer/dll_main.cpp` reconstructs the first-party DLL
entry orchestration behind an injectable platform boundary. Only process
attach logs and requests a patch; the request always names `xboxkrnl.exe`
ordinal 1 and the local return stub. The closed routine ignores lookup/patch
failure and returns success for every reason, a behavior covered explicitly by
the host test.

`reconstruction/aurora_fixer/import_descriptor.cpp` reconstructs the XEX
import-library traversal used before stub mutation. It decodes the big-endian
descriptor header, NUL-separated module-name table, size-prefixed 40-byte
library headers, and stub-address arrays, then performs the original
case-insensitive module match. It preserves the binary's notable use of only
the low byte of the nominal 16-bit name index while adding bounds checks for
offline use.

`reconstruction/aurora_fixer/module_import.cpp` joins that decoder to the
import-stub mutator. Given the loader-resolved export address and an offline
image mapping, it scans every address in the matched library record and
patches both direct IAT words and absolute branch thunks, matching the closed
routine's non-short-circuiting behavior. Its result reports each form
separately; invalid virtual addresses are rejected instead of dereferenced.

`reconstruction/aurora_fixer/patch_runtime.cpp` completes the live behavior of
`PatchAbsoluteBranch` behind an adapter. It optionally saves the original 16
bytes, installs the encoded thunk, calls imported ordinal `0xbe`
(`MmGetPhysicalAddress`), calls ordinal `0x61` (`KeFlushCacheRange`), and then
invokes syscall `0x0c` (`HvxFlushDcacheRange`) with the patched address and
length 16, exactly matching the recovered call sequence.

`reconstruction/aurora_fixer/live_module_import.cpp` supplies the previously
missing live loader path. It obtains and parses the target image's import
descriptor, resolves the requested ordinal, scans every matching library stub,
and applies either the four-byte direct-IAT write or the sixteen-byte branch
rewrite. Both paths reproduce their distinct physical-address and cache-flush
sequences. `live_dll_main.cpp` connects that routine directly to process attach
and provides the source equivalent of the one-instruction
`ReturnImmediately` target. Platform adapters isolate the proprietary XEX
loader calls and syscall boundary; none of the recovered decision-making or
mutation logic is left behind an opaque callback.

Together these modules cover all five Peer-specific routines identified by the
ownership inventory. AuroraFixer's source-level recovery is therefore complete;
the linked MSVC/XDK runtime and Microsoft import thunks remain classified as
third-party platform code rather than falsely rewritten as Peer source.
