# Peer Pressure Stage4 preamble analysis

## Scope and address notation

This report covers only the Peer-specific prefix of
`PeerPressure/System/PeerPressureExploit-4thStage.bin`. All findings are from
static, big-endian PowerPC decoding. No payload bytes were executed.

`B` below is the runtime blob base passed in `r4`; the entry copies it to `r31`
at file offset `0x014` (runtime `B+0x014`). Resident-code virtual addresses are
therefore written as `B+offset`. Code carried inside patch records is also
reported at the 64-bit destination computed by the copier.

Machine-readable evidence is in [peer-stage4-preamble.json](peer-stage4-preamble.json),
and the basic-block graph is in
[peer-stage4-preamble-cfg.dot](peer-stage4-preamble-cfg.dot).
The graph includes the five resident functions and the two multi-instruction
copied-code records. One- and two-instruction patch fragments are catalogued in
the record table rather than misrepresented as standalone functions.

The buildable reconstruction is
[`reconstruction/PeerPressureExploit-4thStage.reconstructed.S`](reconstruction/PeerPressureExploit-4thStage.reconstructed.S).
LLVM assembly plus `.text` extraction produces all 67,456 reference bytes with
SHA-256 `f482f2e13b3e4d04b0a322f4553c628f7afe556c070da25710b8581d3c37df22`.
This proves binary equivalence; it does not claim recovery of the author's
original labels or comments.

## Verified boundary and hashes

| Item | Size | SHA-256 |
|---|---:|---|
| Peer Stage4 whole file | `0x10780` (67,456) | `f482f2e13b3e4d04b0a322f4553c628f7afe556c070da25710b8581d3c37df22` |
| Peer-specific preamble, file `0x000-0x780` | `0x780` (1,920) | `20307fdbe830e2e6ce727b6bb80e45ac94a7e7370d67f5b33e5db69e30c6f63d` |
| clean-HV block, Peer file `0x780-0x10780` | `0x10000` | `3764ec2a48529924b1422b4e19eaada348cb5a2f9fde310a94b75d332c601b47` |
| public BadUpdate Stage4 preamble, file `0x000-0x180` | `0x180` | `9fbe419804bbf824040d5cefed6cf36ec05d29a77aff96cdf881343729a87d0e` |

Peer bytes `0x780-0x10780` and public BadUpdate bytes
`0x180-0x10180` both compare byte-for-byte equal to the pinned source file
`Stage4/Stage4_CleanHvData_Retail_17559.bin`. The clean block is exactly 65,536
bytes. Consequently, no Peer-specific Stage4 byte occurs after file offset
`0x77f`.

The Peer preamble has four mechanically verified regions:

| File range | Runtime range | Interpretation |
|---|---|---|
| `0x000-0x300` | `B+0x000`–`B+0x2ff` | resident executable code: five functions |
| `0x300-0x380` | `B+0x300`–`B+0x37f` | one destination qword followed by reserved zeros |
| `0x380-0x750` | `B+0x380`–`B+0x74f` | 27 big-endian patch records plus sentinel |
| `0x750-0x780` | `B+0x750`–`B+0x77f` | all-zero alignment padding |

## Comparison with public BadUpdate Stage4

The public Stage4 binary is identical to the official v1.3 release and its
assembly rebuild is byte-identical. That permits exact source reuse only where
Peer bytes are themselves identical.

| Public file range | Peer file range | Size | Verified relationship |
|---|---|---:|---|
| `0x000-0x018` | `0x000-0x018` | `0x18` | identical entry prologue |
| `0x018-0x040` | `0x034-0x05c` | `0x28` | identical SMC-command construction |
| `0x050-0x054` | `0x060-0x064` | `0x04` | identical `li r5,0x200` |
| `0x05c-0x068` | `0x06c-0x078` | `0x0c` | identical `0xe14` call setup and `bctrl` |
| `0x0c0-0x0e0` | `0x0cc-0x0ec` | `0x20` | identical return marker and epilogue |
| `0x0e0-0x14c` | `0x294-0x300` | `0x6c` | complete byte-identical SMC helper |
| `0x14c-0x154` | `0x300-0x308` | `0x08` | identical clean-HV destination qword |

The complete helper at Peer file `0x294-0x2ff` (runtime
`B+0x294`–`B+0x2ff`) is the only complete public function presently justified
for verbatim source reuse. It moved by `+0x1b4` from public file
`0x0e0-0x14b`. The destination qword moved by the same amount, from public
`0x14c` to Peer `0x300`.

Two corresponding operands changed because the preamble grew:

- Public file `0x054`, `ld r4,0x14c(r31)`, and Peer file `0x064`,
  `ld r4,0x300(r31)`, both load their image's destination qword.
- Public file `0x058`, `addi r3,r31,0x180`, and Peer file `0x068`,
  `addi r3,r31,0x780`, both form their image's clean-HV-block address.

These are relocation-adjusted correspondences, not byte identity. The public
entry as a whole is not reused: Peer adds an `r5` mode dispatch at
`B+0x018`–`B+0x030`, removes the two public RSA-patch sequences, and calls new
helpers.

## Resident control flow and incremental reconstruction

### Entry: file `0x000-0x0ec`, runtime `B+0x000`–`B+0x0eb`

The entry saves `r30`, `r31`, and LR, reserves `0x30` stack bytes, and records
`B=r4` (`0x000-0x014`). It selects four paths from unsigned `r5`
(`0x018-0x030`): mode `0`, mode `1`, mode `2`, or default. Every path joins at
`0x0cc`, sets `r3=0x41414141`, restores the frame, and returns
(`0x0cc-0x0ec`).

Mode `0` (`0x034-0x080`) does the following:

1. Creates the 16-byte SMC buffer whose first bytes are `99 ff ff`
   (`B+0x034`–`B+0x058`) and calls `B+0x294` (`B+0x05c`).
2. Loads `r5=0x200`, `r4=*(uint64_be *)(B+0x300)`, and `r3=B+0x780`, then
   calls address `0x0e14` (`B+0x060`–`B+0x074`). The destination qword at file
   `0x300` is `0x8000010600030000`. Because this setup is identical to the
   rebuilt public sequence except for the two base-relative offsets, the
   public name `HvpRelocateCacheLines` is justified for this call only.
3. Calls the patch-stream copier at `B+0x0ec` with `r3=B`
   (`B+0x078`–`B+0x07c`).

Mode `1` (`0x084-0x0b8`) treats `r6` as a pointer. It loads the first 32-bit
big-endian value, subtracts `0x10000`, calls unresolved HV address `0x057c`,
clears bit `0x20` in the word at the returned pointer, then calls `B+0x174`
with `r3=B` and `r4=r6`. The effect of the bit clear is exact; the type and
name of the pointed-to object remain hypotheses.

Mode `2` (`0x0bc-0x0c8`) calls `B+0x210` with `r3=B` and `r4=r6`. No check is
made that `r6` is nonzero.

### Patch copier: file `0x0ec-0x174`, runtime `B+0x0ec`–`B+0x173`

The function begins at `B+0x0ec`, sets its cursor to `B+0x380` at
`B+0x108`, and iterates until a target value of `0xffffffff`
(`B+0x114`–`B+0x11c`). Each record is:

```text
u32be raw_target
u32be byte_count
u8    replacement[byte_count]
```

At `B+0x134`–`B+0x13c`, it computes the actual destination exactly as:

```text
effective_target = raw_target | ((raw_target & 0x00030000) << 17)
```

It then copies exactly `byte_count` bytes (`B+0x140`–`B+0x150`). A zero count
is skipped at `B+0x128`–`B+0x12c`. There is no alignment rounding in the
cursor and no cache-maintenance instruction in this helper.

### Pair-table helper: file `0x174-0x210`, runtime `B+0x174`–`B+0x20f`

This function calls unresolved HV address `0x05c0` once and saves its return
value (`B+0x190`–`B+0x19c`). It then consumes 8-byte pairs from the address in
entry `r4` until the first word is `0xffffffff` (`B+0x1a0`–`B+0x1b0`). For
each pair `{address,value}` it:

- calls `0x057c(address)` and stores `value` through the returned pointer
  (`B+0x1b4`–`B+0x1c4`);
- derives `2 * ((value >> 6) & 0x3fff)` and stores halfword `0x0800` at that
  offset from the saved `0x05c0` result (`B+0x1c8`–`B+0x1d8`); and
- calls unresolved HV address `0x2510(address)` (`B+0x1dc`–`B+0x1e8`).

`apply_address_value_pairs` is a behavioral label, not a recovered symbol.
The meaning of the table returned by `0x05c0` and of the `0x0800` flag is not
yet established.

### Range helper: file `0x210-0x294`, runtime `B+0x210`–`B+0x293`

Starting with index zero, this function forms
`address = 0x80040000 + index*0x10000` (`B+0x230`–`B+0x23c`), calls
`0x057c(address)`, clears bits `0..2` of the returned word, and calls
`0x2510(address)` (`B+0x240`–`B+0x268`). It repeats while signed
`index < r4` (`B+0x26c`–`B+0x274`). Because the comparison is after the body,
an input count of zero still processes one address. `clear_mapping_low_bits`
is a behavioral hypothesis; the unresolved callees prevent a stronger type
or purpose claim.

### SMC helper: file `0x294-0x300`, runtime `B+0x294`–`B+0x2ff`

This complete function is byte-identical to public BadUpdate Stage4 file
`0x0e0-0x14b`. It writes four 32-bit words from `r3` to SMC MMIO at
`0x80000200ea001080`, bracketed by writes to the status register at
`0x80000200ea001084` and `eieio` barriers. Exact public source may be reused
for this function.

## Patch records and copied code

The patch stream at file `0x380-0x74f` is `0x3d0` bytes, has SHA-256
`743e6bec6a72161b1590cfaadbbed7b9621ee765eaac31cec696b333f74e94cf`,
and parses without remainder into 27 records and the sentinel at `0x74c`.
Targets below list both the stored 32-bit address and the effective 64-bit
destination used by `B+0x134`–`B+0x13c`.

| Header | Raw target → effective VA | Size | Exact replacement or classification |
|---:|---|---:|---|
| `0x380` | `0x000012d8` → `0x12d8` | 4 | `7084000a` (`andi. r4,r4,0x000a`) |
| `0x38c` | `0x000010e0` → `0x10e0` | 4 | same |
| `0x398` | `0x000010fc` → `0x10fc` | 4 | same |
| `0x3a4` | `0x00015fd0` → `0x200015fd0` | 4 | `000076b8` |
| `0x3b0` | `0x00016004` → `0x200016004` | 4 | `00007698` |
| `0x3bc` | `0x0002a308` → `0x40002a308` | 4 | `li r3,0` |
| `0x3c8` | `0x0002aa80` → `0x40002aa80` | 4 | `nop` |
| `0x3d4` | `0x0002aa8c` → `0x40002aa8c` | 4 | `nop` |
| `0x3e0` | `0x0002ab7c` → `0x40002ab7c` | 4 | `b +0x18` |
| `0x3ec` | `0x000299dc` → `0x4000299dc` | 4 | `nop` |
| `0x3f8` | `0x00024d58` → `0x400024d58` | 8 | `li r3,1; blr` |
| `0x408` | `0x0002cae8` → `0x40002cae8` | 4 | `li r3,0` |
| `0x414` | `0x0002cc70` → `0x40002cc70` | 4 | `li r3,1` |
| `0x420` | `0x0002ccac` → `0x40002ccac` | 4 | `b +0x48` |
| `0x42c` | `0x0002cd94` → `0x40002cd94` | 4 | `b +0x48` |
| `0x438` | `0x00029aec` → `0x400029aec` | 28 | construct `0x600032048`; indirect jump |
| `0x45c` | `0x0002b778` → `0x40002b778` | 4 | `nop` |
| `0x468` | `0x00006c48` → `0x6c48` | 8 | `li r3,0; blr` |
| `0x478` | `0x00006bb0` → `0x6bb0` | 8 | `li r3,0; blr` |
| `0x488` | `0x00006d08` → `0x6d08` | 8 | `li r3,0; blr` |
| `0x498` | `0x00006c98` → `0x6c98` | 8 | `li r3,0; blr` |
| `0x4a8` | `0x00006d58` → `0x6d58` | 8 | `li r3,0; blr` |
| `0x4b8` | `0x00026c04` → `0x400026c04` | 4 | `li r3,1` |
| `0x4c4` | `0x0000a560` → `0xa560` | 8 | `li r3,1; blr` |
| `0x4d4` | `0x0000813c` → `0x813c` | 4 | `b +0x30` |
| `0x4e0` | `0x00007698` → `0x7698` | `0x3c` | status trampoline, file `0x4e8-0x524` |
| `0x524` | `0x00031fb8` → `0x600031fb8` | `0x220` | dispatcher, continuation, structured data; file `0x52c-0x74c` |

The record at file `0x4e0` installs two adjacent blocks:

- VA `0x7698`–`0x76b7` (file `0x4e8-0x507`) compares `r3` with
  `0x41414141`. On inequality it sets `r4=0x2540`; on equality it leaves the
  incoming `r4` unchanged. Both paths branch through `CTR=r4` at
  `0x76b0`–`0x76b4`.
- VA `0x76b8`–`0x76d3` (file `0x508-0x523`) constructs
  `0x600031fb8` with the same address-expansion formula and jumps there.

The `0x220`-byte record separates further:

- VA `0x600031fb8`–`0x600032047` (file `0x52c-0x5bb`) is a dispatcher. It
  requires `r3=0x72627472` (`"rbtr"`); otherwise it absolute-branches to
  `0x1cc8`. Operations `r4=1,2,3` return `1`; operation `4` copies `r7`
  32-bit words from `r6` to `r5`, performs `dcbst` and `icbi` per word followed
  by `sync/isync`, and jumps to the original `r5`; operation `5` remaps
  arguments and absolute-branches to `0xa880`; other operations return `2`.
- VA `0x600032048`–`0x6000320b7` (file `0x5bc-0x62b`) is a continuation that
  assumes live `r28`–`r30` state, calls absolute addresses `0xb3d8` and
  `0xb388`, and finally constructs and jumps to `0x400029b3c`. It has no
  standalone prologue, so a function name or independent signature would be
  speculative.
- VA `0x6000320b8`–`0x6000321c7` (file `0x62c-0x73b`) begins with big-endian
  words `0x20, 0x03, 0, 0`, followed by an opaque 256-byte field whose
  SHA-256 is `056cf89e3a2fae21e6f7ab4b1898e296dabc65cc41f25dde558ecb81a68c7a86`.
  The `0x20` qword count, exponent `3`, reserved qword, 256-byte modulus, and
  total `0x110`-byte size establish the `XECRYPT_RSAPUB_2048` layout. No source
  identity or owner has been demonstrated for this particular modulus.
- VA `0x6000321c8`–`0x6000321d7` (file `0x73c-0x74b`) is 16 zero bytes.

`reconstruction/stage4/rbtr_dispatcher.cpp` is the portable decision model for
the resident dispatcher. It preserves the magic-mismatch branch to `0x1cc8`,
operation 1-3 return value 1, unsupported return value 2, operation-4 source,
destination, low-32-bit word count and execution target, and operation-5
argument remap before the absolute branch to `0xa880`. It performs no live
copy, cache maintenance, or control transfer. Run its host test with
`tools/test_stage4_reconstruction.sh`.

`reconstruction/stage4/patch_stream.cpp` reconstructs the embedded patch-list
decoder as a lossless portable parser. It consumes big-endian target/size
headers and exact payload bytes, requires the `0xffffffff` sentinel to be the
final word, and reproduces the address expansion
`raw | ((raw & 0x00030000) << 17)` used by the Stage4 instructions.

`reconstruction/stage4/maintenance_helpers.cpp` reconstructs the two remaining
resident maintenance algorithms without calling privileged addresses. Its
pair-table parser emits the exact mapped-word value, derived halfword-table
byte offset, and flush address for every entry. Its range planner preserves
the low-word signed comparison and post-tested loop: counts zero and one, and
all values interpreted as negative, execute once; positive counts above one
execute exactly that many iterations from `0x80040000` in `0x10000` steps.

`reconstruction/stage4/entry_flow.cpp` joins those helpers into a portable
model of the top-level mode dispatch. It records mode 0's SMC prefix, clean-HV
source/destination/count, and patch-stream offset; mode 1's initial `0x20` bit
clear and pair-table call; mode 2's range plan; the no-op default; and the
common `0x41414141` return marker. It performs none of the privileged calls.

`reconstruction/stage4/runtime.cpp` is the source-level implementation that
composes the recovered entry, patch parser, pair-table helper, and range helper.
Privileged reads, writes, cache relocation, flushes, and SMC I/O are isolated
behind a `Platform` interface, allowing the full three-mode flow to execute in
host tests against a mock while remaining usable by a target-specific backend.

`reconstruction/stage4/copied_continuation.cpp` reconstructs the copied block
at `0x600032048` as an explicit two-call plan. It preserves the incoming stack,
TOC, `r28`-`r30`, and first-call-result decisions; identifies the embedded
structured-data address `0x6000320b8`, adjacent zero area `0x6000321c8`, and
final branch `0x400029b3c`; and leaves the unresolved absolute callees named by
address. The model performs no external calls or control transfers.

`reconstruction/stage4/resident_runtime.cpp` provides the executable C++ lift
of both the `rbtr` dispatcher and copied continuation. Memory copying, cache
maintenance, external calls, and tail branches are expressed through a
`ResidentPlatform` adapter and exercised with a host mock. Operation 4 retains
the full 64-bit CTR count and its original do/while semantics. The same header
also exposes the status trampoline's exact marker/target selection and the
copied dispatcher jump target.

`reconstruction/stage4/rsa_public_key.cpp` provides the corresponding strict,
big-endian `XECRYPT_RSAPUB_2048` decoder. It exposes the exact header and
256-byte modulus while deliberately making no ownership or trust claim.

## Entry and exit summary

The resident entry is a mode-controlled HV maintenance/patch routine. Mode
zero sends the SMC command, restores the clean 64-KiB HV block, and applies
the embedded records. Modes one and two perform separate address-table and
range operations through three unresolved HV callees. All modes, including
unsupported values, return `0x41414141` at `B+0x0cc`–`B+0x0d0`.

Names for `0x057c`, `0x05c0`, `0x2510`, the patched HV targets, and the
structured data at file `0x62c` remain unresolved. No claim is made that Peer
Pressure source was published.
