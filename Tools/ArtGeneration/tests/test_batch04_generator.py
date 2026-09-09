from __future__ import annotations

import importlib.util
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[3]
GEN = ROOT / 'Tools' / 'ArtGeneration' / 'batch04_generate.py'
MANIFEST = ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest' / 'asset_batch_04.json'

sys.path.insert(0, str(GEN.parent))
spec = importlib.util.spec_from_file_location('batch04_generate', GEN)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def rows():
    d = json.loads(MANIFEST.read_text())
    return [a for g in d['groups'] for a in g['assets']]


def _rgb(scene: trimesh.Scene, geometry_name: str) -> tuple[int, int, int]:
    rgba = np.asarray(scene.geometry[geometry_name].visual.material.baseColorFactor).reshape(-1)
    if rgba.dtype.kind == 'f' and float(np.nanmax(rgba)) <= 1.0:
        rgba = rgba * 255.0
    return tuple(int(x) for x in np.clip(np.rint(rgba[:3]), 0, 255))


def test_manifest_count_and_expected_mixed_ranges():
    r = rows()
    assert len(r) == 50
    assert r[0][0] == 'HH_A101'
    assert r[9][0] == 'HH_A110'
    assert r[10][0] == 'HH_A161'
    assert r[-1][0] == 'HH_A200'


def test_generator_emits_and_reloads_50(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    glbs = sorted(tmp_path.glob('*.glb'))
    sc = sorted(tmp_path.glob('*.asset.json'))
    assert len(glbs) == 50 and len(sc) == 50
    for p in glbs:
        s = trimesh.load(p, force='scene')
        assert len(s.geometry) > 0
        assert s.bounds is not None
        assert all(0.005 <= float(v) <= 8.0 for v in s.extents), (p.name, s.extents)


def test_sidecars_match_manifest_materials(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    e = {r[0]: r for r in rows()}
    for p in tmp_path.glob('*.asset.json'):
        d = json.loads(p.read_text())
        r = e[d['asset_id']]
        assert d['schema'] == 1
        assert d['units'] == 'meters'
        assert d['material_slots'] == [r[3]]
        assert d['source'] == 'Tools/ArtGeneration/batch04_generate.py'


def test_palette_is_encoded_without_white_clamping():
    sink = mod.build('Bathroom Sink Pedestal', 'MAT_CERAMIC_FIXTURE')
    desk = mod.build('Reception Desk Single', 'MAT_WOOD_WARM')
    ceramic = _rgb(sink, 'Pedestal')
    wood = _rgb(desk, 'DeskFront')
    assert ceramic == (230, 227, 217)
    assert wood == (107, 56, 26)
    assert ceramic != (255, 255, 255)
    assert wood != (255, 255, 255)


def test_bathroom_and_reception_assets_have_expected_named_parts(tmp_path):
    subprocess.run([sys.executable, str(GEN), str(MANIFEST), str(tmp_path), '--package'], check=True)
    checks = {'HH_A168': 'Basin', 'HH_A169': 'ToiletBowl', 'HH_A173': 'TubBasin', 'HH_A186': 'DeskFront'}
    for aid, needle in checks.items():
        s = trimesh.load(tmp_path / f'{aid}.glb', force='scene')
        assert any(needle in str(n) for n in s.graph.nodes_geometry), (aid, list(s.graph.nodes_geometry))


def test_reception_service_points_have_distinct_camera_readable_details():
    cases = {
        'Reception Desk Single': {'Countertop', 'Monitor_0'},
        'Reception Desk Grand': {'Countertop', 'Monitor_0', 'Monitor_1', 'BrassInlay'},
        'Reception Back Counter': {'WorkSurface', 'StorageDoor_0'},
        'Concierge Desk': {'BrochureRack', 'ConciergeLamp'},
        'Bell Desk': {'ServiceBell', 'LuggageTagRack'},
        'Host Podium Lobby': {'ReservationBook', 'PodiumLip'},
    }
    for name, expected in cases.items():
        nodes = set(mod.build(name, 'MAT_WOOD_WARM').graph.nodes_geometry)
        assert expected <= nodes, f'{name}: missing {sorted(expected - nodes)}'


def test_lobby_seating_variants_and_curved_sofa_have_complete_silhouettes():
    curved = mod.build('Lobby Sofa Curved', 'MAT_UPHOLSTERY')
    curved_nodes = set(curved.graph.nodes_geometry)
    assert {'Back_0', 'Back_1', 'Back_2', 'Arm_Left', 'Arm_Right'} <= curved_nodes

    variants = {
        'Lobby Armchair Classic': 'ClassicCrest',
        'Lobby Armchair Modern': 'ModernBase',
        'Lobby Lounge Chair': 'LoungeFootrest',
    }
    for name, marker in variants.items():
        nodes = set(mod.build(name, 'MAT_UPHOLSTERY').graph.nodes_geometry)
        assert marker in nodes, (name, sorted(nodes))


def test_floor_contact_profiles_reach_the_placement_plane():
    floor_profiles = {
        'P_FURNITURE_STATIC',
        'P_INTERACTIVE_PREFAB',
        'P_INTERACTIVE_ANIMATED',
        'P_SERVICE_PROP_ANIMATED',
    }
    for asset_id, name, _subcategory, material, profile, _animset, _anchors in rows():
        if profile not in floor_profiles:
            continue
        scene = mod.build(name, material)
        assert scene.bounds is not None
        floor_z = float(scene.bounds[0][2])
        assert floor_z <= 0.12, f'{asset_id} {name}: floor_z={floor_z:.3f}m'
