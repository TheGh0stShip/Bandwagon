# BootEdit.xex analysis

## Provenance and exact reconstruction

- Container: `PeerPressure/System/BootEdit.xex`, 868,352 bytes
- Extracted image: 2,228,224 bytes
- Image SHA-256: `1396cf0f05e5cb07de4f2ca067975376b0b319dd52f3ca4182af8b00d1175df8`
- Image base `0x82000000`; entry point `0x82053a40`
- MSVC `16.0.11886.0`; Xbox/XAPI `2.0.21256.0`

The initialized code and application data are exact, buildable assembly:

| Source | Section | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `BootEdit.text.reconstructed.S` | `.text` | 369,424 | `c2555a4532b28ca8e23da1f93066a9cd383a805826f83676d4910856e6d52df0` |
| `BootEdit.nontext.reconstructed.S` | `.rdata` | 47,928 | `932c99d27b44903e4b76489f4cabf672cbf5e85e75fbdfff599ecae2f633da6b` |
| same | `.pdata` | 6,920 | `37d8e617e346a281da2a4b93873ce7d2a8847e0670b00e128e567d3609e9b6f7` |
| same | `.data` | 13,204 | `c988c18299870ca2b5308ad8a4c5b186b08498831cb4946b81db247aea15b14c` |

The function inventory contains 934 labels (865 exception records and 70 IDC
names), the direct-call graph contains 4,777 edges, and the direct string map
contains 218 references in 69 functions. Regenerate the complete set with
`tools/analyze_bootedit_xex.sh`.

## Exhaustive source ownership

`BootEdit-function-ownership.json` assigns all 934 inventory entries and all
369,424 initialized instruction bytes exactly once:

| Ownership | Entries | Bytes | Recovery treatment |
| --- | ---: | ---: | --- |
| Product theme-editor application | 23 | 13,696 | Recover as readable source |
| Dear ImGui 1.86 | 441 | 231,096 | Use the pinned upstream source |
| MSVC/Xbox platform and toolchain | 351 | 92,920 | Reuse the target ABI/runtime; do not relift |
| Unknown framework/backend/lookup code | 119 | 31,712 | Fingerprint before deciding whether to lift |

The product category is a conservative component classification, not a claim
of individual authorship. It includes the independently fingerprinted
80-byte non-`.pdata` `ColorsDifferExact` leaf; six independently matched
runtime/template helpers / 552 bytes are kept in the platform bucket. A 22-entry /
9,552-byte prefix before the version checker is pinned to ImGui by the shared
136/320/624-byte opening sequence and normalized SettingsApp matches. Four
more entries / 360 bytes exactly normalize to independently proven Updater
runtime bodies. The custom or mixed Xbox 360/DX9 backend and XPR2
packed-resource family remain unknown: role and API use do not establish
source provenance. The 376-byte `0x82053238` container is also unknown: it
combines four generic/XDK-facing bodies, two of which normalize exactly to
SettingsApp, but cross-component reuse alone does not prove platform or public
source provenance. Run
`tools/verify_bootedit_function_ownership.py` to reproduce the partition.

## Recovered application behavior

BootEdit is an ImGui-based editor for boot-animation theme material and mesh
settings. It loads its editor fonts from `UIDATA`, exposes mesh flags and the
diffuse/specular/ambient colors, supplies color copy/paste/undo/default actions,
and applies edited values to the live preview. UI input labels explicitly cover
navigation, selection, restart, rewind, pause, main menu, UI visibility, and a
timeline.

Saved themes use the `.xbat` extension under `Themes:\\`. The recovered flow
enumerates `Themes:\\*.xbat`, selects or loads a theme, tracks unsaved changes,
prompts before discarding them, and reports all save/load/apply outcomes. Eight
application functions carrying this flow have high-confidence semantic names
in `BootEdit-symbols.json`.

`BootEdit-provenance.json` verifies Dear ImGui 1.86 against the embedded exact
version, standard state-file names, and Xbox 360/DX9 backend identifiers; the
declared upstream tag resolves to commit
`512c54bbc062c41c74f8a8bd8ff1fd6bebd1e6d0`.
`BootEdit-thirdparty-symbols.json` additionally recovers
`ImGui::DebugCheckVersionAndDataLayout` from its function-local version check.
Remaining ImGui, standard-library, and rendering functions are preserved
exactly but still need individual attribution. The separate
bundled 9199 `bootanim.xex` is a 2010 Microsoft component and is documented as
out of Peer-specific source scope in `BootAnim-9199-provenance.md`.

The semantic map additionally names `ColorsDifferExact` at `0x82050a00` and
`DrawColorContextMenu` at `0x82050b08`. The former is a non-`.pdata` leaf whose
80 bytes, including its final `blr`, are fingerprinted independently.
`tools/validate_bootedit_analysis.py`
binds both names to the exact image, menu strings, and direct call edge during
every full BootEdit regeneration.

`BootEdit-peer-source-coverage.json` binds 16 of the 23 product entries and
7,936 of 13,696 bytes to readable, tested C++ behavioral models. The exact
remaining queue is 5 entries / 5,760 bytes and is listed with per-entry
blockers in that manifest. `reconstruction/boot_edit/theme_editor.cpp` and
`theme_manager.cpp` cover the first tranche. Saved enumeration results
become a display name by removing the final `.xbat` occurrence and a full
`Themes:\` path. The color context-menu model preserves its shared four-float
clipboard and availability flag: Copy only updates the clipboard, while
enabled Paste, Undo, and Reset replace the edited value and report a change.
The companion four-component difference predicate reproduces the exact float
comparisons used to enable Undo, including treating NaN components as unequal.
The same portable module also recovers the editor's seven-item mesh navigation:
Next wraps six to zero and Previous wraps zero to six.
Run the host test with `tools/test_bootedit_reconstruction.sh`.
