from __future__ import annotations

import json
import sys
from pathlib import Path

import trimesh

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
sys.path.insert(0, str(ART))

from quality_enrichment import enrich_asset_sidecar, scene_quality_metrics


def test_scene_quality_metrics_capture_geometry_and_floor_contact(tmp_path):
    scene = trimesh.Scene()
    primary = trimesh.creation.box(extents=(1.0, 0.6, 0.5))
    primary.apply_translation((0, 0, 0.25))
    detail = trimesh.creation.box(extents=(0.4, 0.1, 0.2))
    detail.apply_translation((0, -0.35, 0.4))
    scene.add_geometry(primary, node_name='PrimaryForm', geom_name='PrimaryForm')
    scene.add_geometry(detail, node_name='FunctionalDetail', geom_name='FunctionalDetail')
    path = tmp_path / 'HH_A001.glb'
    path.write_bytes(scene.export(file_type='glb'))
    metrics = scene_quality_metrics(path)
    assert metrics['face_count'] == 24
    assert metrics['geometry_count'] == 2
    assert metrics['semantic_nodes'] == ['FunctionalDetail', 'PrimaryForm']
    assert abs(metrics['floor_offset_m']) < 1e-6
    assert len(metrics['bounds_m']) == 2


def test_enrich_asset_sidecar_adds_v2_metadata_without_changing_gameplay_fields(tmp_path):
    glb = tmp_path / 'HH_A001.glb'
    scene = trimesh.Scene(trimesh.creation.box(extents=(1, 1, 1)))
    scene.apply_translation((0, 0, 0.5))
    glb.write_bytes(scene.export(file_type='glb'))
    sidecar = tmp_path / 'HH_A001.glb.asset.json'
    original = {
        'schema': 1,
        'asset_id': 'HH_A001',
        'asset_type': 'StaticMeshAsset',
        'source': 'Tools/ArtGeneration/batch01_generate.py',
        'dependencies': ['DEP_X'],
        'interaction_anchors': ['INT_USE_01'],
        'material_slots': ['MAT_WOOD_WARM'],
    }
    sidecar.write_text(json.dumps(original), encoding='utf-8')
    enriched = enrich_asset_sidecar(
        glb,
        sidecar,
        {'profile': 'P_FURNITURE_STATIC', 'family': 'guest_room_furniture_fixtures', 'material_family': 'MAT_WOOD_WARM'},
    )
    assert enriched['quality_revision'] == 2
    assert enriched['quality_contract'] == 'HH_ASSET_QUALITY_V2'
    assert enriched['quality_profile'] == 'P_FURNITURE_STATIC'
    assert enriched['quality_family'] == 'guest_room_furniture_fixtures'
    assert enriched['authoring_provenance']['provider'] == 'hotel_haven_procedural'
    assert enriched['dependencies'] == ['DEP_X']
    assert enriched['interaction_anchors'] == ['INT_USE_01']
    assert enriched['face_count'] > 0
