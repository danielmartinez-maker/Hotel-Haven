from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
sys.path.insert(0, str(ART))

import generate_all_assets as generation


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


qc = load_module('validate_generated_geometry_v2', ART / 'validate_generated_geometry.py')


def test_gameplay_sidecar_normalizer_applies_profile_contract_and_anchors():
    normalize = getattr(generation, 'normalize_gameplay_sidecar_data', None)
    assert callable(normalize)

    sidecar = {
        'schema': 1,
        'asset_id': 'HH_A251',
        'asset_type': 'StaticMeshAsset',
        'source': 'Tools/ArtGeneration/batch06_generate.py',
        'units': 'centimeters',
        'lod_policy': 'wrong_lod',
        'collision_policy': 'wrong_collision',
        'cutaway_policy': 'wrong_cutaway',
        'material_slots': ['MAT_SERVICE_PAINT'],
        'tags': ['batch_06'],
        'dependencies': [],
    }
    meta = {
        'profile': 'P_FURNITURE_STATIC',
        'interaction_anchors': ['INT_USE_01', 'INT_REPAIR_01'],
    }
    contract = {
        'units': 'meters',
        'lod_policy': 'lod_furniture',
        'collision_policy': 'simple_proxy',
        'cutaway_policy': 'normal',
        'pivot_profile': 'floor_contact_center',
    }

    normalized = normalize(sidecar, meta, contract)

    assert normalized['units'] == 'meters'
    assert normalized['lod_policy'] == 'lod_furniture'
    assert normalized['collision_policy'] == 'simple_proxy'
    assert normalized['cutaway_policy'] == 'normal'
    assert normalized['pivot_profile'] == 'floor_contact_center'
    assert normalized['interaction_anchors'] == ['INT_USE_01', 'INT_REPAIR_01']
    assert 'interaction_anchor:INT_USE_01' in normalized['tags']
    assert 'interaction_anchor:INT_REPAIR_01' in normalized['tags']
    assert normalized['asset_id'] == 'HH_A251'
    assert normalized['dependencies'] == []


def test_profile_contract_gate_accepts_flexible_small_prop_collision_policy():
    check = getattr(qc, 'profile_contract_failures', None)
    assert callable(check)

    meta = {'interaction_anchors': ['INT_USE_01']}
    contract = {
        'units': 'meters',
        'lod_policy': 'lod_small_prop',
        'collision_policy': 'none_or_simple_proxy',
        'cutaway_policy': 'normal',
    }
    base = {
        'units': 'meters',
        'lod_policy': 'lod_small_prop',
        'cutaway_policy': 'normal',
        'interaction_anchors': ['INT_USE_01'],
    }

    assert check('HH_A401', meta, {**base, 'collision_policy': 'none'}, contract) == []
    assert check('HH_A401', meta, {**base, 'collision_policy': 'simple_proxy'}, contract) == []

    failures = check('HH_A401', meta, {**base, 'collision_policy': 'capsule_runtime'}, contract)
    assert len(failures) == 1
    assert 'collision_policy' in failures[0]

    failures = check(
        'HH_A401',
        meta,
        {**base, 'collision_policy': 'none', 'interaction_anchors': []},
        contract,
    )
    assert len(failures) == 1
    assert 'interaction_anchors' in failures[0]


def test_floor_contact_placement_gate_catches_floating_sunk_and_off_center_assets():
    check = getattr(qc, 'placement_failures', None)
    assert callable(check)

    assert check(
        'HH_A201',
        'floor_contact_center',
        [-0.5, -0.4, -0.02],
        [0.5, 0.4, 1.0],
    ) == []

    floating = check('HH_A201', 'floor_contact_center', [-0.5, -0.4, 0.30], [0.5, 0.4, 1.30])
    assert any('floating' in failure for failure in floating)

    sunk = check('HH_A201', 'floor_contact_center', [-0.5, -0.4, -0.30], [0.5, 0.4, 0.70])
    assert any('sunk' in failure for failure in sunk)

    off_center = check('HH_A201', 'floor_contact_center', [1.0, -0.4, 0.0], [2.0, 0.4, 1.0])
    assert any('off-center' in failure for failure in off_center)

    assert check(
        'HH_A014',
        'contextual_architecture',
        [3.0, -4.0, -1.0],
        [7.0, 1.0, 3.0],
    ) == []


def test_release_audit_reports_profile_anchor_and_placement_gates():
    build = getattr(qc, 'release_audit_markdown', None)
    assert callable(build)

    summary = {
        'gameplay_asset_count': 500,
        'generated_asset_records': 591,
        'animation_links': 87,
        'expected_animation_links': 87,
        'animation_links_deferred': 0,
        'mechanical_clips': 24,
        'mechanical_sets': 7,
        'humanoid_skeletons': 2,
        'humanoid_clips': 47,
        'humanoid_sets': 11,
    }
    report = {
        'status': 'PASS',
        'asset_count': 500,
        'failure_count': 0,
        'max_faces': 3024,
        'unique_material_colors': 83,
        'nonwhite_material_ratio': 1.0,
        'profile_face_budgets': {'P_CHARACTER': 2500},
        'interaction_anchor_bindings': 118,
        'expected_interaction_anchor_bindings': 118,
        'profile_contract_failure_count': 0,
        'placement_failure_count': 0,
    }

    text = build(summary, report, {f'{i:02d}': 'PRODUCTION_GENERATOR_VALIDATED' for i in range(1, 11)})

    assert '- Interaction anchors normalized: **118 / 118**' in text
    assert '- Profile contract conformance: **PASS** with **0** failures' in text
    assert '- Placement/pivot QC: **PASS** with **0** failures' in text
