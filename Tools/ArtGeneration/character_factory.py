from __future__ import annotations
import numpy as np
import trimesh

COLORS={'skin1':(0.72,0.52,0.38,1),'skin2':(0.52,0.32,0.22,1),'skin3':(0.86,0.68,0.52,1),'skin4':(0.38,0.23,0.16,1),'navy':(0.08,0.12,0.20,1),'blue':(0.18,0.32,0.46,1),'burgundy':(0.38,0.08,0.11,1),'teal':(0.12,0.34,0.34,1),'white':(0.86,0.85,0.80,1),'black':(0.05,0.06,0.07,1),'charcoal':(0.15,0.17,0.19,1),'olive':(0.28,0.32,0.18,1),'tan':(0.52,0.39,0.25,1),'gold':(0.62,0.40,0.12,1),'purple':(0.34,0.20,0.42,1),'hair_dark':(0.08,0.05,0.03,1),'hair_brown':(0.24,0.12,0.06,1),'hair_blond':(0.58,0.43,0.20,1),'hair_grey':(0.46,0.45,0.42,1)}

def mat(name,color):return trimesh.visual.material.PBRMaterial(name=name,baseColorFactor=np.array(color)*255,metallicFactor=0.0,roughnessFactor=.65)
def box(e,c,color,name='mat'):
 m=trimesh.creation.box(extents=e);m.apply_translation(c);m.visual=trimesh.visual.TextureVisuals(material=mat(name,color));return m
def cyl(r,h,c,color,name='mat',sections=14):
 m=trimesh.creation.cylinder(radius=r,height=h,sections=sections);m.apply_translation(c);m.visual=trimesh.visual.TextureVisuals(material=mat(name,color));return m
def sph(r,c,color,name='mat'):
 m=trimesh.creation.icosphere(subdivisions=2,radius=r);m.apply_translation(c);m.visual=trimesh.visual.TextureVisuals(material=mat(name,color));return m
def add(scene,mesh,node,parent=None):
 kw={'node_name':node,'geom_name':node}
 if parent:kw['parent_node_name']=parent
 scene.add_geometry(mesh,**kw)
def role_palette(name,idx):
 if 'Receptionist' in name or 'Concierge' in name:return COLORS['navy'],COLORS['charcoal']
 if 'Bellhop' in name:return COLORS['burgundy'],COLORS['black']
 if 'Housekeeper' in name:return COLORS['teal'],COLORS['charcoal']
 if 'Chef' in name:return COLORS['white'],COLORS['black']
 if any(k in name for k in ('Waiter','Bartender')):return COLORS['black'],COLORS['charcoal']
 if 'Maintenance' in name:return COLORS['blue'],COLORS['charcoal']
 if 'Security' in name:return COLORS['navy'],COLORS['black']
 if 'Manager' in name:return COLORS['charcoal'],COLORS['black']
 guest=[COLORS['navy'],COLORS['olive'],COLORS['tan'],COLORS['burgundy'],COLORS['blue'],COLORS['purple']];return guest[idx%len(guest)],COLORS['charcoal'] if idx%2 else COLORS['tan']

def build_character(name,index):
 child='Child' in name;elderly='Elderly' in name;woman=any(k in name for k in ('Woman','Girl')) or name.endswith('Partner B') or name.endswith('Parent B')
 height=1.28 if child else (1.64 if elderly else (1.70 if woman else 1.77));scale=height/1.77;scene=trimesh.Scene();skin=COLORS[['skin1','skin2','skin3','skin4'][index%4]];cloth,pants=role_palette(name,index);hair=COLORS[['hair_dark','hair_brown','hair_blond','hair_grey'][index%4]]
 add(scene,box((.34*scale,.22*scale,.20*scale),(0,0,.84*scale),pants,'pants'),'Hips');add(scene,box((.42*scale,.24*scale,.48*scale),(0,0,1.18*scale),cloth,'cloth'),'Spine','Hips');add(scene,box((.38*scale,.23*scale,.18*scale),(0,0,1.39*scale),cloth,'cloth'),'Chest','Spine');add(scene,sph(.16*scale,(0,0,1.58*scale),skin,'skin'),'Head','Chest');add(scene,box((.30*scale,.22*scale,.07*scale),(0,.01*scale,1.70*scale),hair,'hair'),'Hair','Head')
 for side,sgn in (('L',-1),('R',1)):
  x=.27*scale*sgn;add(scene,cyl(.055*scale,.34*scale,(x,0,1.34*scale),cloth,'cloth'),'UpperArm_'+side,'Chest');add(scene,cyl(.048*scale,.31*scale,(x,0,1.05*scale),skin,'skin'),'LowerArm_'+side,'UpperArm_'+side);add(scene,sph(.065*scale,(x,0,.88*scale),skin,'skin'),'Hand_'+side,'LowerArm_'+side)
 for side,sgn in (('L',-1),('R',1)):
  x=.10*scale*sgn;add(scene,cyl(.075*scale,.42*scale,(x,0,.61*scale),pants,'pants'),'UpperLeg_'+side,'Hips');add(scene,cyl(.065*scale,.39*scale,(x,0,.24*scale),pants,'pants'),'LowerLeg_'+side,'UpperLeg_'+side);add(scene,box((.13*scale,.24*scale,.08*scale),(x,-.055*scale,.045*scale),COLORS['black'],'shoe'),'Foot_'+side,'LowerLeg_'+side)
 if 'Backpacker' in name:add(scene,box((.34*scale,.16*scale,.48*scale),(0,.18*scale,1.20*scale),COLORS['olive'],'pack'),'Accessory_Backpack','Spine')
 if 'Business' in name or 'Conference Attendee' in name or 'Manager' in name:add(scene,box((.06*scale,.02*scale,.34*scale),(0,-.13*scale,1.22*scale),COLORS['gold'],'tie'),'Accessory_Tie','Spine')
 if 'Chef' in name:add(scene,cyl(.14*scale,.18*scale,(0,0,1.82*scale),COLORS['white'],'hat',18),'Accessory_ChefHat','Head')
 if 'Bellhop' in name:add(scene,cyl(.15*scale,.08*scale,(0,0,1.77*scale),COLORS['burgundy'],'hat',18),'Accessory_BellHat','Head')
 if 'Security' in name:add(scene,box((.10*scale,.025*scale,.14*scale),(.12*scale,-.13*scale,1.36*scale),COLORS['gold'],'badge'),'Accessory_Badge','Chest')
 if 'Influencer' in name:add(scene,box((.16*scale,.08*scale,.11*scale),(.20*scale,-.20*scale,1.16*scale),COLORS['black'],'camera'),'Accessory_Camera','Spine')
 if 'Accessible Traveler' in name:add(scene,cyl(.022*scale,.90*scale,(.36*scale,0,.45*scale),COLORS['black'],'cane',10),'Accessory_Cane','Hand_R')
 scene.units='meters';return scene
