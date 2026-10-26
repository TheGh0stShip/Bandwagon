#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/hdd_security/trigger_payload.cpp" \
    "$root/reconstruction/hdd_security/trigger_payload_test.cpp" \
    -o "$work/trigger_payload_test"
"$work/trigger_payload_test" "$root/Hdd/HddSecuritySectorPayload.bin"
