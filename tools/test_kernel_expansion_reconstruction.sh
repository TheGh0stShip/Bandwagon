#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

python3 "$root/tools/verify_kernel_expansion_source_coverage.py"
"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/bootstrap_mapping.cpp" \
    "$root/reconstruction/kernel_expansion/bootstrap_mapping_test.cpp" \
    -o "$work/bootstrap_mapping_test"
"$work/bootstrap_mapping_test"

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
    "$root/reconstruction/kernel_expansion/volume_capacity.cpp" \
    "$root/reconstruction/kernel_expansion/volume_capacity_test.cpp" \
    -o "$work/volume_capacity_test"
"$work/volume_capacity_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config.cpp" \
    "$root/reconstruction/kernel_expansion/config_test.cpp" \
    -o "$work/config_test"
"$work/config_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config.cpp" \
    "$root/reconstruction/kernel_expansion/runtime.cpp" \
    "$root/reconstruction/kernel_expansion/config_loader.cpp" \
    "$root/reconstruction/kernel_expansion/config_loader_test.cpp" \
    -o "$work/config_loader_test"
"$work/config_loader_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config_persistence.cpp" \
    "$root/reconstruction/kernel_expansion/config_persistence_test.cpp" \
    -o "$work/config_persistence_test"
"$work/config_persistence_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/decimal.cpp" \
    "$root/reconstruction/kernel_expansion/decimal_test.cpp" \
    -o "$work/decimal_test"
"$work/decimal_test"

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
    "$root/reconstruction/kernel_expansion/drive_prefix.cpp" \
    "$root/reconstruction/kernel_expansion/drive_binding.cpp" \
    "$root/reconstruction/kernel_expansion/drive_binding_test.cpp" \
    -o "$work/drive_binding_test"
"$work/drive_binding_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/drive_registry.cpp" \
    "$root/reconstruction/kernel_expansion/drive_registry_test.cpp" \
    -o "$work/drive_registry_test"
"$work/drive_registry_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/file_disposition.cpp" \
    "$root/reconstruction/kernel_expansion/file_disposition_test.cpp" \
    -o "$work/file_disposition_test"
"$work/file_disposition_test"

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
    "$root/reconstruction/kernel_expansion/oem437.cpp" \
    "$root/reconstruction/kernel_expansion/oem437_test.cpp" \
    -o "$work/oem437_test"
"$work/oem437_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/instruction_patch.cpp" \
    "$root/reconstruction/kernel_expansion/instruction_patch_test.cpp" \
    -o "$work/instruction_patch_test"
"$work/instruction_patch_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/version.cpp" \
    "$root/reconstruction/kernel_expansion/version_test.cpp" \
    -o "$work/version_test"
"$work/version_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/runtime.cpp" \
    "$root/reconstruction/kernel_expansion/phase_two_restore.cpp" \
    "$root/reconstruction/kernel_expansion/phase_two_restore_test.cpp" \
    -o "$work/phase_two_restore_test"
"$work/phase_two_restore_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/signin_state.cpp" \
    "$root/reconstruction/kernel_expansion/signin_state_test.cpp" \
    -o "$work/signin_state_test"
"$work/signin_state_test"
