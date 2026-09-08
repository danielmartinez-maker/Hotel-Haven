from __future__ import annotations
import argparse,importlib,json
from pathlib import Path

BATCHES=tuple(range(1,11))

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

def generate_all(repo_root:Path|str):
 from mechanical_animation_generate import generate as generate_mechanical
 from humanoid_animation_generate import generate as generate_humanoid
 from link_animation_dependencies import link
 root=Path(repo_root);manifest_dir=root/'GameData/AssetDefinitions/Manifest';exports=root/'Art/Exports';exports.mkdir(parents=True,exist_ok=True);batch_counts={}
 for batch in BATCHES:
  module=importlib.import_module(f'batch{batch:02d}_generate');manifest=manifest_dir/f'asset_batch_{batch:02d}.json';out=exports/f'Batch{batch:02d}';paths=module.generate_package(manifest,out);batch_counts[f'{batch:02d}']=len(paths)
 mech_clips,mech_sets=generate_mechanical(exports/'Animations');hum_skeletons,hum_clips,hum_sets=generate_humanoid(root/'GameData/AssetDefinitions/animation_sets_v1.json',exports/'AnimationsHumanoid');linked,deferred=link(manifest_dir,exports);normalized=normalize_sidecars(exports)
 summary={'schema':1,'batch_counts':batch_counts,'gameplay_asset_count':sum(batch_counts.values()),'mechanical_clips':mech_clips,'mechanical_sets':mech_sets,'humanoid_skeletons':hum_skeletons,'humanoid_clips':hum_clips,'humanoid_sets':hum_sets,'animation_links':linked,'animation_links_deferred':deferred,'normalized_sidecars':normalized}
 (root/'Art/Validation/full_generation_summary.json').write_text(json.dumps(summary,indent=2,sort_keys=True)+'\n')
 if summary['gameplay_asset_count']!=500:raise RuntimeError(f"expected 500 gameplay assets, got {summary['gameplay_asset_count']}")
 return summary

def main():
 p=argparse.ArgumentParser();p.add_argument('repo_root',nargs='?',default='.');a=p.parse_args();print(json.dumps(generate_all(Path(a.repo_root).resolve()),indent=2,sort_keys=True))
if __name__=='__main__':main()
