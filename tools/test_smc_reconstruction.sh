#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

python3 "$root/tools/verify_trinity_smc_source_coverage.py"
python3 "$root/tools/verify_legacy_smc_source_coverage.py"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/smc/xbpp_handshake.cpp" \
    "$root/reconstruction/smc/xbpp_handshake_test.cpp" \
    -o "$work/xbpp_handshake_test"
"$work/xbpp_handshake_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/smc/xbpp_handshake.cpp" \
    "$root/reconstruction/smc/trinity_runtime.cpp" \
    "$root/reconstruction/smc/trinity_runtime_test.cpp" \
    -o "$work/trinity_runtime_test"
"$work/trinity_runtime_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/smc/xbpp_handshake.cpp" \
    "$root/reconstruction/smc/legacy_runtime.cpp" \
    "$root/reconstruction/smc/legacy_runtime_test.cpp" \
    -o "$work/legacy_runtime_test"
"$work/legacy_runtime_test"

echo "SMC portable reconstruction tests passed"
