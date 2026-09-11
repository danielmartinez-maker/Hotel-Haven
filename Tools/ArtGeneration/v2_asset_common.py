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
    for side, x in (('L', -width / 2 - 0.05), ('R', width / 2 + 0.05)):
        add(scene, box((0.10, 0.68, 0.34), (x, 0, 0.58), mat), f'Arm_{side}')
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
        add(scene, box((w * 0.48, 0.08, h), (-w * 0.25, -0.05, h / 2), mat), 'DoorLeft')
        add(scene, box((w * 0.48, 0.08, h), (w * 0.25, -0.05, h / 2), mat), 'DoorRight')
        add(scene, box((0.30, 0.025, 0.12), (0, -0.10, h + 0.06), 'MAT_ELECTRONICS'), 'Indicator')
    return scene


def stair(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    steps = 8
    width = 1.20 if 'Service' not in name else 1.05
    for i in range(steps):
        rise = 0.17
        run = 0.27
        add(scene, box((width, run, rise), (0, i * run, rise / 2 + i * rise), mat), f'Step_{i}')
    add(scene, box((0.08, steps * 0.27, steps * 0.17), (-width * 0.48, steps * 0.27 / 2, steps * 0.17 / 2), 'MAT_BLACKENED_STEEL'), 'Stringer_L')
    add(scene, box((0.08, steps * 0.27, steps * 0.17), (width * 0.48, steps * 0.27 / 2, steps * 0.17 / 2), 'MAT_BLACKENED_STEEL'), 'Stringer_R')
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
