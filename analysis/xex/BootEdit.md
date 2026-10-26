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
out of Peer-authored source scope in `BootAnim-9199-provenance.md`.
