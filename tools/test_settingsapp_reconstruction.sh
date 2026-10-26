#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/updater/archive_wrapper.cpp" \
    "$root/reconstruction/settings_app/offline_update.cpp" \
    "$root/reconstruction/settings_app/offline_update_test.cpp" \
    -o "$work/offline_update_test"
"$work/offline_update_test"

echo "SettingsApp portable reconstruction tests passed"
