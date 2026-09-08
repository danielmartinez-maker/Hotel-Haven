from __future__ import annotations
import argparse, json
from pathlib import Path
import numpy as np
import trimesh

PALETTE={
'MAT_WOOD_WARM':((0.42,0.22,0.10,1),0,0.52),'MAT_WOOD_DARK':((0.22,0.12,0.08,1),0,0.48),
'MAT_CARPET_LUXURY':((0.22,0.25,0.29,1),0,0.84),'MAT_EMISSIVE_WARM':((0.90,0.68,0.34,1),0,0.36),
'MAT_VEGETATION':((0.25,0.38,0.20,1),0,0.74),'MAT_STONE_LIGHT':((0.60,0.56,0.49,1),0,0.70),
'MAT_BRASS_POLISHED':((0.62,0.38,0.10,1),0.88,0.30),'MAT_BRASS_BRUSHED':((0.58,0.36,0.12,1),0.78,0.42),
'MAT_BLACKENED_STEEL':((0.10,0.12,0.13,1),0.78,0.50),'MAT_SIGNAGE':((0.16,0.17,0.17,1),0.25,0.44),
'MAT_ELECTRONICS':((0.08,0.09,0.10,1),0.12,0.36),'MAT_UPHOLSTERY':((0.42,0.31,0.25,1),0,0.84),
'MAT_PLASTIC_RUBBER':((0.18,0.18,0.18,1),0,0.64),'MAT_STAINLESS':((0.58,0.61,0.62,1),0.85,0.42),
'MAT_GLASS_CLEAR':((0.34,0.55,0.64,0.42),0,0.10),'MAT_LINEN':((0.88,0.86,0.80,1),0,0.88),
'MAT_CERAMIC_FIXTURE':((0.90,0.89,0.85,1),0,0.24),'MAT_MARBLE_LIGHT':((0.88,0.84,0.76,1),0,0.38)}

def material(n):
 rgba,m,r=PALETTE.get(n,PALETTE['MAT_WOOD_WARM']); return trimesh.visual.material.PBRMaterial(name=n,baseColorFactor=np.array(rgba)*255,metallicFactor=m,roughnessFactor=r)
def box(e,c=(0,0,0),m='MAT_WOOD_WARM'):
 x=trimesh.creation.box(extents=e); x.apply_translation(c); x.visual=trimesh.visual.TextureVisuals(material=material(m)); return x
def cyl(rad,h,c=(0,0,0),m='MAT_STAINLESS',sec=16):
 x=trimesh.creation.cylinder(radius=rad,height=h,sections=sec); x.apply_translation(c); x.visual=trimesh.visual.TextureVisuals(material=material(m)); return x
def sph(rad,c=(0,0,0),m='MAT_VEGETATION'):
 x=trimesh.creation.icosphere(subdivisions=2,radius=rad); x.apply_translation(c); x.visual=trimesh.visual.TextureVisuals(material=material(m)); return x
def add(s,x,n): s.add_geometry(x,node_name=n,geom_name=n)

def table(name,mat):
 s=trimesh.Scene(); w,d,h=0.62,0.62,0.56
 if 'Console' in name:w,d,h=1.45,0.38,0.82
 if 'Dining Table Two' in name:w,d,h=0.80,0.80,0.75
 if 'Dining Table Four' in name:w,d,h=1.25,0.85,0.75
 if 'Dining Table Six' in name:w,d,h=1.80,0.90,0.75
 add(s,box((w,d,0.07),(0,0,h),mat),'TableTop')
 for x in (-w*.4,w*.4):
  for y in (-d*.35,d*.35): add(s,box((.055,.055,h),(x,y,h/2),'MAT_WOOD_DARK'),f'Leg_{x}_{y}')
 return s

def rug():
 s=trimesh.Scene(); add(s,box((2.4,1.7,.028),(0,0,.014),'MAT_CARPET_LUXURY'),'Rug'); add(s,box((2.15,1.45,.008),(0,0,.032),'MAT_CARPET_LUXURY'),'PatternInset'); return s

def lamp(name):
 s=trimesh.Scene()
 if 'Chandelier' in name:
  add(s,cyl(.025,.85,(0,0,1.55),'MAT_BRASS_POLISHED',12),'Stem'); add(s,cyl(.36,.035,(0,0,1.15),'MAT_BRASS_POLISHED',24),'Ring')
  for i,a in enumerate(np.linspace(0,2*np.pi,8,endpoint=False)):
   x,y=.36*np.cos(a),.36*np.sin(a); add(s,cyl(.025,.32,(x,y,1.02),'MAT_BRASS_POLISHED',10),f'Arm_{i}'); add(s,sph(.075,(x,y,.84),'MAT_EMISSIVE_WARM'),f'Bulb_{i}')
 elif 'Floor Lamp' in name:
  add(s,cyl(.18,.05,(0,0,.025),'MAT_BRASS_POLISHED',20),'Base'); add(s,cyl(.025,1.35,(0,0,.70),'MAT_BRASS_POLISHED',12),'Stem'); sh=trimesh.creation.cone(.28,.34,sections=24); sh.apply_translation((0,0,1.48)); sh.visual=trimesh.visual.TextureVisuals(material=material('MAT_EMISSIVE_WARM')); add(s,sh,'Shade')
 else:
  add(s,cyl(.13,.04,(0,0,.02),'MAT_BRASS_POLISHED',20),'Base'); add(s,cyl(.02,.35,(0,0,.20),'MAT_BRASS_POLISHED',12),'Stem'); sh=trimesh.creation.cone(.21,.25,sections=24); sh.apply_translation((0,0,.50)); sh.visual=trimesh.visual.TextureVisuals(material=material('MAT_EMISSIVE_WARM')); add(s,sh,'Shade')
 return s

def planter(name):
 s=trimesh.Scene(); tall='Tall' in name; h=.62 if tall else .34; rad=.28 if tall else .38
 add(s,cyl(rad,h,(0,0,h/2),'MAT_STONE_LIGHT',24),'Planter')
 if 'Tree' in name:
  add(s,cyl(.07,1.35,(0,0,h+0.65),'MAT_WOOD_DARK',12),'Trunk')
  for i,(x,y,z) in enumerate(((0,0,1.8),(.25,0,1.55),(-.25,.08,1.58),(0,.22,1.65))): add(s,sph(.34,(x,y,z),'MAT_VEGETATION'),f'Canopy_{i}')
 else:
  for i,a in enumerate(np.linspace(0,2*np.pi,7,endpoint=False)):
   x,y=.18*np.cos(a),.18*np.sin(a); leaf=box((.10,.035,.55),(x,y,h+.27),'MAT_VEGETATION'); leaf.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(25),[-y,x,0],[x,y,h+.27])); add(s,leaf,f'Leaf_{i}')
 return s

def fountain():
 s=trimesh.Scene(); add(s,cyl(.62,.18,(0,0,.09),'MAT_STONE_LIGHT',32),'Basin'); add(s,cyl(.18,.52,(0,0,.35),'MAT_STONE_LIGHT',24),'Pedestal'); add(s,cyl(.35,.10,(0,0,.66),'MAT_STONE_LIGHT',32),'UpperBowl'); add(s,cyl(.025,.42,(0,0,.91),'MAT_STAINLESS',12),'WaterJet'); return s

def cart(name,mat):
 s=trimesh.Scene(); w,l=1.05,1.35; add(s,box((w,l,.10),(0,0,.26),mat),'CartBase')
 for x in (-w*.44,w*.44): add(s,cyl(.025,1.55,(x,0,1.0),mat,12),f'Upright_{x}')
 add(s,box((w,.05,.05),(0,0,1.75),mat),'TopBar')
 if 'Covered' in name:add(s,box((w*.92,l*.75,.75),(0,0,.85),'MAT_LINEN'),'Cover')
 for i,(x,y) in enumerate(((-.42,-.5),(.42,-.5),(-.42,.5),(.42,.5))):
  wheel=cyl(.09,.05,(x,y,.10),'MAT_BLACKENED_STEEL',16); wheel.apply_transform(trimesh.transformations.rotation_matrix(np.pi/2,[1,0,0],[x,y,.10])); add(s,wheel,f'MOV_Wheel_{i}')
 return s

def stanchion(name,mat):
 s=trimesh.Scene()
 for i,x in enumerate((-.62,.62)):
  add(s,cyl(.16,.05,(x,0,.025),mat,20),f'Base_{i}'); add(s,cyl(.025,.95,(x,0,.50),mat,12),f'Post_{i}'); add(s,sph(.055,(x,0,.99),mat),f'Cap_{i}')
 rope=box((1.15,.055,.055),(0,0,.82),'MAT_UPHOLSTERY' if 'Rope' in name else mat); add(s,rope,'Barrier')
 return s

def kiosk(name):
 s=trimesh.Scene(); w,d,h=.55,.38,1.25
 if 'Computer' in name:
  add(s,box((.52,.06,.34),(0,0,.72),'MAT_ELECTRONICS'),'Monitor'); add(s,box((.38,.20,.035),(0,-.12,.47),'MAT_ELECTRONICS'),'Keyboard'); return s
 if 'Telephone' in name:
  add(s,box((.34,.24,.12),(0,0,.06),'MAT_ELECTRONICS'),'PhoneBase'); add(s,box((.30,.06,.07),(0,0,.16),'MAT_ELECTRONICS'),'Handset'); return s
 if 'Encoder' in name:
  add(s,box((.40,.30,.14),(0,0,.07),'MAT_ELECTRONICS'),'EncoderBody'); add(s,box((.24,.13,.02),(0,-.08,.15),'MAT_GLASS_CLEAR'),'CardBed'); return s
 add(s,box((w,d,h),(0,0,h/2),'MAT_ELECTRONICS'),'KioskBody'); add(s,box((w*.72,.025,.38),(0,-d/2-.014,.88),'MAT_GLASS_CLEAR'),'Screen'); add(s,box((w*.55,.05,.08),(0,-d/2-.035,.56),'MAT_ELECTRONICS'),'InputShelf'); return s

def rack(name,mat):
 s=trimesh.Scene(); w,d,h=.75,.32,1.25
 if 'Umbrella' in name:w,d,h=.42,.42,.62
 if 'Coat Check' in name:
  add(s,cyl(.035,1.55,(0,0,.78),'MAT_WOOD_WARM',12),'Post'); add(s,box((1.15,.08,.08),(0,0,1.50),'MAT_WOOD_WARM'),'Rail')
  for i,x in enumerate(np.linspace(-.5,.5,6)):add(s,cyl(.015,.18,(x,0,1.35),'MAT_BRASS_POLISHED',10),f'Hook_{i}')
  return s
 add(s,box((w,d,.07),(0,0,.035),mat),'RackBase')
 for i,z in enumerate(np.linspace(.30,h-.12,4)): add(s,box((w*.90,d*.85,.035),(0,0,z),mat),f'Shelf_{i}')
 for x in (-w/2,w/2):add(s,box((.045,.045,h),(x,0,h/2),mat),f'Upright_{x}')
 return s

def bench(name,mat):
 s=trimesh.Scene(); w,d,h=1.55,.52,.47; add(s,box((w,d,.16),(0,0,h),mat),'Seat')
 if 'Upholstered' in name:add(s,box((w,.14,.62),(0,d/2-.03,h+.28),mat),'Back')
 for x in (-w*.40,w*.40):add(s,box((.07,.07,h),(x,0,h/2),'MAT_WOOD_DARK'),f'Leg_{x}')
 return s

def utility(name,mat):
 s=trimesh.Scene()
 if 'Waste Bin' in name or 'Recycling Bin' in name:
  add(s,cyl(.22,.60,(0,0,.30),mat,20),'Bin'); add(s,cyl(.20,.035,(0,0,.62),'MAT_BLACKENED_STEEL',20),'Rim'); return s
 if 'Water Dispenser' in name:
  add(s,box((.45,.42,.90),(0,0,.45),'MAT_ELECTRONICS'),'Dispenser'); add(s,cyl(.16,.38,(0,0,1.07),'MAT_GLASS_CLEAR',20),'WaterBottle'); return s
 if 'Coffee Station' in name:
  add(s,box((.90,.50,.88),(0,0,.44),'MAT_WOOD_WARM'),'Cabinet'); add(s,box((.40,.30,.38),(.10,-.04,1.08),'MAT_ELECTRONICS'),'CoffeeMachine'); add(s,cyl(.05,.12,(-.28,-.08,.98),'MAT_CERAMIC_FIXTURE',16),'Cup'); return s
 if 'Charging Station' in name:
  add(s,box((.50,.34,.76),(0,0,.38),'MAT_ELECTRONICS'),'ChargingBody')
  for i,x in enumerate((-.15,0,.15)): add(s,box((.06,.02,.04),(x,-.18,.63),'MAT_GLASS_CLEAR'),f'Port_{i}')
  return s
 if 'Feedback' in name:
  add(s,box((.45,.30,1.05),(0,0,.525),'MAT_WOOD_WARM'),'FeedbackStand'); add(s,box((.28,.02,.035),(0,-.16,.90),'MAT_SIGNAGE'),'Slot'); return s
 if 'Clock' in name:
  add(s,cyl(.34,.055,(0,0,.34),'MAT_WOOD_WARM',32),'ClockFrame'); add(s,cyl(.30,.025,(0,-.04,.34),'MAT_SIGNAGE',32),'ClockFace'); return s
 if 'Pedestal' in name or 'Plinth' in name:
  add(s,box((.55,.55,1.00),(0,0,.50),'MAT_STONE_LIGHT'),'Plinth'); add(s,box((.64,.64,.08),(0,0,1.04),'MAT_MARBLE_LIGHT'),'Top'); return s
 if 'Wayfinding' in name or 'Directory' in name:
  add(s,box((.58,.18,1.50),(0,0,.75),'MAT_SIGNAGE'),'SignBody'); add(s,box((.48,.025,.72),(0,-.105,1.02),'MAT_GLASS_CLEAR'),'InformationPanel'); return s
 if 'Floral' in name:
  add(s,cyl(.16,.30,(0,0,.15),'MAT_GLASS_CLEAR',20),'Vase')
  for i,a in enumerate(np.linspace(0,2*np.pi,9,endpoint=False)):add(s,sph(.09,(.18*np.cos(a),.18*np.sin(a),.55+.08*(i%3)),'MAT_VEGETATION'),f'Flower_{i}')
  return s
 if 'Partition Screen' in name:
  for i,x in enumerate((-.60,0,.60)):add(s,box((.55,.08,1.75),(x,0,.875),'MAT_WOOD_WARM'),f'Panel_{i}')
  return s
 if 'Fireplace' in name:
  add(s,box((1.45,.35,.95),(0,0,.475),'MAT_STONE_LIGHT'),'FireplaceBody'); add(s,box((.90,.03,.48),(0,-.19,.42),'MAT_ELECTRONICS'),'EmberScreen'); add(s,box((1.55,.45,.09),(0,0,.99),'MAT_WOOD_WARM'),'Mantel'); return s
 if 'Piano' in name:
  add(s,box((1.45,.62,.88),(0,0,.44),'MAT_WOOD_WARM'),'PianoBody'); add(s,box((1.20,.32,.06),(0,-.34,.70),'MAT_WOOD_WARM'),'KeyboardBed')
  for i,x in enumerate(np.linspace(-.52,.52,12)):add(s,box((.07,.25,.025),(x,-.38,.73),'MAT_LINEN'),f'Key_{i}')
  return s
 return s

def banquette(name):
 s=trimesh.Scene()
 if 'Corner' in name:
  add(s,box((1.40,.62,.18),(-.36,0,.48),'MAT_UPHOLSTERY'),'SeatA'); add(s,box((.62,1.40,.18),(.40,.38,.48),'MAT_UPHOLSTERY'),'SeatB'); add(s,box((1.40,.14,.70),(-.36,.25,.80),'MAT_UPHOLSTERY'),'BackA'); add(s,box((.14,1.40,.70),(.70,.38,.80),'MAT_UPHOLSTERY'),'BackB')
 else:
  add(s,box((1.65,.62,.18),(0,0,.48),'MAT_UPHOLSTERY'),'Seat'); add(s,box((1.65,.14,.70),(0,.25,.80),'MAT_UPHOLSTERY'),'Back')
 return s

def build(name,mat):
 if 'Table' in name and 'Charging' not in name:return table(name,mat)
 if 'Rug' in name:return rug()
 if 'Lamp' in name or 'Chandelier' in name:return lamp(name)
 if 'Planter' in name or 'Indoor Tree' in name:return planter(name)
 if 'Fountain' in name:return fountain()
 if 'Cart' in name:return cart(name,mat)
 if 'Queue' in name:return stanchion(name,mat)
 if 'Kiosk' in name or 'Computer' in name or 'Telephone' in name or 'Encoder' in name:return kiosk(name)
 if any(k in name for k in ('Rack','Newspaper Stand','Umbrella Stand','Coat Check')):return rack(name,mat)
 if 'Bench' in name:return bench(name,mat)
 if 'Banquette' in name:return banquette(name)
 if 'Document Tray' in name:
  s=trimesh.Scene(); add(s,box((.42,.30,.045),(0,0,.022),mat),'Tray'); return s
 return utility(name,mat)

def sidecar(aid,sub,mat,profile,anim):
 tags=['hotel-haven','batch_05',sub]
 if anim:tags+=['animated-binding',anim]
 return {'schema':1,'asset_id':aid,'asset_type':'StaticMeshAsset','source':'Tools/ArtGeneration/batch05_generate.py','units':'meters','lod_policy':'lod_furniture','collision_policy':'simple_proxy','material_slots':[mat],'tags':tags,'dependencies':[],'cutaway_policy':'normal','source_revision':1,'metadata_revision':1,'cooker_schema':1,'lifecycle_state':'PRODUCTION'}
def generate(manifest:Path,out:Path,package:bool):
 rows=[a for g in json.loads(manifest.read_text())['groups'] for a in g['assets']]
 if len(rows)!=50:raise ValueError(len(rows))
 out.mkdir(parents=True,exist_ok=True); res=[]
 for aid,name,sub,mat,profile,anim,anchors in rows:
  s=build(name,mat); p=out/f'{aid}.glb'; p.write_bytes(s.export(file_type='glb'));res.append(p)
  if package:(out/f'{aid}.asset.json').write_text(json.dumps(sidecar(aid,sub,mat,profile,anim),indent=2)+'\n')
  print(p)
 return res
def main():
 ap=argparse.ArgumentParser();ap.add_argument('manifest',type=Path);ap.add_argument('output',type=Path);ap.add_argument('--package',action='store_true');a=ap.parse_args();generate(a.manifest,a.output,a.package)
if __name__=='__main__':main()
