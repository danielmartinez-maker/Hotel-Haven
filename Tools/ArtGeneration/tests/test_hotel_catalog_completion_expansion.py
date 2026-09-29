import json
import re
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from hotel_catalog_completion_factory import build_asset


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_catalog_completion_manifest_reaches_4200_and_matches_family_targets():
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


def test_catalog_completion_batches_have_the_planned_family_mix_and_full_id_range():
    active = load_active_manifest(ROOT)
    expected = {
        75: {'architecture_construction': 50},
        76: {'guest_room_furniture_fixtures': 50},
        77: {'front_of_house_public': 40, 'decor_clutter_signage': 10},
        78: {'decor_clutter_signage': 40, 'exterior_landscaping': 10},
    }
    rows = []
    for batch, families in expected.items():
        entry = active.batch_entries[batch - 1]
        assert entry.generator_family == 'hotel_catalog_completion'
        batch_rows = [(family, row) for family, path, row in active.iter_rows()
                      if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(batch_rows) == 50
        assert Counter(family for family, _row in batch_rows) == families
        rows.extend(batch_rows)
    assert {row[0] for _family, row in rows} == {f'HH_A{i:04d}' for i in range(3701, 3901)}
    for start in range(3701, 3901, 5):
        variants = [row for _family, row in rows
                    if start <= int(row[0].removeprefix('HH_A')) < start + 5]
        assert len(variants) == 5
        names = {re.sub(r'\s+(Compact|Standard|Extended|High Capacity|Grand)$', '', row[1])
                 for row in variants}
        assert len(names) == 1


def test_every_catalog_completion_product_has_five_distinct_valid_variants():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if 75 <= int(Path(path).stem.rsplit('_', 1)[1]) <= 78}
    for start in range(3701, 3901, 5):
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


def test_catalog_completion_semantic_groups_match_the_asset_contract():
    from semantic_asset_quality import DISTINCT_VARIANT_GROUPS

    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    group_ranges = {
        'architecture': (3701, 10),
        'guestroom': (3751, 10),
        'public': (3801, 8),
        'decor': (3841, 10),
        'exterior': (3891, 2),
    }
    expected = set()
    for domain, (first, count) in group_ranges.items():
        for group in range(count):
            name = f'catalog_completion_{domain}_{group}'
            expected.add(name)
            first_id = first + group * 5
            ids = tuple(f'HH_A{i:04d}' for i in range(first_id, first_id + 5))
            assert DISTINCT_VARIANT_GROUPS[name] == ids
            assert tuple(contract['variant_groups'][name]) == ids
    assert expected <= DISTINCT_VARIANT_GROUPS.keys()
    assert expected <= contract['variant_groups'].keys()


def test_catalog_completion_factory_rejects_ids_outside_its_owned_range():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A3700', 'P_FURNITURE_STATIC')
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A3901', 'P_FURNITURE_STATIC')
