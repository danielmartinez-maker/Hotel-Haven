from __future__ import annotations
import numpy as np
import trimesh

COLORS={
 'skin1':(0.72,0.52,0.38,1),'skin2':(0.52,0.32,0.22,1),'skin3':(0.86,0.68,0.52,1),'skin4':(0.38,0.23,0.16,1),
 'navy':(0.08,0.12,0.20,1),'blue':(0.18,0.32,0.46,1),'burgundy':(0.38,0.08,0.11,1),'teal':(0.12,0.34,0.34,1),
 'white':(0.86,0.85,0.80,1),'cream':(0.72,0.66,0.54,1),'black':(0.05,0.06,0.07,1),'charcoal':(0.15,0.17,0.19,1),
 'olive':(0.28,0.32,0.18,1),'tan':(0.52,0.39,0.25,1),'gold':(0.62,0.40,0.12,1),'purple':(0.34,0.20,0.42,1),
 'denim':(0.13,0.23,0.34,1),'slate':(0.28,0.31,0.34,1),'orange':(0.68,0.31,0.09,1),'red':(0.55,0.10,0.10,1),
 'hair_dark':(0.08,0.05,0.03,1),'hair_brown':(0.24,0.12,0.06,1),'hair_blond':(0.58,0.43,0.20,1),'hair_grey':(0.46,0.45,0.42,1)
}

def mat(name,color):return trimesh.visual.material.PBRMaterial(name=name,baseColorFactor=np.array(color,dtype=float),metallicFactor=0.0,roughnessFactor=.65)
def box(e,c,color,name='mat'):
 m=trimesh.creation.box(extents=e);m.apply_translation(c);m.visual=trimesh.visual.TextureVisuals(material=mat(name,color));return m
def cyl(r,h,c,color,name='mat',sections=14):
 m=trimesh.creation.cylinder(radius=r,height=h,sections=sections);m.apply_translation(c);m.visual=trimesh.visual.TextureVisuals(material=mat(name,color));return m
def sph(r,c,color,name='mat',scale=(1,1,1)):
 m=trimesh.creation.icosphere(subdivisions=2,radius=r);m.apply_scale(scale);m.apply_translation(c);m.visual=trimesh.visual.TextureVisuals(material=mat(name,color));return m
def add(scene,mesh,node,parent=None):
 kw={'node_name':node,'geom_name':node}
 if parent:kw['parent_node_name']=parent
 scene.add_geometry(mesh,**kw)

def role_palette(name,idx):
 if 'Receptionist' in name or 'Concierge' in name:return COLORS['navy'],COLORS['charcoal']
 if 'Bellhop' in name:return COLORS['burgundy'],COLORS['black']
 if 'Housekeeper' in name:return COLORS['teal'],COLORS['charcoal']
 if 'Chef' in name:return COLORS['white'],COLORS['black']
 if 'Waiter' in name:return COLORS['black'],COLORS['charcoal']
 if 'Bartender' in name:return COLORS['charcoal'],COLORS['black']
 if 'Maintenance' in name:return COLORS['blue'],COLORS['charcoal']
 if 'Security' in name:return COLORS['navy'],COLORS['black']
 if 'Manager' in name:return COLORS['charcoal'],COLORS['black']
 guest=[COLORS['navy'],COLORS['olive'],COLORS['tan'],COLORS['burgundy'],COLORS['blue'],COLORS['purple'],COLORS['denim']]
 return guest[idx%len(guest)],COLORS['charcoal'] if idx%2 else COLORS['tan']

def body_profile(name,index):
 child='Child' in name;elderly='Elderly' in name
 if child:height=1.20+.05*(index%4)
 elif elderly:height=1.58+.035*(index%4)
 else:height=1.64+.035*(index%7)
 width=(.88,.96,1.04,1.12,1.00)[index%5];depth=(.92,1.00,1.08,.96)[index%4]
 if 'VIP' in name or 'Luxury Traveler' in name:width*=1.04
 if 'Backpacker' in name:width*=.96
 return height,width,depth

def hair_style(scene,name,index,scale,hair):
 woman=any(k in name for k in ('Woman','Girl')) or name.endswith('Partner B') or name.endswith('Parent B')
 elderly='Elderly' in name;style=(index+(2 if woman else 0))%5
 if elderly:hair=COLORS['hair_grey']
 if style==0:
  add(scene,box((.30*scale,.22*scale,.07*scale),(0,.01*scale,1.70*scale),hair,'hair'),'Hair','Head')
 elif style==1:
  add(scene,box((.29*scale,.20*scale,.08*scale),(0,.015*scale,1.70*scale),hair,'hair'),'Hair','Head')
  add(scene,box((.07*scale,.18*scale,.18*scale),(-.125*scale,.03*scale,1.61*scale),hair,'hair'),'HairSide_L','Head')
 elif style==2:
  add(scene,box((.30*scale,.21*scale,.07*scale),(0,.01*scale,1.70*scale),hair,'hair'),'Hair','Head')
  add(scene,sph(.075*scale,(0,.115*scale,1.69*scale),hair,'hair'),'HairBun','Head')
 elif style==3:
  add(scene,box((.30*scale,.21*scale,.08*scale),(0,.01*scale,1.70*scale),hair,'hair'),'Hair','Head')
  add(scene,box((.27*scale,.06*scale,.27*scale),(0,.105*scale,1.54*scale),hair,'hair'),'HairBack','Head')
 else:
  add(scene,box((.27*scale,.19*scale,.055*scale),(0,.015*scale,1.705*scale),hair,'hair'),'Hair','Head')

def add_lapels(scene,scale,color):
 add(scene,box((.07*scale,.018*scale,.29*scale),(-.075*scale,-.132*scale,1.25*scale),color,'lapel'),'Accessory_Lapel_L','Spine')
 add(scene,box((.07*scale,.018*scale,.29*scale),(.075*scale,-.132*scale,1.25*scale),color,'lapel'),'Accessory_Lapel_R','Spine')
def add_buttons(scene,scale,color,count=3):
 for i in range(count):
  add(scene,box((.026*scale,.012*scale,.026*scale),(0,-.149*scale,(1.30-i*.09)*scale),color,'button'),f'Accessory_Button_{i}','Spine')

def add_archetype_details(scene,name,index,scale,cloth,pants):
 if 'Backpacker' in name:
  add(scene,box((.34*scale,.16*scale,.48*scale),(0,.18*scale,1.20*scale),COLORS['olive'],'pack'),'Accessory_Backpack','Spine')
  add(scene,box((.05*scale,.04*scale,.45*scale),(-.14*scale,-.13*scale,1.23*scale),COLORS['black'],'strap'),'Accessory_PackStrap','Spine')
 if 'Tourist' in name:
  add(scene,box((.18*scale,.07*scale,.19*scale),(.16*scale,-.16*scale,1.02*scale),COLORS['tan'],'bag'),'Accessory_CrossbodyBag','Spine')
 if 'Business' in name or 'Conference Attendee' in name or 'Manager' in name or 'VIP' in name:
  add_lapels(scene,scale,COLORS['charcoal']);add(scene,box((.055*scale,.018*scale,.32*scale),(0,-.145*scale,1.22*scale),COLORS['gold'],'tie'),'Accessory_Tie','Spine')
 if 'Business' in name:
  add(scene,box((.28*scale,.10*scale,.34*scale),(.30*scale,-.02*scale,.62*scale),COLORS['tan'],'briefcase'),'Accessory_Briefcase','Hand_R')
  add(scene,box((.12*scale,.035*scale,.05*scale),(.30*scale,-.02*scale,.82*scale),COLORS['black'],'briefcase_handle'),'Accessory_BriefcaseHandle','Hand_R')
 if 'Family Parent' in name:
  add(scene,box((.28*scale,.12*scale,.34*scale),(-.29*scale,-.01*scale,.72*scale),COLORS['cream'],'family_tote'),'Accessory_FamilyTote','Hand_L')
  add(scene,box((.035*scale,.05*scale,.38*scale),(-.29*scale,.02*scale,.95*scale),COLORS['tan'],'tote_strap'),'Accessory_FamilyToteStrap','Hand_L')
 if 'Child' in name:
  add(scene,box((.27*scale,.12*scale,.34*scale),(0,.15*scale,1.13*scale),COLORS['blue'],'child_pack'),'Accessory_ChildBackpack','Spine')
  for side,sgn in (('L',-1),('R',1)):
   add(scene,box((.035*scale,.025*scale,.28*scale),(.10*scale*sgn,-.12*scale,1.19*scale),COLORS['black'],'child_strap'),f'Accessory_ChildPackStrap_{side}','Spine')
 if 'Influencer' in name:
  add(scene,box((.16*scale,.08*scale,.11*scale),(.20*scale,-.20*scale,1.16*scale),COLORS['black'],'camera'),'Accessory_Camera','Spine')
  add(scene,cyl(.025*scale,.20*scale,(.20*scale,-.20*scale,1.30*scale),COLORS['black'],'camera',10),'Accessory_CameraGrip','Spine')
 if 'Long Stay' in name:
  add(scene,box((.37*scale,.025*scale,.40*scale),(0,-.135*scale,1.20*scale),COLORS['cream'],'cardigan'),'Accessory_Cardigan','Spine')
 if 'Budget Traveler' in name:
  add(scene,box((.24*scale,.025*scale,.10*scale),(0,-.14*scale,1.08*scale),COLORS['slate'],'pocket'),'Accessory_HoodiePocket','Spine')
  add(scene,box((.23*scale,.08*scale,.17*scale),(0,.08*scale,1.52*scale),cloth,'hood'),'Accessory_Hood','Chest')
 if 'Luxury Traveler' in name:
  add_lapels(scene,scale,COLORS['gold']);add(scene,box((.44*scale,.025*scale,.54*scale),(0,.13*scale,1.17*scale),cloth,'coat'),'Accessory_CoatBack','Spine')
 if 'Honeymooner' in name:
  add(scene,sph(.025*scale,(.11*scale,-.15*scale,1.36*scale),COLORS['red'],'flower'),'Accessory_Boutonniere','Chest')
 if 'Accessible Traveler' in name:
  add(scene,cyl(.022*scale,.90*scale,(.36*scale,0,.45*scale),COLORS['black'],'cane',10),'Accessory_Cane','Hand_R')

def add_staff_details(scene,name,scale):
 if 'Receptionist' in name or 'Concierge' in name:
  add_lapels(scene,scale,COLORS['gold']);add(scene,box((.08*scale,.02*scale,.04*scale),(.12*scale,-.145*scale,1.36*scale),COLORS['gold'],'badge'),'Accessory_NameBadge','Chest')
 if 'Receptionist' in name:
  add(scene,box((.23*scale,.055*scale,.31*scale),(.20*scale,-.20*scale,1.08*scale),COLORS['slate'],'folder'),'Accessory_ReceptionFolder','Chest')
 if 'Concierge' in name:
  sash=box((.075*scale,.022*scale,.52*scale),(0,-.16*scale,1.20*scale),COLORS['gold'],'sash')
  sash.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(25.0),(0,1,0),point=(0,-.16*scale,1.20*scale)))
  add(scene,sash,'Accessory_ConciergeSash','Chest')
 if 'Bellhop' in name:
  add(scene,cyl(.15*scale,.08*scale,(0,0,1.77*scale),COLORS['burgundy'],'hat',18),'Accessory_BellHat','Head');add_buttons(scene,scale,COLORS['gold'],4)
 if 'Housekeeper' in name:
  add(scene,box((.30*scale,.025*scale,.42*scale),(0,-.145*scale,1.10*scale),COLORS['cream'],'apron'),'Accessory_Apron','Spine')
 if 'Chef' in name:
  add(scene,cyl(.14*scale,.18*scale,(0,0,1.82*scale),COLORS['white'],'hat',18),'Accessory_ChefHat','Head');add_buttons(scene,scale,COLORS['black'],4);add(scene,box((.30*scale,.025*scale,.38*scale),(0,-.145*scale,1.08*scale),COLORS['white'],'apron'),'Accessory_Apron','Spine')
 if 'Waiter' in name:
  add(scene,box((.31*scale,.025*scale,.34*scale),(0,-.145*scale,1.05*scale),COLORS['black'],'apron'),'Accessory_WaiterApron','Spine')
 if 'Bartender' in name:
  add_lapels(scene,scale,COLORS['white']);add(scene,box((.31*scale,.025*scale,.29*scale),(0,-.145*scale,1.18*scale),COLORS['charcoal'],'vest'),'Accessory_Vest','Spine')
 if 'Maintenance' in name:
  add(scene,box((.42*scale,.018*scale,.06*scale),(0,-.145*scale,1.32*scale),COLORS['orange'],'stripe'),'Accessory_HiVisStripe','Chest');add(scene,box((.38*scale,.05*scale,.08*scale),(0,0,.85*scale),COLORS['black'],'belt'),'Accessory_ToolBelt','Hips')
  add(scene,box((.15*scale,.08*scale,.21*scale),(.18*scale,-.02*scale,.78*scale),COLORS['tan'],'tool_pouch'),'Accessory_ToolPouch','Hips')
 if 'Security' in name:
  add(scene,box((.10*scale,.025*scale,.14*scale),(.12*scale,-.13*scale,1.36*scale),COLORS['gold'],'badge'),'Accessory_Badge','Chest');add(scene,box((.38*scale,.05*scale,.07*scale),(0,0,.85*scale),COLORS['black'],'belt'),'Accessory_DutyBelt','Hips')
  add(scene,box((.07*scale,.035*scale,.16*scale),(-.14*scale,-.14*scale,1.34*scale),COLORS['black'],'radio'),'Accessory_Radio','Chest')
  add(scene,box((.012*scale,.012*scale,.15*scale),(-.14*scale,-.14*scale,1.49*scale),COLORS['black'],'antenna'),'Accessory_RadioAntenna','Chest')
 if 'Manager' in name:
  add_lapels(scene,scale,COLORS['gold']);add(scene,box((.055*scale,.018*scale,.32*scale),(0,-.145*scale,1.22*scale),COLORS['burgundy'],'tie'),'Accessory_ManagerTie','Spine')

def build_character(name,index):
 child='Child' in name
 height,width,depth=body_profile(name,index);scale=height/1.77;scene=trimesh.Scene();skin=COLORS[['skin1','skin2','skin3','skin4'][index%4]];cloth,pants=role_palette(name,index);hair=COLORS[['hair_dark','hair_brown','hair_blond','hair_grey'][index%4]]
 hip_w=.34*scale*width;torso_w=.42*scale*width;torso_d=.24*scale*depth;arm_x=.27*scale*width;leg_x=.10*scale*width
 add(scene,box((hip_w,.22*scale*depth,.20*scale),(0,0,.84*scale),pants,'pants'),'Hips')
 add(scene,box((torso_w,torso_d,.48*scale),(0,0,1.18*scale),cloth,'cloth'),'Spine','Hips')
 add(scene,box((.38*scale*width,.23*scale*depth,.18*scale),(0,0,1.39*scale),cloth,'cloth'),'Chest','Spine')
 add(scene,sph(.16*scale,(0,0,1.58*scale),skin,'skin',scale=(.94+.04*(index%3),1.0,.96+(.04 if child else 0))),'Head','Chest')
 hair_style(scene,name,index,scale,hair)
 for side,sgn in (('L',-1),('R',1)):
  x=arm_x*sgn;add(scene,cyl(.055*scale,.34*scale,(x,0,1.34*scale),cloth,'cloth'),'UpperArm_'+side,'Chest');add(scene,cyl(.048*scale,.31*scale,(x,0,1.05*scale),skin,'skin'),'LowerArm_'+side,'UpperArm_'+side);add(scene,sph(.065*scale,(x,0,.88*scale),skin,'skin'),'Hand_'+side,'LowerArm_'+side)
 for side,sgn in (('L',-1),('R',1)):
  x=leg_x*sgn;leg_r=.075*scale*(.92+.08*width);add(scene,cyl(leg_r,.42*scale,(x,0,.61*scale),pants,'pants'),'UpperLeg_'+side,'Hips');add(scene,cyl(.065*scale,.39*scale,(x,0,.24*scale),pants,'pants'),'LowerLeg_'+side,'UpperLeg_'+side);add(scene,box((.13*scale,.24*scale,.08*scale),(x,-.055*scale,.045*scale),COLORS['black'],'shoe'),'Foot_'+side,'LowerLeg_'+side)
 add_archetype_details(scene,name,index,scale,cloth,pants);add_staff_details(scene,name,scale)
 scene.units='meters';return scene
