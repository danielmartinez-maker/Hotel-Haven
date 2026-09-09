from __future__ import annotations
import importlib.util,json
from pathlib import Path
import trimesh
ROOT=Path(__file__).resolve().parents[3]; GEN=ROOT/'Tools'/'ArtGeneration'/'batch05_generate.py'; MAN=ROOT/'GameData'/'AssetDefinitions'/'Manifest'/'asset_batch_05.json'
spec=importlib.util.spec_from_file_location('batch05',GEN); mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
def rows():return [a for g in json.loads(MAN.read_text())['groups'] for a in g['assets']]
def test_manifest():
 r=rows();assert len(r)==50 and r[0][0]=='HH_A201' and r[-1][0]=='HH_A250'
def test_generate(tmp_path):
 mod.generate(MAN,tmp_path,True); assert len(list(tmp_path.glob('*.glb')))==50 and len(list(tmp_path.glob('*.asset.json')))==50
 for p in tmp_path.glob('*.glb'):
  s=trimesh.load(p,force='scene');assert len(s.geometry)>0 and s.bounds is not None;assert all(.005<=float(v)<=8 for v in s.extents),(p.name,s.extents)
def test_cart_moving_wheels(tmp_path):
 mod.generate(MAN,tmp_path,True)
 for aid in ('HH_A211','HH_A212','HH_A213'):
  s=trimesh.load(tmp_path/f'{aid}.glb',force='scene'); assert sum(str(n).startswith('MOV_Wheel') for n in s.graph.nodes_geometry)==4
def test_sidecars(tmp_path):
 mod.generate(MAN,tmp_path,True); e={r[0]:r for r in rows()}
 for p in tmp_path.glob('*.asset.json'):
  d=json.loads(p.read_text());r=e[d['asset_id']]; assert d['source']=='Tools/ArtGeneration/batch05_generate.py' and d['material_slots']==[r[3]] and d['units']=='meters'
