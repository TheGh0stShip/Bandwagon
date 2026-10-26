#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/rbtr_dispatcher.cpp" \
    "$root/reconstruction/stage4/rbtr_dispatcher_test.cpp" \
    -o "$work/rbtr_dispatcher_test"
"$work/rbtr_dispatcher_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/patch_stream.cpp" \
    "$root/reconstruction/stage4/patch_stream_test.cpp" \
    -o "$work/patch_stream_test"
"$work/patch_stream_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/maintenance_helpers.cpp" \
    "$root/reconstruction/stage4/maintenance_helpers_test.cpp" \
    -o "$work/maintenance_helpers_test"
"$work/maintenance_helpers_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/maintenance_helpers.cpp" \
    "$root/reconstruction/stage4/entry_flow.cpp" \
    "$root/reconstruction/stage4/entry_flow_test.cpp" \
    -o "$work/entry_flow_test"
"$work/entry_flow_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/copied_continuation.cpp" \
    "$root/reconstruction/stage4/copied_continuation_test.cpp" \
    -o "$work/copied_continuation_test"
"$work/copied_continuation_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/rsa_public_key.cpp" \
    "$root/reconstruction/stage4/rsa_public_key_test.cpp" \
    -o "$work/rsa_public_key_test"
"$work/rsa_public_key_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/stage4/patch_stream.cpp" \
    "$root/reconstruction/stage4/maintenance_helpers.cpp" \
    "$root/reconstruction/stage4/entry_flow.cpp" \
    "$root/reconstruction/stage4/runtime.cpp" \
    "$root/reconstruction/stage4/runtime_test.cpp" \
    -o "$work/runtime_test"
"$work/runtime_test"

echo "Stage4 portable reconstruction tests passed"
