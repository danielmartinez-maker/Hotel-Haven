from __future__ import annotations
import argparse,json
from pathlib import Path
import numpy as np
import trimesh

EXPECTED_MOVING={
 'ANSET_MECH_DOOR':('MOV_DoorLeaf',),
 'ANSET_MECH_SLIDING_DOOR':('MOV_SlidingPanel','MOV_PocketPanel'),
 'ANSET_MECH_REVOLVING_DOOR':('MOV_RevolvingLeaf',),
 'ANSET_MECH_ELEVATOR':('MOV_ElevatorDoor',),
 'ANSET_MECH_OVERHEAD_DOOR':('MOV_DockPanel',),
 'ANSET_MECH_CURTAIN':('MOV_CurtainPanel',),
 'ANSET_SERVICE_CART':('MOV_Wheel',),
}
CHARACTER_NODES={'Hips','Spine','Chest','Head','UpperArm_L','LowerArm_L','Hand_L','UpperArm_R','LowerArm_R','Hand_R','UpperLeg_L','LowerLeg_L','Foot_L','UpperLeg_R','LowerLeg_R','Foot_R'}
PROFILE_FACE_BUDGETS={
 'P_ARCH_STATIC':5000,
 'P_ARCH_ANIMATED':5000,
 'P_FURNITURE_STATIC':4200,
 'P_SERVICE_PROP_ANIMATED':4200,
 'P_INTERACTIVE_PREFAB':4200,
 'P_INTERACTIVE_ANIMATED':4200,
 'P_SMALL_PROP':3000,
 'P_CHARACTER':2500,
}

def manifest_rows(manifest_dir:Path):
 out={}
 for p in sorted(manifest_dir.glob('asset_batch_*.json')):
  d=json.loads(p.read_text())
  for g in d['groups']:
   for row in g['assets']:
    out[row[0]]={'name':row[1],'profile':row[4],'animation_set':row[5]}
 return out

def material_rgb(geom):
 mat=getattr(getattr(geom,'visual',None),'material',None)
 rgba=getattr(mat,'baseColorFactor',None)
 if rgba is None:return None
 arr=np.asarray(rgba).reshape(-1)
 if arr.size<3:return None
 if arr.dtype.kind=='f' and float(np.nanmax(arr))<=1.0:arr=arr*255.0
 return tuple(int(x) for x in np.clip(np.rint(arr[:3]),0,255))

def animation_dependency_failures(asset_id:str,meta:dict,sidecar:dict,available_animation_sets:set[str])->list[str]:
 aset=meta.get('animation_set')
 if not aset:return []
 failures=[];deps=sidecar.get('dependencies',[])
 if aset not in deps:failures.append(f'{asset_id}: sidecar missing animation dependency {aset}')
 if aset not in available_animation_sets:failures.append(f'{asset_id}: animation dependency {aset} has no generated animation-set asset')
 return failures

def release_audit_markdown(summary:dict,report:dict,batch_statuses:dict[str,str])->str:
 gameplay=summary.get('gameplay_asset_count',0);linked=summary.get('animation_links',0);expected=summary.get('expected_animation_links',0);deferred=summary.get('animation_links_deferred',0)
 qc_failures=report.get('failure_count',0);qc_status=report.get('status','UNKNOWN')
 unresolved=qc_failures+deferred
 if gameplay!=500:unresolved+=1
 if linked!=expected:unresolved+=1
 lines=[
  '# Hotel Haven 500-Asset Library Release Audit V1','',
  '## Release gate','',
  f'- Gameplay-facing assets: **{gameplay} / 500**',
  f'- Generated asset records (gameplay + animation support): **{summary.get("generated_asset_records",0)}**',
  f'- Animation dependencies linked: **{linked} / {expected}**',
  f'- Deferred animation dependencies: **{deferred}**',
  f'- Mechanical animation coverage: **{summary.get("mechanical_clips",0)} clips / {summary.get("mechanical_sets",0)} sets**',
  f'- Humanoid animation coverage: **{summary.get("humanoid_clips",0)} clips / {summary.get("humanoid_sets",0)} sets / {summary.get("humanoid_skeletons",0)} skeletons**',
  f'- Geometry QC: **{qc_status}** with **{qc_failures}** failures',
  f'- Maximum generated face count: **{report.get("max_faces",0)}**',
  f'- Material palette: **{report.get("unique_material_colors",0)}** unique sampled RGB colors; non-white ratio **{report.get("nonwhite_material_ratio",0)}**',
  f'- Unresolved BLOCKER/CRITICAL/MAJOR issues: **{unresolved}**','',
  '## Profile face budgets','',
  '| Profile | Maximum faces |','| --- | ---: |',
 ]
 for profile,budget in sorted(report.get('profile_face_budgets',{}).items()):lines.append(f'| {profile} | {budget} |')
 lines.extend(['','## Batch status','','| Batch | Status |','| --- | --- |'])
 for batch in range(1,11):lines.append(f'| Batch {batch:02d} | {batch_statuses.get(f"{batch:02d}","UNKNOWN")} |')
 lines.extend(['','## Operational note','','Generated GLB binaries and preview boards remain CI artifacts by design; the repository stores deterministic generators, manifests, validation policy, and this audit record.',''])
 return '\n'.join(lines)

def validate(repo_root:Path)->dict:
 manifests=manifest_rows(repo_root/'GameData/AssetDefinitions/Manifest')
 exports=repo_root/'Art/Exports'
 failures=[];stats=[];material_samples=0;nonwhite_materials=0;unique_colors=set()
 available_animation_sets={json.loads(p.read_text())['asset_id'] for p in exports.rglob('ANSET_*.animset.json.asset.json')}
 paths=sorted(exports.glob('Batch*/*.glb'))
 if len(paths)!=500:failures.append(f'expected 500 GLBs, found {len(paths)}')
 for p in paths:
  aid=p.stem;meta=manifests.get(aid)
  if not meta:
   failures.append(f'{aid}: missing manifest row');continue
  sidecar_path=p.with_name(p.name+'.asset.json')
  if not sidecar_path.is_file():
   failures.append(f'{aid}: missing normalized sidecar {sidecar_path.name}');continue
  try:sidecar=json.loads(sidecar_path.read_text())
  except Exception as e:
   failures.append(f'{aid}: sidecar load failed: {e}');continue
  failures.extend(animation_dependency_failures(aid,meta,sidecar,available_animation_sets))
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
  ext=v.max(axis=0)-v.min(axis=0);diag=float(np.linalg.norm(ext));faces=0;deg=0;asset_colors=set()
  for g in geoms:
   if hasattr(g,'faces'):faces+=len(g.faces)
   if hasattr(g,'area_faces'):deg+=int(np.sum(np.asarray(g.area_faces)<1e-10))
   rgb=material_rgb(g)
   if rgb is not None:
    material_samples+=1;asset_colors.add(rgb);unique_colors.add(rgb)
    if rgb!=(255,255,255):nonwhite_materials+=1
  if faces<=0:failures.append(f'{aid}: no faces')
  if deg:failures.append(f'{aid}: {deg} degenerate faces')
  if diag<0.05:failures.append(f'{aid}: implausibly small diagonal {diag:.4f}m')
  if diag>8.0:failures.append(f'{aid}: implausibly large diagonal {diag:.3f}m')
  budget=PROFILE_FACE_BUDGETS.get(meta['profile'],5000)
  if faces>budget:failures.append(f'{aid}: face count {faces} exceeds {meta["profile"]} budget {budget}')
  nodes=set(s.graph.nodes_geometry);aset=meta['animation_set']
  if meta['profile']=='P_CHARACTER':
   missing=sorted(CHARACTER_NODES-nodes)
   if missing:failures.append(f'{aid}: missing character nodes {missing}')
  elif aset in EXPECTED_MOVING:
   prefixes=EXPECTED_MOVING[aset]
   if not any(any(n.startswith(prefix) for prefix in prefixes) for n in nodes):
    failures.append(f'{aid}: {aset} missing one of {prefixes}')
  stats.append({'asset_id':aid,'profile':meta['profile'],'faces':faces,'geometries':len(geoms),'diagonal_m':round(diag,5),'extents_m':[round(float(x),5) for x in ext],'material_colors':len(asset_colors)})

 if set(manifests)!=set(x['asset_id'] for x in stats):
  missing=sorted(set(manifests)-set(x['asset_id'] for x in stats))
  if missing:failures.append(f'missing generated assets: {missing[:20]}')
 colored_ratio=(nonwhite_materials/material_samples) if material_samples else 0.0
 if material_samples==0:failures.append('no PBR material samples found in generated GLBs')
 if colored_ratio<0.60:failures.append(f'PBR palette collapsed toward white: nonwhite ratio={colored_ratio:.3f}')
 if len(unique_colors)<12:failures.append(f'PBR palette has only {len(unique_colors)} unique RGB colors; expected at least 12')
 report={
  'schema':1,'status':'PASS' if not failures else 'FAIL','asset_count':len(stats),
  'failure_count':len(failures),'failures':failures,
  'max_faces':max((x['faces'] for x in stats),default=0),
  'max_diagonal_m':max((x['diagonal_m'] for x in stats),default=0),
  'material_samples':material_samples,'nonwhite_material_ratio':round(colored_ratio,5),'unique_material_colors':len(unique_colors),
  'profile_face_budgets':PROFILE_FACE_BUDGETS,
  'animation_set_assets':len(available_animation_sets),
  'assets':stats,
 }
 validation_dir=repo_root/'Art/Validation';validation_dir.mkdir(parents=True,exist_ok=True)
 (validation_dir/'generated_geometry_qc.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 summary_path=validation_dir/'full_generation_summary.json'
 if summary_path.is_file():
  summary=json.loads(summary_path.read_text());counts=summary.get('batch_counts',{})
  batch_statuses={f'{i:02d}':('PRODUCTION_GENERATOR_VALIDATED' if counts.get(f'{i:02d}')==50 else f'INCOMPLETE_{counts.get(f"{i:02d}",0)}_OF_50') for i in range(1,11)}
  (validation_dir/'library_release_audit_v1.md').write_text(release_audit_markdown(summary,report,batch_statuses))
 if failures:raise RuntimeError('\n'.join(failures))
 print(f"geometry QA PASSED: {len(stats)} assets; max_faces={report['max_faces']}; colors={report['unique_material_colors']}; nonwhite={report['nonwhite_material_ratio']}")
 return report

def main():
 p=argparse.ArgumentParser();p.add_argument('repo_root',nargs='?',default='.');a=p.parse_args();validate(Path(a.repo_root).resolve())
if __name__=='__main__':main()
