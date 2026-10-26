#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/theme_editor.cpp" \
    "$root/reconstruction/boot_edit/theme_editor_test.cpp" \
    -o "$work/theme_editor_test"
"$work/theme_editor_test"

echo "BootEdit portable reconstruction tests passed"
