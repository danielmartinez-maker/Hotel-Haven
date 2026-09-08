from __future__ import annotations
import argparse,json
from pathlib import Path
from amenity_decor_factory import build_asset

def generate_package(manifest_path:Path|str,output_dir:Path|str,source_path='Tools/ArtGeneration/batch09_generate.py'):
    manifest_path=Path(manifest_path); output_dir=Path(output_dir); output_dir.mkdir(parents=True,exist_ok=True); data=json.loads(manifest_path.read_text()); out=[]
    for group in data['groups']:
        family=group['family']
        for asset_id,name,subcategory,mat,profile,animation_set,anchors in group['assets']:
            scene=build_asset(name,mat); scene.units='meters'; p=output_dir/f'{asset_id}.glb'; p.write_bytes(scene.export(file_type='glb')); out.append(p)
            arch=profile=='P_ARCH_STATIC'; small=profile=='P_SMALL_PROP'; side={'schema':1,'asset_id':asset_id,'asset_type':'StaticMeshAsset','source':source_path,'units':'meters','lod_policy':'lod_architecture' if arch else ('lod_small_prop' if small else 'lod_furniture'),'collision_policy':'none_or_simple_proxy' if small else 'simple_proxy','cutaway_policy':'normal','material_slots':[mat],'tags':['batch_09','architectural_diorama_realism',family,subcategory],'dependencies':[],'milestone':'batch_09','source_revision':1,'metadata_revision':1}
            p.with_suffix('.asset.json').write_text(json.dumps(side,indent=2,sort_keys=True)+'\n')
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('manifest');p.add_argument('output');p.add_argument('--package',action='store_true');a=p.parse_args();[print(x) for x in generate_package(a.manifest,a.output)]
