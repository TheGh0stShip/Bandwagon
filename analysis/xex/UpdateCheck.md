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

## Exhaustive source ownership

`UpdateCheck-function-ownership.json` assigns all 804 inventory entries and
all 226,028 initialized instruction bytes exactly once:

| Ownership | Entries | Bytes | Recovery treatment |
| --- | ---: | ---: | --- |
| Product/mixed update integration | 19 | 6,768 | All entries have hash-bound readable source |
| BearSSL 0.6 | 187 | 76,232 | Use the pinned upstream source |
| JsonCpp 00.11.0 | 170 | 43,648 | Use the pinned upstream source |
| MSVC/Xbox platform and toolchain | 428 | 99,380 | Reuse the target ABI/runtime; do not relift |
| Unknown | 0 | 0 | No unresolved ownership entries remain |

The XHttps request path and BearSSL-derived certificate reader are
product/mixed integration code: their lineage is recorded, but their adapter
delta is not stock source, and this is not a claim of personal authorship. The
all 19 functions in `UpdateCheck-peer-source-coverage.json` are covered. The
five former XDK/network-facing wrapper containers are excluded only after all
608 bytes were accounted for: 572 substantive bytes match nine bodies in the
clean Microsoft 2.0.17559 bootanim baseline after constrained relocation
normalization, 12 bytes are direct thunks, and 24 bytes are zero padding. Thus
the component is source-complete at this boundary, and none of the 219,260
proven upstream/platform bytes belongs in the lifting queue. Run
`tools/verify_updatecheck_platform_wrapper_matches.py` and
`tools/verify_updatecheck_function_ownership.py` to reproduce the partition.

## Recovered application behavior

`CheckLatestReleaseVersion` at `0x91a0d860` initializes the networking/TLS
stack, loads the trusted roots from `PPROOT:\\System\\IncludedRootsPEM.txt`, and
performs an HTTPS request to
`https://api.github.com/repos/grimdoomer/Xbox360PeerPressure/releases/latest`.
It accepts only HTTP status 200, parses the JSON response, and delegates the
`tag_name` field to `ParseReleaseVersion` at `0x91a0c3e8`. The outer function
reports success and writes the result only when the parsed remote version is
strictly greater than the installed version.

The accepted release tag format is `%*c%d.%d.%d`: one leading character is
discarded and exactly three decimal components must parse. Those components are
packed into the caller's 32-bit version output. Every network, response, JSON,
and TLS object observed in this path is destroyed before return.

`reconstruction/update_check/release_version.cpp` is the portable, tested
reconstruction of the JsonCpp `tag_name` decision, scanner, and `rlwimi`
packing sequence. It preserves the observed C `sscanf` behavior, including
signed decimal components and trailing input, and truncates the three fields
to 8, 8, and 16 bits. JSON decoding remains at the declared JsonCpp ownership
boundary.

`reconstruction/update_check/latest_release.cpp` lifts the complete outer
workflow: installed-version lookup, network/TLS initialization, trusted-root
load, object allocation, recovered request method 2, HTTP-200 gate, JSON
decode boundary, strict version comparison, output write, and ordered cleanup
on every normal exit. Its service interface makes the Xbox and third-party
boundaries explicit without hiding the Peer control flow.
`reconstruction/update_check/network_lifecycle.cpp` recovers the complete
`XHttps::HttpsClient::OpenConnection` behavior: IPv4 TCP socket creation,
socket option `0x5801`, numeric-address and asynchronous XNDNS paths, status
`10036` waiting, address iteration, result codes 0 through 3, diagnostic and
cleanup ordering, TLS binding, and host adoption. Xbox and TLS operations stay
behind explicit injected interfaces.

`reconstruction/update_check/https_request.cpp` recovers both remaining
entries. `BuildHttpRequest` preserves exact field order, the
`Xbox360XHttps/1.0` user agent, connection policy, caller-header handling, and
the target's case-sensitive seven-byte `Accept:` probe. `SubmitGetRequest`
models XHttp URL cracking, default path selection, host-only connection reuse,
the BearSSL write/read and finalization gates, error codes 4, 6, and 7, HTTP
302 `location:` handling, response reset, and the observed recursive redirect.

`UpdateCheck-peer-source-coverage.json` now binds all 19 product/mixed routines
(6,768 bytes total) to exact image hashes and readable source. In addition to
the three request/connection entries, the covered set includes release-version
policy, the complete 11-entry `HttpsResponseBuffer`/container-helper family,
and all three BearSSL-derived certificate-adapter entries. Run the host tests
and coverage checks with `tools/test_updatecheck_reconstruction.sh`.

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

Three BearSSL functions are also recovered directly from unique TLS labels:
`br_ssl_engine_compute_master`, its static `compute_key_block` helper, and the
large T0-generated `br_ssl_hs_client_run` interpreter. Their labels and
function shapes match the BearSSL 0.6 source at tag commit
`8ef7680081c61b486622f2d983c0d3d21e83caad`.
