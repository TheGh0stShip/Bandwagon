# SettingsApp.xex analysis

## Provenance and exact reconstruction

- Container: `PeerPressure/System/SettingsApp.xex`, 1,015,808 bytes
- Extracted image: 2,555,904 bytes
- Image SHA-256: `a77a2a06d71c9a9512879122187fe5541270056f54657574d5a2fe9a109e035a`
- Image base `0x82000000`; entry point `0x8202e0e0`
- MSVC `16.0.11886.0`; Xbox/XAPI/XHTTP/D3D `2.0.21256.0`

Every initialized application code/data section has buildable, byte-identical
assembly:

| Source | Section | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `SettingsApp.text.reconstructed.S` | `.text` | 628,572 | `051ce3e3064b79729e0785de15f1b53315b89e3ec0bada2db5d6d3c335ed3d1f` |
| `SettingsApp.nontext.reconstructed.S` | `.rdata` | 106,060 | `6ac3b2d4dba7491a0a4d1b05c82fe6a8aa5f52b80a9523c3a683b3d03d182bfc` |
| same | `.pdata` | 13,440 | `f6de93103f6f31c0e463c815eb087d7231c1d6ddcba84f14ca89c8097f8c6186` |
| same | `.data` | 31,384 | `b0b52011e278875d0a4b82c51afe5868ce6b4007b8226a2e7395bd0562060d83` |

The reproducible inventory contains 1,824 labels (1,680 exception records and
144 IDC names), 8,808 direct calls, and 403 direct string references in 154
functions. `tools/analyze_settingsapp_xex.sh` regenerates all artifacts.

## Exhaustive source ownership

`SettingsApp-function-ownership.json` conservatively partitions all 1,824
entries and all 628,572 initialized code bytes:

| Ownership | Entries | Bytes | Recovery treatment |
| --- | ---: | ---: | --- |
| Product application/mixed adapters | 246 | 62,264 | Review at function level and recover the actual product behavior |
| Dear ImGui 1.86 | 376 | 165,512 | Use the pinned upstream source |
| BearSSL 0.6 | 187 | 76,136 | Use the pinned upstream source |
| JsonCpp 00.11.0 | 153 | 39,680 | Use the pinned upstream source |
| miniz 3.1.1 | 17 | 15,848 | Use the pinned upstream source |
| MSVC/Xbox platform/toolchain and imports | 832 | 263,796 | Reuse the target ABI/runtime; do not relift |
| Unknown XPR2/graphics backend | 13 | 5,336 | Fingerprint before deciding whether to lift |

This removes 1,565 entries / 560,972 bytes of pinned-library and platform code
from the closed-source queue. The 246-entry product/glue bucket is a
conservative component boundary, not a claim that every template or helper in
it is original Peer Pressure logic. It is now the auditable upper bound to
refine and source-cover. The three-function BearSSL-derived certificate loader
stays product/mixed because its C++/platform adapter delta is not stock source.
The XPR2 resource helpers and mixed Xbox 360/DX9 backend stay unknown because
role and API use do not establish provenance. Run
`tools/verify_settingsapp_function_ownership.py` to reproduce the partition.

`SettingsApp-peer-source-coverage.json` currently binds 3 exact product/mixed
entries / 3,112 bytes to readable models: `FindOfflineUpdate`,
`FetchLatestRelease`, and `DrawAutoUpdateSettings`. The remaining conservative
upper bound is 243 entries / 59,152 bytes; those entries still require either
positive non-product provenance or readable product source.

## Recovered application behavior

The settings application coordinates the complete user-facing configuration
and update surface:

- scan removable `Mass%d:` devices for an offline
  `$SoftmodUpdate\\UpdateData.zip`;
- query the GitHub latest-release API, select/download `UpdateData.zip` assets,
  retrieve change logs, and store the archive at `PPROOT:\\UpdateData.zip`;
- launch `Updater.xex`, display installation progress and errors, and schedule
  a successful reboot;
- select stock, themed, custom, or disabled boot animation modes and enumerate
  9199 `.xbat` themes;
- launch `PPROOT:\\System\\BootEdit.xex` for theme editing;
- configure the dashboard executable and parse OtherOS `BootOption` entries
  from `PPROOT:\\OtherOS\\boot.ini`;
- display and toggle NAND write protection, warning that disabling it allows
  applications to write console NAND;
- enable automatic startup update checks, show the update log, and persist
  settings; and
- display Peer Pressure, kernel, and SMC versions.

Fifteen orchestration/UI functions are semantically named with local evidence
in `SettingsApp-symbols.json`.

`SettingsApp-provenance.json` independently verifies component-local binary
signatures for the author's declared Dear ImGui 1.86, BearSSL 0.6, and JsonCpp
`00.11.0` dependencies. `SettingsApp-thirdparty-symbols.json` recovers 24
JsonCpp routines, three BearSSL routines, and ImGui's version/layout checker
from exact function-local diagnostics, TLS labels, and one verified parser-wrapper call chain. Rendering, HTTP, and
remaining diagnostic-free library functions are preserved exactly but still
need function-level attribution.

`reconstruction/settings_app/offline_update.cpp` begins the portable
first-party reconstruction at the offline-update boundary. It preserves the
exact two-device scan domain, `Mass%d:\$SoftmodUpdate\UpdateData.zip` path,
shared SHA-1 archive-wrapper validation, big-endian 8/8/16 version fields, and
`v%d.%d.%d` display form. ZIP member extraction remains at the already
identified Miniz/library boundary. Run its host test with
`tools/test_settingsapp_reconstruction.sh`.

`reconstruction/settings_app/release_metadata.cpp` reconstructs the
first-party selection logic after JsonCpp parses GitHub's latest-release
response. It applies the shared release-tag codec, retains `prerelease` and
`body`, and scans every asset for `UpdateData.zip` and `ChangeLog.txt`, with
later duplicate names replacing earlier values. The update is accepted only
when the selected ZIP has both a nonzero asset ID and nonzero size. The closed
function does not check the return from `sscanf("%*c%d.%d.%d")`; malformed
tags can therefore cause it to pack indeterminate stack values. The portable
boundary rejects malformed tags while preserving the observed valid-input
behavior.

`reconstruction/settings_app/update_decision.cpp` recovers the status branch
tree in `DrawAutoUpdateSettings`. It compares packed 8/8/16 versions as
unsigned words, offers nothing unless the available version is strictly newer,
uses source state 3 for the download wording and other ready sources for the
install wording, and distinguishes failed state 1 from all other non-ready
checking states.
