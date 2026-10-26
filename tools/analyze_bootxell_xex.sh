#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="$root/analysis/xex/basefiles/PeerPressure_OtherOS_BootXell.pe"
functions="$root/analysis/xex/BootXell-functions.json"

python3 -B "$root/tools/parse_xbox_pdata_functions.py" "$image" \
    --pdata-offset 0x3a00 --pdata-size 0x678 \
    --text-address 0x91a05000 --text-size 0x12458 \
    --entry 0x91a0c378 \
    --idc "$root/analysis/xex/idc/PeerPressure_OtherOS_BootXell.idc" \
    --output "$functions"
python3 -B "$root/tools/reconstruct_xex_text.py" "$image" "$functions" \
    "$root/analysis/reconstruction/BootXell.text.reconstructed.S" \
    --raw-offset 0x5000 --size 0x12458 --source-label BootXell.xex:.text
python3 -B "$root/tools/reconstruct_xex_nontext.py" "$image" \
    "$root/analysis/reconstruction/BootXell.nontext.reconstructed.S"
python3 -B "$root/tools/analyze_ppc_calls.py" "$image" "$functions" \
    --text-offset 0x5000 --text-address 0x91a05000 --text-size 0x12458 \
    --output "$root/analysis/xex/BootXell-callgraph.json"
python3 -B "$root/tools/analyze_ppc_string_xrefs.py" "$image" "$functions" \
    --image-base 0x91a00000 --text-offset 0x5000 \
    --text-address 0x91a05000 --text-size 0x12458 \
    --string-start 0x91a00400 --string-end 0x91a0382c \
    --output "$root/analysis/xex/BootXell-string-xrefs.json"

echo "BootXell exact sections and static analysis regenerated"
