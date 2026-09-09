import json,sys
from pathlib import Path
import trimesh
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from batch08_generate import generate_package as gen8
from batch09_generate import generate_package as gen9

def make(batch,start,names):
    rows=[[f'HH_A{start+i:03d}',n,'general','MAT_WOOD_WARM','P_FURNITURE_STATIC',None,[]] for i,n in enumerate(names)]
    return {'schema':1,'batch':batch,'asset_count':len(rows),'groups':[{'family':'test','assets':rows}]}
def check(gen,tmp,batch,start,names):
    mf=tmp/'m.json';mf.write_text(json.dumps(make(batch,start,names)));outs=gen(mf,tmp/'o');assert len(outs)==len(names)
    for p in outs:
        s=trimesh.load(p,force='scene');assert s.geometry
        side=json.loads(p.with_suffix('.asset.json').read_text());assert side['schema']==1 and side['units']=='meters'
def test_batch08_smoke(tmp_path):check(gen8,tmp_path,8,351,['Dumbbell Rack','Spa Massage Table','Framed Art Landscape'])
def test_batch09_smoke(tmp_path):check(gen9,tmp_path,9,401,['Room Number Plaque Brass','Hotel Facade Entrance Module','Ornamental Tree Large'])
