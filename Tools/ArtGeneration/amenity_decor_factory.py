from __future__ import annotations
import numpy as np
import trimesh
from service_asset_factory import material, box, cyl, add, chair, table_like, shelf_or_cabinet

def sph(radius, center=(0,0,0), mat='MAT_VEGETATION'):
    m=trimesh.creation.icosphere(subdivisions=2,radius=radius);m.apply_translation(center);m.visual=trimesh.visual.TextureVisuals(material=material(mat));return m

def rack(name,mat):
    s=trimesh.Scene();add(s,box((1.0,.42,.08),(0,0,.04),mat),'Base')
    for x in (-.44,.44):add(s,box((.05,.05,1.4),(x,0,.70),mat),f'Upright_{x}')
    for i,z in enumerate((.34,.68,1.02,1.34)):add(s,box((.90,.36,.035),(0,0,z),mat),f'Shelf_{i}')
    if 'Dumbbell' in name:
        for j,z in enumerate((.42,.76,1.10)):
            for i,x in enumerate(np.linspace(-.33,.33,4)):add(s,cyl(.035,.24,(x,0,z),'MAT_BLACKENED_STEEL',12),f'Weight_{j}_{i}')
    if 'Exercise Ball' in name:
        s=trimesh.Scene()
        for i,(x,z) in enumerate(((-.28,.35),(.28,.35),(-.28,.88),(.28,.88))):add(s,sph(.22,(x,0,z),'MAT_UPHOLSTERY'),f'Ball_{i}')
    return s

def mirror_wall():
    s=trimesh.Scene();add(s,box((2.4,.04,1.8),(0,0,.9),'MAT_GLASS_CLEAR'),'Mirror');add(s,box((2.5,.08,.07),(0,0,1.82),'MAT_BLACKENED_STEEL'),'TopFrame');return s

def water_station():
    s=trimesh.Scene();add(s,box((.52,.42,1.02),(0,0,.51),'MAT_ELECTRONICS'),'Body');add(s,cyl(.16,.34,(0,0,1.20),'MAT_GLASS_CLEAR',20),'Bottle');return s

def lounger(name,mat):
    s=trimesh.Scene();w,d=.68,1.78;add(s,box((w,d,.14),(0,0,.42),mat),'Seat');back=box((w,.12,.76),(0,.72,.80),mat);back.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-18),[1,0,0],[0,.72,.80]));add(s,back,'Back');return s

def massage_table():
    s=trimesh.Scene();add(s,box((.72,1.85,.15),(0,0,.72),'MAT_UPHOLSTERY'),'Pad')
    for i,(x,y) in enumerate(((-.28,-.72),(.28,-.72),(-.28,.72),(.28,.72))):add(s,box((.055,.055,.70),(x,y,.35),'MAT_WOOD_DARK'),f'Leg_{i}')
    add(s,cyl(.10,.05,(0,.82,.82),'MAT_UPHOLSTERY',18),'FaceCradle');return s

def bench(name,mat):
    s=trimesh.Scene();w=1.45;add(s,box((w,.52,.13),(0,0,.47),mat),'Seat');add(s,box((w,.10,.55),(0,.21,.74),mat),'Back');return s

def conference_table(name,mat):
    s=trimesh.Scene();w,d,h=(2.8,1.15,.76) if 'Large' in name else (1.7,.92,.76)
    if 'Round' in name:add(s,cyl(.72,.08,(0,0,h),'MAT_WOOD_WARM',28),'Top');add(s,cyl(.14,h,(0,0,h/2),'MAT_BLACKENED_STEEL',16),'Pedestal');return s
    add(s,box((w,d,.08),(0,0,h),mat),'Top')
    for x in (-w*.38,w*.38):add(s,box((.08,.55,.70),(x,0,.35),'MAT_BLACKENED_STEEL'),f'Leg_{x}')
    return s

def podium():
    s=trimesh.Scene();add(s,box((.66,.48,1.08),(0,0,.54),'MAT_WOOD_WARM'),'PodiumBody');add(s,box((.72,.54,.08),(0,-.03,1.12),'MAT_WOOD_WARM'),'PodiumTop');return s

def screen_or_board(name,mat):
    s=trimesh.Scene();add(s,box((1.9,.055,1.15),(0,0,.92),mat if 'Whiteboard' in name else 'MAT_ELECTRONICS'),'Panel');return s

def stage_or_floor(name,mat):
    s=trimesh.Scene();h=.20 if 'Stage' in name else .06;add(s,box((1.8,1.8,h),(0,0,h/2),mat),'Module');return s

def divider():
    s=trimesh.Scene()
    for i,x in enumerate((-.62,0,.62)):add(s,box((.58,.08,1.82),(x,0,.91),'MAT_WOOD_WARM'),f'Panel_{i}')
    return s

def av_rack():return shelf_or_cabinet('AV Equipment Rack','MAT_BLACKENED_STEEL')
def game_table():return conference_table('Conference Table Small','MAT_WOOD_WARM')
def framed_art(name,mat):
    s=trimesh.Scene();add(s,box((.82,.055,.62),(0,0,.31),'MAT_WOOD_DARK'),'Frame');add(s,box((.70,.018,.50),(0,-.035,.31),mat),'ArtPanel');return s

def sculpture(name,mat):
    s=trimesh.Scene();add(s,box((.38,.38,.16),(0,0,.08),'MAT_STONE_LIGHT'),'Base');add(s,sph(.23,(0,0,.42),mat),'Form');return s

def vase(name,mat):
    s=trimesh.Scene();h=.62 if 'Tall' in name else .34;add(s,cyl(.16,h,(0,0,h/2),mat,24),'Vase');return s

def books_or_magazines(name):
    s=trimesh.Scene()
    for i in range(4):add(s,box((.34+.02*i,.24,.045),(0,0,.025+.05*i),'MAT_SIGNAGE'),f'Item_{i}')
    return s

def candles():
    s=trimesh.Scene()
    for i,(x,h) in enumerate(((-.14,.28),(0,.38),(.14,.22))):add(s,cyl(.045,h,(x,0,h/2),'MAT_EMISSIVE_WARM',16),f'Candle_{i}')
    return s

def clock(name,mat):
    s=trimesh.Scene();r=.28 if 'Wall' in name else .16;add(s,cyl(r,.045,(0,0,r),mat,28),'ClockBody');return s

def plant(name):
    s=trimesh.Scene();scale=.45 if 'Small' in name else .72 if 'Medium' in name else 1.05;add(s,cyl(.18*scale,.30*scale,(0,0,.15*scale),'MAT_STONE_LIGHT',20),'Pot');add(s,cyl(.035*scale,.65*scale,(0,0,.50*scale),'MAT_WOOD_DARK',12),'Stem')
    for i,a in enumerate(np.linspace(0,2*np.pi,7,endpoint=False)):add(s,sph(.16*scale,(.18*scale*np.cos(a),.18*scale*np.sin(a),.78*scale),'MAT_VEGETATION'),f'Leaf_{i}')
    return s

def flower_arrangement(name):return plant('Potted Plant Medium' if 'Table' in name else 'Potted Plant Large')
def sign(name,mat):
    s=trimesh.Scene();w,h=(.62,.72) if 'Directory' in name else (.46,.22);add(s,box((w,.035,h),(0,0,h/2),mat),'SignFace');return s

def cup_or_glass(name,mat):
    s=trimesh.Scene();h=.10 if 'Cup' in name else .14;add(s,cyl(.05,h,(0,0,h/2),mat,18),'Vessel');return s

def tray_or_bowl(name,mat):
    s=trimesh.Scene();add(s,cyl(.18,.035,(0,0,.02),mat,24),'Base');return s

def towel_stack():
    s=trimesh.Scene()
    for i in range(4):add(s,box((.34,.24,.055),(0,0,.03+.06*i),'MAT_LINEN'),f'Towel_{i}')
    return s

def bottle_set():
    s=trimesh.Scene()
    for i,x in enumerate((-.12,0,.12)):add(s,cyl(.035,.16,(x,0,.08),'MAT_PLASTIC_RUBBER',14),f'Bottle_{i}')
    return s

def exterior_module(name,mat):
    s=trimesh.Scene()
    if 'Entrance Module' in name:add(s,box((3.2,.30,3.0),(0,0,1.5),mat),'Facade');add(s,box((1.7,.08,2.35),(0,-.19,1.18),'MAT_GLASS_CLEAR'),'EntranceGlass');return s
    if 'Window Bay' in name:
        add(s,box((2.6,.30,3.0),(0,0,1.5),mat),'Facade')
        for i,x in enumerate((-0.75,0,.75)):add(s,box((.52,.05,1.35),(x,-.18,1.55),'MAT_GLASS_CLEAR'),f'Window_{i}')
        return s
    if 'Balcony' in name:add(s,box((2.6,.30,3.0),(0,0,1.5),mat),'Facade');add(s,box((1.6,.75,.12),(0,-.48,1.18),'MAT_STONE_LIGHT'),'BalconySlab');return s
    if 'Corner Tower' in name:add(s,box((1.8,1.8,3.2),(0,0,1.6),mat),'Tower');return s
    if 'Steps' in name:
        for i in range(4):add(s,box((1.8,1.2-.22*i,.16),(0,.12*i,.08+.16*i),mat),f'Step_{i}')
        return s
    if 'Ramp' in name:add(s,box((1.4,2.8,.14),(0,0,.32),mat),'Ramp');return s
    add(s,box((1.0,1.0,.08),(0,0,.04),mat),'Module');return s

def street_item(name,mat):
    s=trimesh.Scene()
    if 'Street Lamp' in name:add(s,cyl(.06,2.7,(0,0,1.35),mat,16),'Pole');add(s,sph(.16,(0,0,2.62),'MAT_EMISSIVE_WARM'),'Lamp');return s
    if 'Bollard' in name:add(s,cyl(.10,.72,(0,0,.36),mat,18),'Bollard');return s
    add(s,box((1.1,.35,.75),(0,0,.375),mat),'StreetItem');return s

def hedge(name):
    s=trimesh.Scene();add(s,box((1.6,.42,.75),(0,0,.375),'MAT_VEGETATION'),'Hedge');return s

def tree(name):
    s=trimesh.Scene();scale=.75 if 'Small' in name else 1.15;add(s,cyl(.10*scale,1.55*scale,(0,0,.78*scale),'MAT_WOOD_DARK',14),'Trunk');add(s,sph(.55*scale,(0,0,1.6*scale),'MAT_VEGETATION'),'Canopy');return s

def fountain(name):
    s=trimesh.Scene();add(s,cyl(.95,.20,(0,0,.10),'MAT_STONE_LIGHT',32),'Basin');add(s,cyl(.26,.72,(0,0,.46),'MAT_STONE_LIGHT',24),'Pedestal');return s

def build_asset(name,mat):
    if any(k in name for k in ('Dumbbell Rack','Yoga Mat Rack','Exercise Ball Rack','Product Shelf')):return rack(name,mat)
    if 'Mirror Wall' in name:return mirror_wall()
    if 'Water Station' in name:return water_station()
    if 'Massage Table' in name:return massage_table()
    if any(k in name for k in ('Treatment Chair','Lounger','Pool Lounge')):return lounger(name,mat)
    if any(k in name for k in ('Sauna Bench','Steam Room Bench')):return bench(name,mat)
    if 'Towel Warmer' in name or 'Towel Station' in name:return shelf_or_cabinet(name,mat)
    if any(k in name for k in ('Conference Table','Banquet Table','Side Table')):return conference_table(name,mat)
    if 'Conference Chair' in name or 'Banquet Chair' in name:return chair(name,mat)
    if 'Podium' in name:return podium()
    if 'Presentation Screen' in name or 'Whiteboard' in name:return screen_or_board(name,mat)
    if 'Stage Module' in name or 'Dance Floor Module' in name:return stage_or_floor(name,mat)
    if 'Divider Panel' in name:return divider()
    if 'AV Equipment Rack' in name:return av_rack()
    if 'Game Table' in name:return game_table()
    if 'Framed Art' in name:return framed_art(name,mat)
    if 'Sculpture' in name:return sculpture(name,mat)
    if 'Vase' in name:return vase(name,mat)
    if any(k in name for k in ('Book Stack','Magazine Stack','Brochure Stack')):return books_or_magazines(name)
    if 'Candle' in name:return candles()
    if 'Clock' in name:return clock(name,mat)
    if 'Potted Plant' in name:return plant(name)
    if 'Flower Arrangement' in name:return flower_arrangement(name)
    if any(k in name for k in ('Sign','Plaque','Hanger','Door Tag','Menu Board')):return sign(name,mat)
    if 'Key Card' in name:
        s=trimesh.Scene();add(s,box((.086,.054,.003),(0,0,.0015),mat),'Card');return s
    if any(k in name for k in ('Coffee Cup','Water Glass','Wine Glass')):return cup_or_glass(name,mat)
    if 'Fruit Bowl' in name or 'Decorative Tray' in name:return tray_or_bowl(name,mat)
    if 'Folded Towel' in name:return towel_stack()
    if 'Toiletry Bottle' in name:return bottle_set()
    if 'Waste Basket' in name:
        s=trimesh.Scene();add(s,cyl(.16,.32,(0,0,.16),mat,18),'Basket');return s
    if name.startswith('Hotel Facade') or any(k in name for k in ('Entrance Steps','Accessible Ramp','Paver Module','Road Curb')):return exterior_module(name,mat)
    if any(k in name for k in ('Street Lamp','Bollard','Bike Rack')):return street_item(name,mat)
    if any(k in name for k in ('Outdoor Bench','Outdoor Lounge Chair','Outdoor Cafe')):return chair(name,mat) if 'Table' not in name else conference_table(name,mat)
    if 'Planter Exterior' in name:return plant('Potted Plant Large')
    if 'Hedge' in name:return hedge(name)
    if 'Ornamental Tree' in name:return tree(name)
    if 'Fountain Courtyard' in name:return fountain(name)
    if 'Water Feature Wall' in name:
        s=trimesh.Scene();add(s,box((1.6,.24,2.1),(0,0,1.05),'MAT_STONE_LIGHT'),'Wall');return s
    s=trimesh.Scene();add(s,box((.55,.40,.55),(0,0,.275),mat),'Body');return s
