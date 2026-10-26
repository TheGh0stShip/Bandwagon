#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/aurora_fixer/absolute_branch.cpp" \
    "$root/reconstruction/aurora_fixer/absolute_branch_test.cpp" \
    -o "$work/absolute_branch_test"
"$work/absolute_branch_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/aurora_fixer/absolute_branch.cpp" \
    "$root/reconstruction/aurora_fixer/import_stub.cpp" \
    "$root/reconstruction/aurora_fixer/import_stub_test.cpp" \
    -o "$work/import_stub_test"
"$work/import_stub_test"

echo "AuroraFixer portable reconstruction tests passed"
