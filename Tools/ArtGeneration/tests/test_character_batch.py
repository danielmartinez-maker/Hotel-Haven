import json,sys
from pathlib import Path
import trimesh
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from batch10_generate import generate_package
from humanoid_animation_generate import generate

def test_character_hierarchy_and_sidecar(tmp_path):
 mf=tmp_path/'m.json';mf.write_text(json.dumps({'groups':[{'family':'guests_staff','assets':[['HH_A451','Guest Businessman','guest','MAT_CHARACTER_COMPOSITE','P_CHARACTER','ANSET_GUEST_LOCOMOTION',[]],['HH_A461','Guest Child Boy','guest','MAT_CHARACTER_COMPOSITE','P_CHARACTER','ANSET_CHILD_GUEST',[]]]}]}));outs=generate_package(mf,tmp_path/'o');assert len(outs)==2
 for p in outs:
  s=trimesh.load(p,force='scene');nodes=set(map(str,s.graph.nodes_geometry));assert {'Hips','Spine','Head','UpperArm_L','UpperLeg_R'}<=nodes
  side=json.loads(p.with_suffix('.asset.json').read_text());assert side['asset_type']=='SkinnedMeshAsset' and len(side['dependencies'])==2

def test_humanoid_animation_generation(tmp_path):
 cfg={'skeletons':[{'skeleton_id':'SK_HumanoidAdult','forward_axis':'+Y','up_axis':'+Z'},{'skeleton_id':'SK_HumanoidSmall','forward_axis':'+Y','up_axis':'+Z'}],'sets':[{'animation_set_id':'ANSET_GUEST_LOCOMOTION','skeleton_id':'SK_HumanoidAdult','clips':['AN_IDLE','AN_WALK']},{'animation_set_id':'ANSET_CHILD_GUEST','skeleton_id':'SK_HumanoidSmall','clips':['AN_IDLE','AN_CHILD_PLAY']}],'clip_semantics':{'looping_clips':['AN_IDLE','AN_WALK','AN_CHILD_PLAY']}}
 cp=tmp_path/'cfg.json';cp.write_text(json.dumps(cfg));sk,clips,sets=generate(cp,tmp_path/'a');assert (sk,clips,sets)==(2,4,2)
 assert (tmp_path/'a/ANSET_GUEST_LOCOMOTION.animset.asset.json').exists()
