import json
import re
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from design_completion_factory import build_asset


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_asset_catalog_declares_3900_assets_and_balanced_family_targets():
    active = load_active_manifest(ROOT)
    assert active.asset_count == 4200
    assert active.batch_numbers == tuple(range(1, 85))
    counts = Counter(family for family, _path, _row in active.iter_rows())
    assert counts == {
        'architecture_construction': 650,
        'finish_systems': 200,
        'guest_room_furniture_fixtures': 700,
        'front_of_house_public': 500,
        'restaurant_bar_food_service': 500,
        'housekeeping_maintenance_logistics': 450,
        'amenities_events': 400,
        'decor_clutter_signage': 350,
        'exterior_landscaping': 250,
        'guests_staff': 200,
    }


def test_design_completion_batches_define_ten_five_variant_products_per_domain():
    active = load_active_manifest(ROOT)
    expected = {
        66: 'architecture_construction',
        67: 'guest_room_furniture_fixtures',
        68: 'front_of_house_public',
        69: 'amenities_events',
        70: 'decor_clutter_signage',
    }
    rows = []
    for batch, family in expected.items():
        entry = active.batch_entries[batch - 1]
        assert entry.generator_family == 'design_completion'
        batch_rows = [(row_family, row) for row_family, path, row in active.iter_rows()
                      if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(batch_rows) == 50
        assert Counter(row_family for row_family, _row in batch_rows) == {family: 50}
        rows.extend(batch_rows)
    for start in range(3251, 3501, 5):
        variants = [row for _family, row in rows
                    if start <= int(row[0].removeprefix('HH_A')) < start + 5]
        assert len(variants) == 5
        product_names = {re.sub(r'\s+(Compact|Standard|Extended|High Capacity|Grand)$', '', row[1])
                         for row in variants}
        assert len(product_names) == 1


def test_each_design_completion_product_has_five_distinct_valid_variants():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if 66 <= int(Path(path).stem.rsplit('_', 1)[1]) <= 70}
    for start in range(3251, 3501, 5):
        signatures = []
        for number in range(start, start + 5):
            row = rows[f'HH_A{number:04d}']
            scene = build_asset(row[1], row[2], row[3], row[0], row[4])
            assert len(scene.geometry) >= 5
            assert sum(len(geometry.faces) for geometry in scene.geometry.values()) <= 5000
            assert float(scene.bounds[0][2]) >= -0.02
            if row[4] == 'P_FURNITURE_STATIC':
                assert float(scene.bounds[0][2]) <= 0.12
            assert float(scene.bounds[1][2]) <= 3.5
            signatures.append(shape_signature(scene))
        assert len(set(signatures)) == 5, f'product group starting at A{start}'


def test_design_completion_variant_contract_tracks_all_new_products():
    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    declared = {asset_id for ids in contract['variant_groups'].values() for asset_id in ids}
    for number in range(3251, 3501):
        assert f'HH_A{number:04d}' in declared


def test_design_completion_semantic_variant_groups_match_asset_contract():
    from semantic_asset_quality import DISTINCT_VARIANT_GROUPS

    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    expected = {f'completion_{domain}_{group}'
                for domain in ('architecture', 'guestroom', 'public', 'amenity', 'decor')
                for group in range(10)}
    assert expected <= DISTINCT_VARIANT_GROUPS.keys()
    assert expected <= contract['variant_groups'].keys()
    for group_name in expected:
        assert DISTINCT_VARIANT_GROUPS[group_name] == tuple(contract['variant_groups'][group_name])


def test_design_completion_factory_rejects_ids_outside_its_owned_range():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A3250', 'P_FURNITURE_STATIC')
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A3501', 'P_FURNITURE_STATIC')
