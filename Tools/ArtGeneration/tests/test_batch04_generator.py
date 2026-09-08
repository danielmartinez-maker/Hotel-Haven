from __future__ import annotations
import json, subprocess, sys
from pathlib import Path
import trimesh

ROOT=Path(__file__).resolve().parents[3]
GEN=ROOT/'Tools'/'ArtGeneration'/'batch04_generate.py'
MANIFEST=ROOT/'GameData'/'AssetDefinitions'/'Manifest'/'asset_batch_04.json'


def rows():
    d=json.loads(MANIFEST.read_text()); return [a for g in d['groups'] for a in g['assets']]


def test_manifest_count_and_expected_mixed_ranges():
    r=rows(); assert len(r)==50; assert r[0][0]=='HH_A101'; assert r[9][0]=='HH_A110'; assert r[10][0]=='HH_A161'; assert r[-1][0]=='HH_A200'


def test_generator_emits_and_reloads_50(tmp_path):
    subprocess.run([sys.executable,str(GEN),str(MANIFEST),str(tmp_path),'--package'],check=True)
    glbs=sorted(tmp_path.glob('*.glb')); sc=sorted(tmp_path.glob('*.asset.json')); assert len(glbs)==50 and len(sc)==50
    for p in glbs:
        s=trimesh.load(p,force='scene'); assert len(s.geometry)>0; assert s.bounds is not None
        assert all(0.005 <= float(v) <= 8.0 for v in s.extents),(p.name,s.extents)


def test_sidecars_match_manifest_materials(tmp_path):
    subprocess.run([sys.executable,str(GEN),str(MANIFEST),str(tmp_path),'--package'],check=True)
    e={r[0]:r for r in rows()}
    for p in tmp_path.glob('*.asset.json'):
        d=json.loads(p.read_text()); r=e[d['asset_id']]
        assert d['schema']==1; assert d['units']=='meters'; assert d['material_slots']==[r[3]]; assert d['source']=='Tools/ArtGeneration/batch04_generate.py'


def test_bathroom_and_reception_assets_have_expected_named_parts(tmp_path):
    subprocess.run([sys.executable,str(GEN),str(MANIFEST),str(tmp_path),'--package'],check=True)
    checks={'HH_A168':'Basin','HH_A169':'ToiletBowl','HH_A173':'TubBasin','HH_A186':'DeskFront'}
    for aid,needle in checks.items():
        s=trimesh.load(tmp_path/f'{aid}.glb',force='scene'); assert any(needle in str(n) for n in s.graph.nodes_geometry),(aid,list(s.graph.nodes_geometry))
