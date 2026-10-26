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
- enable automatic startup update checks, show the update log, and persist
  settings; and
- display Peer Pressure, kernel, and SMC versions.

Fifteen orchestration/UI functions are semantically named with local evidence
in `SettingsApp-symbols.json`.

`SettingsApp-provenance.json` independently verifies component-local binary
signatures for the author's declared Dear ImGui 1.86, BearSSL 0.6, and JsonCpp
`00.11.0` dependencies. `SettingsApp-thirdparty-symbols.json` recovers 24
JsonCpp and three BearSSL routines from exact function-local diagnostics, TLS
labels, and one verified parser-wrapper call chain. ImGui/rendering, HTTP, and
remaining diagnostic-free library functions are preserved exactly but still
need function-level attribution.
