import json
import re
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from hotel_continuation_factory import build_asset


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_active_manifest_counts_and_spec_aligned_family_targets():
    active = load_active_manifest(ROOT)
    assert active.asset_count == 3900
    assert active.batch_numbers == tuple(range(1, 79))
    counts = Counter(family for family, _path, _row in active.iter_rows())
    assert counts == {
        'architecture_construction': 570,
        'finish_systems': 195,
        'guest_room_furniture_fixtures': 545,
        'front_of_house_public': 500,
        'restaurant_bar_food_service': 495,
        'housekeeping_maintenance_logistics': 450,
        'amenities_events': 397,
        'decor_clutter_signage': 300,
        'exterior_landscaping': 248,
        'guests_staff': 200,
    }


def test_hotel_continuation_batches_and_products_have_declared_five_variants():
    active = load_active_manifest(ROOT)
    expected = {
        61: {'architecture_construction': 50},
        62: {'guest_room_furniture_fixtures': 50},
        63: {'front_of_house_public': 50},
        64: {'restaurant_bar_food_service': 50},
        65: {'housekeeping_maintenance_logistics': 45, 'finish_systems': 5},
    }
    continuation_rows = []
    for batch, families in expected.items():
        entry = active.batch_entries[batch - 1]
        assert entry.generator_family == 'hotel_continuation'
        rows = [(family, row) for family, path, row in active.iter_rows()
                if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(rows) == 50
        assert Counter(family for family, _row in rows) == families
        continuation_rows.extend(rows)
    for start in range(3001, 3251, 5):
        rows = [row for _family, row in continuation_rows
                if start <= int(row[0].removeprefix('HH_A')) < start + 5]
        assert len(rows) == 5, f'product group starting at A{start}'
        assert len({re.sub(r'\s+(Compact|Standard|Extended|High Capacity|Grand)$', '', row[1])
                    for row in rows}) == 1


def test_every_hotel_continuation_product_has_five_distinct_valid_variants():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if 61 <= int(Path(path).stem.rsplit('_', 1)[1]) <= 65}
    for start in range(3001, 3251, 5):
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


def test_hotel_continuation_variant_contract_tracks_all_new_products():
    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    declared = {asset_id for ids in contract['variant_groups'].values() for asset_id in ids}
    for number in range(3001, 3251):
        assert f'HH_A{number:04d}' in declared


def test_hotel_continuation_factory_rejects_ids_outside_its_owned_range():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A3000', 'P_FURNITURE_STATIC')
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A3251', 'P_FURNITURE_STATIC')
