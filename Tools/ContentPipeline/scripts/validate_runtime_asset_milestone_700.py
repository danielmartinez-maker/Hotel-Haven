#!/usr/bin/env python3
"""Validate the Hotel Haven HH_A001-HH_A700 runtime milestone."""
from __future__ import annotations

import json
import re
import sys
from collections import Counter
from pathlib import Path

ASSET_ID_RE = re.compile(r"^HH_A(\d{3})$")
COLUMNS = [
    "asset_id", "display_name", "subcategory", "material_family", "profile",
    "animation_set", "interaction_anchors",
]


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise AssertionError(f"cannot read valid JSON {path}: {exc}") from exc


def validate(repo_root: Path) -> list[str]:
    errors: list[str] = []
    defs = repo_root / "GameData" / "AssetDefinitions"
    milestone_path = defs / "runtime_asset_milestone_700_v1.json"
    v1_path = defs / "hotel_haven_asset_manifest_v1.json"
    try:
        milestone = load_json(milestone_path)
        v1 = load_json(v1_path)
    except AssertionError as exc:
        return [str(exc)]

    if milestone.get("schema") != 1:
        errors.append("A700 milestone schema must be 1")
    if milestone.get("asset_count") != 700:
        errors.append(f"A700 milestone asset_count must be 700, got {milestone.get('asset_count')!r}")
    policy = milestone.get("runtime_policy", {})
    if policy.get("asset_id_prefix") != "HH_A":
        errors.append("A700 milestone runtime prefix must be HH_A")
    if policy.get("first_asset_number") != 1 or policy.get("last_asset_number") != 700:
        errors.append("A700 milestone runtime bounds must be 1..700")
    if policy.get("assets_after_A700_excluded") is not True:
        errors.append("A700 milestone must explicitly exclude assets after HH_A700")

    batch_specs = milestone.get("batches")
    if not isinstance(batch_specs, list) or len(batch_specs) != 14:
        errors.append(f"A700 milestone must declare 14 batches, got {len(batch_specs or [])}")
        batch_specs = []

    profiles = v1.get("profiles", {})
    all_ids: list[str] = []
    for expected_batch, spec in enumerate(batch_specs, start=1):
        if spec.get("batch") != expected_batch:
            errors.append(
                f"milestone batch slot {expected_batch} declares {spec.get('batch')!r}"
            )
        if spec.get("asset_count") != 50:
            errors.append(f"milestone batch {expected_batch} must declare 50 assets")
        rel = spec.get("path")
        if not isinstance(rel, str):
            errors.append(f"milestone batch {expected_batch} is missing a path")
            continue
        try:
            batch = load_json(repo_root / rel)
        except AssertionError as exc:
            errors.append(str(exc))
            continue
        if batch.get("batch") != expected_batch:
            errors.append(
                f"{rel}: declares batch {batch.get('batch')!r}, expected {expected_batch}"
            )
        if batch.get("asset_count") != 50:
            errors.append(f"{rel}: asset_count must be 50")
        if batch.get("columns") != COLUMNS:
            errors.append(f"{rel}: columns differ from runtime milestone schema")
        rows = 0
        for group in batch.get("groups", []):
            for row in group.get("assets", []):
                rows += 1
                if not isinstance(row, list) or len(row) != len(COLUMNS):
                    errors.append(f"{rel}: malformed asset row")
                    continue
                asset_id, name, subcategory, _material, profile, _animation, anchors = row
                all_ids.append(asset_id)
                match = ASSET_ID_RE.fullmatch(asset_id) if isinstance(asset_id, str) else None
                if not match:
                    errors.append(f"{rel}: invalid asset id {asset_id!r}")
                    continue
                number = int(match.group(1))
                lower = (expected_batch - 1) * 50 + 1
                upper = expected_batch * 50
                if not lower <= number <= upper:
                    errors.append(
                        f"{asset_id}: outside batch {expected_batch:02d} range "
                        f"HH_A{lower:03d}-HH_A{upper:03d}"
                    )
                if not isinstance(name, str) or not name.strip():
                    errors.append(f"{asset_id}: display_name must be non-empty")
                if not isinstance(subcategory, str) or not subcategory.strip():
                    errors.append(f"{asset_id}: subcategory must be non-empty")
                if profile not in profiles:
                    errors.append(f"{asset_id}: unknown inherited runtime profile {profile!r}")
                if not isinstance(anchors, list) or any(not isinstance(x, str) for x in anchors):
                    errors.append(f"{asset_id}: interaction_anchors must be strings")
        if rows != 50:
            errors.append(f"{rel}: contains {rows} rows, expected 50")

    if len(all_ids) != 700:
        errors.append(f"A700 milestone contains {len(all_ids)} rows, expected 700")
    duplicates = sorted(k for k, v in Counter(all_ids).items() if v > 1)
    if duplicates:
        errors.append(f"A700 milestone contains duplicate IDs: {duplicates}")
    expected = {f"HH_A{i:03d}" for i in range(1, 701)}
    actual = set(all_ids)
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    if missing:
        errors.append(f"A700 milestone missing IDs: {missing}")
    if extra:
        errors.append(f"A700 milestone has IDs outside A001-A700: {extra}")

    core_manifest = milestone.get("required_core_binding_manifest")
    if core_manifest != "GameData/AssetDefinitions/runtime_world_bindings_v1.json":
        errors.append("A700 milestone must retain runtime_world_bindings_v1.json as its required core")
    return errors


def main() -> int:
    root = Path(__file__).resolve().parents[3]
    errors = validate(root)
    if errors:
        print(f"Hotel Haven A700 milestone validation FAILED ({len(errors)} issue(s)):")
        for error in errors:
            print(f" - {error}")
        return 1
    print("Hotel Haven A700 milestone validation PASSED")
    print(" - 700 unique contiguous gameplay asset IDs (HH_A001-HH_A700)")
    print(" - 14 production batches x 50 assets")
    print(" - A701+ is excluded from this runtime tranche")
    return 0


if __name__ == "__main__":
    sys.exit(main())
