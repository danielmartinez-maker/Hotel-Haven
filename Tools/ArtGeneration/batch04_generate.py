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
    base_color = np.clip(np.rint(np.asarray(rgba, dtype=float) * 255.0), 0, 255).astype(np.uint8)
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=base_color,
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def box(extents, center=(0,0,0), mat='MAT_WOOD_WARM'):
    m = trimesh.creation.box(extents=extents)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return m


def cyl(radius, height, center=(0,0,0), mat='MAT_STAINLESS', sections=20):
    m = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return m


def sphere(radius, center=(0,0,0), mat='MAT_CERAMIC_FIXTURE'):
    m = trimesh.creation.icosphere(subdivisions=2, radius=radius)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return m


def add(scene, mesh, name):
    scene.add_geometry(mesh, node_name=name, geom_name=name)


def rotated_box(extents, center, angle_degrees, mat):
    m = box(extents, center, mat)
    m.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(angle_degrees), [0,0,1], center))
    return m


def finish_asset(name, mat):
    if name.startswith('Marble Wall Cladding'):
        return b2.make_finish(name, mat)
    s = trimesh.Scene()
    if 'Trim Strip' in name:
        add(s, box((1.0,0.035,0.055),(0,0,0.0275),mat), 'TrimStrip')
        return s
    if 'Decorative Wall Molding' in name:
        add(s, box((1.0,0.08,2.8),(0,0,1.4),mat), 'WallPanel')
        for x in (-0.38,0.38):
            add(s, box((0.035,0.025,2.3),(x,-0.052,1.35),mat), f'MoldingV_{x}')
        for z in (0.35,2.35):
            add(s, box((0.78,0.025,0.035),(0,-0.052,z),mat), f'MoldingH_{z}')
        add(s, box((0.90,0.028,0.08),(0,-0.055,1.38),'MAT_BRASS_POLISHED'), 'MoldingAccent')
        return s
    if 'Acoustic Wall Panel' in name:
        add(s, box((1.0,0.10,2.8),(0,0,1.4),mat), 'AcousticBacking')
        for x in np.linspace(-0.44,0.44,10):
            add(s, box((0.045,0.045,2.55),(x,-0.07,1.4),mat), f'AcousticRib_{x:.2f}')
        return s
    if 'Fabric Wall Panel' in name:
        add(s, box((1.0,0.08,2.8),(0,0,1.4),'MAT_PLASTER_WARM'), 'WallBacking')
        for i,x in enumerate((-0.32,0,0.32)):
            add(s, box((0.28,0.08,2.35),(x,-0.07,1.4),'MAT_UPHOLSTERY'), f'FabricPanel_{i}')
            add(s, box((0.025,0.025,2.40),(x+0.16,-0.115,1.4),'MAT_BRASS_POLISHED'), f'FabricTrim_{i}')
        return s
    if 'Wallpaper' in name:
        add(s, box((1.0,0.06,2.8),(0,0,1.4),'MAT_PLASTER_WARM'), 'WallpaperBase')
        if 'Botanical' in name:
            for i,(x,z) in enumerate(((-.3,.5),(.2,.8),(-.1,1.3),(.32,1.8),(-.28,2.2))):
                leaf = box((0.18,0.025,0.34),(x,-0.045,z),'MAT_VEGETATION')
                leaf.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(25 if i%2 else -25),[0,1,0],[x,-0.045,z]))
                add(s, leaf, f'Leaf_{i}')
        else:
            for i,x in enumerate(np.linspace(-0.4,0.4,5)):
                strip = box((0.025,0.025,2.25),(x,-0.045,1.4),'MAT_BRASS_POLISHED')
                strip.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(20 if i%2 else -20),[0,1,0],[x,-0.045,1.4]))
                add(s, strip, f'Geo_{i}')
        return s
    if 'Paver Pattern' in name:
        add(s, box((1.0,1.0,0.08),(0,0,0.04),'MAT_PAVING'), 'PaverBase')
        for x in np.linspace(-0.4,0.4,5):
            add(s, box((0.015,0.94,0.01),(x,0,0.085),'MAT_PLASTER_COOL'), f'JointV_{x:.2f}')
        for y in np.linspace(-0.4,0.4,5):
            add(s, box((0.94,0.015,0.01),(0,y,0.085),'MAT_PLASTER_COOL'), f'JointH_{y:.2f}')
        return s
    return b2.make_finish(name, mat)


def soft_goods(name):
    s = trimesh.Scene()
    if name == 'Area Rug Large':
        add(s, box((2.2,1.5,0.03),(0,0,0.015),'MAT_CARPET_LUXURY'), 'RugLarge')
        add(s, box((1.86,1.16,0.008),(0,0,0.034),'MAT_CARPET_LUXURY'), 'RugInset')
    elif name == 'Bed Runner':
        add(s, box((1.65,0.48,0.035),(0,0,0.018),'MAT_LINEN'), 'BedRunner')
        for x in np.linspace(-0.72,0.72,7):
            add(s, box((0.025,0.44,0.010),(x,0,0.040),'MAT_BRASS_POLISHED'), f'RunnerStripe_{x:.2f}')
    elif name == 'Decorative Pillow Set':
        for i,x in enumerate((-0.35,0,0.35)):
            pillow = box((0.52,0.20,0.42),(x,0,0.22),'MAT_LINEN')
            pillow.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad((-8,0,8)[i]),[0,1,0],[x,0,0.22]))
            add(s,pillow,f'Pillow_{i}')
    elif name == 'Guest Linen Folded Set':
        for i,z in enumerate((0.05,0.14,0.23)):
            add(s, box((0.62-0.04*i,0.42-0.02*i,0.08),(0,0,z),'MAT_LINEN'), f'FoldedLinen_{i}')
    else:
        add(s, box((0.58,0.10,1.58),(0,0,0.79),'MAT_LINEN'), 'RobeBody')
        add(s, box((0.32,0.10,0.72),(-0.35,0,0.82),'MAT_LINEN'), 'SleeveL')
        add(s, box((0.32,0.10,0.72),(0.35,0,0.82),'MAT_LINEN'), 'SleeveR')
        add(s, box((0.70,0.035,0.035),(0,0,1.42),'MAT_WOOD_WARM'), 'Hanger')
        add(s, box((0.68,0.025,0.09),(0,-0.065,0.75),'MAT_LINEN_BLACKOUT'), 'RobeBelt')
    return s


def vanity(name):
    s = trimesh.Scene(); w = 1.0 if 'Single' in name else 1.65; d,h = 0.55,0.82
    add(s, box((w,d,h),(0,0,h/2),'MAT_WOOD_WARM'), 'VanityCabinet')
    add(s, box((w+0.06,d+0.03,0.06),(0,0,h+0.03),'MAT_MARBLE_LIGHT'), 'VanityTop')
    positions = [0] if 'Single' in name else [-0.42,0.42]
    for i,x in enumerate(positions):
        basin = sphere(0.20,(x,0,h+0.13),'MAT_CERAMIC_FIXTURE')
        basin.apply_scale([1.20,0.82,0.42])
        add(s, basin, f'Basin_{i}')
        add(s, cyl(0.018,0.22,(x,0.15,h+0.20),'MAT_STAINLESS',12), f'Faucet_{i}')
    for i,x in enumerate(np.linspace(-w*0.30,w*0.30,2 if w < 1.2 else 4)):
        add(s, box((w*0.18,0.025,h*0.42),(x,-d/2-0.016,h*0.30),'MAT_WOOD_DARK'), f'VanityDoor_{i}')
    return s


def pedestal_sink():
    s = trimesh.Scene()
    add(s,cyl(0.14,0.65,(0,0,0.325),'MAT_CERAMIC_FIXTURE',20),'Pedestal')
    basin = sphere(0.30,(0,0,0.76),'MAT_CERAMIC_FIXTURE')
    basin.apply_scale([1.25,0.75,0.45])
    add(s,basin,'Basin')
    add(s,cyl(0.018,0.22,(0,0.16,0.86),'MAT_STAINLESS',12),'Faucet')
    return s


def toilet(accessible=False):
    s = trimesh.Scene()
    bowl = sphere(0.32,(0,0,0.35),'MAT_CERAMIC_FIXTURE')
    bowl.apply_scale([1.0,1.25,0.55])
    add(s,bowl,'ToiletBowl')
    add(s,box((0.48,0.18,0.52),(0,0.34,0.62),'MAT_CERAMIC_FIXTURE'),'Cistern')
    add(s,box((0.48,0.60,0.055),(0,-0.05,0.53),'MAT_CERAMIC_FIXTURE'),'Seat')
    add(s,box((0.08,0.025,0.035),(0.15,0.24,0.90),'MAT_BRASS_POLISHED'),'FlushControl')
    if accessible:
        add(s,cyl(0.025,0.85,(0.50,0,0.82),'MAT_STAINLESS',12),'GrabBar')
        add(s,box((0.58,0.05,0.05),(0.50,0,1.20),'MAT_STAINLESS'),'GrabBarTop')
    return s


def shower_glass():
    s = trimesh.Scene()
    add(s,box((1.0,0.035,2.05),(0,0,1.025),'MAT_GLASS_CLEAR'),'GlassPanel')
    add(s,box((0.05,0.08,2.10),(-0.5,0,1.05),'MAT_STAINLESS'),'FrameL')
    add(s,box((0.05,0.08,2.10),(0.5,0,1.05),'MAT_STAINLESS'),'FrameR')
    add(s,cyl(0.025,0.36,(0.34,-0.05,1.05),'MAT_STAINLESS',12),'Handle')
    add(s,box((1.08,0.10,0.055),(0,0,0.028),'MAT_STONE_DARK'),'Threshold')
    return s


def shower_curtain():
    s = trimesh.Scene()
    add(s,box((1.55,0.055,1.90),(0,0,0.95),'MAT_LINEN'),'Curtain')
    add(s,box((1.72,0.045,0.045),(0,0,2.02),'MAT_STAINLESS'),'Rod')
    for x in np.linspace(-0.68,0.68,9):
        add(s,cyl(0.025,0.025,(x,0,1.95),'MAT_STAINLESS',10),f'Ring_{x:.2f}')
    return s


def bathtub():
    s = trimesh.Scene()
    add(s,box((1.70,0.78,0.55),(0,0,0.275),'MAT_CERAMIC_FIXTURE'),'TubOuter')
    add(s,box((1.40,0.52,0.38),(0,0.0,0.46),'MAT_GLASS_CLEAR'),'TubBasin')
    add(s,cyl(0.025,0.26,(0.62,0.24,0.68),'MAT_STAINLESS',12),'Faucet')
    add(s,cyl(0.025,0.16,(0.48,0.24,0.65),'MAT_STAINLESS',12),'MixerControl')
    return s


def bathroom_detail(name):
    s = trimesh.Scene()
    if 'Towel Rack' in name:
        add(s,box((0.74,0.05,0.05),(0,0,0.75),'MAT_STAINLESS'),'Rack')
        add(s,box((0.62,0.08,0.62),(0,0,0.43),'MAT_LINEN'),'Towel')
    elif 'Towel Shelf' in name:
        add(s,box((0.78,0.30,0.05),(0,0,0.78),'MAT_STAINLESS'),'Shelf')
        for i,z in enumerate((0.84,0.95)):
            add(s,box((0.50,0.24,0.09),(0,0,z),'MAT_LINEN'),f'TowelFold_{i}')
        for label,x in (('Left',-0.34),('Right',0.34)):
            add(s,box((0.04,0.04,0.78),(x,0.10,0.39),'MAT_STAINLESS'),f'ShelfRail_{label}')
    elif 'Mirror Lit' in name:
        add(s,box((0.82,0.05,0.94),(0,0,0.47),'MAT_EMISSIVE_WARM'),'LightFrame')
        add(s,box((0.72,0.02,0.84),(0,-0.035,0.47),'MAT_GLASS_CLEAR'),'Mirror')
    elif 'Wall Sconce' in name:
        add(s,box((0.18,0.12,0.30),(0,0,0.55),'MAT_EMISSIVE_WARM'),'Sconce')
        add(s,box((0.12,0.08,0.18),(0,0.08,0.34),'MAT_BRASS_POLISHED'),'Bracket')
        add(s,box((0.035,0.035,0.40),(0,0.095,0.20),'MAT_BRASS_POLISHED'),'SconceCableChase')
    elif 'Scale' in name:
        add(s,box((0.36,0.36,0.045),(0,0,0.0225),'MAT_PLASTIC_RUBBER'),'ScaleBody')
        add(s,box((0.11,0.06,0.01),(0,-0.12,0.05),'MAT_ELECTRONICS'),'Display')
    elif 'Hair Dryer' in name:
        add(s,box((0.30,0.11,0.18),(0,0,0.55),'MAT_ELECTRONICS'),'DryerBody')
        add(s,box((0.11,0.08,0.30),(-0.08,0,0.35),'MAT_ELECTRONICS'),'Handle')
        add(s,box((0.38,0.08,0.38),(0,0.08,0.48),'MAT_PLASTIC_RUBBER'),'WallMount')
        add(s,cyl(0.035,0.45,(0.18,0.04,0.225),'MAT_ELECTRONICS',10),'Cord')
    elif 'Toiletry Tray' in name:
        add(s,box((0.55,0.32,0.05),(0,0,0.025),'MAT_PLASTIC_RUBBER'),'Tray')
        for i,x in enumerate((-0.16,0,0.16)):
            add(s,cyl(0.035,0.16,(x,0,0.12),'MAT_PLASTIC_RUBBER',12),f'Bottle_{i}')
    elif 'Room Service Tray' in name:
        add(s,box((0.62,0.42,0.05),(0,0,0.025),'MAT_WOOD_WARM'),'Tray')
        add(s,cyl(0.11,0.03,(-0.16,0,0.065),'MAT_CERAMIC_FIXTURE',20),'Plate')
        add(s,cyl(0.04,0.12,(0.18,0,0.10),'MAT_GLASS_CLEAR',16),'Glass')
        add(s,box((0.12,0.05,0.08),(0.02,0.08,0.09),'MAT_LINEN'),'Napkin')
    elif 'Telephone' in name:
        add(s,box((0.34,0.24,0.12),(0,0,0.06),'MAT_ELECTRONICS'),'PhoneBase')
        add(s,box((0.30,0.06,0.07),(0,0,0.16),'MAT_ELECTRONICS'),'Handset')
        for r in range(3):
            for c in range(3):
                add(s,box((0.035,0.018,0.025),(-0.08+c*0.08,-0.13,0.13-r*0.035),'MAT_STAINLESS'),f'Key_{r}_{c}')
    elif 'Alarm Clock' in name:
        add(s,box((0.25,0.10,0.14),(0,0,0.07),'MAT_ELECTRONICS'),'ClockBody')
        add(s,box((0.16,0.015,0.07),(0,-0.058,0.08),'MAT_GLASS_CLEAR'),'Display')
        add(s,box((0.08,0.03,0.025),(0,0.045,0.15),'MAT_STAINLESS'),'SnoozeButton')
    elif 'Ironing Board' in name:
        board = box((1.10,0.32,0.06),(0,0,0.75),'MAT_WOOD_WARM')
        board.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(80),[0,1,0],[0,0,0.75]))
        add(s,board,'FoldedBoard')
        add(s,box((0.05,0.05,1.05),(0,0,0.52),'MAT_STAINLESS'),'Frame')
    elif 'Suitcase Stand' in name:
        for x in (-0.32,0.32):
            add(s,box((0.06,0.45,0.55),(x,0,0.275),'MAT_WOOD_WARM'),f'Leg_{x}')
        for y in (-0.15,0,0.15):
            add(s,box((0.72,0.05,0.04),(0,y,0.56),'MAT_LINEN'),f'Strap_{y}')
    return s


def _monitor(scene, x, z, index):
    add(scene,box((0.42,0.055,0.27),(x,0.10,z),'MAT_ELECTRONICS'),f'Monitor_{index}')
    add(scene,cyl(0.025,0.16,(x,0.10,z-0.20),'MAT_STAINLESS',12),f'MonitorStand_{index}')


def reception(name):
    if 'Sofa' in name:
        if 'Curved' not in name:
            return b3.sofa(name)
        s = trimesh.Scene()
        segments = ((-0.75,-15),(0,0),(0.75,15))
        for i,(x,ang) in enumerate(segments):
            add(s,rotated_box((0.82,0.72,0.18),(x,0,0.47),ang,'MAT_UPHOLSTERY'),f'Seat_{i}')
            add(s,rotated_box((0.82,0.17,0.68),(x,0.31,0.78),ang,'MAT_UPHOLSTERY'),f'Back_{i}')
        add(s,rotated_box((0.18,0.76,0.40),(-1.19,0.02,0.55),-18,'MAT_UPHOLSTERY'),'Arm_Left')
        add(s,rotated_box((0.18,0.76,0.40),(1.19,0.02,0.55),18,'MAT_UPHOLSTERY'),'Arm_Right')
        for x in (-0.72,0,0.72):
            add(s,box((0.62,0.54,0.10),(x,-0.02,0.58),'MAT_UPHOLSTERY'),f'CurvedCushion_{x}')
        for i,(x,y) in enumerate(((-0.92,-0.22),(-0.30,-0.26),(0.30,-0.26),(0.92,-0.22))):
            add(s,box((0.075,0.075,0.38),(x,y,0.19),'MAT_WOOD_DARK'),f'CurvedSofaLeg_{i}')
        return s

    if 'Armchair' in name or 'Lounge Chair' in name:
        s = b3.chair(name,'MAT_UPHOLSTERY')
        if 'Classic' in name:
            add(s,box((0.52,0.08,0.10),(0,0.32,1.19),'MAT_WOOD_DARK'),'ClassicCrest')
            add(s,box((0.46,0.025,0.035),(0,0.265,1.14),'MAT_BRASS_POLISHED'),'ClassicCrestInlay')
        elif 'Modern' in name:
            add(s,box((0.56,0.50,0.055),(0,0,0.028),'MAT_BLACKENED_STEEL'),'ModernBase')
            add(s,cyl(0.055,0.38,(0,0,0.20),'MAT_BLACKENED_STEEL',16),'ModernPedestal')
        else:
            add(s,box((0.58,0.42,0.14),(0,-0.50,0.31),'MAT_UPHOLSTERY'),'LoungeFootrest')
            for x in (-0.22,0.22):
                add(s,box((0.045,0.30,0.28),(x,-0.50,0.14),'MAT_WOOD_DARK'),f'FootrestLeg_{x}')
        return s

    if 'Coffee Table' in name:
        return b3.table_or_bench('Guest Coffee Table Rectangular','MAT_WOOD_WARM')

    s = trimesh.Scene(); w,d,h = 1.20,0.62,1.05
    if 'Double' in name: w = 2.10
    if 'Grand' in name: w,d,h = 2.70,0.78,1.15
    if 'Back Counter' in name: w,d,h = 1.90,0.55,0.86
    if 'Key Cabinet' in name: w,d,h = 1.20,0.28,1.65
    if 'Concierge' in name or 'Bell Desk' in name or 'Host Podium' in name: w,d,h = 0.85,0.55,1.05
    add(s,box((w,d,h),(0,0,h/2),'MAT_WOOD_WARM'),'DeskFront')

    if 'Reception Desk' in name:
        add(s,box((w+0.08,d+0.04,0.07),(0,0,h+0.035),'MAT_MARBLE_LIGHT'),'Countertop')
        add(s,box((w*0.78,0.025,h*0.55),(0,-d/2-0.015,h*0.48),'MAT_WOOD_DARK'),'FrontInset')
        monitor_count = 2 if ('Double' in name or 'Grand' in name) else 1
        positions = np.linspace(-w*0.25,w*0.25,monitor_count)
        for i,x in enumerate(positions):
            _monitor(s,float(x),h+0.28,i)
        if 'Grand' in name:
            add(s,box((w*0.72,0.020,0.045),(0,-d/2-0.045,h*0.72),'MAT_BRASS_POLISHED'),'BrassInlay')
            for x in (-w*0.38,w*0.38):
                add(s,box((0.045,0.025,h*0.48),(x,-d/2-0.045,h*0.42),'MAT_BRASS_POLISHED'),f'GrandVerticalInlay_{x}')

    if 'Back Counter' in name:
        add(s,box((w+0.04,d+0.03,0.055),(0,0,h+0.028),'MAT_STONE_LIGHT'),'WorkSurface')
        for i,x in enumerate((-0.55,0,0.55)):
            add(s,box((0.48,0.025,0.48),(x,-d/2-0.016,0.38),'MAT_WOOD_DARK'),f'StorageDoor_{i}')
            add(s,box((0.12,0.025,0.025),(x,-d/2-0.045,0.48),'MAT_BRASS_POLISHED'),f'StoragePull_{i}')

    if 'Key Cabinet' in name:
        for r in range(5):
            for c in range(4):
                add(s,box((0.18,0.025,0.20),(-0.42+c*0.28,-d/2-0.015,0.25+r*0.27),'MAT_WOOD_DARK'),f'KeySlot_{r}_{c}')

    if 'Concierge' in name:
        add(s,box((0.60,0.10,0.34),(0,-d/2-0.08,0.72),'MAT_BRASS_POLISHED'),'BrochureRack')
        for x in (-0.18,0,0.18):
            add(s,box((0.14,0.025,0.24),(x,-d/2-0.145,0.76),'MAT_LINEN'),'Brochure')
        add(s,cyl(0.035,0.34,(0.26,0.05,h+0.18),'MAT_BRASS_POLISHED',12),'ConciergeLamp')
        add(s,box((0.22,0.14,0.05),(0.26,0.05,h+0.36),'MAT_EMISSIVE_WARM'),'ConciergeLampShade')

    if 'Bell Desk' in name:
        add(s,cyl(0.08,0.07,(0,0.02,h+0.07),'MAT_BRASS_POLISHED',20),'ServiceBell')
        add(s,box((0.52,0.08,0.32),(0,-d/2-0.08,0.70),'MAT_WOOD_DARK'),'LuggageTagRack')
        for i,x in enumerate((-0.16,0,0.16)):
            add(s,box((0.09,0.02,0.15),(x,-d/2-0.13,0.72),'MAT_LINEN'),f'LuggageTag_{i}')

    if 'Host Podium' in name:
        add(s,box((0.50,0.34,0.055),(0,0,h+0.035),'MAT_WOOD_DARK'),'PodiumLip')
        book = box((0.38,0.25,0.035),(0,-0.02,h+0.09),'MAT_LINEN')
        book.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-8),[0,0,1],[0,-0.02,h+0.09]))
        add(s,book,'ReservationBook')
        add(s,box((0.035,0.24,0.035),(0.24,0,h+0.10),'MAT_BRASS_POLISHED'),'HostPen')

    return s


def build(name, mat):
    if name.startswith(('Marble Wall','Brass Trim','Blackened Steel','Decorative Wall','Acoustic Wall','Fabric Wall','Wallpaper','Exterior Paver')):
        return finish_asset(name,mat)
    if name in ('Area Rug Large','Bed Runner','Decorative Pillow Set','Guest Linen Folded Set','Guest Bathrobe Hanging'):
        return soft_goods(name)
    if name.startswith('Bathroom Vanity'):
        return vanity(name)
    if name == 'Bathroom Sink Pedestal':
        return pedestal_sink()
    if name == 'Bathroom Toilet Standard':
        return toilet(False)
    if name == 'Bathroom Toilet Accessible':
        return toilet(True)
    if name == 'Bathroom Shower Glass':
        return shower_glass()
    if name == 'Bathroom Shower Curtain':
        return shower_curtain()
    if name == 'Bathroom Bathtub':
        return bathtub()
    if name.startswith('Bathroom ') or name in ('Hair Dryer Wall Mount','Toiletry Tray','Room Service Tray','Guest Telephone','Alarm Clock','Ironing Board Folded','Suitcase Stand'):
        return bathroom_detail(name)
    return reception(name)


def sidecar(asset_id, subcategory, mat, profile):
    lod = 'lod_architecture' if profile == 'P_ARCH_STATIC' else 'lod_furniture'
    return {
        'schema':1,
        'asset_id':asset_id,
        'asset_type':'StaticMeshAsset',
        'source':'Tools/ArtGeneration/batch04_generate.py',
        'units':'meters',
        'lod_policy':lod,
        'collision_policy':'simple_proxy',
        'material_slots':[mat],
        'tags':['hotel-haven','batch_04',subcategory],
        'dependencies':[],
        'cutaway_policy':'normal',
        'source_revision':3,
        'metadata_revision':3,
        'cooker_schema':1,
        'lifecycle_state':'PRODUCTION',
    }


def generate(manifest:Path, output:Path, package:bool):
    rows = [a for g in json.loads(manifest.read_text())['groups'] for a in g['assets']]
    if len(rows) != 50:
        raise ValueError(f'Batch 04 must contain 50 assets; got {len(rows)}')
    output.mkdir(parents=True,exist_ok=True)
    out = []
    for aid,name,subcategory,mat,profile,animset,anchors in rows:
        scene = build(name,mat)
        p = output/f'{aid}.glb'
        p.write_bytes(scene.export(file_type='glb'))
        out.append(p)
        if package:
            (output/f'{aid}.asset.json').write_text(json.dumps(sidecar(aid,subcategory,mat,profile),indent=2)+'\n')
        print(p)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('manifest',type=Path)
    ap.add_argument('output',type=Path)
    ap.add_argument('--package',action='store_true')
    a = ap.parse_args()
    generate(a.manifest,a.output,a.package)


if __name__ == '__main__':
    main()
