/*
 * Exact reconstruction of Hdd/HddSecuritySectorPayload.bin.
 * Size: 24 bytes; SHA-256: 9e8933f7f194aa045d4b202a63a7552503ec836d217bb48a3bbfbb2f1f5e6bdd
 */

        .section .text
        .balign 4
hdd_security_sector_payload:
        .long 0x00000000
        .long 0x801ca000
        .long 0x00000000
        .long 0x801ca1c8
        .long 0x800d27a0                /* BadUpdate retail-17559 stack_pivot_entry */
        .ascii "XBPP"
