from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import trimesh

PALETTE = {
    'MAT_PLASTER_WARM': ((0.72, 0.66, 0.56, 1.0), 0.0, 0.80),
    'MAT_PLASTER_COOL': ((0.42, 0.48, 0.54, 1.0), 0.0, 0.82),
    'MAT_WOOD_WARM': ((0.42, 0.22, 0.10, 1.0), 0.0, 0.52),
    'MAT_WOOD_DARK': ((0.22, 0.12, 0.08, 1.0), 0.0, 0.48),
    'MAT_STONE_LIGHT': ((0.60, 0.56, 0.49, 1.0), 0.0, 0.70),
    'MAT_STONE_DARK': ((0.25, 0.26, 0.27, 1.0), 0.0, 0.68),
    'MAT_MARBLE_LIGHT': ((0.88, 0.84, 0.76, 1.0), 0.0, 0.38),
    'MAT_MARBLE_DARK': ((0.20, 0.18, 0.17, 1.0), 0.0, 0.34),
    'MAT_TILE_CERAMIC': ((0.76, 0.78, 0.74, 1.0), 0.0, 0.44),
    'MAT_CARPET_STANDARD': ((0.53, 0.47, 0.39, 1.0), 0.0, 0.90),
    'MAT_CARPET_LUXURY': ((0.22, 0.25, 0.29, 1.0), 0.0, 0.84),
    'MAT_UPHOLSTERY': ((0.42, 0.31, 0.25, 1.0), 0.0, 0.84),
    'MAT_GLASS_CLEAR': ((0.34, 0.55, 0.64, 0.42), 0.0, 0.10),
    'MAT_STAINLESS': ((0.58, 0.61, 0.62, 1.0), 0.85, 0.42),
    'MAT_SERVICE_PAINT': ((0.24, 0.30, 0.34, 1.0), 0.0, 0.68),
    'MAT_BLACKENED_STEEL': ((0.10, 0.12, 0.13, 1.0), 0.78, 0.50),
    'MAT_BRASS_POLISHED': ((0.62, 0.38, 0.10, 1.0), 0.88, 0.30),
    'MAT_CONCRETE': ((0.48, 0.49, 0.47, 1.0), 0.0, 0.74),
}

# Name-specific colorways preserve the shared material-family response while
# keeping finishes readable at Hotel Haven's canonical management camera.
FINISH_RGBA = {
    'Carpet Tile Warm Beige': (0.60, 0.52, 0.41, 1.0),
    'Carpet Tile Charcoal': (0.20, 0.22, 0.23, 1.0),
    'Carpet Tile Muted Navy': (0.20, 0.27, 0.35, 1.0),
    'Luxury Carpet Burgundy': (0.34, 0.10, 0.13, 1.0),
    'Luxury Carpet Deep Green': (0.12, 0.24, 0.18, 1.0),
    'Luxury Carpet Navy Gold': (0.15, 0.20, 0.30, 1.0),
    'Oak Plank Flooring': (0.53, 0.31, 0.15, 1.0),
    'Walnut Plank Flooring': (0.31, 0.16, 0.08, 1.0),
    'Chevron Parquet Flooring': (0.46, 0.26, 0.12, 1.0),
    'Herringbone Parquet Flooring': (0.38, 0.20, 0.10, 1.0),
    'Light Marble Flooring': (0.84, 0.81, 0.74, 1.0),
    'Dark Marble Flooring': (0.18, 0.18, 0.19, 1.0),
    'Travertine Flooring': (0.66, 0.58, 0.46, 1.0),
    'Terracotta Tile Flooring': (0.58, 0.25, 0.15, 1.0),
    'Grey Stone Flooring': (0.42, 0.43, 0.42, 1.0),
    'White Bathroom Tile': (0.87, 0.88, 0.86, 1.0),
    'Blue Bathroom Tile': (0.25, 0.43, 0.55, 1.0),
    'Black White Mosaic Tile': (0.71, 0.71, 0.68, 1.0),
    'Service Kitchen Tile': (0.57, 0.60, 0.59, 1.0),
    'Pool Spa Mosaic Tile': (0.18, 0.50, 0.57, 1.0),
    'Paint Finish Warm Ivory': (0.83, 0.78, 0.66, 1.0),
    'Paint Finish Soft Grey': (0.62, 0.64, 0.64, 1.0),
    'Paint Finish Muted Blue': (0.35, 0.46, 0.56, 1.0),
    'Paint Finish Olive': (0.43, 0.46, 0.28, 1.0),
    'Paint Finish Burgundy Accent': (0.42, 0.15, 0.18, 1.0),
    'Wood Panel Walnut': (0.30, 0.15, 0.08, 1.0),
    'Wood Panel Oak': (0.52, 0.30, 0.14, 1.0),
    'Wood Panel Dark Wenge': (0.15, 0.08, 0.06, 1.0),
    'Stone Wall Cladding Light': (0.63, 0.58, 0.49, 1.0),
    'Stone Wall Cladding Dark': (0.30, 0.31, 0.30, 1.0),
}


def material(name: str, rgba_override=None):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_PLASTER_WARM'])
    if rgba_override is not None:
        rgba = rgba_override
    base_color = np.clip(np.rint(np.asarray(rgba, dtype=float) * 255.0), 0, 255).astype(np.uint8)
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=base_color,
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def box(extents, center=(0, 0, 0), mat='MAT_PLASTER_WARM', rgba_override=None):
    m = trimesh.creation.box(extents=extents)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=material(mat, rgba_override))
    return m


def cyl(radius, height, center=(0, 0, 0), mat='MAT_STAINLESS', sections=16):
    m = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return m


def add(scene, mesh, name, *, parent=None, transform=None):
    kwargs = {}
    if parent is not None:
        kwargs['parent_node_name'] = parent
    if transform is not None:
        kwargs['transform'] = transform
    scene.add_geometry(mesh, node_name=name, geom_name=name, **kwargs)


def frame(scene, width, height, depth, mat='MAT_WOOD_DARK', prefix='Frame'):
    jamb = 0.10
    add(scene, box((jamb, depth, height), (-width / 2 + jamb / 2, 0, height / 2), mat), f'{prefix}_L')
    add(scene, box((jamb, depth, height), (width / 2 - jamb / 2, 0, height / 2), mat), f'{prefix}_R')
    add(scene, box((width, depth, jamb), (0, 0, height - jamb / 2), mat), f'{prefix}_Top')


def make_hatch(mat):
    s = trimesh.Scene()
    add(s, box((1.20, 0.10, 1.35), (0, 0, 0.675), 'MAT_CONCRETE'), 'WallPatch')
    add(s, box((0.82, 0.045, 0.82), (0, -0.075, 0.72), mat), 'HatchPanel')
    for x in (-0.45, 0.45):
        add(s, box((0.06, 0.08, 0.96), (x, -0.055, 0.72), 'MAT_STAINLESS'), f'HatchFrame_{x}')
    add(s, cyl(0.022, 0.10, (0.28, -0.13, 0.72), 'MAT_STAINLESS', 12), 'Latch')
    return s


def make_door(name, mat):
    s = trimesh.Scene()
    w, h, d = 1.08, 2.25, 0.18
    if 'Double Service' in name:
        w = 1.75
    if 'Loading Dock' in name:
        w, h = 2.45, 2.55
    if 'Lobby Automatic' in name:
        w = 2.15
    if 'Accessible' in name:
        w = 1.18
    if 'Pocket' in name:
        w = 1.20

    frame_mat = 'MAT_STAINLESS' if 'Security' in name or 'Service' in name or 'Loading' in name else 'MAT_WOOD_DARK'
    frame(s, w, h, d, frame_mat, 'DoorFrame')

    if 'Lobby Automatic' in name:
        for i, x in enumerate((-0.52, 0.52)):
            add(s, box((0.95, 0.035, 2.00), (x, -0.02, 1.02), 'MAT_GLASS_CLEAR'), f'MOV_SlidingPanel_{i}')
        add(s, box((0.32, 0.10, 0.12), (0, -0.02, 2.20), 'MAT_SERVICE_PAINT'), 'MotionSensor')
        add(s, box((2.00, 0.12, 0.035), (0, 0.0, 0.018), 'MAT_STAINLESS'), 'FloorGuide')
        return s

    if 'Pocket' in name:
        add(s, box((w - 0.20, 0.06, 2.00), (-0.10, -0.02, 1.02), mat), 'MOV_PocketPanel')
        add(s, box((0.42, 0.12, 2.10), (w / 2 + 0.12, 0, 1.05), 'MAT_PLASTER_WARM'), 'PocketWallSleeve')
        add(s, box((0.055, 0.025, 0.24), (0.30, -0.065, 1.05), 'MAT_BRASS_POLISHED'), 'PocketPull', parent='MOV_PocketPanel')
        return s

    if 'Loading Dock' in name:
        for i in range(5):
            add(s, box((w - 0.18, 0.055, 0.44), (0, -0.02, 0.26 + i * 0.46), mat), f'MOV_DockPanel_{i}')
        rail_x = w / 2 - 0.10
        add(s, box((0.07, 0.12, 2.42), (-rail_x, 0.0, 1.21), 'MAT_BLACKENED_STEEL'), 'GuideRail_Left')
        add(s, box((0.07, 0.12, 2.42), (rail_x, 0.0, 1.21), 'MAT_BLACKENED_STEEL'), 'GuideRail_Right')
        add(s, box((0.42, 0.08, 0.06), (0, -0.08, 0.22), 'MAT_STAINLESS'), 'LiftHandle', parent='MOV_DockPanel_0')
        return s

    if 'Double' in name:
        leaf_w = (w - 0.20) / 2.0
        left_hinge = -w / 2 + 0.10
        right_hinge = w / 2 - 0.10
        left_transform = trimesh.transformations.translation_matrix((left_hinge, 0.0, 0.0))
        right_transform = trimesh.transformations.translation_matrix((right_hinge, 0.0, 0.0))
        add(s, box((leaf_w, 0.07, 2.00), (leaf_w / 2, -0.02, 1.02), mat), 'MOV_DoorLeaf_0', transform=left_transform)
        add(s, box((leaf_w, 0.07, 2.00), (-leaf_w / 2, -0.02, 1.02), mat), 'MOV_DoorLeaf_1', transform=right_transform)
        add(s, box((0.06, 0.045, 0.30), (leaf_w * 0.72, -0.075, 1.04), 'MAT_STAINLESS'), 'Handle_Left', parent='MOV_DoorLeaf_0')
        add(s, box((0.06, 0.045, 0.30), (-leaf_w * 0.72, -0.075, 1.04), 'MAT_STAINLESS'), 'Handle_Right', parent='MOV_DoorLeaf_1')
        return s

    leaf_w = w - 0.22
    hinge_x = -w / 2 + 0.10
    leaf_transform = trimesh.transformations.translation_matrix((hinge_x, 0.0, 0.0))
    add(s, box((leaf_w, 0.07, 2.00), (leaf_w / 2, -0.02, 1.02), mat), 'MOV_DoorLeaf', transform=leaf_transform)

    handle_mat = 'MAT_STAINLESS' if 'Security' in name else 'MAT_BRASS_POLISHED'
    if 'Accessible' in name or 'Security' in name or 'Balcony' in name:
        add(s, box((0.28, 0.025, 0.10), (leaf_w * 0.76, -0.08, 1.05), handle_mat), 'LeverHandle', parent='MOV_DoorLeaf')
    if 'Security' in name:
        add(s, box((0.20, 0.02, 0.28), (leaf_w / 2, -0.075, 1.50), 'MAT_GLASS_CLEAR'), 'VisionPanel', parent='MOV_DoorLeaf')
        add(s, box((leaf_w - 0.06, 0.02, 0.18), (leaf_w / 2, -0.08, 0.24), 'MAT_STAINLESS'), 'KickPlate', parent='MOV_DoorLeaf')
    return s


def make_window(name):
    s = trimesh.Scene()
    if 'Corner Glass' in name:
        add(s, box((1.05, 0.035, 1.65), (-0.50, 0, 0.90), 'MAT_GLASS_CLEAR'), 'GlassA')
        add(s, box((0.035, 1.05, 1.65), (0, 0.50, 0.90), 'MAT_GLASS_CLEAR'), 'GlassB')
        add(s, box((0.06, 0.06, 1.85), (0, 0, 0.925), 'MAT_STAINLESS'), 'CornerMullion')
        return s
    w, h = 1.45, 1.65
    frame(s, w, h, 0.10, 'MAT_STONE_LIGHT', 'WindowFrame')
    if 'Double Casement' in name:
        for i, x in enumerate((-0.34, 0.34)):
            add(s, box((0.62, 0.02, 1.42), (x, 0, 0.73), 'MAT_GLASS_CLEAR'), f'Casement_{i}')
        add(s, box((0.05, 0.07, 1.44), (0, 0, 0.74), 'MAT_STAINLESS'), 'Mullion')
    elif 'Arched' in name:
        add(s, box((w - 0.20, 0.02, 1.28), (0, 0, 0.64), 'MAT_GLASS_CLEAR'), 'GlassMain')
        for i, x in enumerate(np.linspace(-0.52, 0.52, 7)):
            z = 1.28 + 0.30 * (1 - (x / 0.60) ** 2)
            add(s, box((0.17, 0.02, 0.20), (x, 0, z), 'MAT_GLASS_CLEAR'), f'GlassArch_{i}')
    return s


def make_balustrade():
    s = trimesh.Scene()
    add(s, box((1.80, 0.035, 0.84), (0, 0, 0.48), 'MAT_GLASS_CLEAR'), 'GlassPanel')
    for x in (-0.88, 0.0, 0.88):
        add(s, cyl(0.025, 1.05, (x, 0, 0.525), 'MAT_STAINLESS', 12), f'Post_{x}')
    add(s, box((1.88, 0.06, 0.055), (0, 0, 1.05), 'MAT_STAINLESS'), 'Handrail')
    return s


def make_awning():
    s = trimesh.Scene()
    canopy = box((1.70, 1.10, 0.08), (0, 0.42, 1.88), 'MAT_UPHOLSTERY')
    canopy.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(18), [1, 0, 0], [0, 0.42, 1.88]))
    add(s, canopy, 'AwningFabric')
    for x in (-0.70, 0.70):
        add(s, cyl(0.022, 1.18, (x, 0.48, 1.30), 'MAT_BLACKENED_STEEL', 10), f'Support_{x}')
    return s


def make_pillar(name, mat):
    s = trimesh.Scene()
    if 'Pilaster' in name:
        add(s, box((0.42, 0.18, 2.70), (0, 0, 1.35), mat), 'Pilaster')
        add(s, box((0.54, 0.24, 0.12), (0, 0, 2.72), mat), 'Capital')
    else:
        add(s, box((0.48, 0.48, 2.70), (0, 0, 1.35), mat), 'Pillar')
        add(s, box((0.62, 0.62, 0.14), (0, 0, 2.74), mat), 'Capital')
        add(s, box((0.62, 0.62, 0.12), (0, 0, 0.06), mat), 'Base')
    return s


def make_drain(name):
    s = trimesh.Scene()
    if 'Roof Drain' in name:
        add(s, cyl(0.055, 2.50, (0, 0, 1.25), 'MAT_STAINLESS', 14), 'Downpipe')
        add(s, cyl(0.09, 0.18, (0, 0, 2.52), 'MAT_STAINLESS', 14), 'Collector')
    else:
        add(s, box((1.80, 0.18, 0.16), (0, 0, 0.08), 'MAT_STAINLESS'), 'GutterChannel')
        for x in (-0.70, 0, 0.70):
            add(s, box((0.05, 0.32, 0.18), (x, 0, 0.10), 'MAT_STAINLESS'), f'Bracket_{x}')
    return s


def make_louver_or_cover(name, mat):
    s = trimesh.Scene()
    add(s, box((1.10, 0.10, 1.20), (0, 0, 0.60), 'MAT_CONCRETE'), 'Backing')
    if 'Louver' in name:
        for i, z in enumerate(np.linspace(0.22, 1.02, 7)):
            blade = box((0.90, 0.12, 0.055), (0, -0.08, z), mat)
            blade.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-22), [1, 0, 0], [0, -0.08, z]))
            add(s, blade, f'LouverBlade_{i}')
    else:
        add(s, box((0.86, 0.055, 0.90), (0, -0.08, 0.62), mat), 'CoverPanel')
        for x in (-0.36, 0.36):
            for z in (0.25, 0.98):
                add(s, cyl(0.018, 0.06, (x, -0.12, z), 'MAT_STAINLESS', 10), f'Fastener_{x}_{z}')
    return s


def make_fire_escape():
    s = trimesh.Scene()
    add(s, box((1.80, 1.20, 0.10), (0, 0, 0.05), 'MAT_BLACKENED_STEEL'), 'Landing')
    for x in (-0.85, 0.85):
        for y in (-0.55, 0.55):
            add(s, cyl(0.022, 1.05, (x, y, 0.525), 'MAT_BLACKENED_STEEL', 10), f'Post_{x}_{y}')
    add(s, box((1.75, 0.04, 0.04), (0, -0.55, 1.05), 'MAT_BLACKENED_STEEL'), 'RailFront')
    add(s, box((1.75, 0.04, 0.04), (0, 0.55, 1.05), 'MAT_BLACKENED_STEEL'), 'RailBack')
    return s


def make_finish(name, mat):
    s = trimesh.Scene()
    variant = FINISH_RGBA.get(name)
    wall_like = 'Paint Finish' in name or 'Wood Panel' in name or 'Wall Cladding' in name
    if wall_like:
        add(s, box((1.0, 0.08, 2.80), (0, 0, 1.40), mat, variant), 'FinishWall')
        if 'Wood Panel' in name:
            for x in (-0.34, 0, 0.34):
                add(s, box((0.025, 0.018, 2.62), (x, -0.05, 1.40), 'MAT_WOOD_DARK'), f'PanelJoint_{x}')
        elif 'Stone Wall Cladding' in name:
            for z in np.linspace(0.35, 2.45, 4):
                add(s, box((0.96, 0.015, 0.025), (0, -0.05, z), 'MAT_PLASTER_COOL'), f'StoneJointH_{z:.2f}')
        return s

    add(s, box((1.0, 1.0, 0.055), (0, 0, 0.0275), mat, variant), 'FinishTile')
    if 'Carpet' in name:
        if 'Luxury' in name or mat == 'MAT_CARPET_LUXURY':
            add(s, box((0.88, 0.88, 0.012), (0, 0, 0.061), mat, variant), 'PileInset')
            if 'Navy Gold' in name:
                add(s, box((0.72, 0.035, 0.008), (0, 0, 0.070), 'MAT_BRASS_POLISHED'), 'AccentBand')
    elif 'Plank' in name:
        for i, y in enumerate(np.linspace(-0.42, 0.42, 6)):
            add(s, box((0.92, 0.012, 0.008), (0, y, 0.061), 'MAT_WOOD_DARK'), f'PlankJoint_{i}')
    elif 'Parquet' in name:
        for i, x in enumerate(np.linspace(-0.36, 0.36, 5)):
            strip = box((0.14, 0.52, 0.008), (x, 0, 0.061), 'MAT_WOOD_DARK')
            strip.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(45 if i % 2 == 0 else -45), [0, 0, 1], [x, 0, 0.061]))
            add(s, strip, f'ParquetStrip_{i}')
    elif 'Tile' in name or 'Mosaic' in name:
        n = 8 if 'Mosaic' in name else 5
        for x in np.linspace(-0.4, 0.4, n):
            add(s, box((0.012, 0.92, 0.008), (x, 0, 0.061), 'MAT_PLASTER_COOL'), f'GroutV_{x:.2f}')
        for y in np.linspace(-0.4, 0.4, n):
            add(s, box((0.92, 0.012, 0.008), (0, y, 0.061), 'MAT_PLASTER_COOL'), f'GroutH_{y:.2f}')
    elif 'Marble' in name:
        for i, x in enumerate((-0.28, 0.16, 0.36)):
            vein = box((0.018, 0.84, 0.006), (x, 0, 0.061), 'MAT_PLASTER_COOL')
            vein.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(18 + i * 11), [0, 0, 1], [x, 0, 0.061]))
            add(s, vein, f'Vein_{i}')
    return s


def build(name, mat):
    if name == 'Service Hatch':
        return make_hatch(mat)
    if 'Door' in name:
        return make_door(name, mat)
    if 'Window' in name:
        return make_window(name)
    if 'Balustrade' in name:
        return make_balustrade()
    if 'Awning' in name:
        return make_awning()
    if 'Pillar' in name or 'Pilaster' in name:
        return make_pillar(name, mat)
    if 'Drain' in name or 'Gutter' in name:
        return make_drain(name)
    if 'Shaft Cover' in name or 'Louver' in name:
        return make_louver_or_cover(name, mat)
    if 'Fire Escape' in name:
        return make_fire_escape()
    return make_finish(name, mat)


def sidecar(asset_id, subcat, mat, animset):
    tags = ['hotel-haven', 'batch_02', subcat]
    if animset:
        tags.extend(['animated-binding', animset])
    return {
        'schema': 1,
        'asset_id': asset_id,
        'asset_type': 'StaticMeshAsset',
        'source': 'Tools/ArtGeneration/batch02_generate.py',
        'units': 'meters',
        'lod_policy': 'lod_architecture',
        'collision_policy': 'simple_proxy',
        'material_slots': [mat],
        'tags': tags,
        'dependencies': [animset] if animset else [],
        'cutaway_policy': 'normal',
        'source_revision': 2,
        'metadata_revision': 2,
        'cooker_schema': 1,
        'lifecycle_state': 'PRODUCTION',
    }


def generate(manifest_path: Path, output: Path, package: bool):
    data = json.loads(manifest_path.read_text())
    rows = [a for g in data['groups'] for a in g['assets']]
    if len(rows) != 50:
        raise ValueError(f'Batch 02 must contain 50 assets; got {len(rows)}')
    output.mkdir(parents=True, exist_ok=True)
    outputs = []
    for row in rows:
        asset_id, name, subcat, mat, profile, animset, anchors = row
        scene = build(name, mat)
        path = output / f'{asset_id}.glb'
        if package:
            path.write_bytes(scene.export(file_type='glb'))
            (output / f'{asset_id}.asset.json').write_text(json.dumps(sidecar(asset_id, subcat, mat, animset), indent=2) + '\n')
        else:
            scene.export(path)
        outputs.append(path)
        print(path)
    return outputs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('manifest', type=Path)
    ap.add_argument('output', type=Path)
    ap.add_argument('--package', action='store_true')
    args = ap.parse_args()
    generate(args.manifest, args.output, args.package)


if __name__ == '__main__':
    main()
