from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Callable

import numpy as np
import trimesh

PALETTE = {
    'MAT_WOOD_WARM': ((0.42, 0.22, 0.10, 1.0), 0.0, 0.52),
    'MAT_UPHOLSTERY': ((0.42, 0.31, 0.25, 1.0), 0.0, 0.84),
    'MAT_STAINLESS': ((0.58, 0.61, 0.62, 1.0), 0.85, 0.42),
    'MAT_BLACKENED_STEEL': ((0.10, 0.12, 0.13, 1.0), 0.78, 0.50),
    'MAT_GLASS_CLEAR': ((0.34, 0.55, 0.64, 0.42), 0.0, 0.10),
    'MAT_SIGNAGE': ((0.16, 0.17, 0.17, 1.0), 0.25, 0.44),
    'MAT_ELECTRONICS': ((0.08, 0.09, 0.10, 1.0), 0.12, 0.36),
    'MAT_LINEN': ((0.88, 0.86, 0.80, 1.0), 0.0, 0.88),
    'MAT_BRASS_POLISHED': ((0.62, 0.38, 0.10, 1.0), 0.88, 0.30),
    'MAT_STONE_LIGHT': ((0.58, 0.55, 0.50, 1.0), 0.0, 0.72),
    'MAT_CERAMIC_FIXTURE': ((0.76, 0.75, 0.70, 1.0), 0.0, 0.30),
    'MAT_CHARACTER_COMPOSITE': ((0.24, 0.26, 0.30, 1.0), 0.0, 0.68),
    'MAT_EMISSIVE_WARM': ((0.92, 0.64, 0.30, 1.0), 0.0, 0.34),
    'MAT_VEGETATION': ((0.18, 0.32, 0.16, 1.0), 0.0, 0.80),
}

PROFILE_ASSET_TYPES = {
    'P_ARCH_STATIC': 'StaticMeshAsset',
    'P_ARCH_ANIMATED': 'SkinnedMeshAsset',
    'P_FURNITURE_STATIC': 'StaticMeshAsset',
    'P_SERVICE_PROP_ANIMATED': 'SkinnedMeshAsset',
    'P_INTERACTIVE_PREFAB': 'PrefabAsset',
    'P_INTERACTIVE_ANIMATED': 'SkinnedMeshAsset',
    'P_SMALL_PROP': 'StaticMeshAsset',
    'P_CHARACTER': 'SkinnedMeshAsset',
}

PROFILE_DEFAULTS = {
    'P_ARCH_STATIC': ('lod_architecture', 'simple_proxy', 'normal'),
    'P_ARCH_ANIMATED': ('lod_architecture', 'simple_proxy', 'normal'),
    'P_FURNITURE_STATIC': ('lod_furniture', 'simple_proxy', 'normal'),
    'P_SERVICE_PROP_ANIMATED': ('lod_furniture', 'simple_proxy', 'normal'),
    'P_INTERACTIVE_PREFAB': ('lod_furniture', 'simple_proxy', 'normal'),
    'P_INTERACTIVE_ANIMATED': ('lod_furniture', 'simple_proxy', 'normal'),
    'P_SMALL_PROP': ('lod_small_prop', 'none', 'normal'),
    'P_CHARACTER': ('lod_character', 'capsule_runtime', 'normal'),
}


def _seed(text: str) -> int:
    return int.from_bytes(hashlib.sha256(text.encode('utf-8')).digest()[:4], 'little')


def material(name: str):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_BLACKENED_STEEL'])
    encoded = np.clip(np.rint(np.asarray(rgba, dtype=float) * 255.0), 0, 255).astype(np.uint8)
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=encoded,
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def box(extents, center=(0.0, 0.0, 0.0), mat='MAT_WOOD_WARM'):
    mesh = trimesh.creation.box(extents=extents)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def cyl(radius, height, center=(0.0, 0.0, 0.0), mat='MAT_STAINLESS', sections=18):
    mesh = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def sphere(radius, center=(0.0, 0.0, 0.0), mat='MAT_VEGETATION', subdivisions=2):
    mesh = trimesh.creation.icosphere(radius=radius, subdivisions=subdivisions)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def add(scene: trimesh.Scene, mesh, name: str):
    scene.add_geometry(mesh, node_name=name, geom_name=name)


def semantic_composite(name: str, mat: str, asset_id: str = '') -> trimesh.Scene:
    """Deterministic non-placeholder composite for secondary prop families.

    The form intentionally carries a base, primary mass, inset panel, functional
    detail, role detail and accent so a fallback can never collapse to a lone
    anonymous box. Dimensions vary deterministically from the logical ID/name.
    """
    s = _seed(asset_id or name)
    w = 0.55 + 0.07 * (s % 6)
    d = 0.38 + 0.05 * ((s >> 3) % 5)
    h = 0.62 + 0.08 * ((s >> 6) % 7)
    scene = trimesh.Scene()
    add(scene, box((w * 0.92, d * 0.88, 0.06), (0, 0, 0.03), 'MAT_BLACKENED_STEEL'), 'Base')
    add(scene, box((w, d, h), (0, 0, 0.06 + h / 2), mat), 'PrimaryForm')
    add(scene, box((w * 0.70, 0.025, h * 0.32), (0, -d / 2 - 0.014, h * 0.58), 'MAT_ELECTRONICS'), 'FunctionalDetail')
    add(scene, box((w * 0.42, d * 0.72, 0.045), (0, 0, h + 0.085), 'MAT_STAINLESS'), 'RoleDetail')
    add(scene, box((w * 0.12, 0.035, h * 0.18), (w * 0.34, -d / 2 - 0.02, h * 0.78), 'MAT_BRASS_POLISHED'), 'Accent')
    return scene


def chair(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    seat_w = 0.52 if 'Lounge' not in name and 'Armchair' not in name else 0.68
    seat_d = 0.52 if seat_w < 0.6 else 0.64
    add(scene, box((seat_w, seat_d, 0.10), (0, 0, 0.48), mat), 'Seat')
    add(scene, box((seat_w, 0.09, 0.58), (0, seat_d * 0.42, 0.78), mat), 'Back')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        add(scene, box((0.045, 0.045, 0.43), (x * seat_w * 0.39, y * seat_d * 0.36, 0.215), 'MAT_BLACKENED_STEEL'), f'Leg_{i}')
    if any(k in name for k in ('Lounge', 'Armchair', 'Club', 'Premium')):
        for side, x in (('L', -seat_w * 0.48), ('R', seat_w * 0.48)):
            add(scene, box((0.08, seat_d * 0.84, 0.18), (x, 0, 0.62), mat), f'Arm_{side}')
    return scene


def sofa(name: str, mat: str) -> trimesh.Scene:
    seats = 3 if 'Three' in name else 2
    width = 0.70 * seats
    scene = trimesh.Scene()
    add(scene, box((width, 0.70, 0.18), (0, 0, 0.46), mat), 'SeatBase')
    add(scene, box((width, 0.12, 0.70), (0, 0.28, 0.78), mat), 'Back')
    for i in range(seats):
        x = (i - (seats - 1) / 2) * 0.65
        add(scene, box((0.59, 0.57, 0.10), (x, -0.02, 0.59), 'MAT_UPHOLSTERY'), f'SeatCushion_{i}')
        add(scene, box((0.59, 0.10, 0.48), (x, 0.20, 0.86), 'MAT_UPHOLSTERY'), f'BackCushion_{i}')
    leg_height = 0.37
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        add(
            scene,
            box(
                (0.055, 0.055, leg_height),
                (x * width * 0.42, y * 0.70 * 0.36, leg_height / 2),
                'MAT_BLACKENED_STEEL',
            ),
            f'BaseLeg_{i}',
        )
    for side, x in (('L', -width / 2 - 0.05), ('R', width / 2 + 0.05)):
        add(scene, box((0.10, 0.68, 0.34), (x, 0, 0.58), mat), f'Arm_{side}')
    return scene



def lamp(name: str, mat: str) -> trimesh.Scene:
    """Purpose-built bedside, table and floor lamps with a grounded base."""
    scene = trimesh.Scene()
    is_floor = 'Floor Lamp' in name
    overall = 1.34 if is_floor else 0.48
    base_radius = 0.18 if is_floor else 0.13
    add(scene, cyl(base_radius, 0.07, (0, 0, 0.035), 'MAT_BRASS_POLISHED', 20), 'Base')
    stem_height = overall * 0.68
    add(
        scene,
        cyl(0.025 if is_floor else 0.018, stem_height,
            (0, 0, 0.07 + stem_height / 2), 'MAT_BRASS_POLISHED', 14),
        'FrameStem',
    )
    shade_z = 0.07 + stem_height + overall * 0.12
    add(
        scene,
        cyl(0.20 if is_floor else 0.15, overall * 0.24,
            (0, 0, shade_z), mat, 24),
        'LampShade',
    )
    add(
        scene,
        sphere(0.055 if is_floor else 0.04,
               (0, 0, shade_z - overall * 0.02), 'MAT_EMISSIVE_WARM', 1),
        'FunctionalBulb',
    )
    return scene


def wall_feature(name: str, mat: str) -> trimesh.Scene:
    """Wall art and mirrors with distinct framed silhouettes."""
    scene = trimesh.Scene()
    is_mirror = 'Mirror' in name
    width = 0.62 if is_mirror else 1.10
    height = 1.55 if is_mirror else 0.72
    depth = 0.055
    add(scene, box((width, depth, height), (0, 0, height / 2), 'MAT_WOOD_WARM'), 'Frame')
    inset_mat = 'MAT_GLASS_CLEAR' if is_mirror else mat
    add(
        scene,
        box((width * 0.88, depth + 0.012, height * 0.88),
            (0, -0.008, height / 2), inset_mat),
        'MirrorGlass' if is_mirror else 'ArtPanel',
    )
    if not is_mirror:
        add(
            scene,
            box((width * 0.28, depth + 0.018, height * 0.46),
                (-width * 0.20, -0.014, height * 0.52), 'MAT_BRASS_POLISHED'),
            'RoleAccent',
        )
    return scene


def bathroom_detail(name: str, mat: str) -> trimesh.Scene:
    """Small bathroom fixtures that should not fall back to a generic cabinet."""
    scene = trimesh.Scene()
    if 'Shower Tray' in name:
        add(scene, box((0.92, 0.92, 0.08), (0, 0, 0.04), mat), 'BaseTray')
        add(scene, box((0.76, 0.76, 0.025), (0, 0, 0.09), 'MAT_STONE_LIGHT'), 'FunctionalInset')
    elif 'Shower Head' in name:
        add(scene, cyl(0.022, 0.68, (0, 0, 0.34), 'MAT_STAINLESS', 12), 'FrameRiser')
        add(scene, cyl(0.16, 0.045, (0, 0, 0.70), 'MAT_STAINLESS', 22), 'FunctionalHead')
    elif 'Towel Rail' in name:
        add(scene, box((0.68, 0.035, 0.035), (0, 0, 0.12), 'MAT_STAINLESS'), 'FrameRail')
        for side, x in (('L', -0.31), ('R', 0.31)):
            add(scene, box((0.035, 0.08, 0.18), (x, 0.03, 0.09), 'MAT_STAINLESS'), f'Mount_{side}')
    elif 'Towel Shelf' in name:
        add(scene, box((0.72, 0.34, 0.05), (0, 0, 0.12), 'MAT_STAINLESS'), 'BaseShelf')
        add(scene, box((0.60, 0.26, 0.12), (0, 0, 0.205), 'MAT_LINEN'), 'FoldedTowel_0')
        add(scene, box((0.52, 0.24, 0.10), (0, 0, 0.315), 'MAT_LINEN'), 'FoldedTowel_1')
    elif 'Amenity Tray' in name:
        add(scene, box((0.46, 0.26, 0.045), (0, 0, 0.023), mat), 'BaseTray')
        for i, x in enumerate((-0.14, 0.0, 0.14)):
            add(scene, cyl(0.035, 0.13 + 0.02 * (i % 2),
                           (x, 0, 0.10), 'MAT_CERAMIC_FIXTURE', 12), f'Bottle_{i}')
    elif 'Tissue Box' in name:
        add(scene, box((0.24, 0.13, 0.11), (0, 0, 0.055), mat), 'BaseBox')
        add(scene, box((0.08, 0.025, 0.10), (0, 0, 0.15), 'MAT_LINEN'), 'FunctionalTissue')
    elif 'Hair Dryer' in name:
        add(scene, box((0.24, 0.08, 0.28), (0, 0, 0.14), 'MAT_ELECTRONICS'), 'FrameMount')
        add(scene, cyl(0.07, 0.22, (0, -0.08, 0.20), 'MAT_ELECTRONICS', 16), 'FunctionalDryer')
        add(scene, box((0.045, 0.045, 0.20), (0.05, -0.08, 0.06), 'MAT_ELECTRONICS'), 'Handle')
    elif 'Grab Bar' in name or 'Toilet Rail' in name:
        width = 0.72
        add(scene, box((width, 0.045, 0.045), (0, 0, 0.24), 'MAT_STAINLESS'), 'FrameRail')
        for side, x in (('L', -width * 0.46), ('R', width * 0.46)):
            add(scene, box((0.045, 0.16, 0.28), (x, 0.06, 0.14), 'MAT_STAINLESS'), f'Mount_{side}')
    else:
        return semantic_composite(name, mat)
    return scene


def ottoman(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    add(scene, box((0.64, 0.56, 0.10), (0, 0, 0.05), 'MAT_BLACKENED_STEEL'), 'Base')
    add(scene, box((0.68, 0.60, 0.30), (0, 0, 0.25), mat), 'SeatCushion')
    add(scene, box((0.54, 0.46, 0.035), (0, 0, 0.42), 'MAT_UPHOLSTERY'), 'TopPad')
    return scene


def divider(name: str, mat: str) -> trimesh.Scene:
    """Lobby divider family with material-specific panels and planter mass."""
    scene = trimesh.Scene()
    width, height = 1.55, 1.72
    add(scene, box((width, 0.34, 0.10), (0, 0, 0.05), 'MAT_BLACKENED_STEEL'), 'Base')
    if 'Planter' in name:
        add(scene, box((width, 0.38, 0.46), (0, 0, 0.23), 'MAT_WOOD_WARM'), 'BasePlanter')
        for i in range(5):
            x = -width * 0.38 + i * width * 0.19
            add(scene, sphere(0.18, (x, 0, 0.56 + 0.05 * (i % 2)), 'MAT_VEGETATION', 1), f'RolePlant_{i}')
        for i in range(7):
            x = -width * 0.43 + i * width * 0.143
            add(scene, box((0.045, 0.10, height - 0.50), (x, 0, 0.50 + (height - 0.50) / 2), mat), f'FrameSlat_{i}')
    elif 'Glass' in name:
        add(scene, box((width * 0.92, 0.045, height - 0.18), (0, 0, height * 0.52), 'MAT_GLASS_CLEAR'), 'PanelGlass')
        for side, x in (('L', -width * 0.48), ('R', width * 0.48)):
            add(scene, box((0.055, 0.10, height), (x, 0, height / 2), 'MAT_BLACKENED_STEEL'), f'Frame_{side}')
    else:
        for i in range(9):
            x = -width * 0.44 + i * width * 0.11
            add(scene, box((0.055, 0.16, height), (x, 0, height / 2), mat), f'FrameSlat_{i}')
    return scene


def brochure_stand(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    add(scene, box((0.42, 0.32, 0.07), (0, 0, 0.035), 'MAT_BLACKENED_STEEL'), 'Base')
    add(scene, box((0.055, 0.055, 1.05), (0, 0.08, 0.56), 'MAT_BLACKENED_STEEL'), 'Frame')
    for i in range(3):
        z = 0.38 + i * 0.25
        add(scene, box((0.38, 0.10, 0.22), (0, -0.05, z), mat), f'Shelf_{i}')
        add(scene, box((0.30, 0.018, 0.16), (0, -0.11, z + 0.02), 'MAT_SIGNAGE'), f'BrochurePanel_{i}')
    return scene


def directory_kiosk(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    add(scene, box((0.52, 0.42, 0.08), (0, 0, 0.04), 'MAT_BLACKENED_STEEL'), 'Base')
    add(scene, box((0.22, 0.20, 0.76), (0, 0, 0.46), mat), 'FramePedestal')
    add(scene, box((0.62, 0.12, 0.88), (0, 0, 1.20), 'MAT_ELECTRONICS'), 'PanelScreen')
    add(scene, box((0.52, 0.025, 0.10), (0, -0.075, 1.53), 'MAT_EMISSIVE_WARM'), 'FunctionalHeader')
    return scene


def table(name: str, mat: str) -> trimesh.Scene:
    round_top = any(k in name for k in ('Round', 'Two Top', 'Side Table', 'Cafe Table'))
    scene = trimesh.Scene()
    h = 1.05 if 'High Top' in name else 0.76
    if round_top:
        radius = 0.42 if 'Side' in name else 0.55
        add(scene, cyl(radius, 0.07, (0, 0, h), mat, 28), 'Top')
        add(scene, cyl(0.075, h - 0.06, (0, 0, (h - 0.06) / 2), 'MAT_BLACKENED_STEEL', 16), 'Pedestal')
        add(scene, cyl(radius * 0.42, 0.06, (0, 0, 0.03), 'MAT_BLACKENED_STEEL', 20), 'Base')
    else:
        width = 1.45 if any(k in name for k in ('Four Top', 'Rectangular', 'Community', 'Banquet')) else 1.10
        depth = 0.82 if width > 1.2 else 0.68
        add(scene, box((width, depth, 0.07), (0, 0, h), mat), 'Top')
        for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
            add(scene, box((0.055, 0.055, h - 0.04), (x * width * 0.42, y * depth * 0.38, (h - 0.04) / 2), 'MAT_BLACKENED_STEEL'), f'Leg_{i}')
    return scene


def cabinet(name: str, mat: str) -> trimesh.Scene:
    w = 1.15 if any(k in name for k in ('Double', 'Bank', 'Back Bar', 'Wall')) else 0.72
    h = 1.85 if any(k in name for k in ('Wardrobe', 'Locker', 'Cabinet', 'Rack')) else 1.10
    d = 0.52
    scene = trimesh.Scene()
    add(scene, box((w, d, h), (0, 0, h / 2), mat), 'PrimaryForm')
    add(scene, box((w * 0.86, 0.03, h * 0.80), (0, -d / 2 - 0.016, h * 0.52), mat), 'DoorPanel')
    doors = 2 if w > 0.9 else 1
    for i in range(doors):
        x = (-0.22 if doors == 2 and i == 0 else 0.22 if doors == 2 else 0.0) * w
        add(scene, cyl(0.012, 0.18, (x + 0.12, -d / 2 - 0.04, h * 0.52), 'MAT_BRASS_POLISHED', 10), f'Handle_{i}')
    add(scene, box((w * 0.92, d * 0.92, 0.06), (0, 0, 0.03), 'MAT_BLACKENED_STEEL'), 'Base')
    return scene


def desk(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    width = 1.65 if any(k in name for k in ('Reception', 'Concierge', 'Business', 'Service')) else 1.25
    depth = 0.68
    h = 0.88
    add(scene, box((width, depth, 0.12), (0, 0, h), mat), 'WorkSurface')
    add(scene, box((width * 0.88, depth * 0.72, h - 0.06), (0, depth * 0.06, (h - 0.06) / 2), mat), 'PrimaryForm')
    add(scene, box((width * 0.76, 0.03, h * 0.46), (0, -depth / 2 - 0.018, h * 0.48), 'MAT_WOOD_WARM'), 'FrontPanel')
    add(scene, box((width * 0.58, 0.025, 0.055), (0, -depth / 2 - 0.02, h * 0.74), 'MAT_BRASS_POLISHED'), 'RoleDetail')
    if 'Accessible' in name:
        add(scene, box((width * 0.38, depth * 0.80, 0.07), (width * 0.27, 0, 0.72), mat), 'AccessibleWorkSurface')
    if 'Corner' in name:
        add(scene, box((0.72, 0.72, h * 0.92), (width * 0.38, depth * 0.40, h * 0.46), mat), 'CornerReturn')
    return scene


def bed(name: str, mat: str) -> trimesh.Scene:
    width = 1.0 if 'Twin' in name else 1.45 if 'Double' in name else 1.62 if 'Queen' in name else 1.82
    length = 2.02
    scene = trimesh.Scene()
    add(scene, box((width, length, 0.24), (0, 0, 0.20), mat), 'BedFrame')
    add(scene, box((width * 0.96, length * 0.96, 0.24), (0, 0, 0.43), 'MAT_LINEN'), 'Mattress')
    add(scene, box((width, 0.10, 0.92), (0, length * 0.46, 0.74), 'MAT_UPHOLSTERY'), 'Headboard')
    for i, x in enumerate((-width * 0.28, width * 0.28)):
        add(scene, box((width * 0.36, 0.36, 0.12), (x, length * 0.25, 0.61), 'MAT_LINEN'), f'Pillow_{i}')
    return scene


def bathroom_fixture(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    if 'Toilet' in name:
        add(scene, cyl(0.28, 0.38, (0, 0, 0.19), mat, 28), 'FixtureBody')
        add(scene, box((0.48, 0.22, 0.66), (0, 0.22, 0.38), mat), 'Cistern')
        add(scene, cyl(0.25, 0.055, (0, -0.02, 0.43), 'MAT_CERAMIC_FIXTURE', 28), 'Seat')
        add(scene, cyl(0.04, 0.025, (0.14, 0.10, 0.72), 'MAT_BRASS_POLISHED', 12), 'FunctionalDetail')
    elif any(k in name for k in ('Sink', 'Vanity')):
        add(scene, box((0.86, 0.52, 0.72), (0, 0, 0.36), 'MAT_WOOD_WARM'), 'VanityBase')
        add(scene, box((0.92, 0.56, 0.08), (0, 0, 0.76), 'MAT_STONE_LIGHT'), 'CounterTop')
        add(scene, cyl(0.24, 0.10, (0, 0, 0.83), mat, 28), 'Basin')
        add(scene, cyl(0.025, 0.28, (0, 0.16, 0.98), 'MAT_STAINLESS', 12), 'FunctionalDetail')
    elif 'Bathtub' in name:
        add(scene, box((1.70, 0.74, 0.52), (0, 0, 0.26), mat), 'FixtureBody')
        add(scene, box((1.48, 0.56, 0.12), (0, 0, 0.50), 'MAT_GLASS_CLEAR'), 'BasinInset')
        add(scene, cyl(0.025, 0.24, (0.58, 0.22, 0.64), 'MAT_STAINLESS', 12), 'FunctionalDetail')
    elif 'Shower' in name:
        add(scene, box((0.98, 0.98, 0.10), (0, 0, 0.05), 'MAT_CERAMIC_FIXTURE'), 'ShowerTray')
        add(scene, box((0.04, 0.98, 1.95), (-0.47, 0, 0.98), 'MAT_GLASS_CLEAR'), 'GlassPanel')
        add(scene, cyl(0.025, 1.70, (0.40, 0.38, 0.90), 'MAT_STAINLESS', 12), 'FunctionalDetail')
        add(scene, cyl(0.14, 0.04, (0.40, 0.38, 1.75), 'MAT_STAINLESS', 18), 'ShowerHead')
    else:
        return semantic_composite(name, mat)
    return scene


def cart(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    width = 0.76
    length = 1.12
    h = 0.88
    add(scene, box((width, length, 0.10), (0, 0, 0.20), mat), 'Chassis')
    add(scene, box((width * 0.90, length * 0.78, 0.54), (0, 0, 0.52), mat), 'PrimaryForm')
    add(scene, box((width * 0.74, 0.06, 0.42), (0, length * 0.48, 0.72), 'MAT_BLACKENED_STEEL'), 'Handle')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        add(scene, cyl(0.09, 0.05, (x * width * 0.40, y * length * 0.38, 0.09), 'MAT_BLACKENED_STEEL', 16), f'Wheel_{i}')
    if 'Housekeeping' in name or 'Linen' in name:
        add(scene, box((width * 0.65, length * 0.34, 0.28), (0, -length * 0.20, h), 'MAT_LINEN'), 'ServiceDetail')
    elif 'Luggage' in name or 'Bell' in name:
        add(scene, box((width * 0.85, 0.06, 1.18), (0, length * 0.42, 0.80), 'MAT_BRASS_POLISHED'), 'RoleDetail')
    else:
        add(scene, box((width * 0.65, length * 0.40, 0.08), (0, 0, h), 'MAT_STAINLESS'), 'ServiceDetail')
    return scene


def elevator_or_door(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    if 'Cab' in name:
        w, d, h = (1.85, 1.55, 2.30) if 'Service' not in name else (2.05, 1.75, 2.35)
        add(scene, box((w, d, 0.10), (0, 0, 0.05), 'MAT_STONE_LIGHT'), 'CabFloor')
        add(scene, box((w, 0.08, h), (0, d / 2 - 0.04, h / 2), mat), 'CabRear')
        for side, x in (('L', -w / 2 + 0.04), ('R', w / 2 - 0.04)):
            add(scene, box((0.08, d, h), (x, 0, h / 2), mat), f'CabSide_{side}')
        add(scene, box((w, d, 0.08), (0, 0, h - 0.04), 'MAT_STAINLESS'), 'CabCeiling')
        add(scene, box((0.20, 0.025, 0.62), (w * 0.42, -d * 0.35, 1.12), 'MAT_ELECTRONICS'), 'ControlPanel')
        add(scene, box((0.32, 0.025, 0.12), (0, d / 2 - 0.09, 2.05), 'MAT_ELECTRONICS'), 'Indicator')
        if 'Accessible' in name:
            add(scene, cyl(0.025, w * 0.72, (0, d / 2 - 0.10, 0.86), 'MAT_STAINLESS', 12), 'AccessibleHandrail')
    else:
        w = 1.30 if 'Single' in name else 1.85
        h = 2.20
        add(scene, box((w + 0.18, 0.16, h + 0.16), (0, 0, (h + 0.16) / 2), mat), 'Frame')
        if 'Elevator' in name:
            left_door, right_door = 'MOV_ElevatorDoor_L', 'MOV_ElevatorDoor_R'
        else:
            left_door, right_door = 'DoorLeft', 'DoorRight'
        add(scene, box((w * 0.48, 0.08, h), (-w * 0.25, -0.05, h / 2), mat), left_door)
        add(scene, box((w * 0.48, 0.08, h), (w * 0.25, -0.05, h / 2), mat), right_door)
        add(scene, box((0.30, 0.025, 0.12), (0, -0.10, h + 0.06), 'MAT_ELECTRONICS'), 'Indicator')
    return scene


def stair(name: str, mat: str) -> trimesh.Scene:
    """Build visibly distinct straight, L-turn and double-flight stair modules."""
    scene = trimesh.Scene()
    rise = 0.17
    run = 0.27
    width = 1.20 if 'Service' not in name else 1.05

    def y_flight(prefix: str, count: int, x: float, y0: float, z0: float, direction: float = 1.0):
        for i in range(count):
            y = y0 + direction * i * run
            z = z0 + rise / 2 + i * rise
            add(scene, box((width, run, rise), (x, y, z), mat), f'{prefix}Step_{i}')

    def x_flight(prefix: str, count: int, x0: float, y: float, z0: float):
        for i in range(count):
            x = x0 + i * run
            z = z0 + rise / 2 + i * rise
            add(scene, box((run, width, rise), (x, y, z), mat), f'{prefix}Step_{i}')

    if 'L Turn' in name:
        first = 4
        second = 4
        y_flight('Lower_', first, 0.0, 0.0, 0.0)
        landing_y = first * run
        landing_z = first * rise
        add(
            scene,
            box((width, width, 0.12), (0.0, landing_y + width * 0.35, landing_z - 0.06), mat),
            'BaseLanding',
        )
        x_flight('Upper_', second, run * 0.5, landing_y + width * 0.35, landing_z)
        top_x = second * run + run * 0.5
        add(
            scene,
            box((0.46, width, 0.12), (top_x, landing_y + width * 0.35, (first + second) * rise - 0.06), mat),
            'TopLanding',
        )
        add(
            scene,
            box((0.07, first * run + width * 0.70, first * rise), (-width * 0.48, landing_y * 0.5, first * rise * 0.5), 'MAT_BLACKENED_STEEL'),
            'FrameLower',
        )
    elif 'Double Flight' in name:
        flight = 5
        offset = width * 0.58
        y_flight('Lower_', flight, -offset, 0.0, 0.0)
        landing_y = flight * run
        landing_z = flight * rise
        add(
            scene,
            box((width * 2.18, 0.56, 0.12), (0.0, landing_y + 0.14, landing_z - 0.06), mat),
            'BaseLanding',
        )
        y_flight('Upper_', flight, offset, landing_y, landing_z, direction=-1.0)
        add(
            scene,
            box((width, 0.46, 0.12), (offset, -0.10, flight * 2 * rise - 0.06), mat),
            'TopLanding',
        )
        for side, x in (('L', -offset - width * 0.48), ('R', offset + width * 0.48)):
            add(
                scene,
                box((0.07, flight * run, flight * rise), (x, flight * run * 0.5, flight * rise * 0.5), 'MAT_BLACKENED_STEEL'),
                f'Frame_{side}',
            )
    else:
        steps = 8
        y_flight('', steps, 0.0, 0.0, 0.0)
        add(scene, box((width, 0.44, 0.12), (0, -0.18, 0.06), mat), 'BaseLanding')
        add(
            scene,
            box((width, 0.46, 0.12), (0, steps * run + 0.13, steps * rise - 0.06), mat),
            'TopLanding',
        )
        add(
            scene,
            box((0.08, steps * run, steps * rise), (-width * 0.48, steps * run / 2, steps * rise / 2), 'MAT_BLACKENED_STEEL'),
            'Frame_L',
        )
        add(
            scene,
            box((0.08, steps * run, steps * rise), (width * 0.48, steps * run / 2, steps * rise / 2), 'MAT_BLACKENED_STEEL'),
            'Frame_R',
        )

    return scene


def luggage_or_accessibility(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    if 'Wheelchair' in name:
        for side, x in (('L', -0.32), ('R', 0.32)):
            add(scene, cyl(0.31, 0.045, (x, 0, 0.36), 'MAT_BLACKENED_STEEL', 28), f'Wheel_{side}')
        add(scene, box((0.54, 0.48, 0.08), (0, 0, 0.53), mat), 'Seat')
        add(scene, box((0.54, 0.08, 0.62), (0, 0.20, 0.82), mat), 'Back')
        add(scene, box((0.62, 0.05, 0.05), (0, 0.28, 1.10), 'MAT_STAINLESS'), 'Handle')
    elif 'Walker' in name:
        for side, x in (('L', -0.28), ('R', 0.28)):
            add(scene, box((0.035, 0.52, 0.88), (x, 0, 0.44), 'MAT_STAINLESS'), f'Frame_{side}')
            add(scene, cyl(0.07, 0.035, (x, -0.23, 0.07), 'MAT_BLACKENED_STEEL', 16), f'Wheel_{side}')
        add(scene, box((0.58, 0.05, 0.05), (0, 0.24, 0.86), 'MAT_STAINLESS'), 'Handle')
    else:
        w = 0.42 if 'Carry On' in name else 0.58
        h = 0.62 if 'Duffel' in name else 0.78
        d = 0.30
        add(scene, box((w, d, h), (0, 0, h / 2 + 0.05), mat), 'PrimaryForm')
        add(scene, box((w * 0.48, 0.04, 0.05), (0, 0, h + 0.10), 'MAT_BLACKENED_STEEL'), 'Handle')
        for i, x in enumerate((-w * 0.32, w * 0.32)):
            add(scene, cyl(0.05, 0.035, (x, 0, 0.05), 'MAT_BLACKENED_STEEL', 14), f'Wheel_{i}')
        add(scene, box((w * 0.72, 0.025, h * 0.20), (0, -d / 2 - 0.014, h * 0.60), 'MAT_SIGNAGE'), 'RoleDetail')
    return scene


def write_generated_asset(
    scene: trimesh.Scene,
    row: list,
    family: str,
    batch: int,
    output_dir: Path,
    source_path: str,
) -> Path:
    asset_id, name, subcategory, mat, profile, animation_set, anchors = row
    output_dir.mkdir(parents=True, exist_ok=True)
    path = output_dir / f'{asset_id}.glb'
    path.write_bytes(scene.export(file_type='glb'))
    lod, collision, cutaway = PROFILE_DEFAULTS[profile]
    dependencies: list[str] = []
    if profile == 'P_CHARACTER':
        skeleton = 'SK_HumanoidSmall' if 'Child' in name else 'SK_HumanoidAdult'
        dependencies.append(skeleton)
    if animation_set:
        dependencies.append(animation_set)
    geometry_names = sorted(str(node) for node in scene.graph.nodes_geometry)
    sidecar = {
        'schema': 1,
        'asset_id': asset_id,
        'asset_type': PROFILE_ASSET_TYPES[profile],
        'source': source_path,
        'units': 'meters',
        'lod_policy': lod,
        'collision_policy': collision,
        'cutaway_policy': cutaway,
        'material_slots': [mat],
        'interaction_anchors': list(anchors),
        'tags': [
            f'batch_{batch:02d}',
            'architectural_diorama_realism',
            'asset_quality_v2',
            family,
            subcategory,
            *[f'interaction_anchor:{anchor}' for anchor in anchors],
        ],
        'dependencies': dependencies,
        'milestone': f'batch_{batch:02d}',
        'source_revision': 2,
        'metadata_revision': 2,
        'quality_revision': 2,
        'quality_contract': 'HH_ASSET_QUALITY_V2',
        'quality_family': family,
        'quality_profile': profile,
        'semantic_nodes': geometry_names,
        'authoring_provenance': {
            'provider': 'hotel_haven_procedural',
            'source': source_path,
            'quality_review_state': 'GENERATED_FOR_V2_QA',
        },
    }
    path.with_suffix('.asset.json').write_text(json.dumps(sidecar, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return path


def generate_manifest_batch(
    manifest_path: Path | str,
    output_dir: Path | str,
    source_path: str,
    build: Callable[[str, str, str, str, str], trimesh.Scene],
) -> list[Path]:
    manifest_path = Path(manifest_path)
    output_dir = Path(output_dir)
    data = json.loads(manifest_path.read_text(encoding='utf-8'))
    batch = int(data['batch'])
    out: list[Path] = []
    for group in data['groups']:
        family = str(group['family'])
        for row in group['assets']:
            asset_id, name, subcategory, mat, profile, _animation_set, _anchors = row
            scene = build(name, subcategory, mat, asset_id, profile)
            if len(scene.geometry) < 2 or set(scene.graph.nodes_geometry) == {'Body'}:
                scene = semantic_composite(name, mat, asset_id)
            out.append(write_generated_asset(scene, row, family, batch, output_dir, source_path))
    return out
