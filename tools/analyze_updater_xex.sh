#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
jeff="$root/third_party/jeff"
artifacts="$root/third_party/artifacts/updater-dtk"
patch_file="$root/patches/jeff-terminal-section-fallthrough.patch"
source_xex="$root/Updater.xex"
expected_jeff="1b4040c152ccfe9e0c87b71247e6739c11f68afb"
rustup_bin="${RUSTUP_BIN:-/home/steve/.cargo/bin/rustup}"

if [[ "$(git -c core.autocrlf=true -C "$jeff" rev-parse HEAD)" != "$expected_jeff" ]]; then
    echo "unexpected jeff revision" >&2
    exit 1
fi
if ! git -c core.autocrlf=true -C "$jeff" diff --quiet; then
    echo "jeff submodule has local changes" >&2
    exit 1
fi

mkdir -p "$artifacts"
work="$(mktemp -d "$artifacts/work.XXXXXX")"
cleanup() {
    git -c core.autocrlf=true -C "$jeff" restore -- src/analysis/slices.rs
    if [[ "$work" == "$artifacts"/work.* && -d "$work" ]]; then
        rm -rf -- "$work"
    fi
}
trap cleanup EXIT

git -c core.autocrlf=true -C "$jeff" apply "$patch_file"
(
    cd "$jeff"
    "$rustup_bin" run stable cargo build --release --bin dtk
)

mkdir -p "$work/out"
(
    cd "$work"
    "$jeff/target/release/dtk" --no-color xex disasm "$source_xex" "$work/out/updater.elf"
)

cp "$work/auto_02_98005000_text.obj" "$artifacts/auto_02_98005000_text.obj"
python3 -B "$root/tools/parse_xbox_coff_symbols.py" \
    "$artifacts/auto_02_98005000_text.obj" \
    --base 0x98005000 \
    --section-end 0x15a40 \
    --source-label third_party/artifacts/updater-dtk/auto_02_98005000_text.obj \
    --output "$root/analysis/xex/Updater-functions.json"
python3 -B "$root/tools/reconstruct_xex_text.py" \
    "$root/analysis/xex/basefiles/Updater.pe" \
    "$root/analysis/xex/Updater-functions.json" \
    "$root/analysis/reconstruction/Updater.text.reconstructed.S" \
    --raw-offset 0x5000 \
    --size 0x15a40 \
    --source-label Updater.xex:.text

echo "Updater function map and exact text reconstruction regenerated"
