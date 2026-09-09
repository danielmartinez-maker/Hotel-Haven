from __future__ import annotations
import argparse, json
from pathlib import Path
from service_asset_factory import build_asset


def generate_package(manifest_path: Path|str, output_dir: Path|str, source_path='Tools/ArtGeneration/batch06_generate.py'):
    manifest_path=Path(manifest_path); output_dir=Path(output_dir); output_dir.mkdir(parents=True,exist_ok=True)
    data=json.loads(manifest_path.read_text(encoding='utf-8')); outputs=[]
    for group in data['groups']:
        family=group['family']
        for row in group['assets']:
            asset_id,name,subcategory,mat,profile,animation_set,anchors=row
            scene=build_asset(name,mat); scene.units='meters'
            glb=output_dir/f'{asset_id}.glb'; glb.write_bytes(scene.export(file_type='glb')); outputs.append(glb)
            tags=['batch_06','architectural_diorama_realism',family,subcategory]
            if animation_set: tags.append(f'animation_binding:{animation_set}')
            sidecar={'schema':1,'asset_id':asset_id,'asset_type':'StaticMeshAsset','source':source_path,'units':'meters','lod_policy':'lod_furniture','collision_policy':'simple_proxy','cutaway_policy':'normal','material_slots':[mat],'tags':tags,'dependencies':[animation_set] if animation_set else [],'milestone':'batch_06','source_revision':2,'metadata_revision':2}
            glb.with_suffix('.asset.json').write_text(json.dumps(sidecar,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    return outputs

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('manifest'); p.add_argument('output'); p.add_argument('--package',action='store_true'); a=p.parse_args()
    for path in generate_package(a.manifest,a.output): print(path)
