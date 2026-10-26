# Motherboard SMC exploit streams

The six `Flash/*_SmcExploit.bin` files are patch streams for the Xbox 360's
8051-compatible system-management controller firmware. Each record has a
big-endian target address and byte count followed by replacement bytes; a final
`0xffffffff` terminates the stream.

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
of these exact comparisons and state transitions. Run it with
`tools/test_smc_reconstruction.sh`.

The strongest pairwise matches are Falcon/Jasper (`0.933031`) and
Xenon/Zephyr (`0.907449`). Corona is the most divergent family member
(`0.408421` to `0.468750` against the others) and uses nine short `ljmp`
redirect records; every other board uses seven. These are byte-sequence ratios,
not source-level identity percentages.
