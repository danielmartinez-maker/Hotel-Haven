#!/usr/bin/env python3
"""Validate Hotel Haven's canonical asset catalog and cooked runtime policy parity.

Uses only Python's standard library so CI can validate manifests before the native
cooker build and inspect deterministic ``.hasset`` outputs after cooking.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
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
HASSSET_MAGIC = b"HHASSET\0"
POLICY_FIELDS = (
    "units", "lod_policy", "collision_policy", "cutaway_policy", "pivot_profile",
)
EXPECTED_GAMEPLAY_ASSETS = 500
EXPECTED_GENERATED_RECORDS = 591
EXPECTED_ANIMATION_LINKS = 87
EXPECTED_INTERACTION_ANCHORS = 184


def load_json(path: Path) -> dict:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, json.JSONDecodeError) as exc:
        raise AssertionError(f"cannot read valid JSON {path}: {exc}") from exc


def repo_root_from_script(script: Path) -> Path:
    return script.resolve().parents[3]


def validate(repo_root: Path) -> list[str]:
    errors: list[str] = []
    defs = repo_root / "GameData" / "AssetDefinitions"
    manifest_path = defs / "hotel_haven_asset_manifest_v1.json"
    material_path = defs / "material_families_v1.json"
    animation_path = defs / "animation_sets_v1.json"

    try:
        manifest = load_json(manifest_path)
        materials = load_json(material_path)
        animations = load_json(animation_path)
    except AssertionError as exc:
        return [str(exc)]

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
        asset_type = profile.get("asset_type") if isinstance(profile, dict) else None
        if asset_type not in SUPPORTED_ASSET_TYPES:
            errors.append(f"profile {profile_id} has unsupported asset_type {asset_type!r}")
        if isinstance(profile, dict) and profile.get("units") != "meters":
            errors.append(f"profile {profile_id} must use meters")

    family_targets = manifest.get("family_targets")
    if not isinstance(family_targets, dict):
        errors.append("family_targets must be an object")
        family_targets = {}
    elif sum(family_targets.values()) != 500:
        errors.append(f"family_targets must sum to 500, got {sum(family_targets.values())}")

    material_rows = materials.get("families", [])
    material_ids = [row.get("material_family_id") for row in material_rows if isinstance(row, dict)]
    if len(material_ids) != len(set(material_ids)):
        errors.append("material family IDs must be unique")
    material_id_set = set(material_ids)

    skeleton_ids = {row.get("skeleton_id") for row in animations.get("skeletons", []) if isinstance(row, dict)}
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
    if not isinstance(batch_specs, list) or len(batch_specs) != 10:
        errors.append(f"master manifest must define 10 batches, got {len(batch_specs or [])}")
        batch_specs = []

    all_rows: list[tuple[int, str, str, str, str, str | None, list]] = []
    actual_families: Counter[str] = Counter()

    for expected_batch, spec in enumerate(batch_specs, start=1):
        if spec.get("batch") != expected_batch:
            errors.append(f"batch index position {expected_batch} declares batch {spec.get('batch')!r}")
        if spec.get("asset_count") != 50:
            errors.append(f"batch {expected_batch} master count must be 50")
        rel_path = spec.get("path")
        if not isinstance(rel_path, str):
            errors.append(f"batch {expected_batch} is missing path")
            continue
        batch_path = repo_root / rel_path
        try:
            batch = load_json(batch_path)
        except AssertionError as exc:
            errors.append(str(exc))
            continue
        if batch.get("schema") != 1:
            errors.append(f"batch {expected_batch} schema must be 1")
        if batch.get("batch") != expected_batch:
            errors.append(f"batch file {batch_path} declares batch {batch.get('batch')!r}, expected {expected_batch}")
        if batch.get("asset_count") != 50:
            errors.append(f"batch {expected_batch} asset_count must be 50")
        if batch.get("columns") != REQUIRED_ENTRY_COLUMNS:
            errors.append(f"batch {expected_batch} columns do not match canonical schema")

        batch_rows = 0
        for group in batch.get("groups", []):
            if not isinstance(group, dict):
                errors.append(f"batch {expected_batch} contains non-object group")
                continue
            family = group.get("family")
            if family not in family_targets:
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
                all_rows.append((expected_batch, family, asset_id, material_id, profile_id, animation_id, anchors))
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
        if batch_rows != 50:
            errors.append(f"batch {expected_batch} contains {batch_rows} rows, expected 50")

    ids = [row[2] for row in all_rows]
    if len(ids) != 500:
        errors.append(f"catalog contains {len(ids)} rows, expected 500")
    if len(ids) != len(set(ids)):
        duplicates = sorted(asset_id for asset_id, count in Counter(ids).items() if count > 1)
        errors.append(f"duplicate asset IDs: {duplicates}")

    expected_ids = {f"HH_A{i:03d}" for i in range(1, 501)}
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

    character_rows = [row for row in all_rows if row[1] == "guests_staff"]
    if len(character_rows) != 50:
        errors.append(f"guests_staff must contain 50 assets, got {len(character_rows)}")
    for _, _, asset_id, _, profile_id, animation_id, _ in character_rows:
        if profile_id != "P_CHARACTER":
            errors.append(f"{asset_id}: character must use P_CHARACTER")
        if animation_id is None:
            errors.append(f"{asset_id}: character must reference an animation set")

    return errors


class _HassetReader:
    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def take(self, count: int) -> bytes:
        if count < 0 or self.offset + count > len(self.data):
            raise ValueError("truncated hasset")
        value = self.data[self.offset:self.offset + count]
        self.offset += count
        return value

    def u32(self) -> int:
        return struct.unpack("<I", self.take(4))[0]

    def u64(self) -> int:
        return struct.unpack("<Q", self.take(8))[0]

    def string(self) -> str:
        size = self.u32()
        try:
            return self.take(size).decode("utf-8")
        except UnicodeDecodeError as exc:
            raise ValueError("invalid UTF-8 hasset string") from exc


def _read_hasset_policy(path: Path) -> dict:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise ValueError(f"cannot read cooked hasset: {exc}") from exc

    reader = _HassetReader(data)
    if reader.take(len(HASSSET_MAGIC)) != HASSSET_MAGIC:
        raise ValueError("invalid hasset magic")
    version = reader.u32()
    asset_type = reader.u32()
    asset_id = reader.string()
    fingerprint = reader.string()
    dependency_count = reader.u32()
    dependencies = [reader.string() for _ in range(dependency_count)]
    source_path = reader.string()
    sidecar_path = reader.string()

    result = {
        "version": version,
        "asset_type": asset_type,
        "asset_id": asset_id,
        "fingerprint": fingerprint,
        "dependencies": dependencies,
        "source_path": source_path,
        "sidecar_path": sidecar_path,
        "units": "",
        "lod_policy": "",
        "collision_policy": "",
        "cutaway_policy": "",
        "pivot_profile": "",
        "interaction_anchors": [],
    }

    if version == 2:
        for field in POLICY_FIELDS:
            result[field] = reader.string()
        anchor_count = reader.u32()
        result["interaction_anchors"] = [reader.string() for _ in range(anchor_count)]
    elif version != 1:
        raise ValueError(f"unsupported hasset version {version}")

    payload_size = reader.u64()
    reader.take(payload_size)
    if reader.offset != len(data):
        raise ValueError("trailing bytes in hasset")
    return result


def validate_cooked_policy_parity(exports_root: Path, cooked_root: Path) -> tuple[list[str], dict]:
    """Compare normalized gameplay sidecars with cooked Hasset v2 policy envelopes."""
    errors: list[str] = []
    sidecar_paths = sorted(Path(exports_root).glob("Batch*/*.asset.json"))
    expected_anchor_bindings = 0
    matched_assets = 0
    matched_anchor_bindings = 0

    for sidecar_path in sidecar_paths:
        try:
            sidecar = load_json(sidecar_path)
        except AssertionError as exc:
            errors.append(str(exc))
            continue

        asset_id = sidecar.get("asset_id")
        if not isinstance(asset_id, str) or not ASSET_ID_RE.fullmatch(asset_id):
            errors.append(f"{sidecar_path}: gameplay sidecar has invalid asset_id {asset_id!r}")
            continue

        sidecar_anchors = sidecar.get("interaction_anchors", [])
        if not isinstance(sidecar_anchors, list) or any(not isinstance(anchor, str) for anchor in sidecar_anchors):
            errors.append(f"{asset_id}: interaction_anchors must be an array of strings")
            continue
        expected_anchors = sorted(set(sidecar_anchors))
        expected_anchor_bindings += len(expected_anchors)

        cooked_path = Path(cooked_root) / f"{asset_id}.hasset"
        if not cooked_path.is_file():
            errors.append(f"{asset_id}: missing cooked hasset {cooked_path}")
            continue
        try:
            cooked = _read_hasset_policy(cooked_path)
        except ValueError as exc:
            errors.append(f"{asset_id}: {exc}")
            continue

        if cooked["version"] != 2:
            errors.append(
                f"{asset_id}: cooked hasset must use version 2 policy envelope, got version {cooked['version']}"
            )
            continue

        asset_errors = []
        if cooked["asset_id"] != asset_id:
            asset_errors.append(
                f"{asset_id}: cooked asset_id {cooked['asset_id']!r} does not match sidecar"
            )
        for field in POLICY_FIELDS:
            expected = sidecar.get(field, "")
            actual = cooked[field]
            if actual != expected:
                asset_errors.append(
                    f"{asset_id}: {field} cooked {actual!r} does not match sidecar {expected!r}"
                )
        if cooked["interaction_anchors"] != expected_anchors:
            asset_errors.append(
                f"{asset_id}: interaction_anchors cooked {cooked['interaction_anchors']!r} "
                f"do not match sidecar {expected_anchors!r}"
            )

        if asset_errors:
            errors.extend(asset_errors)
        else:
            matched_assets += 1
            matched_anchor_bindings += len(expected_anchors)

    counts = {
        "expected_cooked_policy_assets": len(sidecar_paths),
        "cooked_policy_assets": matched_assets,
        "expected_interaction_anchor_bindings": expected_anchor_bindings,
        "cooked_interaction_anchor_bindings": matched_anchor_bindings,
    }
    return errors, counts


def _validate_release_evidence(repo_root: Path, counts: dict) -> list[str]:
    errors: list[str] = []
    summary_path = repo_root / "Art" / "Validation" / "full_generation_summary.json"
    qc_path = repo_root / "Art" / "Validation" / "generated_geometry_qc.json"
    try:
        summary = load_json(summary_path)
        qc = load_json(qc_path)
    except AssertionError as exc:
        return [str(exc)]

    exact = [
        ("gameplay_asset_count", summary.get("gameplay_asset_count"), EXPECTED_GAMEPLAY_ASSETS),
        ("generated_asset_records", summary.get("generated_asset_records"), EXPECTED_GENERATED_RECORDS),
        ("animation_links", summary.get("animation_links"), EXPECTED_ANIMATION_LINKS),
        ("expected_animation_links", summary.get("expected_animation_links"), EXPECTED_ANIMATION_LINKS),
        ("animation_links_deferred", summary.get("animation_links_deferred"), 0),
        ("generation interaction_anchor_bindings", summary.get("interaction_anchor_bindings"), EXPECTED_INTERACTION_ANCHORS),
        ("geometry interaction_anchor_bindings", qc.get("interaction_anchor_bindings"), EXPECTED_INTERACTION_ANCHORS),
        ("geometry expected_interaction_anchor_bindings", qc.get("expected_interaction_anchor_bindings"), EXPECTED_INTERACTION_ANCHORS),
        ("profile_contract_failure_count", qc.get("profile_contract_failure_count"), 0),
        ("placement_failure_count", qc.get("placement_failure_count"), 0),
        ("geometry failure_count", qc.get("failure_count"), 0),
        ("expected_cooked_policy_assets", counts.get("expected_cooked_policy_assets"), EXPECTED_GAMEPLAY_ASSETS),
        ("cooked_policy_assets", counts.get("cooked_policy_assets"), EXPECTED_GAMEPLAY_ASSETS),
        ("expected cooked interaction anchors", counts.get("expected_interaction_anchor_bindings"), EXPECTED_INTERACTION_ANCHORS),
        ("cooked_interaction_anchor_bindings", counts.get("cooked_interaction_anchor_bindings"), EXPECTED_INTERACTION_ANCHORS),
    ]
    for name, actual, expected in exact:
        if actual != expected:
            errors.append(f"release gate {name} must be {expected}, got {actual!r}")
    if qc.get("status") != "PASS":
        errors.append(f"release gate geometry status must be PASS, got {qc.get('status')!r}")
    return errors


def _write_cooked_policy_evidence(repo_root: Path, counts: dict) -> None:
    validation_dir = repo_root / "Art" / "Validation"
    validation_dir.mkdir(parents=True, exist_ok=True)
    evidence = {
        "schema": 1,
        "status": "PASS",
        "hasset_version": 2,
        "checked_policy_fields": list(POLICY_FIELDS),
        **counts,
    }
    (validation_dir / "cooked_policy_parity.json").write_text(
        json.dumps(evidence, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    audit_path = validation_dir / "library_release_audit_v1.md"
    if not audit_path.is_file():
        raise AssertionError(f"cannot update missing generated audit {audit_path}")
    lines = audit_path.read_text(encoding="utf-8").splitlines()
    lines = [
        line for line in lines
        if not line.startswith("- Cooked Hasset v2 policy parity:")
        and not line.startswith("- Cooked interaction anchors:")
    ]
    insertion = None
    for index, line in enumerate(lines):
        if line.startswith("- Interaction anchors normalized:"):
            insertion = index + 1
            break
    if insertion is None:
        raise AssertionError("generated audit is missing interaction-anchor release gate")
    lines[insertion:insertion] = [
        f"- Cooked Hasset v2 policy parity: **{counts['cooked_policy_assets']} / {counts['expected_cooked_policy_assets']}**",
        f"- Cooked interaction anchors: **{counts['cooked_interaction_anchor_bindings']} / {counts['expected_interaction_anchor_bindings']}**",
    ]
    audit_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cooked-root", type=Path)
    parser.add_argument("--exports-root", type=Path)
    parser.add_argument("--write-audit", action="store_true")
    args = parser.parse_args(argv)

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

    if args.cooked_root is None:
        if args.write_audit:
            print("error: --write-audit requires --cooked-root", file=sys.stderr)
            return 2
        return 0

    cooked_root = args.cooked_root if args.cooked_root.is_absolute() else repo_root / args.cooked_root
    exports_root = args.exports_root or Path("Art/Exports")
    if not exports_root.is_absolute():
        exports_root = repo_root / exports_root

    parity_errors, counts = validate_cooked_policy_parity(exports_root, cooked_root)
    release_errors = _validate_release_evidence(repo_root, counts)
    errors = parity_errors + release_errors
    if errors:
        print(f"Hotel Haven cooked-policy validation FAILED ({len(errors)} issue(s)):")
        for error in errors:
            print(f" - {error}")
        return 1

    if args.write_audit:
        try:
            _write_cooked_policy_evidence(repo_root, counts)
        except AssertionError as exc:
            print(f"Hotel Haven cooked-policy validation FAILED: {exc}")
            return 1

    print("Hotel Haven cooked-policy validation PASSED")
    print(f" - cooked policy parity: {counts['cooked_policy_assets']} / {counts['expected_cooked_policy_assets']}")
    print(
        " - cooked interaction anchors: "
        f"{counts['cooked_interaction_anchor_bindings']} / {counts['expected_interaction_anchor_bindings']}"
    )
    print(" - Hasset v2 policy envelopes match normalized gameplay sidecars")
    return 0


if __name__ == "__main__":
    sys.exit(main())
