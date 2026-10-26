#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="$root/analysis/xex/basefiles/PeerPressure_System_UpdateCheck.pe"
functions="$root/analysis/xex/UpdateCheck-functions.json"

python3 -B "$root/tools/parse_xbox_pdata_functions.py" "$image" \
    --pdata-offset 0x8000 --pdata-size 0x1710 \
    --text-address 0x91a0a000 --text-size 0x372ec \
    --entry 0x91a2e158 \
    --idc "$root/analysis/xex/idc/PeerPressure_System_UpdateCheck.idc" \
    --output "$functions"
python3 -B "$root/tools/reconstruct_xex_text.py" "$image" "$functions" \
    "$root/analysis/reconstruction/UpdateCheck.text.reconstructed.S" \
    --raw-offset 0xa000 --size 0x372ec --source-label UpdateCheck.xex:.text
python3 -B "$root/tools/reconstruct_xex_nontext.py" "$image" \
    "$root/analysis/reconstruction/UpdateCheck.nontext.reconstructed.S"
python3 -B "$root/tools/analyze_ppc_calls.py" "$image" "$functions" \
    --text-offset 0xa000 --text-address 0x91a0a000 --text-size 0x372ec \
    --output "$root/analysis/xex/UpdateCheck-callgraph.json"
python3 -B "$root/tools/analyze_ppc_string_xrefs.py" "$image" "$functions" \
    --image-base 0x91a00000 --text-offset 0xa000 \
    --text-address 0x91a0a000 --text-size 0x372ec \
    --string-start 0x91a00400 --string-end 0x91a07f54 \
    --output "$root/analysis/xex/UpdateCheck-string-xrefs.json"
python3 -B "$root/tools/analyze_updatecheck_provenance.py"
python3 -B "$root/tools/recover_updatecheck_jsoncpp_symbols.py"

echo "UpdateCheck exact sections and static analysis regenerated"
