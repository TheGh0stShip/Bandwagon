# Peer Stage3 entry point

## Status

The complete Peer entry point at file `0x0000-0x00f3` (runtime
`0x98030000-0x980300f3`) and its four-byte alignment word at `0x00f4` rebuild
byte-for-byte from
[`PeerPressureExploit-3rdStage.reconstructed.S`](reconstruction/PeerPressureExploit-3rdStage.reconstructed.S).
The verified `0x00f8`-byte range has SHA-256
`3b1ee997820824ad743467c16c55ae607a7180ff138c83784563ee140b042641`.

`GameSaveExploitMain` is retained as the symbol name because the pinned public
BadUpdate source declares that exact undecorated entry point. The control flow
below is independently verified against the Peer binary.

## Boot decision

The entry point first calls the Peer-only probe at `0x980300f8`:

- A return value of `1` branches directly to finalization. This is the path
  taken when either supported resident dispatcher successfully ran the copied
  probe.
- Any other value calls the modified BadUpdate exploit routine at
  `0x980301e8`. It then reads
  `PAYLOAD:\\BadUpdateExploit-4thStage.bin`, constructs its high-bit physical
  alias, and invokes the syscall-`0x0d` wrapper at `0x98030670` with operation
  zero.
- Stage4 success is the marker `0x41414141`. A read failure and a Stage4
  execution failure use fatal codes `0x12414` and `0x12415`, respectively.
- After successful Stage4 execution, the allocated Stage4 buffer is freed.

This establishes why Peer added the probe: it distinguishes an already active
Peer boot environment from a boot that still needs the inherited BadUpdate
race and Stage4 installation path. The probe is a capability test, not a
validation of the sampled hypervisor data.

## Common finalization

Both paths converge at file `0x00a8`. The entry point:

1. zeroes a 16-byte SMC message and sets command byte zero to `0x99`;
2. sends the message through the literal import stub at `0x98031280`; and
3. launches `PAYLOAD:\\default.xex` through the literal import stub at
   `0x98031290`.

The imported absolute targets are preserved exactly in the reconstruction.
Names are assigned only where the target agrees with the pinned public 17559
export map or the public BadUpdate source-level call site.

## Difference from public BadUpdate v1.3

Public BadUpdate always prints a start message and runs its exploit before
loading Stage4. Peer removes those messages, adds the resident-dispatcher
probe, and skips both the race and Stage4 execution when the probe succeeds.
The public entry point is `0x108` bytes; Peer replaces it with a `0x0f8`-byte
entry point followed by the new `0x0f0`-byte probe function.
