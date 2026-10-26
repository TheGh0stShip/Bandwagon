# UpdateCheck.xex analysis

## Provenance and exact reconstruction

- Container: `PeerPressure/System/UpdateCheck.xex`, 129,024 bytes
- Extracted image: 278,528 bytes
- Image SHA-256: `ab6aaf5f2d3f880701269e5835cf9a16aa26c223fcf8ddccb31f8db8ffb72ece`
- Image base `0x91a00000`; entry point `0x91a2e158`
- MSVC `16.0.11886.0`; Xbox/XAPI `2.0.21256.0`

The full initialized image is buildable, byte-identical assembly:

| Source | Section | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `UpdateCheck.text.reconstructed.S` | `.text` | 226,028 | `bde6177f39e9fcee9534b311cea3d420f09e75e45e0385618b9c8b58c3dbdf33` |
| `UpdateCheck.nontext.reconstructed.S` | `.rdata` | 31,572 | `0068f8ffc51e4a1d81bf40f60ff9f47e1b1a3210594e220ec90b9bd618b05aaf` |
| same | `.pdata` | 5,904 | `c4e036e6ff7c634626b22d98456408f59a0e50f8215777b60a69ea14e5bccda0` |
| same | `.data` | 7,344 | `3b0e046588447624100b4a670d6c9aa2643d9f0de3a3fbdb5b4995293015ff91` |

The reproducible inventory contains 804 labels (738 exception records and 65
IDC import names), 3,472 direct calls, and 130 direct string references in 72
labeled functions. Run `tools/analyze_updatecheck_xex.sh` to regenerate it.

## Recovered application behavior

`CheckLatestReleaseVersion` at `0x91a0d860` initializes the networking/TLS
stack, loads the trusted roots from `PPROOT:\\System\\IncludedRootsPEM.txt`, and
performs an HTTPS request to
`https://api.github.com/repos/grimdoomer/Xbox360PeerPressure/releases/latest`.
It accepts only HTTP status 200, parses the JSON response, and delegates the
`tag_name` field to `ParseReleaseVersion` at `0x91a0c3e8`.

The accepted release tag format is `%*c%d.%d.%d`: one leading character is
discarded and exactly three decimal components must parse. Those components are
packed into the caller's 32-bit version output. Every network, response, JSON,
and TLS object observed in this path is destroyed before return.

Large portions of this image are third-party HTTP/TLS and JSON code. The
author's public `ATTRIBUTION.md` identifies BearSSL 0.6 and JsonCpp tag
`00.11.0`; `UpdateCheck-provenance.json` records that declaration and verifies
it against binary-resident signatures. BearSSL evidence includes the TLS key
schedule labels `master secret`, `key expansion`, `server finished`, and
`client finished`. JsonCpp evidence includes the complete modern reader-option
set and distinctive parser/value diagnostics; the recorded tag resolves to
upstream commit `7c186e549fba358ac0541ed75f5a524b393954b3`.

This establishes the third-party ownership boundary and exact declared
versions. Function names are assigned only where function-local evidence
supports them, not merely from membership in a broad address range.

`UpdateCheck-symbols.json` now recovers 22 high-confidence JsonCpp functions
whose exact diagnostics or settings identify their upstream roles. These cover
the modern `OurReader` parse core, `CharReaderBuilder` construction, and core
`Json::Value` conversion/indexing helpers. The generator requires every name's
signature to remain attached to the expected function, preventing silent name
drift when the inventories are regenerated. Broader byte-level matching of
diagnostic-free JsonCpp functions remains pending.
