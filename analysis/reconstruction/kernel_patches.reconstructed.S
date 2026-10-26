/* Lossless source for PeerPressure/System/kernel_patches.bin.
 * Record format: target address, byte count, replacement bytes; big-endian.
 */
.section .text
.balign 4
kernel_patch_stream:
patch_00_return_zero:
    .long 0x8007b920
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_01_force_true_r3:
    .long 0x8007c4b8
    .long 4
    .long 0x38600001
patch_02_force_false_r3:
    .long 0x8007c5e8
    .long 4
    .long 0x38600000
patch_03_clear_r11:
    .long 0x8007c634
    .long 4
    .long 0x39600000
patch_04_clear_r11:
    .long 0x8007c684
    .long 4
    .long 0x39600000
patch_05_return_zero:
    .long 0x8007af08
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_06_compare_r11_with_255:
    .long 0x800998d0
    .long 4
    .long 0x2b0b00ff
patch_07_force_zero_and_nop_four:
    .long 0x800992b4
    .long 20
    .long 0x38600000
    .long 0x60000000
    .long 0x60000000
    .long 0x60000000
    .long 0x60000000
patch_08_return_one:
    .long 0x80109c90
    .long 8
    .long 0x38600001
    .long 0x4e800020
patch_09_return_zero:
    .long 0x8010a788
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_10_return_zero:
    .long 0x8010aa68
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_11_return_one:
    .long 0x8010af30
    .long 8
    .long 0x38600001
    .long 0x4e800020
patch_12_return_zero:
    .long 0x8010b138
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_13_return_zero:
    .long 0x8010b0e8
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_14_return_zero:
    .long 0x8010b278
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_15_return_zero:
    .long 0x8010b3f8
    .long 8
    .long 0x38600000
    .long 0x4e800020
patch_16_return_one_and_optionally_store_one:
    .long 0x8010bf20
    .long 20
    .long 0x28050000
    .long 0x38600001
    .long 0x41820008
    .long 0x90650000
    .long 0x4e800020
patch_17_return_one:
    .long 0x80111990
    .long 8
    .long 0x38600001
    .long 0x4e800020
patch_18_skip_forward_0x18:
    .long 0x800e1754
    .long 4
    .long 0x48000018
patch_19_load_r26_and_nop_two:
    .long 0x800e17cc
    .long 12
    .long 0x835c0098
    .long 0x60000000
    .long 0x60000000
patch_20_return_one:
    .long 0x8015d9d8
    .long 8
    .long 0x38600001
    .long 0x4e800020
patch_21_skip_forward_0x68:
    .long 0x8009d660
    .long 4
    .long 0x48000068
patch_22_return_one:
    .long 0x800d8748
    .long 8
    .long 0x38600001
    .long 0x4e800020
    .long 0xffffffff
kernel_patch_stream_end:
