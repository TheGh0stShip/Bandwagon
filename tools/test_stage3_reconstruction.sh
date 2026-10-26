#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

python3 "$root/tools/generate_stage3_probe_data.py" --check

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage3/resident_probe.cpp" \
    "$root/reconstruction/stage3/resident_probe_test.cpp" \
    -o "$work/resident_probe_test"
"$work/resident_probe_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage3/entry_flow.cpp" \
    "$root/reconstruction/stage3/entry_flow_test.cpp" \
    -o "$work/entry_flow_test"
"$work/entry_flow_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage3/resident_probe.cpp" \
    "$root/reconstruction/stage3/entry_flow.cpp" \
    "$root/reconstruction/stage3/runtime.cpp" \
    "$root/reconstruction/stage3/runtime_test.cpp" \
    -o "$work/runtime_test"
"$work/runtime_test"

echo "Stage3 portable reconstruction tests passed"
