import json
import re
import sys
from collections import Counter
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from hotel_final_catalog_factory import SCALE, build_asset


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def normalized_shape_signature(scene, scale):
    return tuple(sorted(
        (str(name), tuple(round(float(value) / scale, 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def component_signature(scene):
    return tuple(sorted(str(name) for name in scene.geometry))


def test_final_catalog_manifest_reaches_4200_with_spec_family_targets():
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


def test_final_catalog_batches_79_to_82_match_the_planned_mixed_family_ranges():
    active = load_active_manifest(ROOT)
    expected = {
        79: {'architecture_construction': 50},
        80: {'architecture_construction': 30, 'guest_room_furniture_fixtures': 20},
        81: {'guest_room_furniture_fixtures': 35, 'decor_clutter_signage': 15},
        82: {'decor_clutter_signage': 35, 'finish_systems': 5,
             'restaurant_bar_food_service': 5, 'amenities_events': 3,
             'exterior_landscaping': 2},
    }
    seen = []
    for batch, families in expected.items():
        entry = active.batch_entries[batch - 1]
        assert entry.generator_family == 'hotel_final_catalog'
        rows = [(family, row) for family, path, row in active.iter_rows()
                if Path(path).stem == f'asset_batch_{batch:02d}']
        assert len(rows) == 50
        assert Counter(family for family, _row in rows) == families
        seen.extend(row for _family, row in rows)
    assert {row[0] for row in seen} == {f'HH_A{i:04d}' for i in range(3901, 4101)}


def test_final_catalog_five_variant_groups_are_distinct_and_within_geometry_budgets():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if 79 <= int(Path(path).stem.rsplit('_', 1)[1]) <= 82}
    variant_ranges = [(3901, 16), (3981, 11), (4036, 10), (4086, 1), (4091, 1)]
    for first, group_count in variant_ranges:
        for group in range(group_count):
            start = first + group * 5
            signatures = []
            base_names = set()
            for number in range(start, start + 5):
                row = rows[f'HH_A{number:04d}']
                base_names.add(re.sub(r'\s+(Compact|Standard|Extended|High Capacity|Grand)$', '', row[1]))
                scene = build_asset(row[1], row[2], row[3], row[0], row[4])
                assert len(scene.geometry) >= 4
                assert sum(len(geometry.faces) for geometry in scene.geometry.values()) <= 5000
                assert float(scene.bounds[0][2]) >= -0.02
                assert float(scene.bounds[1][2]) <= 3.5
                if row[4] == 'P_FURNITURE_STATIC':
                    assert float(scene.bounds[0][2]) <= 0.12
                signatures.append(shape_signature(scene))
            assert len(base_names) == 1
            assert len(set(signatures)) == 5, f'variant group starting A{start}'


def test_singleton_amenity_and_exterior_catalog_assets_are_individual_products():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if Path(path).stem in {'asset_batch_82'}}
    singleton_ids = [f'HH_A{i:04d}' for i in range(4096, 4101)]
    names = [rows[asset_id][1] for asset_id in singleton_ids]
    assert len(set(names)) == 5
    assert all(not name.endswith((' Compact', ' Standard', ' Extended', ' High Capacity', ' Grand'))
               for name in names)
    signatures = [shape_signature(build_asset(row[1], row[2], row[3], row[0], row[4]))
                  for asset_id in singleton_ids for row in [rows[asset_id]]]
    assert len(set(signatures)) == 5


def test_final_catalog_semantic_variant_groups_match_quality_contract():
    from semantic_asset_quality import DISTINCT_VARIANT_GROUPS

    contract = json.loads((ROOT / 'GameData/AssetDefinitions/asset_quality_contract_v2.json').read_text())
    ranges = {'architecture': (3901, 16), 'guestroom': (3981, 11), 'decor': (4036, 10),
              'finish': (4086, 1), 'restaurant': (4091, 1)}
    for domain, (first, count) in ranges.items():
        for group in range(count):
            key = f'final_catalog_{domain}_{group}'
            ids = tuple(f'HH_A{i:04d}' for i in range(first + group * 5, first + group * 5 + 5))
            assert DISTINCT_VARIANT_GROUPS[key] == ids
            assert tuple(contract['variant_groups'][key]) == ids


def test_guestroom_completion_batches_83_84_have_shape_changes_beyond_global_scale():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _family, path, row in active.iter_rows()
            if Path(path).stem in {'asset_batch_83', 'asset_batch_84'}}
    assert set(rows) == {f'HH_A{i:04d}' for i in range(4101, 4201)}

    weak_groups = []
    structurally_repeated_groups = []
    for first in range(4101, 4201, 5):
        signatures = []
        components = []
        base_names = set()
        for variant, number in enumerate(range(first, first + 5)):
            row = rows[f'HH_A{number:04d}']
            base_names.add(re.sub(r' (Compact|Standard|Extended|High Capacity|Grand)$', '', row[1]))
            scene = build_asset(row[1], row[2], row[3], row[0], row[4])
            signatures.append(normalized_shape_signature(scene, SCALE[variant]))
            components.append(component_signature(scene))
        assert len(base_names) == 1, f'variant group starting A{first}'
        if len(set(signatures)) != 5:
            weak_groups.append(f'A{first}–A{first + 4}')
        if len(set(components)) != 5:
            structurally_repeated_groups.append(f'A{first}–A{first + 4}')

    assert not weak_groups, f'variant groups rely on global scale alone: {weak_groups}'
    assert not structurally_repeated_groups, (
        f'variant groups lack variant-specific visible components: {structurally_repeated_groups}'
    )


def test_guestroom_completion_variants_pass_the_coarse_preview_distinctness_gate(tmp_path):
    from render_asset_previews import preview_quality_failures, render_asset

    rows = _final_catalog_rows_by_number()
    rendered = {}
    for number in range(4101, 4201):
        row = rows[number]
        export = tmp_path / f'{row[0]}.glb'
        _build_row(row).export(export)
        rendered[row[0]] = render_asset(export, size=160)

    assert preview_quality_failures(rendered, expected_count=100) == []


def _final_catalog_rows_by_number():
    return {
        int(row[0].removeprefix('HH_A')): row
        for _family, _path, row in load_active_manifest(ROOT).iter_rows()
        if 3901 <= int(row[0].removeprefix('HH_A')) <= 4200
    }


def _build_row(row):
    return build_asset(row[1], row[2], row[3], row[0], row[4])


def test_pantry_doors_pulls_and_inventory_tag_project_beyond_the_case_front():
    rows = _final_catalog_rows_by_number()
    for number in range(3986, 3991):
        scene = _build_row(rows[number])
        case_front = float(scene.geometry['PantryTowerCase'].bounds[0][1])
        detail_names = [name for name in scene.geometry
                        if name.startswith('PantryDoor_') or name.startswith('PantryPull_')]
        detail_names.append('PantryInventoryTag')
        for name in detail_names:
            detail_front = float(scene.geometry[name].bounds[0][1])
            assert detail_front < case_front - .02, f'{rows[number][0]}:{name} is buried in the case'


def test_safe_door_keypad_and_charging_ports_project_beyond_opaque_fronts():
    rows = _final_catalog_rows_by_number()
    for number in range(4196, 4201):
        scene = _build_row(rows[number])
        safe_front = float(scene.geometry['GuestSafeCase'].bounds[0][1])
        for name in ('GuestSafeDoor', 'SafeKeypad'):
            detail_front = float(scene.geometry[name].bounds[0][1])
            assert detail_front < safe_front - .02, f'{rows[number][0]}:{name} is buried in the safe'
        drawer_front = float(scene.geometry['ChargingDrawer'].bounds[0][1])
        for name, geometry in scene.geometry.items():
            if name.startswith('GuestDeviceChargingPort_'):
                assert float(geometry.bounds[0][1]) < drawer_front - .02, (
                    f'{rows[number][0]}:{name} is buried in the charging drawer'
                )


@pytest.mark.parametrize(('start', 'mattress_names'), [
    (4121, ('RollawayCotMattress',)),
    (4166, ('BunkLowerMattress', 'BunkUpperMattress')),
])
def test_guest_beds_have_usable_mattress_footprints(start, mattress_names):
    rows = _final_catalog_rows_by_number()
    for number in range(start, start + 5):
        scene = _build_row(rows[number])
        for name in mattress_names:
            width, length = scene.geometry[name].extents[:2]
            assert float(width) >= .78, f'{rows[number][0]}:{name} is too narrow ({width:.3f} m)'
            assert float(length) >= 1.80, f'{rows[number][0]}:{name} is too short ({length:.3f} m)'
