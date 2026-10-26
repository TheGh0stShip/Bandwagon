#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage3/resident_probe.cpp" \
    "$root/reconstruction/stage3/resident_probe_test.cpp" \
    -o "$work/resident_probe_test"
"$work/resident_probe_test"

echo "Stage3 portable reconstruction tests passed"
