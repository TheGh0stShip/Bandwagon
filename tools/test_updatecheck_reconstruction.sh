#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/release_version.cpp" \
    "$root/reconstruction/update_check/release_version_test.cpp" \
    -o "$work/release_version_test"
"$work/release_version_test"

echo "UpdateCheck portable reconstruction tests passed"
