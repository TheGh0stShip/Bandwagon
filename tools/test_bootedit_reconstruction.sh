#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

python3 "$root/tools/verify_bootedit_function_ownership.py"
python3 "$root/tools/verify_bootedit_peer_source_coverage.py"
python3 "$root/tools/verify_bootedit_xbox_backend.py"
python3 "$root/tools/verify_bootedit_packed_resource.py"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/theme_editor.cpp" \
    "$root/reconstruction/boot_edit/theme_editor_test.cpp" \
    -o "$work/theme_editor_test"
"$work/theme_editor_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/xbox360_imgui_backend.cpp" \
    "$root/reconstruction/boot_edit/xbox360_imgui_backend_test.cpp" \
    -o "$work/xbox360_imgui_backend_test"
"$work/xbox360_imgui_backend_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/packed_resource.cpp" \
    "$root/reconstruction/boot_edit/packed_resource_test.cpp" \
    -o "$work/packed_resource_test"
"$work/packed_resource_test"

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

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/editor_fonts.cpp" \
    "$root/reconstruction/boot_edit/editor_fonts_test.cpp" \
    -o "$work/editor_fonts_test"
"$work/editor_fonts_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/settings_actions.cpp" \
    "$root/reconstruction/boot_edit/settings_actions_test.cpp" \
    -o "$work/settings_actions_test"
"$work/settings_actions_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/theme_editor.cpp" \
    "$root/reconstruction/boot_edit/theme_selector.cpp" \
    "$root/reconstruction/boot_edit/theme_selector_test.cpp" \
    -o "$work/theme_selector_test"
"$work/theme_selector_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/boot_edit/theme_name_task.cpp" \
    "$root/reconstruction/boot_edit/theme_name_task_test.cpp" \
    -o "$work/theme_name_task_test"
"$work/theme_name_task_test"

echo "BootEdit portable reconstruction tests passed"
