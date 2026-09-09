from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[3]
GEN = ROOT / 'Tools' / 'ArtGeneration' / 'batch02_generate.py'
MANIFEST = ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest' / 'asset_batch_02.json'

spec = importlib.util.spec_from_file_location('batch02_generate', GEN)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def _rgb_for_primary_surface(name: str, material_family: str) -> tuple[int, int, int]:
    scene = mod.build(name, material_family)
    primary = 'FinishWall' if ('Paint Finish' in name or 'Wood Panel' in name or 'Wall Cladding' in name) else 'FinishTile'
    geom = scene.geometry[primary]
    rgba = np.asarray(geom.visual.material.baseColorFactor).reshape(-1)
    if rgba.dtype.kind == 'f' and float(np.nanmax(rgba)) <= 1.0:
        rgba = rgba * 255.0
    return tuple(int(x) for x in np.clip(np.rint(rgba[:3]), 0, 255))


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
    for sidecar_path in sorted(tmp_path.glob('*.asset.json')):
        sidecar = json.loads(sidecar_path.read_text())
        assert sidecar['schema'] == 1
        assert sidecar['units'] == 'meters'
        assert sidecar['asset_type'] == 'StaticMeshAsset'
        assert sidecar['source'] == 'Tools/ArtGeneration/batch02_generate.py'
        assert sidecar['material_slots']
        if sidecar['asset_id'] in animated:
            scene = trimesh.load(tmp_path / f"{sidecar['asset_id']}.glb", force='scene')
            assert any(str(n).startswith('MOV_') for n in scene.graph.nodes_geometry), sidecar['asset_id']


def test_same_family_finish_variants_have_distinct_primary_colors():
    pairs = [
        (('Carpet Tile Warm Beige', 'MAT_CARPET_STANDARD'), ('Carpet Tile Charcoal', 'MAT_CARPET_STANDARD')),
        (('White Bathroom Tile', 'MAT_TILE_CERAMIC'), ('Blue Bathroom Tile', 'MAT_TILE_CERAMIC')),
        (('Wood Panel Walnut', 'MAT_WOOD_WARM'), ('Wood Panel Oak', 'MAT_WOOD_WARM')),
    ]
    for left, right in pairs:
        assert _rgb_for_primary_surface(*left) != _rgb_for_primary_surface(*right), (left[0], right[0])

    paint_colors = {
        _rgb_for_primary_surface('Paint Finish Warm Ivory', 'MAT_PLASTER_WARM'),
        _rgb_for_primary_surface('Paint Finish Soft Grey', 'MAT_PLASTER_COOL'),
        _rgb_for_primary_surface('Paint Finish Muted Blue', 'MAT_PLASTER_COOL'),
        _rgb_for_primary_surface('Paint Finish Olive', 'MAT_PLASTER_WARM'),
        _rgb_for_primary_surface('Paint Finish Burgundy Accent', 'MAT_PLASTER_WARM'),
    }
    assert len(paint_colors) == 5


def test_packaged_animated_assets_declare_animation_set_dependency(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    data = json.loads(MANIFEST.read_text())
    rows = [a for g in data['groups'] for a in g['assets']]
    expected = {row[0]: row[5] for row in rows if row[5] is not None}
    for asset_id, animation_set in expected.items():
        sidecar = json.loads((tmp_path / f'{asset_id}.asset.json').read_text())
        assert sidecar['dependencies'] == [animation_set], asset_id


def test_animated_architecture_has_camera_readable_hardware_and_hierarchy():
    cases = {
        'Double Service Door': ('MAT_SERVICE_PAINT', {'Handle_Left', 'Handle_Right'}),
        'Pocket Door': ('MAT_WOOD_WARM', {'PocketPull'}),
        'Accessible Guest Door': ('MAT_WOOD_WARM', {'LeverHandle'}),
        'Security Door': ('MAT_SERVICE_PAINT', {'LeverHandle', 'VisionPanel', 'KickPlate'}),
        'Loading Dock Door': ('MAT_SERVICE_PAINT', {'GuideRail_Left', 'GuideRail_Right', 'LiftHandle'}),
        'Lobby Automatic Door': ('MAT_GLASS_CLEAR', {'MotionSensor'}),
        'Balcony Door': ('MAT_WOOD_WARM', {'LeverHandle'}),
    }
    for name, (material_family, required) in cases.items():
        scene = mod.build(name, material_family)
        nodes = set(scene.graph.nodes_geometry)
        assert required <= nodes, f'{name}: missing {sorted(required - nodes)}'

    double = mod.build('Double Service Door', 'MAT_SERVICE_PAINT')
    assert double.graph.transforms.parents['Handle_Left'] == 'MOV_DoorLeaf_0'
    assert double.graph.transforms.parents['Handle_Right'] == 'MOV_DoorLeaf_1'

    pocket = mod.build('Pocket Door', 'MAT_WOOD_WARM')
    assert pocket.graph.transforms.parents['PocketPull'] == 'MOV_PocketPanel'

    for name, material_family in [
        ('Accessible Guest Door', 'MAT_WOOD_WARM'),
        ('Security Door', 'MAT_SERVICE_PAINT'),
        ('Balcony Door', 'MAT_WOOD_WARM'),
    ]:
        scene = mod.build(name, material_family)
        assert scene.graph.transforms.parents['LeverHandle'] == 'MOV_DoorLeaf'

    security = mod.build('Security Door', 'MAT_SERVICE_PAINT')
    assert security.graph.transforms.parents['VisionPanel'] == 'MOV_DoorLeaf'
    assert security.graph.transforms.parents['KickPlate'] == 'MOV_DoorLeaf'

    dock = mod.build('Loading Dock Door', 'MAT_SERVICE_PAINT')
    assert dock.graph.transforms.parents['LiftHandle'] == 'MOV_DockPanel_0'


def test_swinging_door_animation_nodes_use_hinge_pivots():
    cases = [
        ('Double Service Door', 'MAT_SERVICE_PAINT', ('MOV_DoorLeaf_0', 'MOV_DoorLeaf_1')),
        ('Accessible Guest Door', 'MAT_WOOD_WARM', ('MOV_DoorLeaf',)),
        ('Security Door', 'MAT_SERVICE_PAINT', ('MOV_DoorLeaf',)),
        ('Balcony Door', 'MAT_WOOD_WARM', ('MOV_DoorLeaf',)),
    ]
    for name, material_family, moving_nodes in cases:
        scene = mod.build(name, material_family)
        for node in moving_nodes:
            transform, _ = scene.graph.get(node)
            assert abs(float(transform[0, 3])) >= 0.35, f'{name}: {node} has no hinge-offset pivot'
