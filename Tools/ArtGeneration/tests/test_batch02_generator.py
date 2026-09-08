from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import trimesh

ROOT = Path(__file__).resolve().parents[3]
GEN = ROOT / 'Tools' / 'ArtGeneration' / 'batch02_generate.py'
MANIFEST = ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest' / 'asset_batch_02.json'


def test_manifest_has_50_assets():
    data = json.loads(MANIFEST.read_text())
    assets = [a for g in data['groups'] for a in g['assets']]
    assert len(assets) == 50
    assert assets[0][0] == 'HH_A051'
    assert assets[-1][0] == 'HH_A100'


def test_generator_emits_50_glbs_and_sidecars(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    glbs = sorted(tmp_path.glob('*.glb'))
    sidecars = sorted(tmp_path.glob('*.asset.json'))
    assert len(glbs) == 50
    assert len(sidecars) == 50
    assert {p.stem for p in glbs} == {f'HH_A{i:03d}' for i in range(51, 101)}


def test_all_generated_glbs_reload_and_are_nonempty(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    for path in sorted(tmp_path.glob('*.glb')):
        scene = trimesh.load(path, force='scene')
        assert len(scene.geometry) > 0, path.name
        assert sum(len(g.vertices) for g in scene.geometry.values()) >= 8, path.name


def test_sidecars_are_hmg070_schema_and_animated_nodes_exist(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    data = json.loads(MANIFEST.read_text())
    rows = [a for g in data['groups'] for a in g['assets']]
    animated = {row[0] for row in rows if row[5] is not None}
    rows_by_id = {row[0]: row for row in rows}
    expected_types = {'P_ARCH_STATIC': 'StaticMeshAsset', 'P_ARCH_ANIMATED': 'SkinnedMeshAsset'}
    for sidecar_path in sorted(tmp_path.glob('*.asset.json')):
        sidecar = json.loads(sidecar_path.read_text())
        assert sidecar['schema'] == 1
        assert sidecar['units'] == 'meters'
        assert sidecar['asset_type'] == expected_types[rows_by_id[sidecar['asset_id']][4]]
        assert sidecar['source'] == 'Tools/ArtGeneration/batch02_generate.py'
        assert sidecar['material_slots']
        if sidecar['asset_id'] in animated:
            scene = trimesh.load(tmp_path / f"{sidecar['asset_id']}.glb", force='scene')
            assert any(str(n).startswith('MOV_') for n in scene.graph.nodes_geometry), sidecar['asset_id']
