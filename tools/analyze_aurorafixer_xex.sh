#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="$root/analysis/xex/basefiles/PeerPressure_SideLoading_AuroraFixer.pe"
functions="$root/analysis/xex/AuroraFixer-functions.json"

python3 -B "$root/tools/parse_xbox_pdata_functions.py" "$image" \
    --pdata-offset 0x1200 --pdata-size 0x1f0 \
    --text-address 0x88010000 --text-size 0x4a2c \
    --entry 0x88010668 \
    --idc "$root/analysis/xex/idc/PeerPressure_SideLoading_AuroraFixer.idc" \
    --output "$functions"
python3 -B "$root/tools/reconstruct_xex_text.py" "$image" "$functions" \
    "$root/analysis/reconstruction/AuroraFixer.text.reconstructed.S" \
    --raw-offset 0x10000 --size 0x4a2c --source-label AuroraFixer.xex:.text
python3 -B "$root/tools/reconstruct_xex_nontext.py" "$image" \
    "$root/analysis/reconstruction/AuroraFixer.nontext.reconstructed.S"
python3 -B "$root/tools/analyze_ppc_calls.py" "$image" "$functions" \
    --text-offset 0x10000 --text-address 0x88010000 --text-size 0x4a2c \
    --output "$root/analysis/xex/AuroraFixer-callgraph.json"
python3 -B "$root/tools/analyze_ppc_string_xrefs.py" "$image" "$functions" \
    --image-base 0x88000000 --text-offset 0x10000 \
    --text-address 0x88010000 --text-size 0x4a2c \
    --string-start 0x88000400 --string-end 0x880010d4 \
    --output "$root/analysis/xex/AuroraFixer-string-xrefs.json"
python3 -B "$root/tools/classify_aurorafixer_functions.py"

echo "AuroraFixer exact sections and static analysis regenerated"
