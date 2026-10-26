# Exact source reconstructions

These files are real GNU-style PowerPC assembly accepted by LLVM's assembler.
They are reconstructed source equivalents, not the unpublished Peer Pressure
original source. Names are derived from proved behavior; unresolved meanings
remain labeled `unknown` or `hypothesis`.

## Stage4

`PeerPressureExploit-4thStage.reconstructed.S` rebuilds the complete 67,456-byte
Peer Stage4 file byte-for-byte, including its resident functions, patch
records, copied HV code, structured data, padding, and the exact public
BadUpdate clean-HV tail. The clean tail is included from the already verified
local public Stage4 artifact rather than duplicated.

## Stage3

`PeerPressureExploit-3rdStage-probe.reconstructed.S` rebuilds the complete
entry point at file `0x0000-0x00f7`, the recovered new function at
`0x0f8-0x1e7`, its embedded probe at `0x15a0-0x15d7`, and the exact
syscall/compiler/import stubs that those regions call. Unreconstructed gaps
are explicit zero placeholders created by `.org`; they are not claimed as
equivalent to the original Stage3 bytes.

## Verification

Run:

```sh
python3 -B tools/rebuild_peer_reconstructions.py
```

The verifier assembles for `powerpc64-unknown-linux-gnu`, extracts `.text`,
compares the entire Stage4 output, and compares only the explicitly recovered
Stage3 ranges. It never executes an Xbox binary or reconstructed payload.
