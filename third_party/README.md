# Pinned upstream source

These submodules are provenance inputs and reusable dependencies, not
Bandwagon reconstruction targets. When binary evidence maps a range to one of
them, Bandwagon records the mapping and concentrates source recovery on the
Peer-specific delta.

| Source | Version | Pinned commit | Role |
|---|---|---|---|
| Xbox360BadUpdate | BadUpdate-v1.3 | `94f9215a00382c218288ea230d5f81d73b7f2f9b` | Public exploit baseline |
| BearSSL | 0.6 | `8ef7680081c61b486622f2d983c0d3d21e83caad` | TLS library declared by Peer Pressure |
| JsonCpp | 00.11.0 | `7c186e549fba358ac0541ed75f5a524b393954b3` | JSON library declared by Peer Pressure |
| Miniz | 3.1.1 | `d10b03cc73475af673df40f06e5cefd1d5f940d9` | ZIP library declared by Peer Pressure |
| inih | r62 | `26254ee9de7681f8825433415443e7116ff24b98` | INI parser declared by Peer Pressure |
| Dear ImGui | 1.86 | `512c54bbc062c41c74f8a8bd8ff1fd6bebd1e6d0` | UI library declared by Peer Pressure |
| FatFs | R0.16 base | archive SHA-256 `99f7dc1f7e095356e4a9e3dbe29959090d8b948afe2bbc5441e52fdf4b85449e` | Vendored official FAT/exFAT core identified in KernelExpansion |
| jeff | `1b4040c` | `1b4040c152ccfe9e0c87b71247e6739c11f68afb` | Analysis tool only; not linked product code |

The dependency declaration establishes a candidate provenance boundary; it
does not by itself assign every binary routine. Component attribution
manifests retain the byte-, function-, or signature-level evidence needed for
that assignment. Conversely, an unmatched range is labeled Peer-specific or
unknown, not attributed to an individual author without independent evidence.
