from __future__ import annotations
import argparse,importlib,json
from pathlib import Path

BATCHES=tuple(range(1,11))

def generator_source(batch:int)->str:
 return f'Tools/ArtGeneration/batch{batch:02d}_generate.py'

def run_batch_generator(module,batch:int,manifest:Path,out:Path):
 if hasattr(module,'generate_package'):
  return module.generate_package(manifest,out,generator_source(batch))
 if hasattr(module,'generate'):
  return module.generate(manifest,out,True)
 raise RuntimeError(f'batch {batch:02d} generator exposes neither generate_package nor generate')

def normalize_sidecars(exports_root:Path)->int:
 renamed=0
 for sidecar in sorted(exports_root.rglob('*.asset.json')):
  if not sidecar.exists():continue
  base=sidecar.name[:-len('.asset.json')]
  candidates=[p for p in sidecar.parent.iterdir() if p.is_file() and p.name.startswith(base+'.') and not p.name.endswith('.asset.json')]
  if len(candidates)!=1:raise RuntimeError(f'expected exactly one export for sidecar {sidecar}, found {[p.name for p in candidates]}')
  target=sidecar.parent/(candidates[0].name+'.asset.json')
  if target!=sidecar:sidecar.replace(target);renamed+=1
 return renamed

def expected_animation_bindings(manifest_dir:Path)->int:
 total=0
 for manifest in sorted(manifest_dir.glob('asset_batch_*.json')):
  data=json.loads(manifest.read_text())
  total+=sum(1 for group in data['groups'] for row in group['assets'] if row[5])
 return total

def validate_generated_tree(root:Path,exports:Path)->dict:
 records={};dependencies=[];paired=0;sources=0
 for sidecar in sorted(exports.rglob('*.asset.json')):
  data=json.loads(sidecar.read_text());asset_id=data['asset_id']
  if asset_id in records:raise RuntimeError(f'duplicate generated asset_id: {asset_id}')
  export=sidecar.with_name(sidecar.name[:-len('.asset.json')])
  if not export.is_file():raise RuntimeError(f'missing export paired with {sidecar}')
  paired+=1
  source=root/Path(data['source'])
  if not source.is_file():raise RuntimeError(f'missing source for {asset_id}: {data["source"]}')
  sources+=1;records[asset_id]=sidecar
  dependencies.extend((asset_id,dep) for dep in data.get('dependencies',[]))
 missing=[(owner,dep) for owner,dep in dependencies if dep not in records]
 if missing:raise RuntimeError(f'missing generated dependencies: {missing[:10]}')
 return {'generated_asset_records':len(records),'paired_exports':paired,'resolved_sources':sources,'resolved_dependencies':len(dependencies)}

def generate_all(repo_root:Path|str):
 from mechanical_animation_generate import generate as generate_mechanical
 from humanoid_animation_generate import generate as generate_humanoid
 from link_animation_dependencies import link
 root=Path(repo_root);manifest_dir=root/'GameData/AssetDefinitions/Manifest';exports=root/'Art/Exports';exports.mkdir(parents=True,exist_ok=True);batch_counts={}
 for batch in BATCHES:
  module=importlib.import_module(f'batch{batch:02d}_generate');manifest=manifest_dir/f'asset_batch_{batch:02d}.json';out=exports/f'Batch{batch:02d}';paths=run_batch_generator(module,batch,manifest,out);batch_counts[f'{batch:02d}']=len(paths)
 mech_clips,mech_sets=generate_mechanical(exports/'Animations');hum_skeletons,hum_clips,hum_sets=generate_humanoid(root/'GameData/AssetDefinitions/animation_sets_v1.json',exports/'AnimationsHumanoid');linked,deferred=link(manifest_dir,exports);normalized=normalize_sidecars(exports);tree=validate_generated_tree(root,exports);expected_links=expected_animation_bindings(manifest_dir)
 summary={'schema':1,'batch_counts':batch_counts,'gameplay_asset_count':sum(batch_counts.values()),'mechanical_clips':mech_clips,'mechanical_sets':mech_sets,'humanoid_skeletons':hum_skeletons,'humanoid_clips':hum_clips,'humanoid_sets':hum_sets,'animation_links':linked,'expected_animation_links':expected_links,'animation_links_deferred':deferred,'normalized_sidecars':normalized,**tree}
 expected_records=500+mech_clips+mech_sets+hum_skeletons+hum_clips+hum_sets
 if summary['gameplay_asset_count']!=500:raise RuntimeError(f"expected 500 gameplay assets, got {summary['gameplay_asset_count']}")
 if tree['generated_asset_records']!=expected_records:raise RuntimeError(f"expected {expected_records} generated records, got {tree['generated_asset_records']}")
 if linked!=expected_links or deferred!=0:raise RuntimeError(f'animation dependency linking incomplete: linked={linked} expected={expected_links} deferred={deferred}')
 (root/'Art/Validation/full_generation_summary.json').write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n')
 return summary

def main():
 p=argparse.ArgumentParser();p.add_argument('repo_root',nargs='?',default='.');a=p.parse_args();print(json.dumps(generate_all(Path(a.repo_root).resolve()),indent=2,sort_keys=True))
if __name__=='__main__':main()
