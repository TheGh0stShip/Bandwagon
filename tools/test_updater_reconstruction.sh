#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

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

echo "Updater portable reconstruction tests passed"
