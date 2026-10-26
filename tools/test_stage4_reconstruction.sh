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

echo "Stage4 portable reconstruction tests passed"
