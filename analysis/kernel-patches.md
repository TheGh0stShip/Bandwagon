# Retail 17559 kernel patch stream

`PeerPressure/System/kernel_patches.bin` is a 372-byte sequence of 23
big-endian replacement records followed by `0xffffffff`. The self-contained
source is `analysis/reconstruction/kernel_patches.reconstructed.S`; it names
each record by its instruction-level effect without speculating about private
Microsoft function names.

The stream contains eight `return 0` replacements, five `return 1`
replacements, two register clears, two unconditional forward skips, one
comparison widened to `0xff`, one `return 0` plus four-NOP replacement, and one
20-byte helper that returns one and stores one through a non-null `r5` pointer.
The remaining short record preserves a load from `0x98(r28)` while replacing
the following two instructions with NOPs.

`reconstruction/kernel_patches/patch_stream.cpp` is the maintainable typed
source form. Its 23-record table contains target addresses and semantic effect
enums rather than opaque byte arrays; a single reviewed encoder emits the
required PowerPC instructions and big-endian record framing. The host test
serializes that table and compares all 372 bytes with the original stream. It
also directly tests the only nontrivial helper behavior, which returns one and
stores one through a non-null output pointer. Run it with
`tools/test_kernel_patch_reconstruction.sh`.

These descriptions state exactly what the replacement instructions do. The
policy meaning of a target is intentionally left unresolved unless corroborated
by a named 17559 symbol or caller evidence; an address alone is not sufficient
to label a check as licensing, signature, networking, or any other subsystem.
