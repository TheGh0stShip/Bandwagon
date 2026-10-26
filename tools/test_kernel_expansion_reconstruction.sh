#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

python3 "$root/tools/verify_kernel_expansion_source_coverage.py"
"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/runtime.cpp" \
    "$root/reconstruction/kernel_expansion/runtime_test.cpp" \
    -o "$work/runtime_test"
"$work/runtime_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/xex_hooks.cpp" \
    "$root/reconstruction/kernel_expansion/xex_hooks_test.cpp" \
    -o "$work/xex_hooks_test"
"$work/xex_hooks_test"
