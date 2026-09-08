from __future__ import annotations
import json, subprocess, sys
from pathlib import Path
import trimesh

ROOT = Path(__file__).resolve().parents[3]
GEN = ROOT / 'Tools' / 'ArtGeneration' / 'batch03_generate.py'
MANIFEST = ROOT / 'GameData' / 'AssetDefinitions' / 'Manifest' / 'asset_batch_03.json'


def rows():
    data=json.loads(MANIFEST.read_text())
    return [a for g in data['groups'] for a in g['assets']]


def test_manifest_exact_range_and_count():
    r=rows(); assert len(r)==50; assert r[0][0]=='HH_A111'; assert r[-1][0]=='HH_A160'


def test_generator_emits_and_reloads_all_assets(tmp_path):
    subprocess.run([sys.executable,str(GEN),str(MANIFEST),str(tmp_path),'--package'],check=True)
    glbs=sorted(tmp_path.glob('*.glb')); sidecars=sorted(tmp_path.glob('*.asset.json'))
    assert len(glbs)==50 and len(sidecars)==50
    for p in glbs:
        s=trimesh.load(p,force='scene'); assert len(s.geometry)>0; assert s.bounds is not None
        assert all(0.005 <= float(v) <= 8.0 for v in s.extents), (p.name,s.extents)


def test_sidecars_match_hmg070_and_manifest_materials(tmp_path):
    subprocess.run([sys.executable,str(GEN),str(MANIFEST),str(tmp_path),'--package'],check=True)
    expected={r[0]:r for r in rows()}
    for p in tmp_path.glob('*.asset.json'):
        d=json.loads(p.read_text()); r=expected[d['asset_id']]
        assert d['schema']==1 and d['asset_type']=='StaticMeshAsset' and d['units']=='meters'
        assert d['source']=='Tools/ArtGeneration/batch03_generate.py'
        assert d['material_slots']==[r[3]]
        assert d['dependencies']==[]


def test_curtains_expose_moving_nodes(tmp_path):
    subprocess.run([sys.executable,str(GEN),str(MANIFEST),str(tmp_path),'--package'],check=True)
    for asset_id in ('HH_A157','HH_A158','HH_A159'):
        s=trimesh.load(tmp_path/f'{asset_id}.glb',force='scene')
        assert any(str(n).startswith('MOV_') for n in s.graph.nodes_geometry)
