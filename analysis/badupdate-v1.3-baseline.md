# BadUpdate v1.3 baseline

## Provenance

- Upstream: `grimdoomer/Xbox360BadUpdate`, tag `BadUpdate-v1.3`
- Source commit: `94f9215a00382c218288ea230d5f81d73b7f2f9b`
- Checked-out source: `/tmp/peerpressure-badupdate-source`
- Official release archive: `/tmp/Xbox360BadUpdate-Retail-USB-v1.3.zip`
- Release SHA-256:
  `cfd56bca11ec94bbb132a29633eeea82c0f02c016a83f9a8e6c9df2714dfe003`

The detailed reproducible comparison is
[badupdate-v1.3-comparison.json](badupdate-v1.3-comparison.json). The public
Stage4 assembly at this pinned revision rebuilds byte-for-byte to the supplied
public Stage4 binary.

## Binary identity results

| Component in the Peer Pressure package | Result against official v1.3 release |
|---|---|
| `BadUpdateExploit-2ndStage.bin` | exact, 384,960 bytes |
| `BadUpdateExploit-3rdStage.bin` | different: Peer 5,600; public 6,352; 3,584 bytes in exact matching blocks of at least 16 bytes |
| `BadUpdateExploit-4thStage.bin` | exact, 65,920 bytes |
| `BadUpdateExploit-Data.bin` | exact, 1,124 bytes |
| `update_data.bin` | exact, 45,360 bytes |
| `xke_update.bin` | exact, 20,480 bytes |
| `bootanim.xex` | exact, 397,312 bytes |
| `default.xex` | different: Peer 1,003,520; public 27,504,640 bytes |
| `DaTArrest_Xbox360BadUpdate` | exact, 90,112 bytes |

These results apply to `BadUpdatePayload/`. Peer Pressure's separate
`PeerPressure/System/PeerPressureExploit-4thStage.bin` is 67,456 bytes and is
not the exact public Stage4. Its first `0x780` bytes are Peer-specific; its
remaining `0x10000` bytes are the exact public clean-HV block. See
[peer-stage4-preamble.md](peer-stage4-preamble.md).

## Source-reuse boundary

Public BadUpdate source may be used as exact reconstruction only for a
byte-identical component or region. For the Peer-specific Stage4 preamble,
only the complete SMC helper at Peer file `0x294-0x2ff` has presently been
shown to be a complete byte-identical public function. Smaller identical
instruction runs and relocation-adjusted correspondences are evidence of
lineage, not permission to label an entire modified function as public source.

Peer Pressure's author intentionally withheld Peer Pressure source. The
availability of BadUpdate v1.3 source does not establish that Peer Pressure
source was published.
