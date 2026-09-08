from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import trimesh

import batch02_generate as b2
import batch03_generate as b3

PALETTE = dict(b2.PALETTE)
PALETTE.update(b3.PALETTE)
PALETTE.update({
    'MAT_CERAMIC_FIXTURE': ((0.90,0.89,0.85,1.0),0.0,0.24),
    'MAT_VEGETATION': ((0.25,0.38,0.20,1.0),0.0,0.74),
    'MAT_PAVING': ((0.42,0.41,0.39,1.0),0.0,0.76),
})


def material(name):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_WOOD_WARM'])
    return trimesh.visual.material.PBRMaterial(name=name,baseColorFactor=np.array(rgba)*255,metallicFactor=metallic,roughnessFactor=roughness)


def box(extents,center=(0,0,0),mat='MAT_WOOD_WARM'):
    m=trimesh.creation.box(extents=extents); m.apply_translation(center); m.visual=trimesh.visual.TextureVisuals(material=material(mat)); return m


def cyl(radius,height,center=(0,0,0),mat='MAT_STAINLESS',sections=20):
    m=trimesh.creation.cylinder(radius=radius,height=height,sections=sections); m.apply_translation(center); m.visual=trimesh.visual.TextureVisuals(material=material(mat)); return m


def sphere(radius,center=(0,0,0),mat='MAT_CERAMIC_FIXTURE'):
    m=trimesh.creation.icosphere(subdivisions=2,radius=radius); m.apply_translation(center); m.visual=trimesh.visual.TextureVisuals(material=material(mat)); return m


def add(s,m,n): s.add_geometry(m,node_name=n,geom_name=n)


def finish_asset(name,mat):
    if name.startswith('Marble Wall Cladding'): return b2.make_finish(name,mat)
    s=trimesh.Scene()
    if 'Trim Strip' in name:
        add(s,box((1.0,0.035,0.055),(0,0,0.0275),mat),'TrimStrip'); return s
    if 'Decorative Wall Molding' in name:
        add(s,box((1.0,0.08,2.8),(0,0,1.4),mat),'WallPanel')
        for x in (-0.38,0.38): add(s,box((0.035,0.025,2.3),(x,-0.052,1.35),mat),f'MoldingV_{x}')
        for z in (0.35,2.35): add(s,box((0.78,0.025,0.035),(0,-0.052,z),mat),f'MoldingH_{z}')
        return s
    if 'Acoustic Wall Panel' in name:
        add(s,box((1.0,0.10,2.8),(0,0,1.4),mat),'AcousticBacking')
        for x in np.linspace(-0.44,0.44,10): add(s,box((0.045,0.045,2.55),(x,-0.07,1.4),mat),f'AcousticRib_{x:.2f}')
        return s
    if 'Fabric Wall Panel' in name:
        add(s,box((1.0,0.08,2.8),(0,0,1.4),'MAT_PLASTER_WARM'),'WallBacking')
        for i,x in enumerate((-0.32,0,0.32)): add(s,box((0.28,0.08,2.35),(x,-0.07,1.4),'MAT_UPHOLSTERY'),f'FabricPanel_{i}')
        return s
    if 'Wallpaper' in name:
        add(s,box((1.0,0.06,2.8),(0,0,1.4),'MAT_PLASTER_WARM'),'WallpaperBase')
        if 'Botanical' in name:
            for i,(x,z) in enumerate(((-.3,.5),(.2,.8),(-.1,1.3),(.32,1.8),(-.28,2.2))):
                leaf=box((0.18,0.025,0.34),(x,-0.045,z),'MAT_VEGETATION'); leaf.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(25 if i%2 else -25),[0,1,0],[x,-0.045,z])); add(s,leaf,f'Leaf_{i}')
        else:
            for i,x in enumerate(np.linspace(-0.4,0.4,5)):
                strip=box((0.025,0.025,2.25),(x,-0.045,1.4),'MAT_BRASS_POLISHED'); strip.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(20 if i%2 else -20),[0,1,0],[x,-0.045,1.4])); add(s,strip,f'Geo_{i}')
        return s
    if 'Paver Pattern' in name:
        add(s,box((1.0,1.0,0.08),(0,0,0.04),'MAT_PAVING'),'PaverBase')
        for x in np.linspace(-0.4,0.4,5): add(s,box((0.015,0.94,0.01),(x,0,0.085),'MAT_PLASTER_COOL'),f'JointV_{x:.2f}')
        for y in np.linspace(-0.4,0.4,5): add(s,box((0.94,0.015,0.01),(0,y,0.085),'MAT_PLASTER_COOL'),f'JointH_{y:.2f}')
        return s
    return b2.make_finish(name,mat)


def soft_goods(name):
    s=trimesh.Scene()
    if name=='Area Rug Large': add(s,box((2.2,1.5,0.03),(0,0,0.015),'MAT_CARPET_LUXURY'),'RugLarge')
    elif name=='Bed Runner': add(s,box((1.65,0.48,0.035),(0,0,0.018),'MAT_LINEN'),'BedRunner')
    elif name=='Decorative Pillow Set':
        for i,x in enumerate((-0.35,0,0.35)): add(s,box((0.52,0.20,0.42),(x,0,0.22),'MAT_LINEN'),f'Pillow_{i}')
    elif name=='Guest Linen Folded Set':
        for i,z in enumerate((0.05,0.14,0.23)): add(s,box((0.62,0.42,0.08),(0,0,z),'MAT_LINEN'),f'FoldedLinen_{i}')
    else:
        add(s,box((0.58,0.10,0.95),(0,0,0.82),'MAT_LINEN'),'RobeBody'); add(s,box((0.32,0.10,0.72),(-0.35,0,0.82),'MAT_LINEN'),'SleeveL'); add(s,box((0.32,0.10,0.72),(0.35,0,0.82),'MAT_LINEN'),'SleeveR'); add(s,box((0.70,0.035,0.035),(0,0,1.42),'MAT_WOOD_WARM'),'Hanger')
    return s


def vanity(name):
    s=trimesh.Scene(); w=1.0 if 'Single' in name else 1.65; d,h=0.55,0.82
    add(s,box((w,d,h),(0,0,h/2),'MAT_WOOD_WARM'),'VanityCabinet'); add(s,box((w+0.06,d+0.03,0.06),(0,0,h+0.03),'MAT_MARBLE_LIGHT'),'VanityTop')
    for i,x in enumerate(([0] if 'Single' in name else [-0.42,0.42])):
        add(s,sphere(0.20,(x,0,h+0.13),'MAT_CERAMIC_FIXTURE'),f'Basin_{i}'); add(s,cyl(0.018,0.22,(x,0.15,h+0.20),'MAT_STAINLESS',12),f'Faucet_{i}')
    return s


def pedestal_sink():
    s=trimesh.Scene(); add(s,cyl(0.14,0.65,(0,0,0.325),'MAT_CERAMIC_FIXTURE',20),'Pedestal'); basin=sphere(0.30,(0,0,0.76),'MAT_CERAMIC_FIXTURE'); basin.apply_scale([1.25,0.75,0.45]); add(s,basin,'Basin'); add(s,cyl(0.018,0.22,(0,0.16,0.86),'MAT_STAINLESS',12),'Faucet'); return s


def toilet(accessible=False):
    s=trimesh.Scene(); bowl=sphere(0.32,(0,0,0.35),'MAT_CERAMIC_FIXTURE'); bowl.apply_scale([1.0,1.25,0.55]); add(s,bowl,'ToiletBowl'); add(s,box((0.48,0.18,0.52),(0,0.34,0.62),'MAT_CERAMIC_FIXTURE'),'Cistern'); add(s,box((0.48,0.60,0.055),(0,-0.05,0.53),'MAT_CERAMIC_FIXTURE'),'Seat')
    if accessible: add(s,cyl(0.025,0.85,(0.50,0,0.82),'MAT_STAINLESS',12),'GrabBar')
    return s


def shower_glass():
    s=trimesh.Scene(); add(s,box((1.0,0.035,2.05),(0,0,1.025),'MAT_GLASS_CLEAR'),'GlassPanel'); add(s,box((0.05,0.08,2.10),(-0.5,0,1.05),'MAT_STAINLESS'),'FrameL'); add(s,box((0.05,0.08,2.10),(0.5,0,1.05),'MAT_STAINLESS'),'FrameR'); add(s,cyl(0.025,0.36,(0.34,-0.05,1.05),'MAT_STAINLESS',12),'Handle'); return s


def shower_curtain():
    s=trimesh.Scene(); add(s,box((1.55,0.055,1.90),(0,0,0.95),'MAT_LINEN'),'Curtain'); add(s,box((1.72,0.045,0.045),(0,0,2.02),'MAT_STAINLESS'),'Rod'); return s


def bathtub():
    s=trimesh.Scene(); add(s,box((1.70,0.78,0.55),(0,0,0.275),'MAT_CERAMIC_FIXTURE'),'TubOuter'); add(s,box((1.40,0.52,0.38),(0,0.0,0.46),'MAT_GLASS_CLEAR'),'TubBasin'); add(s,cyl(0.025,0.26,(0.62,0.24,0.68),'MAT_STAINLESS',12),'Faucet'); return s


def bathroom_detail(name):
    s=trimesh.Scene()
    if 'Towel Rack' in name:
        add(s,box((0.74,0.05,0.05),(0,0,0.75),'MAT_STAINLESS'),'Rack'); add(s,box((0.62,0.08,0.62),(0,0,0.43),'MAT_LINEN'),'Towel')
    elif 'Towel Shelf' in name:
        add(s,box((0.78,0.30,0.05),(0,0,0.78),'MAT_STAINLESS'),'Shelf')
        for i,z in enumerate((0.84,0.95)): add(s,box((0.50,0.24,0.09),(0,0,z),'MAT_LINEN'),f'TowelFold_{i}')
    elif 'Mirror Lit' in name:
        add(s,box((0.82,0.05,0.94),(0,0,0.47),'MAT_EMISSIVE_WARM'),'LightFrame'); add(s,box((0.72,0.02,0.84),(0,-0.035,0.47),'MAT_GLASS_CLEAR'),'Mirror')
    elif 'Wall Sconce' in name:
        add(s,box((0.18,0.12,0.30),(0,0,0.55),'MAT_EMISSIVE_WARM'),'Sconce'); add(s,box((0.12,0.08,0.18),(0,0.08,0.34),'MAT_BRASS_POLISHED'),'Bracket')
    elif 'Scale' in name:
        add(s,box((0.36,0.36,0.045),(0,0,0.0225),'MAT_PLASTIC_RUBBER'),'ScaleBody'); add(s,box((0.11,0.06,0.01),(0,-0.12,0.05),'MAT_ELECTRONICS'),'Display')
    elif 'Hair Dryer' in name:
        add(s,box((0.30,0.11,0.18),(0,0,0.55),'MAT_ELECTRONICS'),'DryerBody'); add(s,box((0.11,0.08,0.30),(-0.08,0,0.35),'MAT_ELECTRONICS'),'Handle'); add(s,box((0.38,0.08,0.38),(0,0.08,0.48),'MAT_PLASTIC_RUBBER'),'WallMount')
    elif 'Toiletry Tray' in name:
        add(s,box((0.55,0.32,0.05),(0,0,0.025),'MAT_PLASTIC_RUBBER'),'Tray')
        for i,x in enumerate((-0.16,0,0.16)): add(s,cyl(0.035,0.16,(x,0,0.12),'MAT_PLASTIC_RUBBER',12),f'Bottle_{i}')
    elif 'Room Service Tray' in name:
        add(s,box((0.62,0.42,0.05),(0,0,0.025),'MAT_WOOD_WARM'),'Tray'); add(s,cyl(0.11,0.03,(-0.16,0,0.065),'MAT_CERAMIC_FIXTURE',20),'Plate'); add(s,cyl(0.04,0.12,(0.18,0,0.10),'MAT_GLASS_CLEAR',16),'Glass')
    elif 'Telephone' in name:
        add(s,box((0.34,0.24,0.12),(0,0,0.06),'MAT_ELECTRONICS'),'PhoneBase'); add(s,box((0.30,0.06,0.07),(0,0,0.16),'MAT_ELECTRONICS'),'Handset')
    elif 'Alarm Clock' in name:
        add(s,box((0.25,0.10,0.14),(0,0,0.07),'MAT_ELECTRONICS'),'ClockBody'); add(s,box((0.16,0.015,0.07),(0,-0.058,0.08),'MAT_GLASS_CLEAR'),'Display')
    elif 'Ironing Board' in name:
        board=box((1.10,0.32,0.06),(0,0,0.75),'MAT_WOOD_WARM'); board.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(80),[0,1,0],[0,0,0.75])); add(s,board,'FoldedBoard'); add(s,box((0.05,0.05,1.05),(0,0,0.52),'MAT_STAINLESS'),'Frame')
    elif 'Suitcase Stand' in name:
        for x in (-0.32,0.32): add(s,box((0.06,0.45,0.55),(x,0,0.275),'MAT_WOOD_WARM'),f'Leg_{x}')
        for y in (-0.15,0,0.15): add(s,box((0.72,0.05,0.04),(0,y,0.56),'MAT_LINEN'),f'Strap_{y}')
    return s


def reception(name):
    if 'Sofa' in name:
        if 'Curved' not in name: return b3.sofa(name)
        s=trimesh.Scene()
        for i,(x,ang) in enumerate(((-0.75,-15),(0,0),(0.75,15))):
            seat=box((0.82,0.72,0.18),(x,0,0.47),'MAT_UPHOLSTERY'); seat.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(ang),[0,0,1],[x,0,0.47])); add(s,seat,f'Seat_{i}')
        return s
    if 'Armchair' in name or 'Lounge Chair' in name: return b3.chair(name,'MAT_UPHOLSTERY')
    if 'Coffee Table' in name: return b3.table_or_bench('Guest Coffee Table Rectangular','MAT_WOOD_WARM')
    s=trimesh.Scene(); w,d,h=1.20,0.62,1.05
    if 'Double' in name: w=2.10
    if 'Grand' in name: w,d,h=2.70,0.78,1.15
    if 'Back Counter' in name: w,d,h=1.90,0.55,0.86
    if 'Key Cabinet' in name: w,d,h=1.20,0.28,1.65
    if 'Concierge' in name or 'Bell Desk' in name or 'Host Podium' in name: w,d,h=0.85,0.55,1.05
    add(s,box((w,d,h),(0,0,h/2),'MAT_WOOD_WARM'),'DeskFront')
    if 'Reception Desk' in name:
        add(s,box((w+0.08,d+0.04,0.07),(0,0,h+0.035),'MAT_MARBLE_LIGHT'),'Countertop'); add(s,box((w*0.78,0.025,h*0.55),(0,-d/2-0.015,h*0.48),'MAT_WOOD_DARK'),'FrontInset')
    if 'Key Cabinet' in name:
        for r in range(5):
            for c in range(4): add(s,box((0.18,0.025,0.20),(-0.42+c*0.28,-d/2-0.015,0.25+r*0.27),'MAT_WOOD_DARK'),f'KeySlot_{r}_{c}')
    return s


def build(name,mat):
    if name.startswith(('Marble Wall','Brass Trim','Blackened Steel','Decorative Wall','Acoustic Wall','Fabric Wall','Wallpaper','Exterior Paver')): return finish_asset(name,mat)
    if name in ('Area Rug Large','Bed Runner','Decorative Pillow Set','Guest Linen Folded Set','Guest Bathrobe Hanging'): return soft_goods(name)
    if name.startswith('Bathroom Vanity'): return vanity(name)
    if name=='Bathroom Sink Pedestal': return pedestal_sink()
    if name=='Bathroom Toilet Standard': return toilet(False)
    if name=='Bathroom Toilet Accessible': return toilet(True)
    if name=='Bathroom Shower Glass': return shower_glass()
    if name=='Bathroom Shower Curtain': return shower_curtain()
    if name=='Bathroom Bathtub': return bathtub()
    if name.startswith('Bathroom ') or name in ('Hair Dryer Wall Mount','Toiletry Tray','Room Service Tray','Guest Telephone','Alarm Clock','Ironing Board Folded','Suitcase Stand'): return bathroom_detail(name)
    return reception(name)


def sidecar(asset_id,subcategory,mat,profile):
    lod='lod_architecture' if profile=='P_ARCH_STATIC' else 'lod_furniture'
    return {'schema':1,'asset_id':asset_id,'asset_type':'StaticMeshAsset','source':'Tools/ArtGeneration/batch04_generate.py','units':'meters','lod_policy':lod,'collision_policy':'simple_proxy','material_slots':[mat],'tags':['hotel-haven','batch_04',subcategory],'dependencies':[],'cutaway_policy':'normal','source_revision':1,'metadata_revision':1,'cooker_schema':1,'lifecycle_state':'PRODUCTION'}


def generate(manifest:Path,output:Path,package:bool):
    rows=[a for g in json.loads(manifest.read_text())['groups'] for a in g['assets']]
    if len(rows)!=50: raise ValueError(f'Batch 04 must contain 50 assets; got {len(rows)}')
    output.mkdir(parents=True,exist_ok=True); out=[]
    for aid,name,subcategory,mat,profile,animset,anchors in rows:
        scene=build(name,mat); p=output/f'{aid}.glb'; p.write_bytes(scene.export(file_type='glb')); out.append(p)
        if package: (output/f'{aid}.asset.json').write_text(json.dumps(sidecar(aid,subcategory,mat,profile),indent=2)+'\n')
        print(p)
    return out


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('manifest',type=Path); ap.add_argument('output',type=Path); ap.add_argument('--package',action='store_true'); a=ap.parse_args(); generate(a.manifest,a.output,a.package)

if __name__=='__main__': main()
