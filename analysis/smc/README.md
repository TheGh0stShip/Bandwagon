# Motherboard SMC exploit streams

The six `Flash/*_SmcExploit.bin` files are patch streams for the Xbox 360's
8051-compatible system-management controller firmware. Each record has a
big-endian target address and byte count followed by replacement bytes; these
six streams end exactly after their final record rather than carrying the
optional `0xffffffff` sentinel accepted by the generic decoder.

Every board stream now has a self-contained assembly reconstruction under
`analysis/reconstruction/` and a mapped 8051 disassembly of its single large
embedded replacement routine in this directory. `SmcExploit-summary.json`
records hashes, record counts, target addresses, and embedded-routine sizes.
It also reports pairwise byte-sequence similarity for the embedded routines;
this makes shared ancestry and board-specific divergence measurable without
claiming that shifted instruction addresses are equivalent.
The common reconstruction verifier proves all six complete streams, including
unaligned three-byte `ljmp` redirects and board-specific data edits.

The disassemblies deliberately distinguish instruction recovery from semantic
naming. Short records beginning with opcode `0x02` are proven absolute `ljmp`
redirects. The larger routines manipulate SMC RAM and hardware-facing special
function registers, but register meanings are not assigned without
board-specific firmware evidence.

One semantic block is now established across every board. Each embedded
routine contains a handler that initializes SFRs `EB=1`, `EA=0x10`, and
`F5=0`, then compares SFRs `F1` through `F4` with ASCII `XBPP`. A match stages
`F3,F2,F1` in internal RAM, sets state bit 0, and selects transfer value
`EA=0x18`; Corona additionally selects `EA=0x1c`. The board-specific state RAM
byte is `0x3a` on Corona/Trinity and `0x3c` elsewhere; the staging base is
`0xb1` on Xenon and `0x31` elsewhere. Trinity alone does not repeat the
`EB=1` write on the matched path.

`reconstruction/smc/xbpp_handshake.cpp` is a portable, side-effect-free model
of these exact comparisons and state transitions. It also models the adjacent
sender: state bit 1 and, on five boards, a clear `EE.0` gate a transfer of the three staged
bytes to `EB/EC/ED`, followed by `EA=0`, `E3 |= 0xc0`,
`E4 = (E4 & 0xfc) | 2`, and `F5=6`. Corona instead sets `P2.6` and waits for
`EE.0` to clear before performing that transfer. Run it with
`tools/test_smc_reconstruction.sh`.

Trinity has complete source-level coverage of every executable replacement
record. `reconstruction/smc/trinity_runtime.cpp` lifts
its boot, command, status, notification, state-gate, reset, XBPP receive,
EE-redirect, sender, and timeout/finalization hooks. External firmware calls
and tail jumps are explicit address plans rather than hidden callbacks. The
machine-readable partition in `Trinity-source-coverage.json` covers all 335
replacement-code bytes across records 17 through 19; records 0 through 16 are
classified as declarative call injection, redirect, NOP, or register-setup
integration records. `tools/verify_trinity_smc_source_coverage.py` checks the
artifact hash, record boundaries, gap-free executable coverage, and source
symbols before the host tests run.

Falcon, Jasper, Xenon, and Zephyr also have complete source-level coverage of
every executable replacement record. Their common algorithms and explicit
board profiles live in `reconstruction/smc/legacy_runtime.cpp`. The profiles
retain each board's internal-RAM locations, command byte, firmware call and
tail-jump addresses, and Jasper's shorter notification sequence. The
machine-readable partition in `Legacy-source-coverage.json` covers all 1,767
embedded executable bytes and classifies all 48 small integration records.
`tools/verify_legacy_smc_source_coverage.py` checks the four artifact and
payload hashes, record ownership, gap-free routine coverage, required source
symbols, and the complete ordered board-profile values before the host tests
run.

The strongest pairwise matches are Falcon/Jasper (`0.933031`) and
Xenon/Zephyr (`0.907449`). Corona is the most divergent family member
(`0.408421` to `0.468750` against the others) and uses nine short `ljmp`
redirect records; every other board uses seven. These are byte-sequence ratios,
not source-level identity percentages.
