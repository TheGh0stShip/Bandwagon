# FatFs R0.16 provenance

This directory is an unmodified copy of the `source/` directory and
`LICENSE.txt` from ChaN's official FatFs R0.16 archive dated July 22, 2025.

- archive: `https://elm-chan.org/fsw/ff/arc/ff16.zip`
- archive SHA-256: `99f7dc1f7e095356e4a9e3dbe29959090d8b948afe2bbc5441e52fdf4b85449e`
- imported source: base R0.16, without patch 1 or patch 2
- `source/ff.c` SHA-256: `dbeae3d7d22dbc070983e31af897fa5e43d29c726a0a240fe9589ffe2ec0ef1e`
- `LICENSE.txt` SHA-256: `2576183a3e1c2d2f4b3628a7ef93c2f937879698abdd6d554dd718e79062e079`

Bandwagon uses this copy to attribute the generic FAT/exFAT core in
`KernelExpansion.bin`. Xbox disk I/O glue, integration changes, and
Peer-specific callers remain separate reconstruction targets.

This base source is retained for provenance, not recommended as the replacement
dependency. Official R0.16 patches published after the release include
correctness and security fixes: `https://elm-chan.org/fsw/ff/patches.html`.
