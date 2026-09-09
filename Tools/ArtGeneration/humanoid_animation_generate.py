from __future__ import annotations
import argparse,json
from pathlib import Path

BONES=['Hips','Spine','Chest','Head','UpperArm_L','LowerArm_L','Hand_L','UpperArm_R','LowerArm_R','Hand_R','UpperLeg_L','LowerLeg_L','Foot_L','UpperLeg_R','LowerLeg_R','Foot_R']

def ch(target,transform,frames):return {'target_pattern':target,'space':'local','transform':transform,'keyframes':[{'t':float(t),'value':v} for t,v in frames]}
def duration(name):
 if 'WALK' in name:return .8
 if 'SLEEP' in name:return 1.4
 if any(k in name for k in ('CHECKIN','CHECKOUT','SERVE','REPAIR','MAKE_BED','VACUUM','COOK','WASH','LOAD','UNLOAD')):return 1.2
 return .9

def channels(name):
 u=name.upper();c=[]
 if 'WALK' in u or 'PUSH_' in u:c=[ch('UpperArm_L','rotation_x_degrees',[(0,-25),(.5,25),(1,-25)]),ch('UpperArm_R','rotation_x_degrees',[(0,25),(.5,-25),(1,25)]),ch('UpperLeg_L','rotation_x_degrees',[(0,30),(.5,-30),(1,30)]),ch('UpperLeg_R','rotation_x_degrees',[(0,-30),(.5,30),(1,-30)])]
 elif 'TURN_90' in u:c=[ch('Hips','rotation_z_degrees',[(0,0),(1,90)])]
 elif 'SIT' in u:c=[ch('Hips','translation_z_m',[(0,0),(1,-.35)]),ch('UpperLeg_*','rotation_x_degrees',[(0,0),(1,-75)]),ch('LowerLeg_*','rotation_x_degrees',[(0,0),(1,80)])]
 elif 'STAND' in u:c=[ch('Hips','translation_z_m',[(0,-.35),(1,0)]),ch('UpperLeg_*','rotation_x_degrees',[(0,-75),(1,0)])]
 elif 'SLEEP' in u:c=[ch('Hips','rotation_x_degrees',[(0,0),(1,90)]),ch('Spine','rotation_y_degrees',[(0,0),(.5,4),(1,0)])]
 elif any(k in u for k in ('TYPE','CHECKIN','CHECKOUT','PHONE','TABLET','CLIPBOARD','RADIO')):c=[ch('UpperArm_L','rotation_x_degrees',[(0,0),(1,-45)]),ch('UpperArm_R','rotation_x_degrees',[(0,0),(1,-45)]),ch('LowerArm_*','rotation_x_degrees',[(0,0),(1,-70)])]
 elif any(k in u for k in ('CLEAN','MAKE_BED','VACUUM','SERVE','CLEAR','ORDER','COOK','STIR','PLATE','WASH','REPAIR','INSPECT','LOAD','UNLOAD','CARRY')):c=[ch('UpperArm_L','rotation_x_degrees',[(0,-15),(.5,-65),(1,-15)]),ch('UpperArm_R','rotation_x_degrees',[(0,-10),(.5,-55),(1,-10)]),ch('Spine','rotation_x_degrees',[(0,0),(.5,12),(1,0)])]
 elif 'KNEEL' in u:c=[ch('Hips','translation_z_m',[(0,0),(1,-.42)]),ch('UpperLeg_*','rotation_x_degrees',[(0,0),(1,-55)])]
 elif 'CHILD_PLAY' in u:c=[ch('UpperArm_L','rotation_z_degrees',[(0,0),(.5,-55),(1,0)]),ch('UpperArm_R','rotation_z_degrees',[(0,0),(.5,55),(1,0)]),ch('Hips','translation_z_m',[(0,0),(.5,.08),(1,0)])]
 elif 'TALK' in u or 'DIRECT' in u:c=[ch('UpperArm_R','rotation_z_degrees',[(0,0),(.5,28),(1,0)]),ch('Head','rotation_z_degrees',[(0,-4),(.5,4),(1,-4)])]
 else:c=[ch('Spine','translation_z_m',[(0,0),(.5,.012),(1,0)]),ch('Head','rotation_z_degrees',[(0,-2),(.5,2),(1,-2)])]
 return c

def sidecar(asset_id,asset_type,source,deps):return {'schema':1,'asset_id':asset_id,'asset_type':asset_type,'source':source,'units':'meters','lod_policy':'animation_shared','collision_policy':'none','material_slots':[],'tags':['hotel-haven','animation','shared-humanoid'],'dependencies':list(deps),'source_revision':1,'metadata_revision':1,'cooker_schema':1,'lifecycle_state':'PRODUCTION'}
def generate(config_path,output):
 config=json.loads(Path(config_path).read_text());output=Path(output);output.mkdir(parents=True,exist_ok=True);source='Tools/ArtGeneration/humanoid_animation_generate.py';skeleton_ids=[]
 for sk in config['skeletons']:
  if sk['skeleton_id'] not in ('SK_HumanoidAdult','SK_HumanoidSmall'):continue
  sid=sk['skeleton_id'];payload={'schema':1,'skeleton_id':sid,'root':'Hips','bones':BONES,'forward_axis':sk['forward_axis'],'up_axis':sk['up_axis']};(output/f'{sid}.skeleton.json').write_text(json.dumps(payload,indent=2)+'\n');(output/f'{sid}.skeleton.asset.json').write_text(json.dumps(sidecar(sid,'SkeletonAsset',source,[]),indent=2)+'\n');skeleton_ids.append(sid)
 loops=set(config.get('clip_semantics',{}).get('looping_clips',[]));clip_cache={};set_count=0
 for aset in config['sets']:
  sk=aset['skeleton_id']
  if sk not in skeleton_ids:continue
  prefix='ADULT' if sk=='SK_HumanoidAdult' else 'CHILD';deps=[sk];mappings=[]
  for logical in aset['clips']:
   aid=f'HH_AN_{prefix}_{logical}'
   if aid not in clip_cache:
    spec={'schema':1,'animation_clip_id':aid,'logical_clip':logical,'skeleton_id':sk,'duration_seconds':duration(logical),'loop':logical in loops,'channels':channels(logical),'events':[]};(output/f'{aid}.anim.json').write_text(json.dumps(spec,indent=2)+'\n');(output/f'{aid}.anim.asset.json').write_text(json.dumps(sidecar(aid,'AnimationClipAsset',source,[sk]),indent=2)+'\n');clip_cache[aid]=True
   deps.append(aid);mappings.append({'logical_clip':logical,'asset_id':aid})
  sp={'schema':1,'animation_set_id':aset['animation_set_id'],'skeleton_id':sk,'clips':mappings};(output/f"{aset['animation_set_id']}.animset.json").write_text(json.dumps(sp,indent=2)+'\n');(output/f"{aset['animation_set_id']}.animset.asset.json").write_text(json.dumps(sidecar(aset['animation_set_id'],'AnimationSetAsset',source,deps),indent=2)+'\n');set_count+=1
 return len(skeleton_ids),len(clip_cache),set_count
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('config');p.add_argument('output');a=p.parse_args();print(generate(a.config,a.output))
