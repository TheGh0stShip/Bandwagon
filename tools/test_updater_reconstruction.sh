#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

python3 "$root/tools/verify_updater_function_ownership.py"
python3 "$root/tools/verify_updater_peer_source_coverage.py"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/bootstrap_record.cpp" \
    "$root/reconstruction/updater/bootstrap_record_test.cpp" \
    -o "$work/bootstrap_record_test"
"$work/bootstrap_record_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/softmod_manifest.cpp" \
    "$root/reconstruction/updater/softmod_manifest_test.cpp" \
    -o "$work/softmod_manifest_test"
"$work/softmod_manifest_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/archive_wrapper.cpp" \
    "$root/reconstruction/updater/archive_wrapper_test.cpp" \
    -o "$work/archive_wrapper_test"
"$work/archive_wrapper_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/phase_two_list.cpp" \
    "$root/reconstruction/updater/phase_two_list_test.cpp" \
    -o "$work/phase_two_list_test"
"$work/phase_two_list_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/parent_directories.cpp" \
    "$root/reconstruction/updater/parent_directories_test.cpp" \
    -o "$work/parent_directories_test"
"$work/parent_directories_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/archive_wrapper.cpp" \
    "$root/reconstruction/updater/bootstrap_record.cpp" \
    "$root/reconstruction/updater/parent_directories.cpp" \
    "$root/reconstruction/updater/phase_two_list.cpp" \
    "$root/reconstruction/updater/softmod_manifest.cpp" \
    "$root/reconstruction/updater/updater_runtime.cpp" \
    "$root/reconstruction/updater/updater_runtime_test.cpp" \
    -o "$work/updater_runtime_test"
"$work/updater_runtime_test"

echo "Updater portable reconstruction tests passed"
