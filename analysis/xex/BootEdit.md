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
| Product theme-editor application | 48 | 18,392 | Recover as readable source |
| Dear ImGui 1.86 | 442 | 231,184 | Use the pinned upstream source |
| MSVC/Xbox platform and toolchain | 404 | 107,528 | Reuse the target ABI/runtime; do not relift |
| Unknown framework/backend/lookup code | 40 | 12,320 | Fingerprint before deciding whether to lift |

The product category is a conservative component classification, not a claim
of individual authorship. It includes the independently fingerprinted
80-byte non-`.pdata` `ColorsDifferExact` leaf; 43 proven compiler/template
helpers / 11,920 bytes are kept in the platform bucket. A 22-entry /
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

The 608-byte pre-CRT body at `0x820537e0` was removed from the product queue
after its data table was decoded: it is the Microsoft/XDK PAL-50 compatibility
check with 16 localized warning and confirmation strings plus XAM/kernel video
configuration calls. It belongs to platform startup code and is not relifted.

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
The formerly unknown `0x82050f08` entry is also pinned upstream: it is the
inline ImGui 1.86 `ImFontGlyphRangesBuilder` constructor/`Clear` body, with the
exact 2,048-word resize and 8,192-byte bitmap clear from `imgui.h`.
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

`BootEdit-peer-source-coverage.json` binds all 48 product entries and
all 18,392 bytes to readable, tested C++ behavioral models. This includes the
two one-shot singleton accessors at `0x82010068` and `0x820100d0`: they request
the exact 736-byte ThemeManager and 1,184-byte top-level startup objects, call
their recovered constructors after successful allocation, and commit their
guards before allocation so a null result is returned forever rather than
retried. It also includes the kernel-ordinal-932 resolver and the version-9199
`bootanim.xex` thread adapter,
including exact ordinal-1 resolution, diagnostic failure, and successful
entry invocation with the shared boot context. The fixed-four-controller query
wrapper is explicit too, including its 52-byte stride, exact mask/consume
forwarding, and first-match short circuit. It also includes the
exact payload, live snapshot, and paired wrappers around the fixed 41-record
decoder/encoder codec—including the low-level `0x8204d4c0` read-mode wrapper
that restores version 9199 before dispatch—the `0x82011598` theme-name task
constructor and its exact cleared keyboard/result state, frame lifecycle glue, and the complete top-level
startup-state constructor at `0x82051238`. It also covers the three-entry
deferred discard-action family at `0x82051f28` through `0x82052060`, including
accepted-result gating and firmware-return dispatch, plus the synchronized
saved-theme publication wrapper at `0x82050900` with its pre-lock snapshot and
locked replacement ordering. The modal class is now explicit as well: its base
and typed constructors preserve the target's `(-1,-1)` placement, `500x100`
size, message/title/type state, while the completion adapter preserves the
application-type result mapping and continuation ordering. The adjacent
four-controller subsystem is also reconstructed in full, including 52-byte
record initialization, previous/current state rollover, status-1167 disconnect
handling, fatal-error forwarding, and both embedded modal-choice leaves. The
discard workflow now also includes its exact 96-byte constructor at
`0x8204c790`: policy flags, current theme name/path snapshots, cleared child
slots, and initial-phase precedence. The selector's `0x8204cbd8` enumeration
poller is explicit too: zero-timeout polling, pending preservation, and
signaled close/clear ordering. The known-product queue is empty; the
remaining work is provenance resolution for the 40 unknown
entries in the ownership manifest. `reconstruction/boot_edit/theme_editor.cpp` and
`theme_manager.cpp` cover these behaviors. Saved enumeration results
become a display name by removing the final `.xbat` occurrence and a full
`Themes:\` path. The color context-menu model preserves its shared four-float
clipboard and availability flag: Copy only updates the clipboard, while
enabled Paste, Undo, and Reset replace the edited value and report a change.
The companion four-component difference predicate reproduces the exact float
comparisons used to enable Undo, including treating NaN components as unequal.
The same portable module also recovers the editor's seven-item mesh navigation:
Next wraps six to zero and Previous wraps zero to six.
Run the host test with `tools/test_bootedit_reconstruction.sh`.

`BootEdit-xbox-backend.json` removes the next source gap inside the remaining
provenance queue. It hash-binds and source-covers all 11 entries /
5,128 bytes of the mixed Xbox 360/DX9 backend into its shader/declaration
constructors, 97-slot render-state capture/restore layer, font upload,
initialization, controller frame update, render-state setup, and draw submission.
The portable implementation and tests are in
`reconstruction/boot_edit/xbox360_imgui_backend.*`. The block remains
provenance-unknown—source recovery does not prove whether its original Xbox
adapter was product-authored, SDK-derived, or adapted from another source.

`BootEdit-packed-resource.json` likewise hash-binds readable source for the
four-entry / 712-byte XPR2 loader and accessor family. The implementation
preserves the split metadata/physical payload, 16-byte record table, relocated
name and object offsets, TX texture-data fixups, case-insensitive lookup, size,
and data access while adding bounds checks for malformed archives. The family
shares the format and three relocation-normalized helper bodies with
SettingsApp, so the portable implementation is reusable rather than duplicated
application logic.
