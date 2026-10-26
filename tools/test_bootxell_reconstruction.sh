#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
cxx="${CXX:-g++}"
cc="${CC:-gcc}"

python3 "$root/tools/verify_bootxell_peer_source_coverage.py"
python3 "$root/tools/verify_bootxell_function_ownership.py"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/boot_options.cpp" \
    "$root/reconstruction/bootxell/boot_options_test.cpp" \
    -o "$work/boot_options_test"
"$work/boot_options_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/xell_scan.cpp" \
    "$root/reconstruction/bootxell/xell_scan_test.cpp" \
    -o "$work/xell_scan_test"
"$work/xell_scan_test"

"$cc" -std=c99 -Wall -Wextra -Werror -Wno-unused-function \
    -I"$root/reconstruction/bootxell" -I"$root/third_party/miniz" \
    -c "$root/third_party/miniz/miniz.c" -o "$work/miniz.o"
"$cc" -std=c99 -Wall -Wextra -Werror -Wno-unused-function \
    -I"$root/reconstruction/bootxell" -I"$root/third_party/miniz" \
    -c "$root/third_party/miniz/miniz_tdef.c" -o "$work/miniz_tdef.o"
"$cc" -std=c99 -Wall -Wextra -Werror -Wno-unused-function \
    -I"$root/reconstruction/bootxell" -I"$root/third_party/miniz" \
    -c "$root/third_party/miniz/miniz_tinfl.c" -o "$work/miniz_tinfl.o"
"$cxx" -std=c++17 -Wall -Wextra -Werror -Wno-unused-function \
    -I"$root/reconstruction/bootxell" -I"$root/third_party/miniz" \
    "$root/reconstruction/bootxell/compression_adapters.cpp" \
    "$root/reconstruction/bootxell/compression_adapters_test.cpp" \
    "$work/miniz.o" "$work/miniz_tdef.o" "$work/miniz_tinfl.o" \
    -o "$work/compression_adapters_test"
"$work/compression_adapters_test"

"$cxx" -std=c++17 -Wall -Wextra -Werror \
    "$root/reconstruction/bootxell/boot_options.cpp" \
    "$root/reconstruction/bootxell/xell_scan.cpp" \
    "$root/reconstruction/bootxell/runtime.cpp" \
    "$root/reconstruction/bootxell/runtime_test.cpp" \
    -o "$work/runtime_test"
"$work/runtime_test"
