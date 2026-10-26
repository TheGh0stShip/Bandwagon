# Updater.xex analysis

## Provenance

- Container: `Updater.xex`, 53,248 bytes
- Extracted PE image: 118,784 bytes
- PE SHA-256: `2f8210528112a5cf5df514db7a3fa2c05322e845c6c2c747e8fc7c1485f4522c`
- Load address: `0x98000000`
- Entry point: `0x9800d130`
- Toolchain metadata: MSVC/Xbox 360 SDK `16.0.11886.0`; kernel/XAPI
  `2.0.21256.0`
- Original debug path:
  `Z:\\D\\Xbox 360\\Peer Pressure\\XboxApps\\Updater\\Release_Retail\\Updater.pdb`

## Function recovery

The pinned `freeqaz/jeff` revision recovers 250 exception-table functions and
expands them to 376 code symbols through control-flow analysis. It also names
48 kernel import thunks. The complete address inventory is in
[`Updater-functions.json`](Updater-functions.json).

[`Updater.text.reconstructed.S`](../reconstruction/Updater.text.reconstructed.S)
is a complete buildable reconstruction of the 88,640-byte `.text` section.
Assembling it reproduces SHA-256
`3a2f4088b82f70165888a152480c3bb0d4543fa0e3503cea03ac476194aa5022`
exactly. All 376 recovered function boundaries are represented by labels.

The final `.text` function falls through exactly at the section boundary.
Upstream jeff currently rejects that valid terminal case. The narrowly scoped
patch in `patches/jeff-terminal-section-fallthrough.patch` permits only an
unfinished block within the final 16 bytes of a code section to terminate at
that section's end. `tools/analyze_updater_xex.sh` applies the patch
temporarily, restores the submodule on exit, and regenerates the function map.

## Confirmed application behavior

Static strings and call targets establish these major responsibilities:

- validate the custom SoftmodData wrapper and `XBPP` manifest;
- verify archive paths, sizes, hashes, and ZIP integrity;
- stage the phase-two update file list;
- extract the `PeerPressure/` tree;
- read `Hdd/PeerPressureExploit-Stage2.bin`;
- validate the Stage2 size and write a modified copy to
  `\\Device\\Harddisk0\\PhysicalDisk`; and
- report and preserve explicit failure states for archive, allocation, file,
  and raw-disk operations.

The executable code section is now exactly reconstructable, but this is not
yet a source-complete claim for the application. The next Updater phase is to
assign semantic names and types to its application functions, reconstruct the
Stage2 mutation algorithm, recover `.rdata` and `.data`, and separate bundled
runtime/library code from Peer-authored code.
