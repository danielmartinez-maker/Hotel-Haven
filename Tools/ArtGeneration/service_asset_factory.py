from __future__ import annotations
import numpy as np
import trimesh

PALETTE = {
    'MAT_WOOD_WARM': ((0.42,0.22,0.10,1),0.0,0.52),
    'MAT_WOOD_DARK': ((0.22,0.12,0.08,1),0.0,0.48),
    'MAT_UPHOLSTERY': ((0.42,0.31,0.25,1),0.0,0.84),
    'MAT_STAINLESS': ((0.58,0.61,0.62,1),0.85,0.42),
    'MAT_SERVICE_PAINT': ((0.24,0.30,0.34,1),0.0,0.68),
    'MAT_PLASTIC_RUBBER': ((0.18,0.18,0.18,1),0.0,0.64),
    'MAT_GLASS_CLEAR': ((0.34,0.55,0.64,0.42),0.0,0.10),
    'MAT_BLACKENED_STEEL': ((0.10,0.12,0.13,1),0.78,0.50),
    'MAT_SIGNAGE': ((0.16,0.17,0.17,1),0.25,0.44),
    'MAT_PAVING': ((0.42,0.42,0.39,1),0.0,0.78),
    'MAT_ELECTRONICS': ((0.08,0.09,0.10,1),0.12,0.36),
    'MAT_LINEN': ((0.88,0.86,0.80,1),0.0,0.88),
    'MAT_BRASS_POLISHED': ((0.62,0.38,0.10,1),0.88,0.30),
}

def material(name: str):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_SERVICE_PAINT'])
    return trimesh.visual.material.PBRMaterial(name=name, baseColorFactor=np.array(rgba)*255, metallicFactor=metallic, roughnessFactor=roughness)

def box(extents, center=(0,0,0), mat='MAT_SERVICE_PAINT'):
    m=trimesh.creation.box(extents=extents); m.apply_translation(center); m.visual=trimesh.visual.TextureVisuals(material=material(mat)); return m

def cyl(radius, height, center=(0,0,0), mat='MAT_STAINLESS', sections=18):
    m=trimesh.creation.cylinder(radius=radius,height=height,sections=sections); m.apply_translation(center); m.visual=trimesh.visual.TextureVisuals(material=material(mat)); return m

def add(scene, mesh, name): scene.add_geometry(mesh,node_name=name,geom_name=name)

def chair(name, mat):
    s=trimesh.Scene(); add(s,box((.48,.50,.10),(0,0,.48),mat),'Seat'); add(s,box((.48,.10,.58),(0,.20,.76),mat),'Back')
    for i,(x,y) in enumerate(((-.19,-.19),(.19,-.19),(-.19,.19),(.19,.19))): add(s,box((.045,.045,.45),(x,y,.225),'MAT_WOOD_DARK'),f'Leg_{i}')
    if 'Upholstered' in name:
        add(s,box((.43,.42,.055),(0,-.01,.555),'MAT_UPHOLSTERY'),'SeatCushion'); add(s,box((.42,.055,.48),(0,.135,.80),'MAT_UPHOLSTERY'),'BackCushion')
    return s

def table_like(name, mat):
    s=trimesh.Scene(); w,d,h=1.20,.65,.90
    if 'Cocktail Table' in name: w,d,h=.72,.72,1.08
    if 'Folding Table' in name: w,d,h=1.50,.72,.86
    if 'Workbench' in name: w,d,h=1.55,.72,.92
    add(s,box((w,d,.075),(0,0,h),mat),'Top')
    for i,(x,y) in enumerate(((-w*.42,-d*.36),(w*.42,-d*.36),(-w*.42,d*.36),(w*.42,d*.36))): add(s,box((.06,.06,h),(x,y,h/2),'MAT_BLACKENED_STEEL'),f'Leg_{i}')
    if 'Workbench' in name: add(s,box((w,.06,.78),(0,d/2-.03,1.28),'MAT_SERVICE_PAINT'),'Backboard')
    return s

def counter(name, mat):
    s=trimesh.Scene(); w,d,h=1.45,.68,.92
    if 'Corner' in name: w=1.20
    add(s,box((w,d,h),(0,0,h/2),mat),'CounterBody'); add(s,box((w+.08,d+.08,.07),(0,0,h+.035),'MAT_STAINLESS' if any(k in name for k in ('Kitchen','Service','Prep')) else mat),'CounterTop')
    if 'Sneeze Guard' in name:
        s=trimesh.Scene(); add(s,box((1.40,.025,.50),(0,0,.90),'MAT_GLASS_CLEAR'),'Glass')
        for x in (-.62,.62): add(s,cyl(.018,.90,(x,0,.45),'MAT_STAINLESS',10),f'Post_{x}')
    if 'Hot Well' in name or 'Cold Well' in name or 'Ice Bin' in name:
        for i,x in enumerate((-.42,0,.42)): add(s,box((.34,.42,.08),(x,-.05,h+.075),'MAT_STAINLESS'),f'Well_{i}')
    return s

def appliance(name, mat):
    s=trimesh.Scene(); w,d,h=.72,.70,1.45
    if any(k in name for k in ('Refrigerator','Freezer','Warmer Cabinet')): w,d,h=.78,.72,1.90
    if any(k in name for k in ('Oven','Dishwasher','Washer','Dryer')): w,d,h=.82,.75,1.05
    if any(k in name for k in ('Range','Griddle','Fryer')): w,d,h=.84,.72,.90
    if 'Salamander' in name: w,d,h=.86,.42,.46
    add(s,box((w,d,h),(0,0,h/2),mat),'Body'); add(s,box((w*.72,.025,h*.36),(0,-d/2-.014,h*.55),'MAT_ELECTRONICS'),'ControlOrDoor')
    if any(k in name for k in ('Washer','Dryer')): add(s,cyl(.22,.025,(0,-d/2-.032,.54),'MAT_GLASS_CLEAR',24),'DrumWindow')
    if 'Range' in name:
        for i,(x,y) in enumerate(((-.22,-.16),(.22,-.16),(-.22,.16),(.22,.16))): add(s,cyl(.11,.025,(x,y,h+.02),'MAT_BLACKENED_STEEL',20),f'Burner_{i}')
    if 'Sink' in name:
        s=counter(name,'MAT_STAINLESS')
        for i,x in enumerate((-.42,0,.42)): add(s,box((.34,.42,.12),(x,-.04,1.00),'MAT_STAINLESS'),f'Basin_{i}')
    return s

def display_or_dispenser(name, mat):
    s=trimesh.Scene()
    if 'Display Case' in name:
        add(s,box((1.05,.55,.78),(0,0,.39),'MAT_WOOD_WARM'),'Base'); add(s,box((1.00,.48,.70),(0,0,1.10),'MAT_GLASS_CLEAR'),'GlassCase'); return s
    if any(k in name for k in ('Cereal','Juice','Detergent')):
        add(s,box((.58,.34,.88),(0,0,.44),mat),'DispenserBody')
        for i,x in enumerate((-.16,.16)): add(s,cyl(.09,.34,(x,-.06,1.05),'MAT_GLASS_CLEAR',16),f'Canister_{i}')
        return s
    if 'Coffee Urn' in name:
        add(s,cyl(.22,.55,(0,0,.28),'MAT_STAINLESS',24),'Urn'); add(s,cyl(.12,.12,(0,0,.61),'MAT_BLACKENED_STEEL',16),'Lid'); return s
    if 'Beer Tap' in name:
        add(s,cyl(.09,.55,(0,0,.28),'MAT_STAINLESS',18),'Tower')
        for i,x in enumerate((-.13,.13)): add(s,box((.04,.16,.04),(x,-.10,.55),'MAT_BLACKENED_STEEL'),f'Tap_{i}')
        return s
    if 'Wine Rack' in name or 'Bottle Display' in name:
        add(s,box((1.05,.30,1.45),(0,0,.725),mat),'RackFrame')
        for z in (.35,.70,1.05): add(s,box((.90,.25,.035),(0,0,z),mat),f'Shelf_{z}')
        for j,z in enumerate((.44,.79,1.14)):
            for i,x in enumerate(np.linspace(-.34,.34,4)): add(s,cyl(.035,.24,(x,0,z),'MAT_GLASS_CLEAR',12),f'Bottle_{j}_{i}')
        return s
    return counter(name,mat)

def shelf_or_cabinet(name, mat):
    s=trimesh.Scene(); w,d,h=1.10,.48,1.80
    if 'Single' in name: w=.72
    if 'Double' in name: w=1.45
    if 'Pegboard' in name: d=.08
    add(s,box((w,d,.07),(0,0,.035),mat),'Base')
    for x in (-w/2+.035,w/2-.035): add(s,box((.07,.07,h),(x,0,h/2),mat),f'Upright_{x}')
    for i,z in enumerate((.38,.76,1.14,1.52)): add(s,box((w*.94,d*.90,.045),(0,0,z),mat),f'Shelf_{i}')
    if 'Cabinet' in name or 'Locker' in name:
        add(s,box((w,d,h),(0,0,h/2),mat),'CabinetBody')
        for i,x in enumerate(np.linspace(-w*.35,w*.35,3)): add(s,box((.025,.03,.18),(x,-d/2-.02,1.0),'MAT_BLACKENED_STEEL'),f'Handle_{i}')
    return s

def cart(name, mat):
    s=trimesh.Scene(); w,l,h=1.00,1.30,.82
    if 'Compact' in name: w,l=.78,1.05
    if 'Flatbed' in name or 'Platform Dolly' in name: h=.22
    if 'Hand Truck' in name:
        add(s,box((.46,.12,.06),(0,-.12,.06),mat),'ToePlate')
        for x in (-.18,.18): add(s,box((.05,.05,1.25),(x,0,.65),mat),f'Rail_{x}')
        wheel_pos=((-0.22,.06,.18),(0.22,.06,.18))
    else:
        add(s,box((w,l,.09),(0,0,.25),mat),'Base')
        if h>.3: add(s,box((w*.92,l*.82,h),(0,0,.25+h/2),mat),'Body')
        wheel_pos=((-w*.42,-l*.40,.10),(w*.42,-l*.40,.10),(-w*.42,l*.40,.10),(w*.42,l*.40,.10))
    for i,(x,y,z) in enumerate(wheel_pos):
        wh=cyl(.09,.05,(x,y,z),'MAT_BLACKENED_STEEL',16); wh.apply_transform(trimesh.transformations.rotation_matrix(np.pi/2,[1,0,0],[x,y,z])); add(s,wh,f'MOV_Wheel_{i}')
    if 'Housekeeping' in name: add(s,box((w*.82,.18,.40),(0,-l*.28,.85),'MAT_LINEN'),'LinenStack')
    return s

def small_cleaning(name, mat):
    s=trimesh.Scene()
    if 'Vacuum' in name or 'Floor Buffer' in name:
        add(s,cyl(.20,.18,(0,0,.09),mat,20),'Base'); add(s,box((.08,.08,.95),(0,.04,.62),'MAT_BLACKENED_STEEL'),'Handle'); return s
    if 'Mop Bucket' in name:
        add(s,box((.48,.34,.38),(0,0,.19),mat),'Bucket'); add(s,box((.20,.30,.24),(.18,0,.48),'MAT_BLACKENED_STEEL'),'Wringer'); return s
    if 'Caddy' in name or 'Toolbox' in name:
        add(s,box((.48,.26,.26),(0,0,.13),mat),'Box'); add(s,box((.25,.04,.20),(0,0,.36),'MAT_BLACKENED_STEEL'),'Handle'); return s
    if any(k in name for k in ('Hamper','Sorting Bin','Trash Bin','Recycling Bin','Compactor Bin')):
        h=.68 if 'Compactor' not in name else 1.10; w=.52 if 'Compactor' not in name else 1.10
        add(s,box((w,.48,h),(0,0,h/2),mat),'Bin'); add(s,box((w*.95,.44,.06),(0,0,h+.03),'MAT_BLACKENED_STEEL'),'Lid'); return s
    if 'Wet Floor Sign' in name:
        add(s,box((.36,.06,.70),(-.12,0,.35),'MAT_SIGNAGE'),'PanelA'); add(s,box((.36,.06,.70),(.12,0,.35),'MAT_SIGNAGE'),'PanelB'); return s
    return shelf_or_cabinet(name,mat)

def ladder(name, mat):
    s=trimesh.Scene(); h=1.65 if 'Portable' in name else 1.20; w=.48
    for x in (-w/2,w/2): add(s,box((.045,.05,h),(x,0,h/2),mat),f'Rail_{x}')
    for i,z in enumerate(np.linspace(.20,h-.18,6)): add(s,box((w,.12,.045),(0,0,z),mat),f'Step_{i}')
    return s

def dock_or_pallet(name, mat):
    s=trimesh.Scene()
    if 'Pallet' in name:
        for y in (-.38,0,.38): add(s,box((1.10,.16,.08),(0,y,.18),'MAT_WOOD_WARM'),f'Slat_{y}')
        for x in (-.43,0,.43): add(s,box((.14,.92,.14),(x,0,.07),'MAT_WOOD_DARK'),f'Runner_{x}')
    elif 'Ramp' in name:
        ramp=box((1.40,1.80,.12),(0,0,.30),mat); ramp.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-12),[1,0,0],[0,0,.30])); add(s,ramp,'Ramp')
    elif 'Bumper' in name: add(s,box((.50,.22,.55),(0,0,.275),'MAT_PLASTIC_RUBBER'),'Bumper')
    return s

def security(name, mat):
    s=trimesh.Scene()
    if 'Camera Dome' in name:
        add(s,cyl(.16,.08,(0,0,.04),'MAT_STAINLESS',24),'Mount'); add(s,cyl(.12,.12,(0,0,-.02),'MAT_GLASS_CLEAR',24),'Dome'); return s
    if 'Camera Bullet' in name:
        add(s,cyl(.10,.42,(0,0,.22),'MAT_STAINLESS',18),'CameraBody'); add(s,box((.04,.04,.35),(0,.12,.40),'MAT_BLACKENED_STEEL'),'Bracket'); return s
    if 'Monitor Desk' in name:
        add(s,box((1.30,.65,.72),(0,0,.36),'MAT_WOOD_WARM'),'Desk')
        for i,x in enumerate((-.38,0,.38)): add(s,box((.32,.04,.26),(x,-.24,.92),'MAT_ELECTRONICS'),f'Monitor_{i}')
        return s
    return appliance(name,mat)

def gym(name, mat):
    s=trimesh.Scene()
    if 'Weight Bench' in name:
        add(s,box((1.25,.38,.14),(0,0,.48),'MAT_UPHOLSTERY'),'Bench')
        for x in (-.48,.48): add(s,box((.08,.32,.46),(x,0,.23),'MAT_BLACKENED_STEEL'),f'Leg_{x}')
        return s
    if 'Treadmill' in name:
        add(s,box((1.35,.55,.12),(0,0,.12),'MAT_BLACKENED_STEEL'),'Deck'); add(s,box((.08,.08,1.05),(0,.22,.68),'MAT_BLACKENED_STEEL'),'Upright'); add(s,box((.45,.12,.28),(0,.22,1.12),'MAT_ELECTRONICS'),'Console'); return s
    if 'Stationary Bike' in name:
        add(s,cyl(.34,.06,(0,0,.38),'MAT_BLACKENED_STEEL',24),'MOV_Flywheel'); add(s,box((.06,.06,.80),(0,0,.75),'MAT_BLACKENED_STEEL'),'Frame'); add(s,box((.24,.20,.08),(0,0,1.10),'MAT_UPHOLSTERY'),'Seat'); return s
    if 'Elliptical' in name:
        add(s,cyl(.26,.06,(0,0,.28),'MAT_BLACKENED_STEEL',24),'MOV_Flywheel')
        for x in (-.20,.20): add(s,box((.05,.05,1.30),(x,0,.80),'MAT_BLACKENED_STEEL'),f'Handle_{x}')
        return s
    if 'Rowing' in name:
        add(s,box((1.75,.10,.10),(0,0,.20),'MAT_BLACKENED_STEEL'),'Rail'); add(s,box((.36,.26,.10),(-.40,0,.36),'MAT_UPHOLSTERY'),'Seat'); add(s,cyl(.28,.10,(.68,0,.38),'MAT_BLACKENED_STEEL',24),'MOV_Flywheel'); return s
    return s

def build_asset(name: str, mat: str):
    if 'Chair' in name or 'Stool' in name or name.endswith('Bench'): return chair(name,mat)
    if any(k in name for k in ('Table','Workbench')): return table_like(name,mat)
    if any(k in name for k in ('Counter','Prep Station','Service Station','Buffet')): return counter(name,mat)
    if any(k in name for k in ('Refrigerator','Freezer','Range','Oven','Griddle','Fryer','Salamander','Dishwasher','Sink','Warmer Cabinet','Hot Box','Washer','Dryer','Ironing Press')): return appliance(name,mat)
    if any(k in name for k in ('Dispenser','Display Case','Wine Rack','Bottle Display','Beer Tap','Coffee Urn')): return display_or_dispenser(name,mat)
    if any(k in name for k in ('Shelf','Cabinet','Rack','Locker')): return shelf_or_cabinet(name,mat)
    if any(k in name for k in ('Cart','Trolley','Dolly','Hand Truck')): return cart(name,mat)
    if any(k in name for k in ('Vacuum','Mop Bucket','Caddy','Hamper','Bin','Floor Buffer','Wet Floor Sign','Toolbox')): return small_cleaning(name,mat)
    if 'Ladder' in name: return ladder(name,mat)
    if any(k in name for k in ('Loading Dock','Pallet')): return dock_or_pallet(name,mat)
    if any(k in name for k in ('Security','Time Clock','First Aid','Fire Extinguisher')): return security(name,mat)
    if any(k in name for k in ('Treadmill','Elliptical','Stationary Bike','Rowing Machine','Weight Bench')): return gym(name,mat)
    s=trimesh.Scene(); add(s,box((.65,.45,.72),(0,0,.36),mat),'Body'); return s
