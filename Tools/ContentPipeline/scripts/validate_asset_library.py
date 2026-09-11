#!/usr/bin/env python3
"""Validate Hotel Haven's active production asset catalog.

The newest present gameplay manifest is authoritative for release scope. Uses
only Python's standard library so CI can run before the native cooker build.
"""
from __future__ import annotations

import json
import re
import sys
from collections import Counter
from pathlib import Path

SUPPORTED_ASSET_TYPES = {
    "StaticMeshAsset", "SkinnedMeshAsset", "SkeletonAsset", "AnimationClipAsset",
    "AnimationSetAsset", "MaterialAsset", "TextureAsset", "PrefabAsset",
    "UIAtlasAsset", "FontAsset", "VFXAsset", "AudioClipAsset", "AudioBankAsset",
}
REQUIRED_ENTRY_COLUMNS = [
    "asset_id", "display_name", "subcategory", "material_family", "profile",
    "animation_set", "interaction_anchors",
]
ANIMATED_PROFILES = {
    "P_ARCH_ANIMATED", "P_SERVICE_PROP_ANIMATED", "P_INTERACTIVE_ANIMATED", "P_CHARACTER",
}
ASSET_ID_RE = re.compile(r"^HH_A(\d{3})$")


def load_json(path: Path) -> dict:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, json.JSONDecodeError) as exc:
        raise AssertionError(f"cannot read valid JSON {path}: {exc}") from exc


def repo_root_from_script(script: Path) -> Path:
    return script.resolve().parents[3]


def active_manifest_path(definitions: Path) -> Path:
    for filename in (
        "hotel_haven_asset_manifest_v2.json",
        "hotel_haven_asset_manifest_v1.json",
    ):
        candidate = definitions / filename
        if candidate.is_file():
            return candidate
    raise AssertionError(f"no Hotel Haven gameplay asset manifest found below {definitions}")


def validate(repo_root: Path) -> list[str]:
    errors: list[str] = []
    defs = repo_root / "GameData" / "AssetDefinitions"
    material_path = defs / "material_families_v1.json"
    animation_path = defs / "animation_sets_v1.json"

    try:
        manifest_path = active_manifest_path(defs)
        manifest = load_json(manifest_path)
        materials = load_json(material_path)
        animations = load_json(animation_path)
    except AssertionError as exc:
        return [str(exc)]

    schema = manifest.get("schema")
    if schema not in (1, 2):
        errors.append(f"master manifest schema must be 1 or 2, got {schema!r}")

    asset_count = manifest.get("asset_count")
    if not isinstance(asset_count, int) or asset_count <= 0:
        errors.append(f"master asset_count must be a positive integer, got {asset_count!r}")
        asset_count = 0
    if manifest.get("storage") != "sharded_by_production_batch":
        errors.append("master storage must be sharded_by_production_batch")

    profiles = manifest.get("profiles")
    if not isinstance(profiles, dict) or not profiles:
        errors.append("master profiles must be a non-empty object")
        profiles = {}
    for profile_id, profile in profiles.items():
        asset_type = profile.get("asset_type") if isinstance(profile, dict) else None
        if asset_type not in SUPPORTED_ASSET_TYPES:
            errors.append(f"profile {profile_id} has unsupported asset_type {asset_type!r}")
        if isinstance(profile, dict) and profile.get("units") != "meters":
            errors.append(f"profile {profile_id} must use meters")

    family_targets = manifest.get("family_targets")
    if not isinstance(family_targets, dict):
        errors.append("family_targets must be an object")
        family_targets = {}
    elif sum(family_targets.values()) != asset_count:
        errors.append(
            f"family_targets must sum to {asset_count}, got {sum(family_targets.values())}"
        )

    material_rows = materials.get("families", [])
    material_ids = [row.get("material_family_id") for row in material_rows if isinstance(row, dict)]
    if len(material_ids) != len(set(material_ids)):
        errors.append("material family IDs must be unique")
    material_id_set = set(material_ids)

    skeleton_ids = {
        row.get("skeleton_id")
        for row in animations.get("skeletons", [])
        if isinstance(row, dict)
    }
    animation_rows = animations.get("sets", [])
    animation_ids = [row.get("animation_set_id") for row in animation_rows if isinstance(row, dict)]
    if len(animation_ids) != len(set(animation_ids)):
        errors.append("animation set IDs must be unique")
    animation_id_set = set(animation_ids)
    for row in animation_rows:
        if isinstance(row, dict) and row.get("skeleton_id") not in skeleton_ids:
            errors.append(
                f"animation set {row.get('animation_set_id')} references unknown skeleton {row.get('skeleton_id')!r}"
            )

    batch_specs = manifest.get("batches")
    if not isinstance(batch_specs, list) or not batch_specs:
        errors.append("master manifest must define a non-empty batches array")
        batch_specs = []

    declared_total = sum(
        spec.get("asset_count", 0) for spec in batch_specs if isinstance(spec, dict)
    )
    if declared_total != asset_count:
        errors.append(
            f"declared batch asset counts must sum to {asset_count}, got {declared_total}"
        )

    all_rows: list[tuple[int, str, str, str, str, str | None, list]] = []
    actual_families: Counter[str] = Counter()
    seen_batch_numbers: set[int] = set()
    previous_batch = 0

    for position, spec in enumerate(batch_specs, start=1):
        if not isinstance(spec, dict):
            errors.append(f"batch index position {position} is not an object")
            continue
        declared_batch = spec.get("batch")
        if not isinstance(declared_batch, int):
            errors.append(f"batch index position {position} has invalid batch number {declared_batch!r}")
            continue
        if declared_batch in seen_batch_numbers:
            errors.append(f"duplicate batch number {declared_batch}")
        if declared_batch <= previous_batch:
            errors.append("master batches must be strictly sorted by batch number")
        seen_batch_numbers.add(declared_batch)
        previous_batch = declared_batch

        declared_batch_count = spec.get("asset_count")
        if not isinstance(declared_batch_count, int) or declared_batch_count <= 0:
            errors.append(f"batch {declared_batch} master count must be positive")
            continue
        rel_path = spec.get("path")
        if not isinstance(rel_path, str):
            errors.append(f"batch {declared_batch} is missing path")
            continue
        batch_path = repo_root / rel_path
        try:
            batch = load_json(batch_path)
        except AssertionError as exc:
            errors.append(str(exc))
            continue

        if batch.get("schema") not in (1, 2):
            errors.append(f"batch {declared_batch} schema must be 1 or 2")
        if batch.get("batch") != declared_batch:
            errors.append(
                f"batch file {batch_path} declares batch {batch.get('batch')!r}, expected {declared_batch}"
            )
        if batch.get("asset_count") != declared_batch_count:
            errors.append(
                f"batch {declared_batch} asset_count must be {declared_batch_count}"
            )
        if batch.get("columns") != REQUIRED_ENTRY_COLUMNS:
            errors.append(f"batch {declared_batch} columns do not match canonical schema")

        batch_rows = 0
        for group in batch.get("groups", []):
            if not isinstance(group, dict):
                errors.append(f"batch {declared_batch} contains non-object group")
                continue
            family = group.get("family")
            if family not in family_targets:
                errors.append(f"batch {declared_batch} uses unknown family {family!r}")
            assets = group.get("assets", [])
            if not isinstance(assets, list):
                errors.append(f"batch {declared_batch}/{family} assets must be an array")
                continue
            for row in assets:
                batch_rows += 1
                if not isinstance(row, list) or len(row) != len(REQUIRED_ENTRY_COLUMNS):
                    errors.append(f"batch {declared_batch}/{family} has malformed row")
                    continue
                asset_id, display_name, subcategory, material_id, profile_id, animation_id, anchors = row
                all_rows.append((declared_batch, family, asset_id, material_id, profile_id, animation_id, anchors))
                actual_families[family] += 1
                if not isinstance(display_name, str) or not display_name.strip():
                    errors.append(f"{asset_id}: display_name must be non-empty")
                if not isinstance(subcategory, str) or not subcategory.strip():
                    errors.append(f"{asset_id}: subcategory must be non-empty")
                if material_id not in material_id_set:
                    errors.append(f"{asset_id}: unknown material family {material_id!r}")
                if profile_id not in profiles:
                    errors.append(f"{asset_id}: unknown profile {profile_id!r}")
                if animation_id is not None and animation_id not in animation_id_set:
                    errors.append(f"{asset_id}: unknown animation set {animation_id!r}")
                if profile_id in ANIMATED_PROFILES and animation_id is None:
                    errors.append(f"{asset_id}: profile {profile_id} requires an animation set")
                if not isinstance(anchors, list) or any(not isinstance(anchor, str) for anchor in anchors):
                    errors.append(f"{asset_id}: interaction_anchors must be an array of strings")
        if batch_rows != declared_batch_count:
            errors.append(
                f"batch {declared_batch} contains {batch_rows} rows, expected {declared_batch_count}"
            )

    ids = [row[2] for row in all_rows]
    if len(ids) != asset_count:
        errors.append(f"catalog contains {len(ids)} rows, expected {asset_count}")
    if len(ids) != len(set(ids)):
        duplicates = sorted(asset_id for asset_id, count in Counter(ids).items() if count > 1)
        errors.append(f"duplicate asset IDs: {duplicates}")

    expected_ids = {f"HH_A{i:03d}" for i in range(1, asset_count + 1)}
    actual_ids = set(ids)
    missing = sorted(expected_ids - actual_ids)
    extra = sorted(actual_ids - expected_ids)
    if missing:
        errors.append(f"missing asset IDs: {missing}")
    if extra:
        errors.append(f"unexpected asset IDs: {extra}")
    for asset_id in ids:
        if not isinstance(asset_id, str) or not ASSET_ID_RE.fullmatch(asset_id):
            errors.append(f"invalid asset ID format: {asset_id!r}")

    for family, target in family_targets.items():
        actual = actual_families.get(family, 0)
        if actual != target:
            errors.append(f"family {family} has {actual} assets, expected {target}")

    if "guests_staff" in family_targets:
        character_rows = [row for row in all_rows if row[1] == "guests_staff"]
        expected_characters = family_targets["guests_staff"]
        if len(character_rows) != expected_characters:
            errors.append(
                f"guests_staff must contain {expected_characters} assets, got {len(character_rows)}"
            )
        for _, _, asset_id, _, profile_id, animation_id, _ in character_rows:
            if profile_id != "P_CHARACTER":
                errors.append(f"{asset_id}: character must use P_CHARACTER")
            if animation_id is None:
                errors.append(f"{asset_id}: character must reference an animation set")

    return errors


def main() -> int:
    repo_root = repo_root_from_script(Path(__file__))
    errors = validate(repo_root)
    if errors:
        print(f"Hotel Haven asset-library validation FAILED ({len(errors)} issue(s)):")
        for error in errors:
            print(f" - {error}")
        return 1

    defs = repo_root / "GameData" / "AssetDefinitions"
    manifest = load_json(active_manifest_path(defs))
    asset_count = int(manifest["asset_count"])
    batch_count = len(manifest["batches"])
    print("Hotel Haven asset-library validation PASSED")
    print(f" - {asset_count} unique gameplay-facing assets")
    print(f" - {batch_count} manifest-declared production batches")
    print(" - family totals, profiles, materials, animations, skeletons, and character mappings resolve")
    return 0


if __name__ == "__main__":
    sys.exit(main())
