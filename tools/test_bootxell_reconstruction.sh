#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

python3 "$root/tools/verify_bootxell_peer_source_coverage.py"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/boot_options.cpp" \
    "$root/reconstruction/bootxell/boot_options_test.cpp" \
    -o "$work/boot_options_test"
"$work/boot_options_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/xell_scan.cpp" \
    "$root/reconstruction/bootxell/xell_scan_test.cpp" \
    -o "$work/xell_scan_test"
"$work/xell_scan_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/boot_options.cpp" \
    "$root/reconstruction/bootxell/xell_scan.cpp" \
    "$root/reconstruction/bootxell/runtime.cpp" \
    "$root/reconstruction/bootxell/runtime_test.cpp" \
    -o "$work/runtime_test"
"$work/runtime_test"
