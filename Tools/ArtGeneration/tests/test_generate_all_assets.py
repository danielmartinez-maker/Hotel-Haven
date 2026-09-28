import sys
from pathlib import Path
import numpy as np
import trimesh
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
import generate_all_assets as g

def test_batch_registry_is_complete():
    assert g.BATCHES==tuple(range(1,15))
    assert [g.generator_source(i) for i in g.BATCHES]==[f'Tools/ArtGeneration/batch{i:02d}_generate.py' for i in range(1,15)]

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
