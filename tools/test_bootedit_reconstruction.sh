#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

python3 "$root/tools/verify_bootedit_function_ownership.py"
python3 "$root/tools/verify_bootedit_peer_source_coverage.py"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/theme_editor.cpp" \
    "$root/reconstruction/boot_edit/theme_editor_test.cpp" \
    -o "$work/theme_editor_test"
"$work/theme_editor_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/theme_editor.cpp" \
    "$root/reconstruction/boot_edit/theme_manager.cpp" \
    "$root/reconstruction/boot_edit/theme_manager_test.cpp" \
    -o "$work/theme_manager_test"
"$work/theme_manager_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/bootanim_bootstrap.cpp" \
    "$root/reconstruction/boot_edit/bootanim_bootstrap_test.cpp" \
    -o "$work/bootanim_bootstrap_test"
"$work/bootanim_bootstrap_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/modal_state.cpp" \
    "$root/reconstruction/boot_edit/modal_state_test.cpp" \
    -o "$work/modal_state_test"
"$work/modal_state_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/discard_confirmation.cpp" \
    "$root/reconstruction/boot_edit/discard_confirmation_test.cpp" \
    -o "$work/discard_confirmation_test"
"$work/discard_confirmation_test"

echo "BootEdit portable reconstruction tests passed"
