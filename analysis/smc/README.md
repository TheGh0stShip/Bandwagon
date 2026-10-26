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

The strongest pairwise matches are Falcon/Jasper (`0.933031`) and
Xenon/Zephyr (`0.907449`). Corona is the most divergent family member
(`0.408421` to `0.468750` against the others) and uses nine short `ljmp`
redirect records; every other board uses seven. These are byte-sequence ratios,
not source-level identity percentages.
