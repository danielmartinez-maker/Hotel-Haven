import json
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from catalog_expansion_factory import build_asset
from validate_generated_geometry import CONTEXTUAL_PLACEMENT_OVERRIDES


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_catalog_expansion_manifest_balances_all_five_hotel_families():
    active = load_active_manifest(ROOT)
    assert active.asset_count == 3000
    assert active.batch_numbers == tuple(range(1, 61))
    counts = Counter(family for family, _path, _row in active.iter_rows())
    assert counts == {
        'architecture_construction': 370,
        'finish_systems': 190,
        'guest_room_furniture_fixtures': 345,
        'front_of_house_public': 310,
        'restaurant_bar_food_service': 445,
        'housekeeping_maintenance_logistics': 405,
        'amenities_events': 302,
        'decor_clutter_signage': 195,
        'exterior_landscaping': 238,
        'guests_staff': 200,
    }


def test_catalog_batches_split_characters_and_four_service_domains():
    active = load_active_manifest(ROOT)
    expected = {
        26: Counter({'guests_staff': 30, 'guest_room_furniture_fixtures': 20}),
        27: Counter({'guest_room_furniture_fixtures': 50}),
        28: Counter({'restaurant_bar_food_service': 50}),
        29: Counter({'housekeeping_maintenance_logistics': 50}),
        30: Counter({'decor_clutter_signage': 50}),
    }
    for batch, family_counts in expected.items():
        entry = active.batch_entries[batch - 1]
        assert entry.generator_family == 'catalog_expansion'
        rows = [(family, row) for family, path, row in active.iter_rows()
                if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(rows) == 50
        assert Counter(family for family, _row in rows) == family_counts


def test_every_new_product_family_has_five_distinct_grounded_variants():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if int(Path(path).stem.rsplit('_', 1)[1]) >= 26}
    group_starts = list(range(1251, 1281, 5)) + list(range(1281, 1351, 5))
    group_starts += list(range(1351, 1401, 5)) + list(range(1401, 1451, 5))
    group_starts += list(range(1451, 1501, 5))
    assert len(group_starts) == 50
    for start in group_starts:
        variants = []
        for number in range(start, start + 5):
            row = rows[f'HH_A{number:04d}']
            scene = build_asset(row[1], row[2], row[3], row[0], row[4])
            assert len(scene.geometry) >= 5
            assert float(scene.bounds[0][2]) >= -0.02
            assert float(scene.bounds[1][2]) <= 3.5
            variants.append(shape_signature(scene))
        assert len(set(variants)) == 5, f'family starting at A{start}'


def test_character_roles_and_role_specific_details_are_preserved():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if Path(path).stem in {'asset_batch_26', 'asset_batch_27'}}
    expected = {
        'HH_A1251': ('ANSET_GUEST_LOCOMOTION', 'Accessory_Boutonniere'),
        'HH_A1256': ('ANSET_GUEST_LOCOMOTION', 'Accessory_Cane'),
        'HH_A1261': ('ANSET_HOUSEKEEPING', 'Accessory_SpaTunic'),
        'HH_A1266': ('ANSET_SECURITY', 'Accessory_RescueCan'),
        'HH_A1271': ('ANSET_MANAGER', 'Accessory_EventHeadset'),
        'HH_A1276': ('ANSET_BELL_SERVICE', 'Accessory_ValetCap'),
    }
    for asset_id, (animation_set, detail) in expected.items():
        row = rows[asset_id]
        assert row[5] == animation_set
        scene = build_asset(row[1], row[2], row[3], asset_id, row[4])
        assert detail in set(map(str, scene.graph.nodes_geometry))
        assert {'Root', 'Hips', 'Spine', 'Head'} <= set(map(str, scene.graph.nodes))


def test_quality_contract_tracks_each_new_five_variant_group():
    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    starts = list(range(1251, 1281, 5)) + list(range(1281, 1351, 5))
    starts += list(range(1351, 1401, 5)) + list(range(1401, 1451, 5))
    starts += list(range(1451, 1501, 5))
    declared_ids = {asset_id for ids in contract['variant_groups'].values() for asset_id in ids}
    for start in starts:
        assert all(f'HH_A{number:04d}' in declared_ids for number in range(start, start + 5))


def test_catalog_factory_rejects_ids_outside_its_owned_range():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A1250', 'P_FURNITURE_STATIC')
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A1501', 'P_FURNITURE_STATIC')


def test_wall_and_ceiling_mounted_decor_have_documented_contextual_placement():
    assert all(CONTEXTUAL_PLACEMENT_OVERRIDES[f'HH_A{number:04d}'] == 'wall-mounted lobby sconce'
               for number in range(1456, 1461))
    assert all(CONTEXTUAL_PLACEMENT_OVERRIDES[f'HH_A{number:04d}'] == 'ceiling-mounted lobby chandelier'
               for number in range(1461, 1466))
