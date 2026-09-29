import json
import sys
from pathlib import Path
import numpy as np
import trimesh
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
import generate_all_assets as g

def test_batch_registry_is_complete():
    from asset_manifest import load_active_manifest
    batches = load_active_manifest(ROOT.parent.parent).batch_numbers
    assert batches == tuple(range(1, 85))
    assert [g.generator_source(i) for i in batches] == [f'Tools/ArtGeneration/batch{i:02d}_generate.py' for i in batches]

def test_sidecar_normalization_matches_exact_export_filename(tmp_path):
    export=tmp_path/'HH_A001.glb'; export.write_bytes(b'x')
    side=tmp_path/'HH_A001.asset.json'; side.write_text('{}')
    assert g.normalize_sidecars(tmp_path)==1
    assert not side.exists()
    assert (tmp_path/'HH_A001.glb.asset.json').exists()

def test_animation_sidecar_normalization(tmp_path):
    export=tmp_path/'AN_IDLE.anim.json'; export.write_text('{}')
    side=tmp_path/'AN_IDLE.anim.asset.json'; side.write_text('{}')
    assert g.normalize_sidecars(tmp_path)==1
    assert (tmp_path/'AN_IDLE.anim.json.asset.json').exists()

def test_sidecar_normalization_does_not_match_an_unrelated_dotted_export(tmp_path):
    (tmp_path/'cart.wheels.glb').write_bytes(b'x')
    side = tmp_path/'cart.asset.json'
    side.write_text('{}')

    try:
        g.normalize_sidecars(tmp_path)
    except RuntimeError as error:
        assert 'expected exactly one export' in str(error)
    else:
        raise AssertionError('an unrelated dotted export must not satisfy a short sidecar')

    assert side.exists()
    assert not (tmp_path/'cart.wheels.glb.asset.json').exists()

def test_trimesh_color_guard_preserves_float_255_palette_values():
    g.install_trimesh_color_guard()
    mat=trimesh.visual.material.PBRMaterial(baseColorFactor=np.array([112.0,64.0,32.0,255.0]))
    assert mat.baseColorFactor.tolist()==[112,64,32,255]


def test_gameplay_sidecar_normalization_uses_authoritative_profile_asset_type():
    sidecar = {
        'asset_id': 'HH_A999',
        'asset_type': 'StaticMeshAsset',
        'units': 'centimeters',
        'lod_policy': 'legacy',
        'collision_policy': 'legacy',
        'cutaway_policy': 'legacy',
        'interaction_anchors': [],
        'tags': [],
    }
    meta = {'interaction_anchors': ['INT_USE_01']}
    contract = {
        'asset_type': 'PrefabAsset',
        'units': 'meters',
        'lod_policy': 'lod_furniture',
        'collision_policy': 'simple_proxy',
        'cutaway_policy': 'normal',
    }

    normalized = g.normalize_gameplay_sidecar_data(sidecar, meta, contract)

    assert normalized['asset_type'] == 'PrefabAsset'
    assert normalized['units'] == 'meters'
    assert normalized['lod_policy'] == 'lod_furniture'
    assert normalized['collision_policy'] == 'simple_proxy'
    assert normalized['cutaway_policy'] == 'normal'
    assert normalized['interaction_anchors'] == ['INT_USE_01']
    assert 'interaction_anchor:INT_USE_01' in normalized['tags']


def test_generated_tree_validation_ignores_transient_rsync_sidecars(tmp_path):
    exports = tmp_path / 'Art' / 'Exports'
    transient = exports / 'Batch01' / '.rsync-tmp' / 'HH_A001.asset.json'
    transient.parent.mkdir(parents=True)
    transient.write_text('{"asset_id":"HH_A001"}')

    summary = g.validate_generated_tree(tmp_path, exports)

    assert summary == {
        'generated_asset_records': 0,
        'paired_exports': 0,
        'resolved_sources': 0,
        'resolved_dependencies': 0,
    }


def test_sidecar_normalization_ignores_transient_rsync_sidecars(tmp_path):
    exports = tmp_path / 'Art' / 'Exports'
    transient = exports / 'Batch01' / '.rsync-tmp' / 'HH_A001.asset.json'
    transient.parent.mkdir(parents=True)
    transient.write_text('{"asset_id":"HH_A001"}')

    assert g.normalize_sidecars(exports) == 0


def test_generated_tree_validation_ignores_shadowed_legacy_sidecars(tmp_path):
    exports = tmp_path / 'Art' / 'Exports'
    batch = exports / 'Batch01'
    batch.mkdir(parents=True)
    (batch / 'HH_A001.glb').write_bytes(b'glb')
    (tmp_path / 'source.blend').write_bytes(b'source')
    data = {'asset_id': 'HH_A001', 'source': 'source.blend', 'dependencies': []}
    (batch / 'HH_A001.glb.asset.json').write_text(json.dumps(data))
    (batch / 'HH_A001.asset.json').write_text(json.dumps(data))

    summary = g.validate_generated_tree(tmp_path, exports)

    assert summary == {
        'generated_asset_records': 1,
        'paired_exports': 1,
        'resolved_sources': 1,
        'resolved_dependencies': 0,
    }
