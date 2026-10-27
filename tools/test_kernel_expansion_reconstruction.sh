#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

python3 "$root/tools/verify_kernel_expansion_source_coverage.py"
python3 "$root/tools/verify_kernel_expansion_source_attribution.py"
python3 "$root/tools/verify_kernel_expansion_fatfs_map.py"
python3 "$root/tools/verify_kernel_expansion_function_ownership.py"
python3 "$root/tools/verify_kernel_expansion_config_schema.py"
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
    "$root/reconstruction/kernel_expansion/boot_animation_theme.cpp" \
    "$root/reconstruction/kernel_expansion/boot_animation_theme_test.cpp" \
    -o "$work/boot_animation_theme_test"
"$work/boot_animation_theme_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/boot_partition_handoff.cpp" \
    "$root/reconstruction/kernel_expansion/boot_partition_handoff_test.cpp" \
    -o "$work/boot_partition_handoff_test"
"$work/boot_partition_handoff_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config_hash.cpp" \
    "$root/reconstruction/kernel_expansion/config_hash_test.cpp" \
    -o "$work/config_hash_test"
"$work/config_hash_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config_hash.cpp" \
    "$root/reconstruction/kernel_expansion/config_schema.cpp" \
    "$root/reconstruction/kernel_expansion/config_schema_test.cpp" \
    -o "$work/config_schema_test"
"$work/config_schema_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config_hash.cpp" \
    "$root/reconstruction/kernel_expansion/decimal.cpp" \
    "$root/reconstruction/kernel_expansion/config_value.cpp" \
    "$root/reconstruction/kernel_expansion/config_value_test.cpp" \
    -o "$work/config_value_test"
"$work/config_value_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/runtime.cpp" \
    "$root/reconstruction/kernel_expansion/config_writer.cpp" \
    "$root/reconstruction/kernel_expansion/config_writer_test.cpp" \
    -o "$work/config_writer_test"
"$work/config_writer_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/runtime.cpp" \
    "$root/reconstruction/kernel_expansion/config_writer.cpp" \
    "$root/reconstruction/kernel_expansion/config_value.cpp" \
    "$root/reconstruction/kernel_expansion/config_hash.cpp" \
    "$root/reconstruction/kernel_expansion/decimal.cpp" \
    "$root/reconstruction/kernel_expansion/config_schema.cpp" \
    "$root/reconstruction/kernel_expansion/config_reader.cpp" \
    "$root/reconstruction/kernel_expansion/config_reader_test.cpp" \
    -o "$work/config_reader_test"
"$work/config_reader_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config.cpp" \
    "$root/reconstruction/kernel_expansion/runtime.cpp" \
    "$root/reconstruction/kernel_expansion/config_schema.cpp" \
    "$root/reconstruction/kernel_expansion/config_loader.cpp" \
    "$root/reconstruction/kernel_expansion/config_loader_test.cpp" \
    -o "$work/config_loader_test"
"$work/config_loader_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config_schema.cpp" \
    "$root/reconstruction/kernel_expansion/config_persistence.cpp" \
    "$root/reconstruction/kernel_expansion/config_persistence_test.cpp" \
    -o "$work/config_persistence_test"
"$work/config_persistence_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/config_hash.cpp" \
    "$root/reconstruction/kernel_expansion/config_schema.cpp" \
    "$root/reconstruction/kernel_expansion/config_persistence.cpp" \
    "$root/reconstruction/kernel_expansion/config_access.cpp" \
    "$root/reconstruction/kernel_expansion/config_access_test.cpp" \
    -o "$work/config_access_test"
"$work/config_access_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/decimal.cpp" \
    "$root/reconstruction/kernel_expansion/decimal_test.cpp" \
    -o "$work/decimal_test"
"$work/decimal_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/disk_ioctl.cpp" \
    "$root/reconstruction/kernel_expansion/disk_ioctl_test.cpp" \
    -o "$work/disk_ioctl_test"
"$work/disk_ioctl_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/update_check_launch.cpp" \
    "$root/reconstruction/kernel_expansion/update_check_launch_test.cpp" \
    -o "$work/update_check_launch_test"
"$work/update_check_launch_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/unsupported_operation.cpp" \
    "$root/reconstruction/kernel_expansion/unsupported_operation_test.cpp" \
    -o "$work/unsupported_operation_test"
"$work/unsupported_operation_test"

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
    "$root/reconstruction/kernel_expansion/filesystem_close.cpp" \
    "$root/reconstruction/kernel_expansion/filesystem_close_test.cpp" \
    -o "$work/filesystem_close_test"
"$work/filesystem_close_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/filesystem_completion.cpp" \
    "$root/reconstruction/kernel_expansion/filesystem_completion_test.cpp" \
    -o "$work/filesystem_completion_test"
"$work/filesystem_completion_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/filesystem_name_node.cpp" \
    "$root/reconstruction/kernel_expansion/filesystem_name_node_test.cpp" \
    -o "$work/filesystem_name_node_test"
"$work/filesystem_name_node_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/file_disposition.cpp" \
    "$root/reconstruction/kernel_expansion/file_disposition_test.cpp" \
    -o "$work/file_disposition_test"
"$work/file_disposition_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/fat_timestamp.cpp" \
    "$root/reconstruction/kernel_expansion/fat_timestamp_test.cpp" \
    -o "$work/fat_timestamp_test"
"$work/fat_timestamp_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/filesystem_pool.cpp" \
    "$root/reconstruction/kernel_expansion/filesystem_pool_test.cpp" \
    -o "$work/filesystem_pool_test"
"$work/filesystem_pool_test"

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
    "$root/reconstruction/kernel_expansion/kernel_patch_loader.cpp" \
    "$root/reconstruction/kernel_expansion/instruction_patch.cpp" \
    "$root/reconstruction/kernel_expansion/xex_hooks.cpp" \
    "$root/reconstruction/kernel_expansion/sfcx_write_protection.cpp" \
    "$root/reconstruction/kernel_expansion/kernel_patch_loader_test.cpp" \
    -o "$work/kernel_patch_loader_test"
"$work/kernel_patch_loader_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/sata_udma.cpp" \
    "$root/reconstruction/kernel_expansion/sata_udma_test.cpp" \
    -o "$work/sata_udma_test"
"$work/sata_udma_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/xconfig_hooks.cpp" \
    "$root/reconstruction/kernel_expansion/xconfig_hooks_test.cpp" \
    -o "$work/xconfig_hooks_test"
"$work/xconfig_hooks_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/xex_load_hooks.cpp" \
    "$root/reconstruction/kernel_expansion/xex_load_hooks_test.cpp" \
    -o "$work/xex_load_hooks_test"
"$work/xex_load_hooks_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/xui_scene_override.cpp" \
    "$root/reconstruction/kernel_expansion/xui_scene_override_test.cpp" \
    -o "$work/xui_scene_override_test"
"$work/xui_scene_override_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/flash_versions.cpp" \
    "$root/reconstruction/kernel_expansion/flash_versions_test.cpp" \
    -o "$work/flash_versions_test"
"$work/flash_versions_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/nand_write_protection.cpp" \
    "$root/reconstruction/kernel_expansion/nand_write_protection_test.cpp" \
    -o "$work/nand_write_protection_test"
"$work/nand_write_protection_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/kernel_expansion/sfcx_write_protection.cpp" \
    "$root/reconstruction/kernel_expansion/sfcx_write_protection_test.cpp" \
    -o "$work/sfcx_write_protection_test"
"$work/sfcx_write_protection_test"

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
