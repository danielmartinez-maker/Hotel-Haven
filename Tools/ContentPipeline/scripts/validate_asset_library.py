#!/usr/bin/env python3
"""Validate Hotel Haven's canonical 500-asset production catalog.

Uses only Python's standard library so CI can run it before the native cooker build.
The validator is deliberately fail-closed: malformed structures are reported as
validation errors rather than being allowed to raise incidental TypeError or
AttributeError exceptions.
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


def load_json(path: Path) -> object:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, json.JSONDecodeError) as exc:
        raise AssertionError(f"cannot read valid JSON {path}: {exc}") from exc


def repo_root_from_script(script: Path) -> Path:
    return script.resolve().parents[3]


def _object(value: object, name: str, errors: list[str]) -> dict:
    if isinstance(value, dict):
        return value
    errors.append(f"{name} must be an object")
    return {}


def _array(value: object, name: str, errors: list[str]) -> list:
    if isinstance(value, list):
        return value
    errors.append(f"{name} must be an array")
    return []


def _safe_batch_path(repo_root: Path, rel_path: object, batch: int, errors: list[str]) -> Path | None:
    if not isinstance(rel_path, str) or not rel_path.strip():
        errors.append(f"batch {batch} is missing path")
        return None
    path = Path(rel_path)
    if path.is_absolute():
        errors.append(f"batch {batch} path must be repository-relative")
        return None
    root = repo_root.resolve()
    candidate = (repo_root / path).resolve()
    if not candidate.is_relative_to(root):
        errors.append(f"batch {batch} path escapes repository root: {rel_path!r}")
        return None
    return candidate


def validate(repo_root: Path) -> list[str]:
    errors: list[str] = []
    defs = repo_root / "GameData" / "AssetDefinitions"
    manifest_path = defs / "hotel_haven_asset_manifest_v1.json"
    material_path = defs / "material_families_v1.json"
    animation_path = defs / "animation_sets_v1.json"

    try:
        manifest_raw = load_json(manifest_path)
        materials_raw = load_json(material_path)
        animations_raw = load_json(animation_path)
    except AssertionError as exc:
        return [str(exc)]

    manifest = _object(manifest_raw, "master manifest", errors)
    materials = _object(materials_raw, "material library", errors)
    animations = _object(animations_raw, "animation library", errors)

    if manifest.get("schema") != 1:
        errors.append("master manifest schema must be 1")
    if manifest.get("asset_count") != 500:
        errors.append(f"master asset_count must be 500, got {manifest.get('asset_count')!r}")
    if manifest.get("storage") != "sharded_by_production_batch":
        errors.append("master storage must be sharded_by_production_batch")

    profiles = manifest.get("profiles")
    if not isinstance(profiles, dict) or not profiles:
        errors.append("master profiles must be a non-empty object")
        profiles = {}
    for profile_id, profile in profiles.items():
        if not isinstance(profile_id, str) or not profile_id:
            errors.append(f"invalid profile ID {profile_id!r}")
        asset_type = profile.get("asset_type") if isinstance(profile, dict) else None
        if asset_type not in SUPPORTED_ASSET_TYPES:
            errors.append(f"profile {profile_id} has unsupported asset_type {asset_type!r}")
        if not isinstance(profile, dict):
            errors.append(f"profile {profile_id} must be an object")
        elif profile.get("units") != "meters":
            errors.append(f"profile {profile_id} must use meters")

    family_targets_raw = manifest.get("family_targets")
    family_targets: dict[str, int] = {}
    if not isinstance(family_targets_raw, dict):
        errors.append("family_targets must be an object")
    else:
        for family, target in family_targets_raw.items():
            if not isinstance(family, str) or not family:
                errors.append(f"family_targets contains invalid family ID {family!r}")
                continue
            if isinstance(target, bool) or not isinstance(target, int) or target < 0:
                errors.append(f"family_targets[{family!r}] must be a non-negative integer")
                continue
            family_targets[family] = target
        if len(family_targets) == len(family_targets_raw):
            total = sum(family_targets.values())
            if total != 500:
                errors.append(f"family_targets must sum to 500, got {total}")

    material_rows = _array(materials.get("families", []), "material families", errors)
    material_ids: list[str] = []
    for index, row in enumerate(material_rows):
        if not isinstance(row, dict):
            errors.append(f"material family entry {index} must be an object")
            continue
        material_id = row.get("material_family_id")
        if not isinstance(material_id, str) or not material_id:
            errors.append(f"material family entry {index} has invalid material_family_id")
            continue
        material_ids.append(material_id)
    if len(material_ids) != len(set(material_ids)):
        errors.append("material family IDs must be unique")
    material_id_set = set(material_ids)

    skeleton_rows = _array(animations.get("skeletons", []), "skeletons", errors)
    skeleton_ids: list[str] = []
    for index, row in enumerate(skeleton_rows):
        if not isinstance(row, dict):
            errors.append(f"skeleton entry {index} must be an object")
            continue
        skeleton_id = row.get("skeleton_id")
        if not isinstance(skeleton_id, str) or not skeleton_id:
            errors.append(f"skeleton entry {index} has invalid skeleton_id")
            continue
        skeleton_ids.append(skeleton_id)
    if len(skeleton_ids) != len(set(skeleton_ids)):
        errors.append("skeleton IDs must be unique")
    skeleton_id_set = set(skeleton_ids)

    animation_rows = _array(animations.get("sets", []), "animation sets", errors)
    animation_ids: list[str] = []
    for index, row in enumerate(animation_rows):
        if not isinstance(row, dict):
            errors.append(f"animation set entry {index} must be an object")
            continue
        animation_id = row.get("animation_set_id")
        if not isinstance(animation_id, str) or not animation_id:
            errors.append(f"animation set entry {index} has invalid animation_set_id")
            continue
        animation_ids.append(animation_id)
        if row.get("skeleton_id") not in skeleton_id_set:
            errors.append(
                f"animation set {animation_id} references unknown skeleton {row.get('skeleton_id')!r}"
            )
        clips = row.get("clips")
        if not isinstance(clips, list) or not clips or any(not isinstance(clip, str) or not clip for clip in clips):
            errors.append(f"animation set {animation_id} clips must be a non-empty array of strings")
        elif len(clips) != len(set(clips)):
            errors.append(f"animation set {animation_id} clip IDs must be unique")
    if len(animation_ids) != len(set(animation_ids)):
        errors.append("animation set IDs must be unique")
    animation_id_set = set(animation_ids)

    batch_specs_raw = manifest.get("batches")
    if not isinstance(batch_specs_raw, list):
        errors.append("master manifest batches must be an array")
        batch_specs: list = []
    else:
        batch_specs = batch_specs_raw
        if len(batch_specs) != 10:
            errors.append(f"master manifest must define 10 batches, got {len(batch_specs)}")

    all_rows: list[tuple[int, str, str, str, str, str | None, list[str]]] = []
    actual_families: Counter[str] = Counter()
    display_names: list[str] = []

    for expected_batch, spec in enumerate(batch_specs, start=1):
        if not isinstance(spec, dict):
            errors.append(f"batch specification {expected_batch} must be an object")
            continue
        if spec.get("batch") != expected_batch:
            errors.append(f"batch index position {expected_batch} declares batch {spec.get('batch')!r}")
        if spec.get("asset_count") != 50:
            errors.append(f"batch {expected_batch} master count must be 50")
        batch_path = _safe_batch_path(repo_root, spec.get("path"), expected_batch, errors)
        if batch_path is None:
            continue
        try:
            batch_raw = load_json(batch_path)
        except AssertionError as exc:
            errors.append(str(exc))
            continue
        if not isinstance(batch_raw, dict):
            errors.append(f"batch {expected_batch} document must be an object")
            continue
        batch = batch_raw
        if batch.get("schema") != 1:
            errors.append(f"batch {expected_batch} schema must be 1")
        if batch.get("batch") != expected_batch:
            errors.append(f"batch file {batch_path} declares batch {batch.get('batch')!r}, expected {expected_batch}")
        if batch.get("asset_count") != 50:
            errors.append(f"batch {expected_batch} asset_count must be 50")
        if batch.get("columns") != REQUIRED_ENTRY_COLUMNS:
            errors.append(f"batch {expected_batch} columns do not match canonical schema")

        groups = batch.get("groups", [])
        if not isinstance(groups, list):
            errors.append(f"batch {expected_batch} groups must be an array")
            continue

        batch_rows = 0
        for group in groups:
            if not isinstance(group, dict):
                errors.append(f"batch {expected_batch} contains non-object group")
                continue
            family = group.get("family")
            family_valid = isinstance(family, str) and family in family_targets
            if not family_valid:
                errors.append(f"batch {expected_batch} uses unknown family {family!r}")
            assets = group.get("assets", [])
            if not isinstance(assets, list):
                errors.append(f"batch {expected_batch}/{family} assets must be an array")
                continue
            for row in assets:
                batch_rows += 1
                if not isinstance(row, list) or len(row) != len(REQUIRED_ENTRY_COLUMNS):
                    errors.append(f"batch {expected_batch}/{family} has malformed row")
                    continue
                asset_id, display_name, subcategory, material_id, profile_id, animation_id, anchors = row
                id_valid = isinstance(asset_id, str) and ASSET_ID_RE.fullmatch(asset_id) is not None
                if not id_valid:
                    errors.append(f"invalid asset ID format: {asset_id!r}")
                    asset_label = repr(asset_id)
                else:
                    asset_label = asset_id

                if family_valid and id_valid:
                    actual_families[family] += 1

                if not isinstance(display_name, str) or not display_name.strip() or display_name != display_name.strip():
                    errors.append(f"{asset_label}: display_name must be non-empty and trimmed")
                else:
                    display_names.append(display_name)
                if not isinstance(subcategory, str) or not subcategory.strip() or subcategory != subcategory.strip():
                    errors.append(f"{asset_label}: subcategory must be non-empty and trimmed")
                if material_id not in material_id_set:
                    errors.append(f"{asset_label}: unknown material family {material_id!r}")
                if profile_id not in profiles:
                    errors.append(f"{asset_label}: unknown profile {profile_id!r}")
                if animation_id is not None and animation_id not in animation_id_set:
                    errors.append(f"{asset_label}: unknown animation set {animation_id!r}")

                profile = profiles.get(profile_id)
                profile_type = profile.get("asset_type") if isinstance(profile, dict) else None
                if profile_type == "SkinnedMeshAsset" and animation_id is None:
                    errors.append(f"{asset_label}: profile {profile_id} requires an animation set")
                elif profile_type != "SkinnedMeshAsset" and animation_id is not None:
                    errors.append(f"{asset_label}: static profile {profile_id} cannot declare animation metadata")

                anchors_valid = isinstance(anchors, list) and all(
                    isinstance(anchor, str) and anchor.startswith("INT_") for anchor in anchors
                )
                if not anchors_valid:
                    errors.append(f"{asset_label}: interaction_anchors must be an array of INT_ strings")
                    safe_anchors: list[str] = []
                else:
                    safe_anchors = anchors
                    if len(anchors) != len(set(anchors)):
                        errors.append(f"{asset_label}: interaction anchors must be unique")

                if id_valid and family_valid:
                    all_rows.append(
                        (expected_batch, family, asset_id, material_id, profile_id, animation_id, safe_anchors)
                    )
        if batch_rows != 50:
            errors.append(f"batch {expected_batch} contains {batch_rows} rows, expected 50")

    ids = [row[2] for row in all_rows]
    if len(ids) != 500:
        errors.append(f"catalog contains {len(ids)} valid rows, expected 500")
    if len(ids) != len(set(ids)):
        duplicates = sorted(asset_id for asset_id, count in Counter(ids).items() if count > 1)
        errors.append(f"duplicate asset IDs: {duplicates}")
    if len(display_names) != len(set(display_names)):
        errors.append("asset display names must be unique")

    expected_ids = {f"HH_A{i:03d}" for i in range(1, 501)}
    actual_ids = set(ids)
    missing = sorted(expected_ids - actual_ids)
    extra = sorted(actual_ids - expected_ids)
    if missing:
        errors.append(f"missing asset IDs: {missing}")
    if extra:
        errors.append(f"unexpected asset IDs: {extra}")

    for family, target in family_targets.items():
        actual = actual_families.get(family, 0)
        if actual != target:
            errors.append(f"family {family} has {actual} assets, expected {target}")

    character_rows = [row for row in all_rows if row[1] == "guests_staff"]
    if len(character_rows) != 50:
        errors.append(f"guests_staff must contain 50 assets, got {len(character_rows)}")
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
    print("Hotel Haven asset-library validation PASSED")
    print(" - 500 unique gameplay-facing assets")
    print(" - 10 production batches x 50 assets")
    print(" - family totals, profiles, materials, animations, skeletons, and character mappings resolve")
    return 0


if __name__ == "__main__":
    sys.exit(main())
