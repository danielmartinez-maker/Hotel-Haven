import json
from pathlib import Path
import sys

import trimesh

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from batch01_generate import generate_batch  # noqa: E402


def manifest_path():
    return ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest' / 'asset_batch_01.json'


def load_entries():
    data = json.loads(manifest_path().read_text())
    return [asset for group in data['groups'] for asset in group['assets']]


def test_batch01_generates_all_manifest_assets(tmp_path):
    entries = load_entries()
    result = generate_batch(manifest_path(), tmp_path)

    assert len(entries) == 50
    assert len(result) == 50
    assert {p.stem for p in result} == {row[0] for row in entries}

    for path in result:
        assert path.suffix == '.glb'
        scene = trimesh.load(path, force='scene')
        assert len(scene.geometry) > 0
        assert scene.bounds is not None
        assert all(0.005 <= float(v) <= 15.0 for v in scene.extents)


def test_animated_architecture_contains_named_moving_part_nodes(tmp_path):
    entries = load_entries()
    animated = {row[0] for row in entries if row[5] is not None}
    result = generate_batch(manifest_path(), tmp_path)

    for path in result:
        if path.stem not in animated:
            continue
        scene = trimesh.load(path, force='scene')
        node_names = set(scene.graph.nodes_geometry)
        assert any(name.startswith('MOV_') for name in node_names), path.stem


def test_batch01_package_writes_hmg070_sidecars(tmp_path):
    from batch01_generate import generate_package

    outputs = generate_package(
        manifest_path(), tmp_path, source_path='Tools/ArtGeneration/batch01_generate.py'
    )
    assert len(outputs) == 50

    for glb_path in outputs:
        sidecar = glb_path.with_suffix('.asset.json')
        assert sidecar.exists()
        meta = json.loads(sidecar.read_text())
        assert meta['schema'] == 1
        assert meta['asset_id'] == glb_path.stem
        assert meta['asset_type'] == 'StaticMeshAsset'
        assert meta['source'] == 'Tools/ArtGeneration/batch01_generate.py'
        assert meta['units'] == 'meters'
        assert meta['lod_policy'] == 'lod_architecture'
        assert meta['collision_policy'] == 'simple_proxy'
        assert meta['cutaway_policy'] == 'normal'
        assert isinstance(meta['material_slots'], list) and meta['material_slots']
        assert 'batch_01' in meta['tags']
        assert meta['dependencies'] == []


def test_cli_package_mode_generates_sidecars(tmp_path):
    import subprocess

    script = ROOT / 'Tools' / 'ArtGeneration' / 'batch01_generate.py'
    result = subprocess.run(
        [sys.executable, str(script), str(manifest_path()), str(tmp_path), '--package'],
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    assert len(list(tmp_path.glob('*.glb'))) == 50
    assert len(list(tmp_path.glob('*.asset.json'))) == 50
