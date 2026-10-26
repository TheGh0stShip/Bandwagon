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

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/storage.cpp" \
    "$root/reconstruction/kernel_expansion/storage_test.cpp" \
    -o "$work/storage_test"
"$work/storage_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config.cpp" \
    "$root/reconstruction/kernel_expansion/config_test.cpp" \
    -o "$work/config_test"
"$work/config_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/update_check_launch.cpp" \
    "$root/reconstruction/kernel_expansion/update_check_launch_test.cpp" \
    -o "$work/update_check_launch_test"
"$work/update_check_launch_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/package_callbacks.cpp" \
    "$root/reconstruction/kernel_expansion/package_callbacks_test.cpp" \
    -o "$work/package_callbacks_test"
"$work/package_callbacks_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/drive_prefix.cpp" \
    "$root/reconstruction/kernel_expansion/drive_prefix_test.cpp" \
    -o "$work/drive_prefix_test"
"$work/drive_prefix_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/path_fields.cpp" \
    "$root/reconstruction/kernel_expansion/path_fields_test.cpp" \
    -o "$work/path_fields_test"
"$work/path_fields_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/namespace_links.cpp" \
    "$root/reconstruction/kernel_expansion/namespace_links_test.cpp" \
    -o "$work/namespace_links_test"
"$work/namespace_links_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/instruction_patch.cpp" \
    "$root/reconstruction/kernel_expansion/instruction_patch_test.cpp" \
    -o "$work/instruction_patch_test"
"$work/instruction_patch_test"
