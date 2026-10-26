#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/smc/xbpp_handshake.cpp" \
    "$root/reconstruction/smc/xbpp_handshake_test.cpp" \
    -o "$work/xbpp_handshake_test"
"$work/xbpp_handshake_test"

echo "SMC portable reconstruction tests passed"
