import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

import render_asset_previews as previews
import validate_generated_geometry as geometry_qc
from amenity_decor_factory import build_asset
from character_factory import build_character


def node_names(scene):
    return set(map(str, scene.graph.nodes_geometry))


def scene_signature(scene):
    faces = sum(len(g.faces) for g in scene.geometry.values() if hasattr(g, 'faces'))
    extents = tuple(np.round(scene.bounds[1] - scene.bounds[0], 3).tolist())
    return (len(scene.geometry), faces, extents)


def test_batch08_amenities_have_management_camera_readable_role_geometry():
    requirements = {
        'Yoga Mat Rack': {'YogaMat_0', 'YogaMat_1'},
        'Spa Treatment Chair': {'Pedestal', 'Footrest'},
        'Pool Lounge Chair': {'PoolFrame'},
    }
    for name, required in requirements.items():
        nodes = node_names(build_asset(name, 'MAT_UPHOLSTERY'))
        assert required <= nodes, (name, required - nodes)

    classic = build_asset('Wall Clock Classic', 'MAT_SIGNAGE')
    modern = build_asset('Wall Clock Modern', 'MAT_SIGNAGE')
    assert scene_signature(classic) != scene_signature(modern)


def test_batch09_signage_and_exterior_variants_are_semantically_distinct():
    requirements = {
        'Directional Sign Ceiling': {'CeilingMount'},
        'Bike Rack': {'RackLoop_0'},
        'Planter Exterior Rectangular': {'PlanterBox'},
        'Hedge Corner': {'HedgeLeg_A', 'HedgeLeg_B'},
    }
    for name, required in requirements.items():
        nodes = node_names(build_asset(name, 'MAT_BLACKENED_STEEL'))
        assert required <= nodes, (name, required - nodes)

    round_planter = build_asset('Planter Exterior Large', 'MAT_VEGETATION')
    rectangular_planter = build_asset('Planter Exterior Rectangular', 'MAT_VEGETATION')
    assert scene_signature(round_planter) != scene_signature(rectangular_planter)


def test_batch10_guest_and_staff_archetypes_expose_distinctive_accessories():
    requirements = {
        'Guest Businessman': 'Accessory_Briefcase',
        'Guest Family Parent A': 'Accessory_FamilyTote',
        'Guest Child Boy': 'Accessory_ChildBackpack',
        'Maintenance Technician Man': 'Accessory_ToolPouch',
        'Security Officer Woman': 'Accessory_Radio',
    }
    for index, (name, required) in enumerate(requirements.items()):
        nodes = node_names(build_character(name, index))
        assert required in nodes, (name, required)


def test_semantic_library_gate_rejects_generic_fallbacks_and_duplicate_variants():
    semantic = getattr(geometry_qc, 'semantic_contract_failures', None)
    variants = getattr(geometry_qc, 'variant_signature_failures', None)
    assert callable(semantic)
    assert callable(variants)

    failures = semantic('HH_A448', {'Body'})
    assert failures and 'Bike Rack' in failures[0]
    assert semantic('HH_A448', {'RackBase', 'RackLoop_0'}) == []

    duplicate = variants({'HH_A392': ('same',), 'HH_A393': ('same',)})
    assert duplicate and 'HH_A392' in duplicate[0] and 'HH_A393' in duplicate[0]
    assert variants({'HH_A392': ('classic',), 'HH_A393': ('modern',)}) == []


def test_preview_release_gate_rejects_blank_and_duplicate_required_variants():
    content_ratio = getattr(previews, 'image_content_ratio', None)
    preview_failures = getattr(previews, 'preview_quality_failures', None)
    assert callable(content_ratio)
    assert callable(preview_failures)

    blank = Image.new('RGB', (48, 48), previews.BG)
    drawn = blank.copy()
    ImageDraw.Draw(drawn).rectangle((10, 10, 36, 36), fill=(80, 80, 80))
    drawn_alt = blank.copy()
    ImageDraw.Draw(drawn_alt).ellipse((8, 8, 38, 38), fill=(80, 80, 80))

    assert content_ratio(blank) == 0.0
    assert content_ratio(drawn) > 0.10

    failures = preview_failures({'HH_A392': drawn, 'HH_A393': drawn.copy()}, expected_count=2)
    assert any('duplicate preview' in failure for failure in failures)

    assert preview_failures({'HH_A392': drawn, 'HH_A393': drawn_alt}, expected_count=2) == []
