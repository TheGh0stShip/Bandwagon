# Peer-modified Stage3 probe at file `0x0f8-0x1e8`

## Status

This function is absent at the corresponding location in public BadUpdate
v1.3 and is treated as Peer-specific. The analyzed file range
`0x0f8-0x1e8` has SHA-256
`bd587120cf45f4a454317fdfab2b374d1ce3b88c541fcbce266c4125d1f68dd2`;
executable instructions end at `0x1e3` and `0x1e4-0x1e7` is alignment zero.
With
the established Stage3 mapping `file offset 0 -> 0x98030000`, its runtime range
is `0x980300f8-0x980301e3`.

The function is recovered at high confidence as `ProbeResidentPeerDispatcher`.
It tests two resident execution protocols installed or supported by Peer:
the Stage4-patched `HvxPostOutput` trampoline and the syscall-zero `rbtr`
dispatcher.

Buildable assembly for the recovered regions is in
[`reconstruction/PeerPressureExploit-3rdStage.reconstructed.S`](reconstruction/PeerPressureExploit-3rdStage.reconstructed.S).
The function, embedded probe, syscall stubs, save/restore fragments, and direct
import stubs rebuild byte-for-byte. The complete Stage3 file now also rebuilds
byte-for-byte; generated regions preserve exact instructions while semantic
naming and higher-level recovery continue.

## Instruction-level proof

| File offsets / VAs | Verified behavior |
|---|---|
| `0x104-0x120` / `0x98030104-0x98030120` | Set `r3=1`, `r4=0x1000`, `r5=0x204`, `r6=0`, `r7=-1` (encoded immediate `0xffff`), `r8=0x80`; call stub `0x980312a0`, whose literal target is kernel `0x800801d8` (`MmAllocatePhysicalMemoryEx` in the pinned 17559 export map). |
| `0x124-0x128` / `0x98030124-0x98030128` | Save the result in `r29`; on null, branch to the false return path at `0x980301d8`. |
| `0x12c-0x138` / `0x9803012c-0x98030138` | Call `memset(allocation,0,0x1000)` through stub `0x98031270` → literal `0x8010d110`. |
| `0x13c-0x14c` / `0x9803013c-0x9803014c` | Call `memcpy(allocation,0x980315a0,0x38)` through stub `0x980312b0` → literal `0x8010cc40`. |
| `0x150` / `0x98030150` | Execute `sync`. |
| `0x154-0x16c` / `0x98030154-0x9803016c` | Call stub `0x98031250` → literal `0x80080048` (`MmGetPhysicalAddress`), zero-extend its low 32 bits, and OR in `0x8000000000000000`; save the result as `r31`. |
| `0x170-0x18c` / `0x98030170-0x9803018c` | Set `r8=r31+0x100`, `r4=r31`, `r5=r6=r7=0`, `r3=0x41414141`; call `0x98030680`, which is exactly `li r0,0x0d; sc; blr`. |
| `0x190-0x19c` / `0x98030190-0x9803019c` | Compare the syscall result with `0x41410001`; equality branches to set the Boolean true. |
| `0x1a0-0x1bc` / `0x980301a0-0x980301bc` | Fallback: set `r3=0x72627472` (`"rbtr"`), `r4=4`, `r5=r6=r31`, `r7=1`, `r8=r31+0x100`; call `0x98030690`, exactly `li r0,0; sc; blr`. |
| `0x1c0-0x1c8` / `0x980301c0-0x980301c8` | Set the Boolean true only if this second result equals `0x41410001`. |
| `0x1cc-0x1d4` / `0x980301cc-0x980301d4` | Call stub `0x98031260` → literal `0x800807b8` (`MmFreePhysicalMemory`) with `r3=1`, `r4=allocation`. |
| `0x1d8-0x1e0` / `0x980301d8-0x980301e0` | Return `r27`, initialized to zero at `0x9803011c` and set to one only at `0x980301c8`; restore through the shared epilogue at `0x98031220`. |

The public symbol names above are used only where the Peer stubs contain the
same literal 17559 addresses as the pinned public kernel-export map. The new
function itself is not reconstructed from public source.

## The copied `0x38`-byte probe

The source bytes are Stage3 file `0x15a0-0x15d7`, runtime
`0x980315a0-0x980315d7`, SHA-256
`4e68f78012145a4e9ef1d444d564b691a0d98fa8db4bfc01417ca08a07276c8c`.
Decoded statically, they:

1. construct source address `0x8000020000020000`
   (`0x980315a0-0x980315ac`);
2. set `CTR=12` (`0x980315b0-0x980315b4`);
3. loop 12 times, loading one 64-bit value, storing it through input `r8`,
   adding `0x200` to the source and `8` to the destination
   (`0x980315b8-0x980315c8`); and
4. return `0x41410001` (`0x980315cc-0x980315d4`).

Thus the probe copies `12 * 8 = 0x60` bytes from strided HV addresses into
the allocation at offset `0x100`, then returns the marker. The surrounding
function never consumes those copied bytes before freeing the allocation;
the observed Boolean is therefore evidence that the probe executed and
returned, not evidence that the sampled data was validated.

## Link to the Stage4 dispatcher

Stage4 file `0x52c-0x5bb`, copied to VA
`0x600031fb8-0x600032047`, independently proves the fallback calling
convention. It checks `r3` for `"rbtr"`; operation `r4=4` copies `r7` 32-bit
words from `r6` to `r5`, flushes the written instruction range, and jumps to
the original `r5`. Stage3 supplies `r5=r6=probe physical alias` and `r7=1`, so
the one-word self-copy is inert apart from cache maintenance, after which the
dispatcher executes the probe. Stage3 supplies `r8=alias+0x100`, exactly the
probe's output destination.

The first path is no longer unresolved. Syscall `0x0d` is `HvxPostOutput` in
the pinned public BadUpdate source. Peer's Stage4 record at file `0x4e8`
installs an exact trampoline at the syscall implementation address `0x7698`:
it compares `r3` with `0x41414141`, preserves the incoming `r4` on equality,
and branches through `CTR=r4`. Stage3 supplies the probe alias in `r4`, so this
path directly executes the copied probe. On ordinary calls the trampoline
replaces `r4` with the original target `0x2540` before branching.

The second path invokes syscall zero with the independently recovered `rbtr`
operation-4 convention. Trying both protocols explains compatibility with two
resident Peer dispatch mechanisms. Either must execute the probe and return
`0x41410001`; the function therefore has the final source-level name
`ProbeResidentPeerDispatcher`.
