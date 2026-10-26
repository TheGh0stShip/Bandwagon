#!/usr/bin/env python3
"""Print the measured function-ownership/source-coverage dashboard.

Only hash-bound ownership manifests are counted. A portable behavioral model
is useful work, but is not reported as verified function coverage until a
component source-coverage manifest binds it to the original image.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent

TARGETS = (
    (
        "KernelExpansion",
        "analysis/kernel-expansion/KernelExpansion-function-ownership.json",
        "analysis/kernel-expansion/KernelExpansion-peer-source-coverage.json",
    ),
    (
        "BootXell",
        "analysis/xex/BootXell-function-ownership.json",
        "analysis/xex/BootXell-peer-source-coverage.json",
    ),
    ("Updater", "analysis/xex/Updater-function-ownership.json", None),
    (
        "UpdateCheck",
        "analysis/xex/UpdateCheck-function-ownership.json",
        "analysis/xex/UpdateCheck-peer-source-coverage.json",
    ),
    ("SettingsApp", "analysis/xex/SettingsApp-function-ownership.json", None),
    ("BootEdit", "analysis/xex/BootEdit-function-ownership.json", None),
)

CATEGORY_BUCKETS = {
    "KernelExpansion": {
        "product": {"peer_product_adapter_or_policy"},
        "excluded": {"platform_toolchain", "upstream_fatfs_r016"},
        "unknown": {"unknown"},
    },
    "BootXell": {
        "product": {"peer_product_adapter_or_policy"},
        "excluded": {"upstream_inih_r62", "upstream_miniz_311", "platform_toolchain"},
        "unknown": set(),
    },
    "Updater": {
        "product": {"product_application"},
        "excluded": {"upstream_miniz", "platform_toolchain"},
        "unknown": {"unknown"},
    },
    "UpdateCheck": {
        "product": {"product_application_or_glue"},
        "excluded": {"platform_toolchain", "bearssl_0_6", "jsoncpp_00_11_0"},
        "unknown": {"unknown"},
    },
    "SettingsApp": {
        "product": {"peer_product_application_glue"},
        "excluded": {
            "platform_toolchain_graphics",
            "platform_import_thunks",
            "upstream_dear_imgui_1_86",
            "upstream_bearssl_0_6",
            "upstream_jsoncpp_00_11_0",
            "upstream_miniz_3_1_1",
        },
        "unknown": {"unknown"},
    },
    "BootEdit": {
        "product": {"product_application"},
        "excluded": {"upstream_imgui_1_86", "platform_toolchain"},
        "unknown": {"unknown"},
    },
}


def pair(entries: int, size: int) -> str:
    return f"{entries} / {size:,} B"


def category_totals(
    component: str, manifest: dict[str, Any],
) -> tuple[list[int], list[int], list[int]]:
    buckets = {"product": [0, 0], "excluded": [0, 0], "unknown": [0, 0]}
    policy = CATEGORY_BUCKETS[component]
    policy_categories = set().union(*policy.values())
    actual_categories = set(manifest["categories"])
    if policy_categories != actual_categories:
        missing = sorted(actual_categories - policy_categories)
        stale = sorted(policy_categories - actual_categories)
        raise SystemExit(
            f"{component} category policy differs: unhandled={missing}, stale={stale}"
        )
    for name, category in manifest["categories"].items():
        values = [
            int(category["inventory_entry_count"]),
            int(category["size"]),
        ]
        matches = [bucket for bucket, names in policy.items() if name in names]
        if len(matches) != 1:
            raise SystemExit(f"{component} category has ambiguous policy: {name}")
        aggregate = buckets[matches[0]]
        aggregate[0] += values[0]
        aggregate[1] += values[1]
    return buckets["product"], buckets["excluded"], buckets["unknown"]


def source_coverage(path: str | None) -> list[int]:
    if path is None:
        return [0, 0]
    coverage = json.loads((ROOT / path).read_text(encoding="utf-8"))
    if "summary" in coverage:
        totals = coverage["summary"]
        return [
            int(totals["peer_product_adapter_or_policy_function_count"]),
            int(totals["peer_product_adapter_or_policy_instruction_bytes"]),
        ]
    if "functions" in coverage:
        functions = coverage["functions"]
        return [len(functions), sum(int(item["size"]) for item in functions)]
    raise SystemExit(f"unrecognized source-coverage schema: {path}")


def main() -> None:
    print(
        "| Component | Total entries / bytes | Excluded upstream/platform | "
        "Product candidates | Verified readable product source | Unknown |"
    )
    print("| --- | ---: | ---: | ---: | ---: | ---: |")
    found = 0
    xex_found = 0
    xex_total = [0, 0]
    xex_excluded = [0, 0]
    xex_product = [0, 0]
    xex_covered = [0, 0]
    xex_unknown = [0, 0]
    for component, ownership_path, coverage_path in TARGETS:
        full_path = ROOT / ownership_path
        if not full_path.exists():
            print(f"| {component} | ownership pending | - | - | - | - |")
            continue
        ownership = json.loads(full_path.read_text(encoding="utf-8"))
        product, excluded, unknown = category_totals(component, ownership)
        total = [
            product[0] + excluded[0] + unknown[0],
            product[1] + excluded[1] + unknown[1],
        ]
        declared = ownership["totals"]
        declared_size = declared.get("instruction_size", declared.get("code_size"))
        if total[0] != int(declared["inventory_entry_count"]) or total[1] != int(declared_size):
            raise SystemExit(
                f"{component} computed totals differ from its manifest totals"
            )
        covered = source_coverage(coverage_path)
        if covered[0] > product[0] or covered[1] > product[1]:
            raise SystemExit(
                f"{component} source coverage exceeds its product boundary"
            )
        print(
            f"| {component} | {pair(*total)} | {pair(*excluded)} | "
            f"{pair(*product)} | {pair(*covered)} | {pair(*unknown)} |"
        )
        if component != "KernelExpansion":
            xex_found += 1
            for aggregate, values in (
                (xex_total, total),
                (xex_excluded, excluded),
                (xex_product, product),
                (xex_covered, covered),
                (xex_unknown, unknown),
            ):
                aggregate[0] += values[0]
                aggregate[1] += values[1]
        found += 1
    if found == 0:
        raise SystemExit("no ownership manifests found")
    if xex_found == len(TARGETS) - 1:
        print(
            f"| **Five-XEX aggregate** | **{pair(*xex_total)}** | "
            f"**{pair(*xex_excluded)}** | **{pair(*xex_product)}** | "
            f"**{pair(*xex_covered)}** | **{pair(*xex_unknown)}** |"
        )


if __name__ == "__main__":
    main()
