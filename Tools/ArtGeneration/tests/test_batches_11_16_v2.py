from __future__ import annotations

import importlib
import json
from pathlib import Path

import trimesh

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
MANIFEST = ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest'


def _rows(batch: int):
    data = json.loads((MANIFEST / f'asset_batch_{batch:02d}.json').read_text(encoding='utf-8'))
    return [row for group in data['groups'] for row in group['assets']]


def test_v2_batch_generator_modules_exist():
    for batch in range(11, 17):
        assert (ART / f'batch{batch:02d}_generate.py').is_file(), batch


def test_v2_batches_generate_50_semantic_assets_each(tmp_path):
    for batch in range(11, 17):
        module = importlib.import_module(f'batch{batch:02d}_generate')
        out = tmp_path / f'Batch{batch:02d}'
        paths = module.generate_package(
            MANIFEST / f'asset_batch_{batch:02d}.json',
            out,
            f'Tools/ArtGeneration/batch{batch:02d}_generate.py',
        )
        assert len(paths) == 50
        expected_ids = {row[0] for row in _rows(batch)}
        assert {p.stem for p in paths} == expected_ids
        sidecars = list(out.glob('*.asset.json'))
        assert len(sidecars) == 50

        for path in paths:
            scene = trimesh.load(path, force='scene')
            names = set(scene.graph.nodes_geometry)
            assert len(scene.geometry) >= 2, path.stem
            assert names != {'Body'}, path.stem
            assert any(
                token in name
                for name in names
                for token in ('Primary', 'Base', 'Seat', 'Top', 'Frame', 'Shell', 'Root', 'Hips', 'Door', 'Cab', 'Panel', 'Wheel', 'Fixture', 'Functional', 'Role')
            ), (path.stem, sorted(names))

            sidecar = json.loads(path.with_suffix('.asset.json').read_text(encoding='utf-8'))
            assert sidecar['asset_id'] == path.stem
            assert sidecar['units'] == 'meters'
            assert sidecar['quality_revision'] == 2
            assert sidecar['authoring_provenance']['provider'] == 'hotel_haven_procedural'
            assert sidecar['quality_contract'] == 'HH_ASSET_QUALITY_V2'
            assert sidecar['material_slots']


def test_batch16_character_assets_keep_humanoid_dependencies(tmp_path):
    module = importlib.import_module('batch16_generate')
    out = tmp_path / 'Batch16'
    module.generate_package(MANIFEST / 'asset_batch_16.json', out, 'Tools/ArtGeneration/batch16_generate.py')
    for row in _rows(16):
        asset_id, _name, _subcat, _mat, profile, animation_set, _anchors = row
        if profile != 'P_CHARACTER':
            continue
        path = out / f'{asset_id}.glb'
        scene = trimesh.load(path, force='scene')
        nodes = set(scene.graph.nodes_geometry)
        assert {'Hips', 'Spine', 'Head'} <= nodes
        sidecar = json.loads(path.with_suffix('.asset.json').read_text(encoding='utf-8'))
        assert sidecar['asset_type'] == 'SkinnedMeshAsset'
        assert animation_set in sidecar['dependencies']
        assert 'SK_HumanoidAdult' in sidecar['dependencies']
