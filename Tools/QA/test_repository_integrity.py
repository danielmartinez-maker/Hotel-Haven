from __future__ import annotations

from collections import Counter
import json
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
DEFS = ROOT / "GameData" / "AssetDefinitions"
MASTER_PATH = DEFS / "hotel_haven_asset_manifest_v1.json"
MATERIALS_PATH = DEFS / "material_families_v1.json"
ANIMATIONS_PATH = DEFS / "animation_sets_v1.json"
PRODUCTION_PATH = DEFS / "production_batches_v1.json"
ID_RE = re.compile(r"^HH_A(\d{3})$")


def _load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _all_rows() -> tuple[dict, list[tuple[int, str, list]]]:
    master = _load(MASTER_PATH)
    rows: list[tuple[int, str, list]] = []
    for batch in master["batches"]:
        path = ROOT / batch["path"]
        data = _load(path)
        for group in data["groups"]:
            for row in group["assets"]:
                rows.append((data["batch"], group["family"], row))
    return master, rows


def test_every_asset_definition_json_parses() -> None:
    json_files = sorted(DEFS.rglob("*.json"))
    assert json_files
    for path in json_files:
        _load(path)


def test_master_manifest_and_shards_cover_exactly_500_assets() -> None:
    master, rows = _all_rows()
    assert master["asset_count"] == 500
    assert len(master["batches"]) == 10
    assert len(rows) == 500

    ids = [row[2][0] for row in rows]
    assert len(set(ids)) == 500
    assert ids == [f"HH_A{i:03d}" for i in range(1, 501)]

    for batch_info in master["batches"]:
        path = ROOT / batch_info["path"]
        assert path.is_relative_to(ROOT)
        assert path.exists()
        data = _load(path)
        shard_rows = [row for group in data["groups"] for row in group["assets"]]
        assert data["schema"] == 1
        assert data["batch"] == batch_info["batch"]
        assert data["asset_count"] == batch_info["asset_count"] == len(shard_rows) == 50
        assert data["columns"] == master["entry_columns"]


def test_asset_rows_have_valid_shapes_and_references() -> None:
    master, rows = _all_rows()
    materials = {m["material_family_id"] for m in _load(MATERIALS_PATH)["families"]}
    animation_data = _load(ANIMATIONS_PATH)
    animation_sets = {a["animation_set_id"] for a in animation_data["sets"]}
    profiles = set(master["profiles"])

    display_names: list[str] = []
    for batch, family, row in rows:
        assert len(row) == len(master["entry_columns"]) == 7, (batch, row)
        asset_id, display_name, subcategory, material, profile, animation_set, anchors = row
        match = ID_RE.fullmatch(asset_id)
        assert match, asset_id
        assert display_name.strip() == display_name and display_name
        display_names.append(display_name)
        assert subcategory.strip() == subcategory and subcategory
        assert material in materials, (asset_id, material)
        assert profile in profiles, (asset_id, profile)
        assert animation_set is None or animation_set in animation_sets, (asset_id, animation_set)
        assert isinstance(anchors, list)
        assert len(anchors) == len(set(anchors)), asset_id
        assert all(isinstance(a, str) and a.startswith("INT_") for a in anchors), asset_id
        assert family in master["family_targets"], (asset_id, family)

    assert len(display_names) == len(set(display_names)), "duplicate asset display names"


def test_family_targets_match_actual_catalog_distribution() -> None:
    master, rows = _all_rows()
    counts = Counter(family for _batch, family, _row in rows)
    assert dict(counts) == master["family_targets"]
    assert sum(master["family_targets"].values()) == master["asset_count"] == 500


def test_animation_and_profile_contracts_are_coherent() -> None:
    master, rows = _all_rows()
    animation_data = _load(ANIMATIONS_PATH)
    skeletons = {s["skeleton_id"] for s in animation_data["skeletons"]}
    sets = animation_data["sets"]
    assert len({s["animation_set_id"] for s in sets}) == len(sets)
    assert all(s["skeleton_id"] in skeletons for s in sets)
    assert all(len(s["clips"]) == len(set(s["clips"])) and s["clips"] for s in sets)

    animated_types = {"SkinnedMeshAsset"}
    for _batch, _family, row in rows:
        asset_id, _name, _subcategory, _material, profile, animation_set, _anchors = row
        declared_type = master["profiles"][profile]["asset_type"]
        if animation_set is not None:
            assert declared_type in animated_types, (asset_id, profile, declared_type, animation_set)
        if declared_type in animated_types:
            assert animation_set is not None, (asset_id, profile)


def test_material_library_ranges_and_ids_are_valid() -> None:
    data = _load(MATERIALS_PATH)
    families = data["families"]
    ids = [m["material_family_id"] for m in families]
    assert len(ids) == len(set(ids))
    for material in families:
        lo, hi = material["roughness_range"]
        assert 0.0 <= lo <= hi <= 1.0, material["material_family_id"]
        assert isinstance(material["metallic"], bool)


def test_production_plan_is_consistent_with_master_manifest() -> None:
    master = _load(MASTER_PATH)
    production = _load(PRODUCTION_PATH)
    assert production["batch_size"] == 50
    assert len(production["batches"]) == len(master["batches"]) == 10
    for expected, planned in zip(master["batches"], production["batches"], strict=True):
        assert planned["batch"] == expected["batch"]
        assert planned["asset_count"] == expected["asset_count"]
        assert planned["catalog"] == expected["path"]
        assert planned["catalog_status"] == "LOCKED"
