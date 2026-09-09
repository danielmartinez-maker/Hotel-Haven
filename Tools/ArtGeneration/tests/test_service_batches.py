import json, sys
from pathlib import Path
import trimesh
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from batch06_generate import generate_package as gen6
from batch07_generate import generate_package as gen7


def manifest(batch,start,names,anim_indexes=()):
    rows=[]
    for i,name in enumerate(names):
        rows.append([f'HH_A{start+i:03d}',name,'general','MAT_SERVICE_PAINT','P_FURNITURE_STATIC','ANSET_SERVICE_CART' if i in anim_indexes else None,['INT_PUSH_01'] if i in anim_indexes else []])
    return {'schema':1,'batch':batch,'asset_count':len(rows),'groups':[{'family':'test','assets':rows}]}

def run(gen,tmp_path,batch,start,names,anim=()):
    mf=tmp_path/f'm{batch}.json'; mf.write_text(json.dumps(manifest(batch,start,names,anim)))
    out=tmp_path/f'b{batch}'; outputs=gen(mf,out)
    assert len(outputs)==len(names)
    for p in outputs:
        scene=trimesh.load(p,force='scene')
        assert len(scene.geometry)>0
        side=json.loads(p.with_suffix('.asset.json').read_text())
        assert side['schema']==1 and side['units']=='meters'

def test_batch06_smoke(tmp_path):
    run(gen6,tmp_path,6,251,['Restaurant Chair Upholstered','Kitchen Range','Housekeeping Cart Standard'],(2,))

def test_batch07_smoke(tmp_path):
    run(gen7,tmp_path,7,301,['Laundry Washer Commercial','Utility Cart Flatbed','Treadmill'],(1,))

def test_cart_has_moving_wheels(tmp_path):
    mf=tmp_path/'m.json'; mf.write_text(json.dumps(manifest(6,291,['Housekeeping Cart Standard'],(0,))))
    [p]=gen6(mf,tmp_path/'out')
    scene=trimesh.load(p,force='scene')
    nodes=set(scene.graph.nodes_geometry)
    assert sum(str(n).startswith('MOV_Wheel_') for n in nodes)==4
