from __future__ import annotations

import importlib.util
import json
from pathlib import Path

import trimesh

ROOT = Path(__file__).resolve().parents[3]
QC_PATH = ROOT / 'Tools' / 'ArtGeneration' / 'validate_generated_geometry.py'
CHARACTER_PATH = ROOT / 'Tools' / 'ArtGeneration' / 'character_factory.py'


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


qc = load_module('validate_generated_geometry', QC_PATH)
characters = load_module('character_factory', CHARACTER_PATH)


def node_names(scene: trimesh.Scene) -> set[str]:
    return set(map(str, scene.graph.nodes_geometry))


def test_profile_face_budgets_scale_with_gameplay_readability():
    budgets = getattr(qc, 'PROFILE_FACE_BUDGETS', None)
    assert isinstance(budgets, dict)
    assert budgets == {
        'P_ARCH_STATIC': 5000,
        'P_ARCH_ANIMATED': 5000,
        'P_FURNITURE_STATIC': 4200,
        'P_SERVICE_PROP_ANIMATED': 4200,
        'P_INTERACTIVE_PREFAB': 4200,
        'P_INTERACTIVE_ANIMATED': 4200,
        'P_SMALL_PROP': 3000,
        'P_CHARACTER': 2500,
    }


def test_animation_dependency_gate_rejects_missing_or_unknown_bindings():
    check = getattr(qc, 'animation_dependency_failures', None)
    assert callable(check)
    meta = {'animation_set': 'ANSET_MECH_DOOR'}
    assert check('HH_A001', meta, {'dependencies': []}, {'ANSET_MECH_DOOR'}) == [
        'HH_A001: sidecar missing animation dependency ANSET_MECH_DOOR'
    ]
    assert check(
        'HH_A001', meta, {'dependencies': ['ANSET_MECH_DOOR']}, set()
    ) == ['HH_A001: animation dependency ANSET_MECH_DOOR has no generated animation-set asset']
    assert check(
        'HH_A001', meta, {'dependencies': ['ANSET_MECH_DOOR']}, {'ANSET_MECH_DOOR'}
    ) == []


def test_front_desk_roles_have_distinct_management_camera_markers():
    receptionist = node_names(characters.build_character('Receptionist Woman', 31))
    concierge = node_names(characters.build_character('Concierge Woman', 33))
    assert 'Accessory_ReceptionFolder' in receptionist
    assert 'Accessory_ConciergeSash' in concierge
    assert 'Accessory_ReceptionFolder' not in concierge
    assert 'Accessory_ConciergeSash' not in receptionist


def test_release_audit_markdown_reports_release_gate_evidence():
    build = getattr(qc, 'release_audit_markdown', None)
    assert callable(build)
    summary = {
        'gameplay_asset_count': 500,
        'generated_asset_records': 641,
        'animation_links': 71,
        'expected_animation_links': 71,
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
        'max_faces': 2496,
        'unique_material_colors': 32,
        'nonwhite_material_ratio': 0.91,
        'profile_face_budgets': {'P_CHARACTER': 2500},
    }
    text = build(summary, report, {f'{i:02d}': 'PRODUCTION_GENERATOR_VALIDATED' for i in range(1, 11)})
    assert '# Hotel Haven 500-Asset Library Release Audit V1' in text
    assert '- Gameplay-facing assets: **500 / 500**' in text
    assert '- Animation dependencies linked: **71 / 71**' in text
    assert '- Geometry QC: **PASS** with **0** failures' in text
    assert '- Unresolved BLOCKER/CRITICAL/MAJOR issues: **0**' in text
    assert 'Batch 10 | PRODUCTION_GENERATOR_VALIDATED' in text
