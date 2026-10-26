#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="$root/analysis/xex/basefiles/PeerPressure_System_SettingsApp.pe"
functions="$root/analysis/xex/SettingsApp-functions.json"

python3 -B "$root/tools/parse_xbox_pdata_functions.py" "$image" \
    --pdata-offset 0x1a400 --pdata-size 0x3480 \
    --text-address 0x82020000 --text-size 0x9975c \
    --entry 0x8202e0e0 \
    --idc "$root/analysis/xex/idc/PeerPressure_System_SettingsApp.idc" \
    --output "$functions"
python3 -B "$root/tools/reconstruct_xex_text.py" "$image" "$functions" \
    "$root/analysis/reconstruction/SettingsApp.text.reconstructed.S" \
    --raw-offset 0x20000 --size 0x9975c --source-label SettingsApp.xex:.text
python3 -B "$root/tools/reconstruct_xex_nontext.py" "$image" \
    "$root/analysis/reconstruction/SettingsApp.nontext.reconstructed.S"
python3 -B "$root/tools/analyze_ppc_calls.py" "$image" "$functions" \
    --text-offset 0x20000 --text-address 0x82020000 --text-size 0x9975c \
    --output "$root/analysis/xex/SettingsApp-callgraph.json"
python3 -B "$root/tools/analyze_ppc_string_xrefs.py" "$image" "$functions" \
    --image-base 0x82000000 --text-offset 0x20000 \
    --text-address 0x82020000 --text-size 0x9975c \
    --string-start 0x82000400 --string-end 0x8201a24c \
    --output "$root/analysis/xex/SettingsApp-string-xrefs.json"

echo "SettingsApp exact sections and static analysis regenerated"
