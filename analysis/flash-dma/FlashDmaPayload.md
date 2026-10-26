# Flash DMA payload reconstruction

`Flash/FlashDmaPayload.bin` is a 16,384-byte sparse, big-endian data image
loaded at `0x801ca000`. It is not a conventional PowerPC text image: no standard
function prologues are present, and its active words form retail-17559 ROP call
frames separated by zero-filled register-save slots.

The lossless reconstruction is
`analysis/reconstruction/FlashDmaPayload.reconstructed.S`; regenerate it with
`python tools/reconstruct_flash_dma_payload.py` and verify it with
`python tools/rebuild_peer_reconstructions.py`. The rebuilt SHA-256 is
`d3c73c043eb6e679b9b598caaf5b39c108bedaabbc48fe5ac2211ac7d50cac2d`.

The maintainable source reconstruction is
`reconstruction/flash_dma/payload_builder.{hpp,cpp}`. It represents the retail
kernel exports and gadgets as a typed symbol set, divides the ROP image into
named construction phases, and exposes a semantic operation inventory. It does
not embed the original as a byte array. The host test rebuilds the complete
16 KiB image and compares every byte with the immutable artifact; run it with
`tools/test_flash_dma_reconstruction.sh`.

## Recovered layout

| File range | Runtime range | Meaning |
| --- | --- | --- |
| `0x0000-0x01ef` | `0x801ca000-0x801ca1ef` | zero-filled prefix |
| `0x01f0-0x01fb` | `0x801ca1f0-0x801ca1fb` | initial stack-switch record |
| `0x0250-0x0dfb` | `0x801ca250-0x801cadfb` | sparse ROP chain and return stack record |
| `0x0e00-0x0e6f` | `0x801cae00-0x801cae6f` | SMC/DMA control pointers and mutable words |
| `0x1400-0x1417` | `0x801cb400-0x801cb417` | six-word DMA descriptor/configuration block |
| `0x1600-0x17ff` | `0x801cb600-0x801cb7ff` | 512-byte `0x41` scratch sector |
| remaining bytes | corresponding runtime addresses | zero fill |

The chain reuses the same `CALL_FUNC` ABI and filler constants as the pinned
Xbox360BadUpdate source. Resolved operations include stack switching,
`MiAllocateMappedMemory`, captured-result stores, computed pointer updates,
`IoSynchronousFsdRequest`, three SMC-message records, and a processor stall.
The exact register-frame words are preserved even where the private structure
behind a kernel pointer is not publicly named. In particular, `0x801c7f80` and
`0x801a614c` are retained as unresolved retail-kernel globals rather than given
speculative names.

The C++ operation model records the image offset and relevant operands for
each of those behaviors. The six-word DMA descriptor and 512-byte `0x41`
scratch sector are likewise constructed as typed regions. Zero-filled save
slots and unused ranges arise from value-initializing the output image, making
the sparse layout explicit without concealing any bytes.

## Provenance boundary

The gadget and export names are independently corroborated by
`third_party/Xbox360BadUpdate/Common/KernelConfig_Retail_17559.asm` and
`KernelExports_Retail_17559.asm`. The overall chain and its DMA descriptor
values are Peer Pressure material; no matching source file exists in the pinned
public BadUpdate tree.

## Recovery status

Source-level recovery is complete for this payload: the readable builder owns
all non-zero words, all zero-filled gaps, and the full output extent, while its
semantic model covers every identified action. The assembly file remains an
independent binary-exact evidence oracle rather than the implementation layer.
