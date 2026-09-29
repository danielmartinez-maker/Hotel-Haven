import json
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from property_expansion_factory import build_asset


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_property_expansion_manifest_balances_all_five_asset_families():
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


def test_property_batches_assign_the_expected_generator_and_content_domain():
    active = load_active_manifest(ROOT)
    expected = {
        31: 'architecture_construction',
        32: 'front_of_house_public',
        33: 'finish_systems',
        34: 'amenities_events',
        35: 'exterior_landscaping',
    }
    for batch, family in expected.items():
        assert active.batch_entries[batch - 1].generator_family == 'property_expansion'
        rows = [(row_family, row) for row_family, path, row in active.iter_rows()
                if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(rows) == 50
        assert Counter(row_family for row_family, _row in rows) == {family: 50}


def test_every_new_property_product_has_five_distinct_grounded_variants():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if 31 <= int(Path(path).stem.rsplit('_', 1)[1]) <= 35}
    starts = list(range(1501, 1751, 5))
    assert len(starts) == 50
    for start in starts:
        signatures = []
        for number in range(start, start + 5):
            row = rows[f'HH_A{number:04d}']
            scene = build_asset(row[1], row[2], row[3], row[0], row[4])
            assert len(scene.geometry) >= 5
            assert float(scene.bounds[0][2]) >= -0.02
            assert float(scene.bounds[1][2]) <= 3.5
            signatures.append(shape_signature(scene))
        assert len(set(signatures)) == 5, f'family starting at A{start}'


def test_quality_contract_tracks_each_new_property_variant_group():
    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    declared = {asset_id for ids in contract['variant_groups'].values() for asset_id in ids}
    for start in range(1501, 1751, 5):
        assert all(f'HH_A{number:04d}' in declared for number in range(start, start + 5))


def test_property_factory_rejects_ids_outside_its_owned_range():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A1500', 'P_ARCH_STATIC')
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A1751', 'P_ARCH_STATIC')


def test_arch_and_parquet_details_read_as_curved_stone_and_raised_woodwork():
    door = build_asset('Arched Guestroom Door Surround', 'architecture', 'MAT_WOOD_WARM',
                       'HH_A1511', 'P_ARCH_STATIC')
    voussoirs = [mesh for name, mesh in door.geometry.items() if name.startswith('ArchVoussoir_')]
    assert len(voussoirs) >= 7
    assert voussoirs[0].visual.material.name == 'MAT_STONE_LIGHT'
    assert voussoirs[0].extents[2] > voussoirs[0].extents[0]

    parquet = build_asset('Chevron Parquet Floor Module Compact', 'finish', 'MAT_WOOD_WARM',
                          'HH_A1616', 'P_ARCH_STATIC')
    backing = parquet.geometry['ParquetBacking']
    chevrons = [mesh for name, mesh in parquet.geometry.items() if name.startswith('ParquetChevron_')]
    assert backing.visual.material.name == 'MAT_WOOD_WARM'
    assert len(chevrons) >= 2
    assert min(mesh.bounds[0][2] for mesh in chevrons) >= backing.bounds[1][2] - 1e-4

    plank_floor = build_asset('Hardwood Plank Floor Panel Compact', 'finish', 'MAT_WOOD_WARM',
                              'HH_A1611', 'P_ARCH_STATIC')
    assert plank_floor.geometry['FloorUnderlay'].visual.material.name == 'MAT_WOOD_WARM'
