from __future__ import annotations
import argparse,json
from pathlib import Path
from character_factory import build_character

def generate_package(manifest_path:Path|str,output_dir:Path|str,source_path='Tools/ArtGeneration/batch10_generate.py'):
 manifest_path=Path(manifest_path);output_dir=Path(output_dir);output_dir.mkdir(parents=True,exist_ok=True);data=json.loads(manifest_path.read_text());out=[];index=0
 for group in data['groups']:
  for asset_id,name,subcategory,mat,profile,animation_set,anchors in group['assets']:
   scene=build_character(name,index);index+=1;p=output_dir/f'{asset_id}.glb';p.write_bytes(scene.export(file_type='glb'));out.append(p)
   sk='SK_HumanoidSmall' if 'Child' in name else 'SK_HumanoidAdult'
   side={'schema':1,'asset_id':asset_id,'asset_type':'SkinnedMeshAsset','source':source_path,'units':'meters','lod_policy':'lod_character','collision_policy':'capsule_runtime','cutaway_policy':'normal','material_slots':[mat],'tags':['batch_10','architectural_diorama_realism','character',subcategory,f'skeleton:{sk}',f'animation_binding:{animation_set}'],'dependencies':[sk,animation_set],'milestone':'batch_10','source_revision':1,'metadata_revision':1}
   p.with_suffix('.asset.json').write_text(json.dumps(side,indent=2,sort_keys=True)+'\n')
 return out
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('manifest');p.add_argument('output');p.add_argument('--package',action='store_true');a=p.parse_args();[print(x) for x in generate_package(a.manifest,a.output)]
