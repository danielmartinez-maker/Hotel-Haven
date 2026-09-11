from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest
import trimesh

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
sys.path.insert(0, str(ART))

from asset_manifest import load_active_manifest
from meshy_import_adapter import normalize_meshy_asset


def test_meshy_contract_is_offline_only_and_never_serializes_credentials():
    data = json.loads((ROOT / 'GameData' / 'AssetDefinitions' / 'meshy_authoring_contract_v1.json').read_text(encoding='utf-8'))
    assert data['runtime_dependency'] is False
    assert data['offline_only'] is True
    assert data['preferred_source_format'] == 'glb'
    serialized = json.dumps(data).lower()
    assert 'api_key' not in serialized
    assert 'bearer ' not in serialized


def test_meshy_adapter_rejects_unsupported_source_extension(tmp_path):
    bad = tmp_path / 'model.stl'
    bad.write_bytes(b'not-a-supported-source')
    with pytest.raises(ValueError, match='unsupported Meshy source format'):
        normalize_meshy_asset(bad, 'HH_A501', load_active_manifest(ROOT), tmp_path / 'out', task_id='test-task')


def test_meshy_adapter_normalizes_glb_into_v2_sidecar(tmp_path):
    source = tmp_path / 'meshy.glb'
    scene = trimesh.Scene()
    body = trimesh.creation.box(extents=(1.0, 0.6, 0.8))
    body.apply_translation((0, 0, 0.4))
    detail = trimesh.creation.box(extents=(0.4, 0.1, 0.2))
    detail.apply_translation((0, -0.35, 0.55))
    scene.add_geometry(body, node_name='PrimaryForm', geom_name='PrimaryForm')
    scene.add_geometry(detail, node_name='FunctionalDetail', geom_name='FunctionalDetail')
    source.write_bytes(scene.export(file_type='glb'))

    model, sidecar = normalize_meshy_asset(
        source,
        'HH_A501',
        load_active_manifest(ROOT),
        tmp_path / 'out',
        task_id='meshy-task-501',
    )
    assert model.is_file()
    data = json.loads(sidecar.read_text(encoding='utf-8'))
    assert data['asset_id'] == 'HH_A501'
    assert data['quality_contract'] == 'HH_ASSET_QUALITY_V2'
    assert data['authoring_provenance']['provider'] == 'meshy'
    assert data['authoring_provenance']['task_id'] == 'meshy-task-501'
    assert data['authoring_provenance']['pbr_enabled'] is True
    assert 'api_key' not in json.dumps(data).lower()
