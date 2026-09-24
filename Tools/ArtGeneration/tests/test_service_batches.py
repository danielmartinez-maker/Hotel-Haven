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

def test_service_storage_has_readable_front_identity():
    from service_asset_factory import build_asset

    cabinet = build_asset('Cleaning Supply Cabinet', 'MAT_SERVICE_PAINT')
    cabinet_nodes = set(map(str, cabinet.graph.nodes_geometry))
    assert {
        'CabinetDoor_0', 'CabinetDoor_1',
        'CabinetHandle_0', 'CabinetHandle_1',
        'CabinetDoorGap', 'CabinetPlinth',
        'CleaningLabel', 'CleaningShelf',
    } <= cabinet_nodes

    lockers = build_asset('Staff Locker Bank', 'MAT_SERVICE_PAINT')
    locker_nodes = set(map(str, lockers.graph.nodes_geometry))
    for index in range(3):
        assert f'LockerDoor_{index}' in locker_nodes
        assert f'LockerHandle_{index}' in locker_nodes
        assert {f'LockerVent_{index}_{vent}' for vent in range(3)} <= locker_nodes
    assert {'CabinetPlinth', 'LockerToeReveal'} <= locker_nodes


def test_service_storage_stays_floor_supported_and_nontrivial():
    from service_asset_factory import build_asset

    for name in ('Cleaning Supply Cabinet', 'Staff Locker Bank'):
        scene = build_asset(name, 'MAT_SERVICE_PAINT')
        assert scene.bounds is not None
        assert float(scene.bounds[0][2]) <= 0.01
        assert len(scene.geometry) >= 10

