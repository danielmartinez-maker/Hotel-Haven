import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from curated_hotel_factory import build_asset
from semantic_asset_quality import variant_signature_failures
from asset_manifest import load_active_manifest


def signature(scene):
    return tuple(sorted((name, tuple(round(float(x), 4) for x in mesh.extents))
                        for name, mesh in scene.geometry.items()))


def test_four_content_domains_have_grounded_distinct_products():
    for batch in range(18, 22):
        for product in range(10):
            variants = []
            for variant in range(5):
                number = (batch - 1) * 50 + product * 5 + variant + 1
                scene = build_asset('catalog name', 'subcategory', 'MAT_WOOD_WARM',
                                    f'HH_A{number:03d}', 'P_FURNITURE_STATIC')
                assert len(scene.geometry) >= 4
                assert abs(float(scene.bounds[0][2])) < 0.001
                assert float(scene.bounds[1][2]) < 3.5
                variants.append(signature(scene))
            assert len(set(variants)) == 5, (batch, product)


def test_curated_factory_rejects_unowned_ids():
    import pytest
    with pytest.raises(ValueError, match='outside'):
        build_asset('wrong', 'none', 'MAT_WOOD_WARM', 'HH_A1051', 'P_FURNITURE_STATIC')


def test_release_gate_detects_duplicate_geometry_in_new_family():
    assert variant_signature_failures({'HH_A851': ('same',), 'HH_A852': ('same',)})


def test_front_details_and_countertop_equipment_are_visible():
    cabinet = build_asset('Cabinet', 'cabinet', 'MAT_WOOD_WARM', 'HH_A856', 'P_FURNITURE_STATIC')
    body_depth = cabinet.geometry['PrimaryForm'].bounds[1][1]
    assert cabinet.geometry['FunctionalDrawer_0'].bounds[1][1] > body_depth
    coffee = build_asset('Coffee', 'coffee', 'MAT_STAINLESS', 'HH_A926', 'P_FURNITURE_STATIC')
    top_height = coffee.geometry['PrimaryForm'].bounds[1][2]
    assert coffee.geometry['FunctionalStation_0'].bounds[1][2] > top_height + .14


def test_products_use_role_appropriate_materials():
    rows = {row[0]: row for _, _, row in load_active_manifest(ROOT).iter_rows()}
    assert rows['HH_A901'][3] == 'MAT_WOOD_WARM'  # dining table
    assert rows['HH_A906'][3] == 'MAT_UPHOLSTERY'  # banquette
    assert rows['HH_A931'][3] == 'MAT_GLASS_CLEAR'  # pastry display
    assert rows['HH_A1001'][3] == 'MAT_VEGETATION'  # planter
