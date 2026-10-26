#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/boot_options.cpp" \
    "$root/reconstruction/bootxell/boot_options_test.cpp" \
    -o "$work/boot_options_test"
"$work/boot_options_test"
