import sys
import re
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from extended_hotel_factory import build_asset
from semantic_asset_quality import variant_signature_failures


def shape_signature(scene):
    return tuple(sorted(
        (str(name), tuple(round(float(value), 4) for value in geometry.extents))
        for name, geometry in scene.geometry.items()
    ))


def test_new_tranche_declares_ten_product_families_per_batch():
    active = load_active_manifest(ROOT)
    assert active.asset_count == 3900
    assert active.batch_numbers == tuple(range(1, 79))
    rows_by_batch = {entry.batch: [] for entry in active.batch_entries}
    for _family, path, row in active.iter_rows():
        batch = int(Path(path).stem.rsplit('_', 1)[1])
        rows_by_batch[batch].append(row)
    for batch in range(22, 26):
        assert len(rows_by_batch[batch]) == 50
        product_names = set()
        for index in range(10):
            variants = rows_by_batch[batch][index * 5:(index + 1) * 5]
            normalized = {
                re.sub(r'\s+(Compact|Standard|Extended|High Capacity|Grand)$', '', row[1]).strip()
                for row in variants
            }
            normalized = {
                re.sub(r'\b(Man|Woman|Boy|Girl)\b', '', name).strip()
                for name in normalized
            }
            assert len(normalized) == 1
            product_names.update(normalized)
        assert len(product_names) == 10
        assert active.batch_entries[batch - 1].generator_family == 'extended_hotel'


@pytest.mark.parametrize('start', [1051, 1101, 1151, 1201])
def test_each_product_family_has_five_distinct_grounded_variants(start):
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _, _, row in active.iter_rows()}
    for family in range(10):
        variants = []
        for offset in range(5):
            number = start + family * 5 + offset
            row = rows[f'HH_A{number:04d}']
            scene = build_asset(row[1], row[2], row[3], row[0], row[4])
            assert len(scene.geometry) >= 5
            assert float(scene.bounds[0][2]) >= -0.02
            assert float(scene.bounds[0][2]) <= 0.12
            assert float(scene.bounds[1][2]) <= 3.5
            variants.append(shape_signature(scene))
        assert len(set(variants)) == 5, f'family starting at A{start + family * 5}'


def test_new_characters_keep_skeleton_nodes_and_role_specific_animation_sets():
    active = load_active_manifest(ROOT)
    rows = {row[0]: row for _, _, row in active.iter_rows()}
    required = {'Hips', 'Spine', 'Chest', 'Head', 'UpperArm_L', 'UpperLeg_R', 'Root'}
    for number in (1051, 1061, 1096, 1101, 1121, 1146):
        row = rows[f'HH_A{number:04d}']
        scene = build_asset(row[1], row[2], row[3], row[0], row[4])
        assert required <= set(map(str, scene.graph.nodes_geometry))
        assert row[5] in {'ANSET_GUEST_LOCOMOTION', 'ANSET_GUEST_ROOM', 'ANSET_CHILD_GUEST', 'ANSET_FRONT_DESK',
                          'ANSET_BELL_SERVICE', 'ANSET_HOUSEKEEPING', 'ANSET_KITCHEN',
                          'ANSET_RESTAURANT_SERVICE', 'ANSET_MAINTENANCE', 'ANSET_SECURITY',
                          'ANSET_MANAGER'}


def test_release_variant_audit_tracks_all_new_five_variant_sets():
    failures = variant_signature_failures({
        'HH_A1051': ('same',),
        'HH_A1052': ('same',),
    })
    assert any('HH_A1051 and HH_A1052' in failure for failure in failures)


def test_extended_factory_rejects_asset_ids_outside_its_batches():
    with pytest.raises(ValueError, match='outside'):
        build_asset('Unowned', 'none', 'MAT_WOOD_WARM', 'HH_A1251', 'P_FURNITURE_STATIC')
