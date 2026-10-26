# Stage4 source composition

The complete Peer-authored Stage4 preamble is represented by the C++ in this
directory. It covers entry modes, patch decoding and writes, mapping-table
maintenance, the exact SMC MMIO transaction, the status trampoline, resident
`rbtr` dispatcher, copied continuation, patch payloads, and RSA-key structure.
Target calls and cache instructions remain narrow platform boundaries.

The final 65,536 bytes of the artifact are not Peer code. They are an exact
copy of the clean Xbox 360 hypervisor segment already distributed by public
BadUpdate. Bandwagon records and verifies that ownership boundary instead of
turning Microsoft firmware data into a misleading C++ byte array.

`analysis/stage4/source-coverage.json` partitions the complete 67,456-byte
artifact. Verify the source paths, symbols, ownership boundary, pinned public
input, and byte equality with:

```sh
python3 tools/verify_stage4_source_coverage.py
bash tools/test_stage4_reconstruction.sh
python3 tools/rebuild_peer_reconstructions.py
```

The exact assembly remains the binary oracle. It is not counted as the
high-level implementation except for alignment and genuinely target-specific
instruction evidence.
