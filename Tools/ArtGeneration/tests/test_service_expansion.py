import json
import re
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from service_expansion_factory import build_asset
from validate_generated_geometry import CONTEXTUAL_PLACEMENT_OVERRIDES


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_service_expansion_declares_five_new_batches_and_balanced_family_targets():
    active = load_active_manifest(ROOT)
    assert active.asset_count == 2500
    assert active.batch_numbers == tuple(range(1, 51))
    counts = Counter(family for family, _path, _row in active.iter_rows())
    assert counts == {
        'architecture_construction': 320,
        'finish_systems': 140,
        'guest_room_furniture_fixtures': 295,
        'front_of_house_public': 260,
        'restaurant_bar_food_service': 345,
        'housekeeping_maintenance_logistics': 355,
        'amenities_events': 252,
        'decor_clutter_signage': 145,
        'exterior_landscaping': 188,
        'guests_staff': 200,
    }


def test_service_batches_are_distinct_domains_with_ten_product_families_each():
    active = load_active_manifest(ROOT)
    expected = {
        41: 'restaurant_bar_food_service',
        42: 'restaurant_bar_food_service',
        43: 'housekeeping_maintenance_logistics',
        44: 'housekeeping_maintenance_logistics',
        45: 'architecture_construction',
    }
    for batch, family in expected.items():
        entry = active.batch_entries[batch - 1]
        assert entry.generator_family == 'service_expansion'
        rows = [(row_family, row) for row_family, path, row in active.iter_rows()
                if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(rows) == 50
        assert Counter(row_family for row_family, _row in rows) == {family: 50}
        for group in range(10):
            variants = rows[group * 5:(group + 1) * 5]
            names = {re.sub(r'\s+(Compact|Standard|Extended|High Capacity|Grand)$', '', row[1])
                     for _row_family, row in variants}
            assert len(names) == 1


def test_every_service_product_has_five_distinct_grounded_variants():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if 41 <= int(Path(path).stem.rsplit('_', 1)[1]) <= 45}
    for start in range(2001, 2251, 5):
        signatures = []
        for number in range(start, start + 5):
            row = rows[f'HH_A{number:04d}']
            scene = build_asset(row[1], row[2], row[3], row[0], row[4])
            assert len(scene.geometry) >= 5
            assert float(scene.bounds[0][2]) >= -0.02
            assert float(scene.bounds[1][2]) <= 3.5
            signatures.append(shape_signature(scene))
        assert len(set(signatures)) == 5, f'family starting at A{start}'


def test_quality_contract_tracks_all_service_variant_groups():
    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    declared = {asset_id for ids in contract['variant_groups'].values() for asset_id in ids}
    for start in range(2001, 2251, 5):
        assert all(f'HH_A{number:04d}' in declared for number in range(start, start + 5))


def test_service_factory_rejects_ids_outside_its_owned_range():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A2000', 'P_FURNITURE_STATIC')
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A2251', 'P_FURNITURE_STATIC')


def test_ceiling_suspended_pot_rack_documents_its_contextual_placement():
    assert all(CONTEXTUAL_PLACEMENT_OVERRIDES[f'HH_A{number:04d}'] ==
               'ceiling-suspended commercial pot rack'
               for number in range(2061, 2066))
