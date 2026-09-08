from __future__ import annotations
import argparse,json,math
from pathlib import Path
import numpy as np
import trimesh

EXPECTED_MOVING={
 'ANSET_MECH_DOOR':'MOV_DoorLeaf','ANSET_MECH_SLIDING_DOOR':'MOV_SlidingPanel',
 'ANSET_MECH_REVOLVING_DOOR':'MOV_RevolvingLeaf','ANSET_MECH_ELEVATOR':'MOV_ElevatorDoor',
 'ANSET_MECH_CURTAIN':'MOV_CurtainPanel','ANSET_SERVICE_CART':'MOV_Wheel',
}
CHARACTER_NODES={'Hips','Spine','Chest','Head','UpperArm_L','LowerArm_L','Hand_L','UpperArm_R','LowerArm_R','Hand_R','UpperLeg_L','LowerLeg_L','Foot_L','UpperLeg_R','LowerLeg_R','Foot_R'}

def manifest_rows(manifest_dir:Path):
 out={}
 for p in sorted(manifest_dir.glob('asset_batch_*.json')):
  d=json.loads(p.read_text())
  for g in d['groups']:
   for row in g['assets']:
    out[row[0]]={'name':row[1],'profile':row[4],'animation_set':row[5]}
 return out

def validate(repo_root:Path)->dict:
 manifests=manifest_rows(repo_root/'GameData/AssetDefinitions/Manifest')
 exports=repo_root/'Art/Exports'
 failures=[];stats=[]
 paths=sorted(exports.glob('Batch*/*.glb'))
 if len(paths)!=500:failures.append(f'expected 500 GLBs, found {len(paths)}')
 for p in paths:
  aid=p.stem;meta=manifests.get(aid)
  if not meta:
   failures.append(f'{aid}: missing manifest row');continue
  try:s=trimesh.load(p,force='scene')
  except Exception as e:
   failures.append(f'{aid}: load failed: {e}');continue
  geoms=list(s.geometry.values())
  if not geoms:
   failures.append(f'{aid}: empty scene');continue
  verts=[g.vertices for g in geoms if hasattr(g,'vertices') and len(g.vertices)]
  if not verts:
   failures.append(f'{aid}: no vertices');continue
  v=np.vstack(verts)
  if not np.isfinite(v).all():failures.append(f'{aid}: non-finite vertices')
  ext=v.max(axis=0)-v.min(axis=0);diag=float(np.linalg.norm(ext));faces=0;deg=0
  for g in geoms:
   if hasattr(g,'faces'):faces+=len(g.faces)
   if hasattr(g,'area_faces'):deg+=int(np.sum(np.asarray(g.area_faces)<1e-10))
  if faces<=0:failures.append(f'{aid}: no faces')
  if deg:failures.append(f'{aid}: {deg} degenerate faces')
  if diag<0.05:failures.append(f'{aid}: implausibly small diagonal {diag:.4f}m')
  if diag>8.0:failures.append(f'{aid}: implausibly large diagonal {diag:.3f}m')
  budget=2500 if meta['profile']=='P_CHARACTER' else 5000
  if faces>budget:failures.append(f'{aid}: face count {faces} exceeds budget {budget}')
  nodes=set(s.graph.nodes_geometry)
  aset=meta['animation_set']
  if meta['profile']=='P_CHARACTER':
   missing=sorted(CHARACTER_NODES-nodes)
   if missing:failures.append(f'{aid}: missing character nodes {missing}')
  elif aset in EXPECTED_MOVING and not any(n.startswith(EXPECTED_MOVING[aset]) for n in nodes):
   failures.append(f'{aid}: {aset} missing {EXPECTED_MOVING[aset]}* node')
  stats.append({'asset_id':aid,'faces':faces,'geometries':len(geoms),'diagonal_m':round(diag,5),'extents_m':[round(float(x),5) for x in ext]})

 if set(manifests)!=set(x['asset_id'] for x in stats):
  missing=sorted(set(manifests)-set(x['asset_id'] for x in stats))
  if missing:failures.append(f'missing generated assets: {missing[:20]}')
 report={
  'schema':1,'status':'PASS' if not failures else 'FAIL','asset_count':len(stats),
  'failure_count':len(failures),'failures':failures,
  'max_faces':max((x['faces'] for x in stats),default=0),
  'max_diagonal_m':max((x['diagonal_m'] for x in stats),default=0),
  'assets':stats,
 }
 out=repo_root/'Art/Validation/generated_geometry_qc.json';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 if failures:raise RuntimeError('\n'.join(failures))
 print(f"geometry QA PASSED: {len(stats)} assets; max_faces={report['max_faces']}; max_diagonal_m={report['max_diagonal_m']}")
 return report

def main():
 p=argparse.ArgumentParser();p.add_argument('repo_root',nargs='?',default='.');a=p.parse_args();validate(Path(a.repo_root).resolve())
if __name__=='__main__':main()
