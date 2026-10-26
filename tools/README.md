# Reverse-engineering tools

`xex2ool` was built from `landaire/acceleration` commit
`e7a68517ad587d0837fe7bca0d7b0ea0326d6324` (xex2ool 0.2.0).

The upstream source is available at:
<https://github.com/landaire/acceleration>

The checked-in executable includes a local correction to
`crates/xex2/src/basefile.rs`. In `try_decrypt_with_key`, the normal-compression
probe originally required the first decrypted `next_block_size` to be nonzero:

```diff
- block_size > 0 && (block_size as usize) < MAX_REASONABLE_BLOCK_SIZE
+ (block_size as usize) < MAX_REASONABLE_BLOCK_SIZE
```

A zero next-block size is valid for a single-block LZX stream. The original
condition misclassified such retail XEX images as devkit-encrypted and then
attempted to decompress ciphertext. The correction was verified by extracting
all XEX files supplied with Peer Pressure as valid PE32 XBOX PowerPC images.

The tool is distributed under MIT OR Apache-2.0; see `LICENSE-xex2ool-MIT`.

`report_recovery_dashboard.py` reads the hash-bound function-ownership and
source-coverage manifests and prints the live product/upstream/platform/unknown
denominator. It intentionally reports unbound behavioral models as zero
verified function coverage:

```sh
python3 tools/report_recovery_dashboard.py
```

`analyze_stage4_preamble.py` statically validates the Peer/public Stage4 clean
block boundary, decodes the Peer big-endian patch stream, maps raw targets to
the exact effective 64-bit addresses used by the payload, verifies selected
byte-identical public ranges, and emits JSON plus a DOT control-flow graph. It
does not execute any payload bytes. Regenerate the checked-in evidence with:

```sh
python3 tools/analyze_stage4_preamble.py \
  --clean-hv /tmp/peerpressure-badupdate-source/Stage4/Stage4_CleanHvData_Retail_17559.bin \
  --json-output analysis/peer-stage4-preamble.json \
  --dot-output analysis/peer-stage4-preamble-cfg.dot
```

`analyze_stage3_probe.py` validates the Peer Stage3 probe range, literal import
stubs, syscall stubs, copied probe bytes, and hashes without executing them:

```sh
python3 tools/analyze_stage3_probe.py
```

`rebuild_peer_reconstructions.py` assembles the recovered big-endian PowerPC
source and enforces whole-file byte identity for both Stage3 and Stage4:

```sh
python3 -B tools/rebuild_peer_reconstructions.py
```

`recover_ppc_region.py` deterministically converts a selected raw big-endian
PowerPC range into LLVM-compatible assembly with source offsets and original
bytes recorded on every line. `--data` emits literal bytes for non-code
regions. Generated Stage3 includes live under
`analysis/reconstruction/generated/`; the whole-file rebuild gate prevents
decoder or transcription drift.

Regenerate all generated Stage3 regions, or verify that the checked-in files
are deterministic, with:

```sh
python3 -B tools/regenerate_stage3_generated.py
python3 -B tools/regenerate_stage3_generated.py --check
```

`analyze_updater_xex.sh` builds the pinned jeff submodule with a temporary,
documented terminal-section fix, recovers the Updater XEX function map,
regenerates its exact `.text` assembly, and restores the submodule before
exiting:

```sh
bash tools/analyze_updater_xex.sh
```

`reconstruct_xex_text.py` can also regenerate or check a mapped XEX text
section directly. It batch-decodes big-endian PowerPC instructions, preserves
invalid/data words losslessly, emits recovered function labels, assembles the
result, and requires byte identity with the selected PE section.
