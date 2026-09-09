from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[3]
GEN = ROOT / 'Tools' / 'ArtGeneration' / 'batch03_generate.py'
MANIFEST = ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest' / 'asset_batch_03.json'

spec = importlib.util.spec_from_file_location('batch03_generate', GEN)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def rows():
    data = json.loads(MANIFEST.read_text())
    return [a for g in data['groups'] for a in g['assets']]


def _rgb(scene: trimesh.Scene, geometry_name: str) -> tuple[int, int, int]:
    rgba = np.asarray(scene.geometry[geometry_name].visual.material.baseColorFactor).reshape(-1)
    if rgba.dtype.kind == 'f' and float(np.nanmax(rgba)) <= 1.0:
        rgba = rgba * 255.0
    return tuple(int(x) for x in np.clip(np.rint(rgba[:3]), 0, 255))


def test_manifest_exact_range_and_count():
    r = rows()
    assert len(r) == 50
    assert r[0][0] == 'HH_A111'
    assert r[-1][0] == 'HH_A160'


def test_generator_emits_and_reloads_all_assets(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    glbs = sorted(tmp_path.glob('*.glb'))
    sidecars = sorted(tmp_path.glob('*.asset.json'))
    assert len(glbs) == 50 and len(sidecars) == 50
    for p in glbs:
        s = trimesh.load(p, force='scene')
        assert len(s.geometry) > 0
        assert s.bounds is not None
        assert all(0.005 <= float(v) <= 8.0 for v in s.extents), (p.name, s.extents)


def test_sidecars_match_hmg070_and_manifest_materials(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    expected = {r[0]: r for r in rows()}
    for p in tmp_path.glob('*.asset.json'):
        d = json.loads(p.read_text())
        r = expected[d['asset_id']]
        assert d['schema'] == 1 and d['asset_type'] == 'StaticMeshAsset' and d['units'] == 'meters'
        assert d['source'] == 'Tools/ArtGeneration/batch03_generate.py'
        assert d['material_slots'] == [r[3]]
        expected_dependencies = [r[5]] if r[5] else []
        assert d['dependencies'] == expected_dependencies


def test_palette_is_encoded_without_white_clamping():
    bed_scene = mod.build('Single Guest Bed', 'MAT_LINEN')
    nightstand_scene = mod.build('Standard Nightstand', 'MAT_WOOD_WARM')
    linen = _rgb(bed_scene, 'Mattress')
    wood = _rgb(nightstand_scene, 'CabinetBody')
    assert linen == (224, 219, 204)
    assert wood == (107, 56, 26)
    assert linen != (255, 255, 255)
    assert wood != (255, 255, 255)


def test_curtains_expose_moving_nodes_and_parent_all_moving_detail():
    for name in ('Curtain Standard', 'Curtain Blackout', 'Sheer Curtain'):
        s = mod.build(name, 'MAT_LINEN')
        assert {'MOV_CurtainPanel_0', 'MOV_CurtainPanel_1'} <= set(s.graph.nodes_geometry)
        for i in (0, 1):
            assert s.graph.transforms.parents[f'Pleat_{i}_0'] == f'MOV_CurtainPanel_{i}'
            assert s.graph.transforms.parents[f'CurtainRing_{i}_0'] == f'MOV_CurtainPanel_{i}'

    blackout = mod.build('Curtain Blackout', 'MAT_LINEN')
    for i in (0, 1):
        assert blackout.graph.transforms.parents[f'BlackoutBacking_{i}'] == f'MOV_CurtainPanel_{i}'

    sheer = mod.build('Sheer Curtain', 'MAT_LINEN')
    for i in (0, 1):
        assert sheer.graph.transforms.parents[f'SheerOverlay_{i}'] == f'MOV_CurtainPanel_{i}'


def test_storage_and_amenity_casegoods_have_readable_feature_nodes():
    cases = {
        'Standard Wardrobe': {'Door_Left', 'Door_Right', 'Handle_Left', 'Handle_Right'},
        'Luxury Armoire': {'Door_Left', 'Door_Right', 'CrownMolding'},
        'Minibar Cabinet': {'MinibarDoor', 'MinibarHandle'},
        'Coffee Station Cabinet': {'CoffeeMachine', 'CupStack'},
        'TV Console Luxury': {'ConsoleShelf', 'CablePort'},
    }
    for name, expected in cases.items():
        nodes = set(mod.build(name, 'MAT_WOOD_WARM').graph.nodes_geometry)
        assert expected <= nodes, f'{name}: missing {sorted(expected - nodes)}'
