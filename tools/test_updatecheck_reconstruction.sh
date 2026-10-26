#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
flags=(-std=c++17 -Wall -Wextra -Werror)

python3 "$root/tools/verify_updatecheck_platform_wrapper_matches.py"
python3 "$root/tools/verify_updatecheck_peer_source_coverage.py"
python3 "$root/tools/verify_updatecheck_function_ownership.py"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/release_version.cpp" \
    "$root/reconstruction/update_check/release_version_test.cpp" \
    -o "$work/release_version_test"
"$work/release_version_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/release_version.cpp" \
    "$root/reconstruction/update_check/latest_release.cpp" \
    "$root/reconstruction/update_check/latest_release_test.cpp" \
    -o "$work/latest_release_test"
"$work/latest_release_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/http_response.cpp" \
    "$root/reconstruction/update_check/http_response_test.cpp" \
    -o "$work/http_response_test"
"$work/http_response_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/certificate_store.cpp" \
    "$root/reconstruction/update_check/certificate_store_test.cpp" \
    -o "$work/certificate_store_test"
"$work/certificate_store_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/network_lifecycle.cpp" \
    "$root/reconstruction/update_check/network_lifecycle_test.cpp" \
    -o "$work/network_lifecycle_test"
"$work/network_lifecycle_test"

"$cxx" "${flags[@]}" \
    "$root/reconstruction/update_check/http_response.cpp" \
    "$root/reconstruction/update_check/https_request.cpp" \
    "$root/reconstruction/update_check/https_request_test.cpp" \
    -o "$work/https_request_test"
"$work/https_request_test"

echo "UpdateCheck portable reconstruction tests passed"
