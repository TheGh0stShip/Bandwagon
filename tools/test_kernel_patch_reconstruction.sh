#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_patches/patch_stream.cpp" \
    "$root/reconstruction/kernel_patches/patch_stream_test.cpp" \
    -o "$work/patch_stream_test"
"$work/patch_stream_test" "$root/PeerPressure/System/kernel_patches.bin"
