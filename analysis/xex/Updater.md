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

Direct PPC address-construction references into `.rdata` are generated in
[`Updater-string-xrefs.json`](Updater-string-xrefs.json). The initial semantic
name overlay, with confidence and evidence kept separate from recovered COFF
symbols, is in [`Updater-symbols.json`](Updater-symbols.json).
[`Updater-callgraph.json`](Updater-callgraph.json) records 1,099 direct
branch-and-link call sites, 967 of which resolve exactly to a recovered function
entry. Indirect virtual calls and `bctrl` import patterns are intentionally not
guessed; each edge retains its instruction and target address for review.

[`Updater.text.reconstructed.S`](../reconstruction/Updater.text.reconstructed.S)
is a complete buildable reconstruction of the 88,640-byte `.text` section.
Assembling it reproduces SHA-256
`3a2f4088b82f70165888a152480c3bb0d4543fa0e3503cea03ac476194aa5022`
exactly. All 376 recovered function boundaries are represented by labels.

[`Updater.nontext.reconstructed.S`](../reconstruction/Updater.nontext.reconstructed.S)
is the corresponding buildable reconstruction of every initialized non-code
application section present in the extracted in-memory image:

| Section | Virtual address | Bytes | SHA-256 |
| --- | ---: | ---: | --- |
| `.rdata` | `0x98000400` | 16,268 | `fe5608061c679e1fc88197515ba6e331c590d06eab9a047a582b63cd50cc76b7` |
| `.pdata` | `0x98004400` | 2,000 | `81ef6b4dc5d6ad9cf7ae1572fd6139bef247cc01aeb1180ff6221080ccb52406` |
| `.data` | `0x9801b000` | 7,576 | `b66413e91afc0719c718c42e2910fce11db16d8d7ab7f7999405b468da631b0a` |

`tools/reconstruct_xex_nontext.py` derives these ranges from the PE section
table, regenerates the assembly, and can assemble and compare all three ranges
with `--check --verify-bytes`. This covers constants, exception metadata, and
initialized writable state. It does not claim the later `.edata`, `.idata`,
`.XBLD`, or `.reloc` virtual sections: the extracted image ends at `0x1d000`,
where those section ranges begin, so they require evidence from the XEX loader
metadata or another extraction route rather than invented bytes.

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

### Stage2 HDD bootstrap record

`fn_98006D40` is identified at high confidence as `InstallHddBootstrap`. Its
control flow reconstructs the following operation without executing the XEX:

1. Open `\\Device\\Harddisk0\\PhysicalDisk` for writing.
2. Query `Hdd/PeerPressureExploit-Stage2.bin` in the SoftmodData archive and
   reject a zero length or any payload larger than `0x7c000` bytes.
3. Round `payload_size + 0x1ff` down to a 512-byte boundary, allocate that
   rounded size with 128-byte alignment, and read the archive member into it.
4. Compute a CRC over the unrounded payload, then overwrite the first 16 bytes
   with an `XBPP` bootstrap header (all multibyte fields are stored in native
   big-endian PPC order):

   | Offset | Size | Reconstructed value |
   | ---: | ---: | --- |
   | `0x00` | 4 | magic `XBPP` (`0x58425050`) |
   | `0x04` | 4 | SoftmodData context field at offset `0x80` |
   | `0x08` | 4 | unrounded archive-member size |
   | `0x0c` | 4 | `RtlComputeCrc32(0, buffer, payload_size)` |

5. Call `NtWriteFile` once with the rounded buffer size and byte offset
   `0x4000`, then free the buffer and close the disk handle on every exit path.

The CRC is calculated before the four header words are written. Consequently,
it describes the original Stage2 archive bytes, including their original first
16 bytes, rather than the finalized on-disk record. This ordering follows the
instructions at `0x98006ec4` through `0x98006f00`; the import calls resolve to
`RtlComputeCrc32` at `0x9801a760`, `NtWriteFile` at `0x9801a750`, and `NtClose`
at `0x9801a740`.

No instruction initializes the bytes between the unrounded payload length and
the rounded write length. Unless the allocator itself returns cleared memory,
the final partial sector therefore contains stale allocation data. This is a
faithful description of the closed binary, not behavior an open reimplementation
should preserve unintentionally.

The executable code section is now exactly reconstructable, but this is not
yet a source-complete claim for the application. The next Updater phase is to
expand semantic names and types across the remaining application functions,
decode the reconstructed data structures, recover the loader-only metadata,
and separate bundled runtime/library code from Peer-authored code.
