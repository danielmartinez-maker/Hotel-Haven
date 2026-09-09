from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import trimesh

PALETTE = {
    'MAT_WOOD_WARM': ((0.42, 0.22, 0.10, 1), 0.0, 0.52),
    'MAT_WOOD_DARK': ((0.22, 0.12, 0.08, 1), 0.0, 0.48),
    'MAT_CARPET_LUXURY': ((0.22, 0.25, 0.29, 1), 0.0, 0.84),
    'MAT_EMISSIVE_WARM': ((0.90, 0.68, 0.34, 1), 0.0, 0.36),
    'MAT_VEGETATION': ((0.25, 0.38, 0.20, 1), 0.0, 0.74),
    'MAT_STONE_LIGHT': ((0.60, 0.56, 0.49, 1), 0.0, 0.70),
    'MAT_BRASS_POLISHED': ((0.62, 0.38, 0.10, 1), 0.88, 0.30),
    'MAT_BRASS_BRUSHED': ((0.58, 0.36, 0.12, 1), 0.78, 0.42),
    'MAT_BLACKENED_STEEL': ((0.10, 0.12, 0.13, 1), 0.78, 0.50),
    'MAT_SIGNAGE': ((0.16, 0.17, 0.17, 1), 0.25, 0.44),
    'MAT_ELECTRONICS': ((0.08, 0.09, 0.10, 1), 0.12, 0.36),
    'MAT_UPHOLSTERY': ((0.42, 0.31, 0.25, 1), 0.0, 0.84),
    'MAT_PLASTIC_RUBBER': ((0.18, 0.18, 0.18, 1), 0.0, 0.64),
    'MAT_STAINLESS': ((0.58, 0.61, 0.62, 1), 0.85, 0.42),
    'MAT_GLASS_CLEAR': ((0.34, 0.55, 0.64, 0.42), 0.0, 0.10),
    'MAT_LINEN': ((0.88, 0.86, 0.80, 1), 0.0, 0.88),
    'MAT_CERAMIC_FIXTURE': ((0.90, 0.89, 0.85, 1), 0.0, 0.24),
    'MAT_MARBLE_LIGHT': ((0.88, 0.84, 0.76, 1), 0.0, 0.38),
}


def material(name):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_WOOD_WARM'])
    encoded = np.clip(np.rint(np.asarray(rgba, dtype=float) * 255.0), 0, 255).astype(np.uint8)
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=encoded,
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def box(extents, center=(0, 0, 0), mat='MAT_WOOD_WARM'):
    mesh = trimesh.creation.box(extents=extents)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def cyl(radius, height, center=(0, 0, 0), mat='MAT_STAINLESS', sections=18):
    mesh = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def sph(radius, center=(0, 0, 0), mat='MAT_VEGETATION'):
    mesh = trimesh.creation.icosphere(subdivisions=2, radius=radius)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def add(scene, mesh, name):
    scene.add_geometry(mesh, node_name=name, geom_name=name)


def table(name, mat):
    scene = trimesh.Scene()
    w, d, h = 0.62, 0.62, 0.56
    if 'Console' in name:
        w, d, h = 1.45, 0.38, 0.82
    elif 'Dining Table Two' in name:
        w, d, h = 0.80, 0.80, 0.75
    elif 'Dining Table Four' in name:
        w, d, h = 1.25, 0.85, 0.75
    elif 'Dining Table Six' in name:
        w, d, h = 1.80, 0.90, 0.75
    add(scene, box((w, d, 0.07), (0, 0, h), mat), 'TableTop')
    for i, (x, y) in enumerate(((-w * 0.4, -d * 0.35), (w * 0.4, -d * 0.35), (-w * 0.4, d * 0.35), (w * 0.4, d * 0.35))):
        add(scene, box((0.055, 0.055, h - 0.03), (x, y, (h - 0.03) / 2), 'MAT_WOOD_DARK'), f'Leg_{i}')
    if 'Console' in name:
        add(scene, box((w * 0.72, 0.04, 0.24), (0, d * 0.30, h - 0.24), 'MAT_BRASS_BRUSHED'), 'ConsoleBrace')
    return scene


def rug():
    scene = trimesh.Scene()
    add(scene, box((2.4, 1.7, 0.028), (0, 0, 0.014), 'MAT_CARPET_LUXURY'), 'Rug')
    add(scene, box((2.15, 1.45, 0.008), (0, 0, 0.032), 'MAT_CARPET_LUXURY'), 'PatternInset')
    return scene


def lamp(name):
    scene = trimesh.Scene()
    if 'Chandelier' in name:
        add(scene, cyl(0.18, 0.05, (0, 0, 2.02), 'MAT_BRASS_POLISHED', 20), 'CeilingCanopy')
        add(scene, cyl(0.025, 0.86, (0, 0, 1.57), 'MAT_BRASS_POLISHED', 12), 'Stem')
        add(scene, cyl(0.36, 0.035, (0, 0, 1.15), 'MAT_BRASS_POLISHED', 24), 'Ring')
        for i, angle in enumerate(np.linspace(0, 2 * np.pi, 8, endpoint=False)):
            x, y = 0.36 * np.cos(angle), 0.36 * np.sin(angle)
            add(scene, cyl(0.025, 0.32, (x, y, 1.02), 'MAT_BRASS_POLISHED', 10), f'Arm_{i}')
            add(scene, sph(0.075, (x, y, 0.84), 'MAT_EMISSIVE_WARM'), f'Bulb_{i}')
        return scene
    if 'Floor Lamp' in name:
        add(scene, cyl(0.18, 0.05, (0, 0, 0.025), 'MAT_BRASS_POLISHED', 20), 'Base')
        add(scene, cyl(0.025, 1.35, (0, 0, 0.70), 'MAT_BRASS_POLISHED', 12), 'Stem')
        shade = trimesh.creation.cone(0.28, 0.34, sections=24)
        shade.apply_translation((0, 0, 1.48)); shade.visual = trimesh.visual.TextureVisuals(material=material('MAT_EMISSIVE_WARM'))
        add(scene, shade, 'Shade')
        return scene
    add(scene, cyl(0.13, 0.04, (0, 0, 0.02), 'MAT_BRASS_POLISHED', 20), 'Base')
    add(scene, cyl(0.02, 0.35, (0, 0, 0.20), 'MAT_BRASS_POLISHED', 12), 'Stem')
    shade = trimesh.creation.cone(0.21, 0.25, sections=24)
    shade.apply_translation((0, 0, 0.50)); shade.visual = trimesh.visual.TextureVisuals(material=material('MAT_EMISSIVE_WARM'))
    add(scene, shade, 'Shade')
    return scene


def planter(name):
    scene = trimesh.Scene()
    tall = 'Tall' in name
    h = 0.62 if tall else 0.34
    radius = 0.28 if tall else 0.38
    add(scene, cyl(radius, h, (0, 0, h / 2), 'MAT_STONE_LIGHT', 24), 'Planter')
    if 'Tree' in name:
        add(scene, cyl(0.07, 1.35, (0, 0, h + 0.65), 'MAT_WOOD_DARK', 12), 'Trunk')
        for i, (x, y, z) in enumerate(((0, 0, 1.8), (0.25, 0, 1.55), (-0.25, 0.08, 1.58), (0, 0.22, 1.65))):
            add(scene, sph(0.34, (x, y, z), 'MAT_VEGETATION'), f'Canopy_{i}')
    else:
        for i, angle in enumerate(np.linspace(0, 2 * np.pi, 7, endpoint=False)):
            x, y = 0.18 * np.cos(angle), 0.18 * np.sin(angle)
            add(scene, box((0.10, 0.035, 0.55), (x, y, h + 0.27), 'MAT_VEGETATION'), f'Leaf_{i}')
    return scene


def fountain():
    scene = trimesh.Scene()
    add(scene, cyl(0.62, 0.18, (0, 0, 0.09), 'MAT_STONE_LIGHT', 32), 'Basin')
    add(scene, cyl(0.18, 0.52, (0, 0, 0.35), 'MAT_STONE_LIGHT', 24), 'Pedestal')
    add(scene, cyl(0.35, 0.10, (0, 0, 0.66), 'MAT_STONE_LIGHT', 32), 'UpperBowl')
    add(scene, cyl(0.025, 0.42, (0, 0, 0.91), 'MAT_STAINLESS', 12), 'WaterJet')
    return scene


def cart(name, mat):
    scene = trimesh.Scene()
    w, length = 1.05, 1.35
    if 'Modern' in name:
        w, length = 0.96, 1.22
    add(scene, box((w, length, 0.10), (0, 0, 0.23), mat), 'CartBase')
    for i, x in enumerate((-w * 0.44, w * 0.44)):
        add(scene, cyl(0.025, 1.55, (x, 0, 1.0), mat, 12), f'Upright_{i}')
    add(scene, box((w, 0.05, 0.05), (0, 0, 1.75), mat), 'TopBar')
    if 'Modern' in name:
        add(scene, box((w * 0.88, 0.06, 0.06), (0, length * 0.42, 1.20), 'MAT_BLACKENED_STEEL'), 'ModernPushBar')
    if 'Covered' in name:
        add(scene, box((w * 0.92, length * 0.75, 0.75), (0, 0, 0.85), 'MAT_LINEN'), 'Cover')
    for i, (x, y) in enumerate(((-w * 0.40, -length * 0.38), (w * 0.40, -length * 0.38), (-w * 0.40, length * 0.38), (w * 0.40, length * 0.38))):
        wheel = cyl(0.11, 0.05, (x, y, 0.11), 'MAT_BLACKENED_STEEL', 16)
        wheel.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0], [x, y, 0.11]))
        add(scene, wheel, f'MOV_Wheel_{i}')
    return scene


def stanchion(name, mat):
    scene = trimesh.Scene()
    for i, x in enumerate((-0.62, 0.62)):
        add(scene, cyl(0.16, 0.05, (x, 0, 0.025), mat, 20), f'Base_{i}')
        add(scene, cyl(0.025, 0.95, (x, 0, 0.50), mat, 12), f'Post_{i}')
        add(scene, sph(0.055, (x, 0, 0.99), mat), f'Cap_{i}')
    barrier_mat = 'MAT_UPHOLSTERY' if 'Rope' in name else mat
    add(scene, box((1.15, 0.055, 0.055), (0, 0, 0.82), barrier_mat), 'Barrier')
    return scene


def kiosk(name):
    scene = trimesh.Scene()
    if 'Computer' in name:
        add(scene, box((0.52, 0.06, 0.34), (0, 0, 0.72), 'MAT_ELECTRONICS'), 'Monitor')
        add(scene, box((0.08, 0.10, 0.28), (0, 0.03, 0.47), 'MAT_BLACKENED_STEEL'), 'MonitorStand')
        add(scene, box((0.38, 0.20, 0.035), (0, -0.12, 0.47), 'MAT_ELECTRONICS'), 'Keyboard')
        return scene
    if 'Telephone' in name:
        add(scene, box((0.34, 0.24, 0.12), (0, 0, 0.06), 'MAT_ELECTRONICS'), 'PhoneBase')
        add(scene, box((0.30, 0.06, 0.07), (0, 0, 0.16), 'MAT_ELECTRONICS'), 'Handset')
        return scene
    if 'Encoder' in name:
        add(scene, box((0.40, 0.30, 0.14), (0, 0, 0.07), 'MAT_ELECTRONICS'), 'EncoderBody')
        add(scene, box((0.24, 0.13, 0.02), (0, -0.08, 0.15), 'MAT_GLASS_CLEAR'), 'CardBed')
        add(scene, box((0.20, 0.04, 0.05), (0, -0.17, 0.08), 'MAT_SIGNAGE'), 'EncoderSlot')
        return scene
    w, d, h = 0.55, 0.38, 1.25
    add(scene, box((w, d, h), (0, 0, h / 2), 'MAT_ELECTRONICS'), 'KioskBody')
    add(scene, box((w * 0.72, 0.025, 0.38), (0, -d / 2 - 0.014, 0.88), 'MAT_GLASS_CLEAR'), 'Screen')
    add(scene, box((w * 0.55, 0.05, 0.08), (0, -d / 2 - 0.035, 0.56), 'MAT_ELECTRONICS'), 'InputShelf')
    if 'Self Check In' in name:
        add(scene, box((0.10, 0.03, 0.16), (0.18, -d / 2 - 0.055, 0.62), 'MAT_SIGNAGE'), 'CardReader')
        add(scene, box((0.24, 0.025, 0.045), (0, -d / 2 - 0.055, 0.43), 'MAT_SIGNAGE'), 'ReceiptSlot')
    elif 'Information' in name:
        add(scene, box((0.42, 0.025, 0.25), (0, -d / 2 - 0.055, 1.05), 'MAT_SIGNAGE'), 'MapPanel')
        add(scene, sph(0.06, (0, 0, 1.38), 'MAT_EMISSIVE_WARM'), 'InfoBeacon')
    return scene


def rack(name, mat):
    scene = trimesh.Scene()
    w, d, h = 0.75, 0.32, 1.25
    if 'Umbrella' in name:
        add(scene, cyl(0.22, 0.62, (0, 0, 0.31), mat, 20), 'UmbrellaBin')
        add(scene, cyl(0.19, 0.04, (0, 0, 0.64), 'MAT_BRASS_POLISHED', 20), 'UmbrellaRim')
        return scene
    if 'Coat Check' in name:
        add(scene, cyl(0.035, 1.55, (0, 0, 0.775), 'MAT_WOOD_WARM', 12), 'Post')
        add(scene, box((1.15, 0.08, 0.08), (0, 0, 1.50), 'MAT_WOOD_WARM'), 'Rail')
        add(scene, cyl(0.22, 0.05, (0, 0, 0.025), 'MAT_WOOD_DARK', 20), 'Foot')
        for i, x in enumerate(np.linspace(-0.5, 0.5, 6)):
            add(scene, cyl(0.015, 0.18, (x, 0, 1.35), 'MAT_BRASS_POLISHED', 10), f'Hook_{i}')
        return scene
    add(scene, box((w, d, 0.07), (0, 0, 0.035), mat), 'RackBase')
    for i, z in enumerate(np.linspace(0.30, h - 0.12, 4)):
        add(scene, box((w * 0.90, d * 0.85, 0.035), (0, 0, z), mat), f'Shelf_{i}')
    for i, x in enumerate((-w / 2, w / 2)):
        add(scene, box((0.045, 0.045, h), (x, 0, h / 2), mat), f'Upright_{i}')
    if 'Newspaper' in name:
        add(scene, box((w * 0.80, 0.04, 0.28), (0, -d / 2 - 0.02, 0.95), 'MAT_SIGNAGE'), 'HeadlinePanel')
    return scene


def bench(name, mat):
    scene = trimesh.Scene()
    w, d, h = 1.55, 0.52, 0.47
    add(scene, box((w, d, 0.16), (0, 0, h), mat), 'Seat')
    if 'Upholstered' in name:
        add(scene, box((w, 0.14, 0.62), (0, d / 2 - 0.03, h + 0.28), mat), 'Back')
    for i, x in enumerate((-w * 0.40, w * 0.40)):
        add(scene, box((0.07, 0.07, h - 0.08), (x, 0, (h - 0.08) / 2), 'MAT_WOOD_DARK'), f'Leg_{i}')
    return scene


def banquette(name):
    scene = trimesh.Scene()
    if 'Corner' in name:
        add(scene, box((1.40, 0.62, 0.18), (-0.36, 0, 0.48), 'MAT_UPHOLSTERY'), 'SeatA')
        add(scene, box((0.62, 1.40, 0.18), (0.40, 0.38, 0.48), 'MAT_UPHOLSTERY'), 'SeatB')
        add(scene, box((1.40, 0.14, 0.70), (-0.36, 0.25, 0.80), 'MAT_UPHOLSTERY'), 'BackA')
        add(scene, box((0.14, 1.40, 0.70), (0.70, 0.38, 0.80), 'MAT_UPHOLSTERY'), 'BackB')
        feet = ((-0.80, -0.20), (0.10, -0.20), (0.55, 0.15), (0.55, 0.75))
    else:
        add(scene, box((1.65, 0.62, 0.18), (0, 0, 0.48), 'MAT_UPHOLSTERY'), 'Seat')
        add(scene, box((1.65, 0.14, 0.70), (0, 0.25, 0.80), 'MAT_UPHOLSTERY'), 'Back')
        feet = ((-0.65, -0.18), (0.65, -0.18), (-0.65, 0.18), (0.65, 0.18))
    for i, (x, y) in enumerate(feet):
        add(scene, box((0.07, 0.07, 0.39), (x, y, 0.195), 'MAT_WOOD_DARK'), f'Foot_{i}')
    return scene


def utility(name, mat):
    scene = trimesh.Scene()
    if 'Waste Bin' in name or 'Recycling Bin' in name:
        add(scene, cyl(0.22, 0.60, (0, 0, 0.30), mat, 20), 'Bin')
        add(scene, cyl(0.20, 0.035, (0, 0, 0.62), 'MAT_BLACKENED_STEEL', 20), 'Rim')
        if 'Recycling' in name:
            add(scene, box((0.18, 0.02, 0.18), (0, -0.225, 0.38), 'MAT_SIGNAGE'), 'RecycleMark')
        return scene
    if 'Water Dispenser' in name:
        add(scene, box((0.45, 0.42, 0.90), (0, 0, 0.45), 'MAT_ELECTRONICS'), 'Dispenser')
        add(scene, cyl(0.16, 0.38, (0, 0, 1.07), 'MAT_GLASS_CLEAR', 20), 'WaterBottle')
        add(scene, box((0.12, 0.04, 0.08), (0, -0.23, 0.58), 'MAT_SIGNAGE'), 'Tap')
        return scene
    if 'Coffee Station' in name:
        add(scene, box((0.90, 0.50, 0.88), (0, 0, 0.44), 'MAT_WOOD_WARM'), 'Cabinet')
        add(scene, box((0.40, 0.30, 0.38), (0.10, -0.04, 1.08), 'MAT_ELECTRONICS'), 'CoffeeMachine')
        add(scene, cyl(0.05, 0.12, (-0.28, -0.08, 0.98), 'MAT_CERAMIC_FIXTURE', 16), 'Cup')
        return scene
    if 'Charging Station' in name:
        add(scene, box((0.50, 0.34, 0.76), (0, 0, 0.38), 'MAT_ELECTRONICS'), 'ChargingBody')
        for i, x in enumerate((-0.15, 0, 0.15)):
            add(scene, box((0.06, 0.02, 0.04), (x, -0.18, 0.63), 'MAT_GLASS_CLEAR'), f'Port_{i}')
        return scene
    if 'Feedback' in name:
        add(scene, box((0.45, 0.30, 1.05), (0, 0, 0.525), 'MAT_WOOD_WARM'), 'FeedbackStand')
        add(scene, box((0.28, 0.02, 0.035), (0, -0.16, 0.90), 'MAT_SIGNAGE'), 'Slot')
        return scene
    if 'Clock' in name:
        add(scene, cyl(0.34, 0.055, (0, 0, 0.34), 'MAT_WOOD_WARM', 32), 'ClockFrame')
        add(scene, cyl(0.30, 0.025, (0, -0.04, 0.34), 'MAT_SIGNAGE', 32), 'ClockFace')
        add(scene, box((0.015, 0.02, 0.20), (0, -0.065, 0.39), 'MAT_BRASS_POLISHED'), 'MinuteHand')
        return scene
    if 'Pedestal' in name or 'Plinth' in name:
        add(scene, box((0.55, 0.55, 1.00), (0, 0, 0.50), 'MAT_STONE_LIGHT'), 'Plinth')
        add(scene, box((0.64, 0.64, 0.08), (0, 0, 1.04), 'MAT_MARBLE_LIGHT'), 'Top')
        return scene
    if 'Wayfinding' in name or 'Directory' in name:
        add(scene, box((0.58, 0.18, 1.50), (0, 0, 0.75), 'MAT_SIGNAGE'), 'SignBody')
        add(scene, box((0.48, 0.025, 0.72), (0, -0.105, 1.02), 'MAT_GLASS_CLEAR'), 'InformationPanel')
        return scene
    if 'Floral' in name:
        add(scene, cyl(0.16, 0.30, (0, 0, 0.15), 'MAT_GLASS_CLEAR', 20), 'Vase')
        for i, angle in enumerate(np.linspace(0, 2 * np.pi, 9, endpoint=False)):
            add(scene, sph(0.09, (0.18 * np.cos(angle), 0.18 * np.sin(angle), 0.55 + 0.08 * (i % 3)), 'MAT_VEGETATION'), f'Flower_{i}')
        return scene
    if 'Partition Screen' in name:
        for i, x in enumerate((-0.60, 0, 0.60)):
            add(scene, box((0.55, 0.08, 1.75), (x, 0, 0.875), 'MAT_WOOD_WARM'), f'Panel_{i}')
        return scene
    if 'Fireplace' in name:
        add(scene, box((1.45, 0.35, 0.95), (0, 0, 0.475), 'MAT_STONE_LIGHT'), 'FireplaceBody')
        add(scene, box((0.90, 0.03, 0.48), (0, -0.19, 0.42), 'MAT_ELECTRONICS'), 'EmberScreen')
        add(scene, box((1.55, 0.45, 0.09), (0, 0, 0.99), 'MAT_WOOD_WARM'), 'Mantel')
        return scene
    if 'Piano' in name:
        add(scene, box((1.45, 0.62, 0.88), (0, 0, 0.44), 'MAT_WOOD_WARM'), 'PianoBody')
        add(scene, box((1.12, 0.30, 0.08), (0, -0.36, 0.72), 'MAT_CERAMIC_FIXTURE'), 'Keys')
        for i, x in enumerate((-0.56, 0.56)):
            add(scene, box((0.08, 0.08, 0.44), (x, 0.20, 0.22), 'MAT_WOOD_DARK'), f'PianoLeg_{i}')
        return scene
    add(scene, box((0.65, 0.45, 0.72), (0, 0, 0.36), mat), 'Body')
    return scene


def build(name, mat):
    if 'Table' in name and 'Charging' not in name:
        return table(name, mat)
    if 'Rug' in name:
        return rug()
    if 'Lamp' in name or 'Chandelier' in name:
        return lamp(name)
    if 'Planter' in name or 'Indoor Tree' in name:
        return planter(name)
    if 'Fountain' in name:
        return fountain()
    if 'Cart' in name:
        return cart(name, mat)
    if 'Queue' in name:
        return stanchion(name, mat)
    if 'Kiosk' in name or 'Computer' in name or 'Telephone' in name or 'Encoder' in name:
        return kiosk(name)
    if any(k in name for k in ('Rack', 'Newspaper Stand', 'Umbrella Stand', 'Coat Check')):
        return rack(name, mat)
    if 'Bench' in name:
        return bench(name, mat)
    if 'Banquette' in name:
        return banquette(name)
    if 'Document Tray' in name:
        scene = trimesh.Scene(); add(scene, box((0.42, 0.30, 0.045), (0, 0, 0.0225), mat), 'Tray'); return scene
    return utility(name, mat)


def sidecar(aid, sub, mat, profile, anim):
    tags = ['hotel-haven', 'batch_05', sub]
    if anim:
        tags += ['animated-binding', anim]
    return {
        'schema': 1,
        'asset_id': aid,
        'asset_type': 'StaticMeshAsset',
        'source': 'Tools/ArtGeneration/batch05_generate.py',
        'units': 'meters',
        'lod_policy': 'lod_furniture',
        'collision_policy': 'simple_proxy',
        'material_slots': [mat],
        'tags': tags,
        'dependencies': [anim] if anim else [],
        'cutaway_policy': 'normal',
        'source_revision': 2,
        'metadata_revision': 2,
        'cooker_schema': 1,
        'lifecycle_state': 'PRODUCTION',
    }


def generate(manifest: Path, out: Path, package: bool):
    rows = [asset for group in json.loads(manifest.read_text(encoding='utf-8'))['groups'] for asset in group['assets']]
    if len(rows) != 50:
        raise ValueError(len(rows))
    out.mkdir(parents=True, exist_ok=True)
    outputs = []
    for aid, name, sub, mat, profile, anim, anchors in rows:
        scene = build(name, mat); scene.units = 'meters'
        path = out / f'{aid}.glb'; path.write_bytes(scene.export(file_type='glb')); outputs.append(path)
        if package:
            (out / f'{aid}.asset.json').write_text(json.dumps(sidecar(aid, sub, mat, profile, anim), indent=2) + '\n', encoding='utf-8')
        print(path)
    return outputs


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('manifest', type=Path); parser.add_argument('output', type=Path); parser.add_argument('--package', action='store_true')
    args = parser.parse_args(); generate(args.manifest, args.output, args.package)


if __name__ == '__main__':
    main()
