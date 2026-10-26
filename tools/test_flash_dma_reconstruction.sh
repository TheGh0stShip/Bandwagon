#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/flash_dma/payload_builder.cpp" \
    "$root/reconstruction/flash_dma/payload_builder_test.cpp" \
    -o "$work/payload_builder_test"
"$work/payload_builder_test" "$root/Flash/FlashDmaPayload.bin"
