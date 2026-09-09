from __future__ import annotations

import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / 'Tools' / 'ArtGeneration' / 'link_animation_dependencies.py'
spec = importlib.util.spec_from_file_location('link_animation_dependencies', SCRIPT)
mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)


def write_json(path: Path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data))


def test_links_only_available_animation_sets(tmp_path):
    manifests = tmp_path / 'Manifest'; exports = tmp_path / 'Exports'
    write_json(manifests/'asset_batch_01.json', {'groups':[{'assets':[
        ['HH_A001','Door','doors','MAT_WOOD_WARM','P_ARCH_ANIMATED','ANSET_MECH_DOOR',[]],
        ['HH_A002','Static','walls','MAT_PLASTER_WARM','P_ARCH_STATIC',None,[]],
        ['HH_A003','Future','character','MAT_CHARACTER','P_CHARACTER','ANSET_GUEST_LOCOMOTION',[]],
    ]}]})
    write_json(exports/'Animations'/'ANSET_MECH_DOOR.animset.asset.json', {'asset_id':'ANSET_MECH_DOOR'})
    base={'schema':1,'dependencies':[]}
    write_json(exports/'Batch01'/'HH_A001.asset.json', {'asset_id':'HH_A001',**base})
    write_json(exports/'Batch01'/'HH_A002.asset.json', {'asset_id':'HH_A002',**base})
    write_json(exports/'Batch01'/'HH_A003.asset.json', {'asset_id':'HH_A003',**base})
    linked,deferred=mod.link(manifests,exports)
    assert linked==1 and deferred==1
    assert json.loads((exports/'Batch01'/'HH_A001.asset.json').read_text())['dependencies']==['ANSET_MECH_DOOR']
    assert json.loads((exports/'Batch01'/'HH_A002.asset.json').read_text())['dependencies']==[]
    assert json.loads((exports/'Batch01'/'HH_A003.asset.json').read_text())['dependencies']==[]


def test_linking_is_idempotent(tmp_path):
    manifests=tmp_path/'Manifest'; exports=tmp_path/'Exports'
    write_json(manifests/'asset_batch_01.json', {'groups':[{'assets':[['HH_A001','Door','doors','M','P','ANSET_MECH_DOOR',[]]]}]})
    write_json(exports/'Animations'/'ANSET_MECH_DOOR.animset.asset.json', {'asset_id':'ANSET_MECH_DOOR'})
    write_json(exports/'Batch01'/'HH_A001.asset.json', {'asset_id':'HH_A001','dependencies':['BASE_DEP']})
    mod.link(manifests,exports); mod.link(manifests,exports)
    assert json.loads((exports/'Batch01'/'HH_A001.asset.json').read_text())['dependencies']==['ANSET_MECH_DOOR','BASE_DEP']
