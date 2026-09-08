from __future__ import annotations

import json
from pathlib import Path
import re

import numpy as np
import trimesh

ASSET_ID_RE = re.compile(r'^HH_A\d{3}$')
PROFILE_ASSET_TYPES = {
    'P_ARCH_STATIC': 'StaticMeshAsset',
    'P_ARCH_ANIMATED': 'SkinnedMeshAsset',
}


def _validated_entries(data: dict) -> list[list]:
    try:
        entries = [row for group in data['groups'] for row in group['assets']]
    except (KeyError, TypeError) as exc:
        raise ValueError('manifest must contain groups with asset rows') from exc
    if len(entries) != 50:
        raise ValueError(f'Batch 01 must contain 50 assets; got {len(entries)}')
    seen: set[str] = set()
    for row in entries:
        if not isinstance(row, list) or len(row) != 7:
            raise ValueError(f'invalid asset row: {row!r}')
        asset_id = row[0]
        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:
            raise ValueError(f'invalid asset_id: {asset_id!r}')
        if asset_id in seen:
            raise ValueError(f'duplicate asset_id: {asset_id}')
        seen.add(asset_id)
        material_family = row[3]
        if material_family not in PALETTE:
            raise ValueError(f'unknown material family: {material_family!r}')
        profile = row[4]
        if profile not in PROFILE_ASSET_TYPES:
            raise ValueError(f'unsupported profile for batch 01: {profile!r}')
        animation_set = row[5]
        if profile == 'P_ARCH_ANIMATED':
            if not isinstance(animation_set, str) or not animation_set:
                raise ValueError(f'animated profile requires animation_set: {asset_id}')
        elif animation_set is not None:
            raise ValueError(f'static profile must not define animation_set: {asset_id}')
    return entries


PALETTE = {
    'MAT_MARBLE_LIGHT': ((0.88, 0.84, 0.76, 1.0), 0.0, 0.38),
    'MAT_PLASTER_WARM': ((0.72, 0.66, 0.56, 1.0), 0.0, 0.80),
    'MAT_PLASTER_COOL': ((0.42, 0.48, 0.54, 1.0), 0.0, 0.82),
    'MAT_TILE_CERAMIC': ((0.76, 0.78, 0.74, 1.0), 0.0, 0.44),
    'MAT_GLASS_CLEAR': ((0.34, 0.55, 0.64, 0.42), 0.0, 0.10),
    'MAT_STONE_LIGHT': ((0.60, 0.56, 0.49, 1.0), 0.0, 0.70),
    'MAT_BRICK': ((0.49, 0.24, 0.16, 1.0), 0.0, 0.76),
    'MAT_WOOD_WARM': ((0.42, 0.22, 0.10, 1.0), 0.0, 0.52),
    'MAT_WOOD_DARK': ((0.22, 0.12, 0.08, 1.0), 0.0, 0.48),
    'MAT_SERVICE_PAINT': ((0.24, 0.30, 0.34, 1.0), 0.0, 0.68),
    'MAT_STAINLESS': ((0.58, 0.61, 0.62, 1.0), 0.85, 0.42),
    'MAT_BLACKENED_STEEL': ((0.10, 0.12, 0.13, 1.0), 0.78, 0.50),
    'MAT_SIGNAGE': ((0.16, 0.17, 0.17, 1.0), 0.25, 0.44),
    'MAT_CONCRETE': ((0.48, 0.49, 0.47, 1.0), 0.0, 0.74),
    'MAT_BRASS_POLISHED': ((0.62, 0.38, 0.10, 1.0), 0.88, 0.30),
}


def _material(name: str):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_PLASTER_WARM'])
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=np.array(rgba) * 255,
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def _box(extents, center=(0, 0, 0), material='MAT_PLASTER_WARM'):
    mesh = trimesh.creation.box(extents=extents)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=_material(material))
    return mesh


def _cylinder(radius, height, center=(0, 0, 0), material='MAT_STONE_LIGHT', sections=20):
    mesh = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=_material(material))
    return mesh


def _add(scene: trimesh.Scene, mesh, name: str):
    scene.add_geometry(mesh, node_name=name, geom_name=name)


def _frame(scene, width, height, depth, material, prefix='Frame'):
    jamb = 0.10
    _add(scene, _box((jamb, depth, height), (-width / 2 + jamb / 2, 0, height / 2), material), f'{prefix}_L')
    _add(scene, _box((jamb, depth, height), (width / 2 - jamb / 2, 0, height / 2), material), f'{prefix}_R')
    _add(scene, _box((width, depth, jamb), (0, 0, height - jamb / 2), material), f'{prefix}_Top')


def _make_floor(name, material):
    s = trimesh.Scene()
    thickness = 0.10 if 'Slab' not in name else 0.22
    _add(s, _box((1.0, 1.0, thickness), (0, 0, thickness / 2), material), 'FloorBody')
    if 'Raised Service' in name:
        for x in (-0.36, 0.36):
            for y in (-0.36, 0.36):
                _add(s, _cylinder(0.025, 0.18, (x, y, -0.09), 'MAT_STAINLESS', 12), f'Pedestal_{x}_{y}')
    if 'Inlay' in name:
        _add(s, _box((0.56, 0.56, 0.012), (0, 0, thickness + 0.006), 'MAT_SIGNAGE'), 'Inlay')
    return s


def _make_wall(name, material):
    s = trimesh.Scene()
    _add(s, _box((1.0, 0.12, 2.8), (0, 0, 1.4), material), 'WallBody')
    if 'Tile' in name:
        for z in np.linspace(0.25, 2.55, 7):
            _add(s, _box((0.98, 0.008, 0.012), (0, -0.064, z), 'MAT_PLASTER_COOL'), f'GroutH_{z:.2f}')
    if 'Glass Partition' in name:
        s = trimesh.Scene()
        _frame(s, 1.0, 2.8, 0.08, 'MAT_STAINLESS', 'GlassFrame')
        _add(s, _box((0.78, 0.018, 2.52), (0, 0, 1.30), 'MAT_GLASS_CLEAR'), 'GlassPane')
    if 'Half Wall' in name:
        s = trimesh.Scene()
        _add(s, _box((1.0, 0.16, 1.15), (0, 0, 0.575), material), 'HalfWall')
        _add(s, _box((1.06, 0.20, 0.06), (0, 0, 1.18), 'MAT_WOOD_WARM'), 'Cap')
    if 'Wood Wall Panel Trim' in name:
        s = trimesh.Scene()
        _add(s, _box((1.0, 0.08, 2.8), (0, 0, 1.4), material), 'PanelBase')
        for x in (-0.33, 0, 0.33):
            _add(s, _box((0.035, 0.02, 2.55), (x, -0.05, 1.38), 'MAT_WOOD_DARK'), f'Mullion_{x}')
    return s


def _make_door(name, material):
    s = trimesh.Scene()
    w, h, d = (1.12, 2.25, 0.18)
    if 'Suite Double' in name or 'Double' in name:
        w = 1.72
    if 'Sliding Glass' in name:
        w = 1.80
    if 'Revolving' in name:
        _add(s, _cylinder(0.82, 0.10, (0, 0, 0.05), 'MAT_BRASS_POLISHED', 32), 'BaseRing')
        _add(s, _cylinder(0.82, 0.08, (0, 0, 2.30), 'MAT_BRASS_POLISHED', 32), 'TopRing')
        for i in range(2):
            pane = _box((1.45 if i == 0 else 0.04, 0.04 if i == 0 else 1.45, 2.12), (0, 0, 1.18), 'MAT_GLASS_CLEAR')
            _add(s, pane, f'MOV_RevolvingLeaf_{i}')
        return s
    _frame(s, w, h, d, 'MAT_WOOD_DARK' if 'Guest Room' in name or 'Deluxe' in name or 'Suite' in name else material, 'DoorFrame')
    if 'Sliding Glass' in name:
        for i, x in enumerate((-0.43, 0.43)):
            _add(s, _box((0.82, 0.035, 2.02), (x, 0, 1.02), 'MAT_GLASS_CLEAR'), f'MOV_SlidingPanel_{i}')
        return s
    panels = 2 if 'Double' in name else 1
    if panels == 2:
        for i, x in enumerate((-0.41, 0.41)):
            _add(s, _box((0.75, 0.07, 2.02), (x, -0.02, 1.02), material), f'MOV_DoorLeaf_{i}')
    else:
        _add(s, _box((w - 0.22, 0.07, 2.02), (0, -0.02, 1.02), material), 'MOV_DoorLeaf')
    _add(s, _cylinder(0.024, 0.12, (w * 0.30, -0.08, 1.05), 'MAT_BRASS_POLISHED', 12), 'Handle')
    return s


def _make_window(name, material):
    s = trimesh.Scene()
    w, h = 1.2, 1.55
    if 'Tall' in name:
        h = 2.10
    if 'Narrow' in name:
        w = 0.75
    if 'Bay' in name:
        _add(s, _box((0.58, 0.07, 1.50), (-0.48, 0.10, 0.90), 'MAT_GLASS_CLEAR'), 'BayGlassL')
        _add(s, _box((0.72, 0.07, 1.50), (0, 0.22, 0.90), 'MAT_GLASS_CLEAR'), 'BayGlassC')
        _add(s, _box((0.58, 0.07, 1.50), (0.48, 0.10, 0.90), 'MAT_GLASS_CLEAR'), 'BayGlassR')
        for x in (-0.78, -0.2, 0.2, 0.78):
            _add(s, _box((0.07, 0.12, 1.65), (x, 0.1, 0.92), 'MAT_STONE_LIGHT'), f'BayFrame_{x}')
        return s
    _frame(s, w, h, 0.10, 'MAT_STONE_LIGHT', 'WindowFrame')
    _add(s, _box((w - 0.20, 0.02, h - 0.20), (0, 0, (h - 0.20) / 2), 'MAT_GLASS_CLEAR'), 'GlassPane')
    if 'Awning' in name:
        _add(s, _box((w - 0.25, 0.03, 0.06), (0, -0.22, h - 0.18), 'MAT_STAINLESS'), 'AwningBar')
    return s


def _make_elevator(name):
    s = trimesh.Scene()
    w, h = 1.62, 2.45
    _frame(s, w, h, 0.20, 'MAT_STONE_LIGHT' if 'Guest' in name else 'MAT_SERVICE_PAINT', 'ElevatorFrame')
    open_state = 'Open' in name
    xs = (-0.61, 0.61) if open_state else (-0.39, 0.39)
    panel_w = 0.34 if open_state else 0.72
    for i, x in enumerate(xs):
        _add(s, _box((panel_w, 0.055, 2.18), (x, -0.025, 1.09), 'MAT_STAINLESS'), f'MOV_ElevatorDoor_{i}')
    _add(s, _box((0.22, 0.07, 0.34), (0.92, -0.05, 1.15), 'MAT_BLACKENED_STEEL'), 'CallPanel')
    return s


def _make_stair(name):
    s = trimesh.Scene()
    width = 2.65 if 'Grand' in name else 1.05 if 'Maintenance' in name else 1.55
    steps = 9
    for i in range(steps):
        z = (i + 1) * 0.18 / 2
        y = i * 0.28
        _add(s, _box((width, 0.30, (i + 1) * 0.18), (0, y, z), 'MAT_STONE_LIGHT' if 'Grand' in name else 'MAT_CONCRETE'), f'Step_{i:02d}')
    rail_mat = 'MAT_BRASS_POLISHED' if 'Grand' in name else 'MAT_BLACKENED_STEEL'
    for x in (-width / 2 + 0.08, width / 2 - 0.08):
        for i in range(0, steps, 2):
            _add(s, _cylinder(0.018, 0.82, (x, i * 0.28, i * 0.18 + 0.54), rail_mat, 10), f'RailPost_{x}_{i}')
    return s


def _make_railing():
    s = trimesh.Scene()
    length, height = 1.8, 1.0
    for x in np.linspace(-0.85, 0.85, 6):
        _add(s, _cylinder(0.022, height, (x, 0, height / 2), 'MAT_BLACKENED_STEEL', 10), f'Post_{x:.2f}')
    _add(s, _box((length, 0.05, 0.05), (0, 0, height), 'MAT_BLACKENED_STEEL'), 'TopRail')
    _add(s, _box((length, 0.04, 0.04), (0, 0, 0.48), 'MAT_BLACKENED_STEEL'), 'MidRail')
    return s


def _make_column(name, material):
    s = trimesh.Scene()
    if 'Round' in name:
        _add(s, _cylinder(0.22, 2.65, (0, 0, 1.325), material, 24), 'ColumnShaft')
    else:
        _add(s, _box((0.44, 0.44, 2.65), (0, 0, 1.325), material), 'ColumnShaft')
    _add(s, _box((0.58, 0.58, 0.12), (0, 0, 0.06), material), 'ColumnBase')
    _add(s, _box((0.58, 0.58, 0.14), (0, 0, 2.72), material), 'ColumnCapital')
    return s


def _make_archway(name, material):
    s = trimesh.Scene()
    mat = 'MAT_WOOD_WARM' if 'Wood' in name else material
    _add(s, _box((0.24, 0.40, 2.40), (-0.72, 0, 1.20), mat), 'ArchPierL')
    _add(s, _box((0.24, 0.40, 2.40), (0.72, 0, 1.20), mat), 'ArchPierR')
    for i, x in enumerate(np.linspace(-0.58, 0.58, 7)):
        z = 2.35 + 0.34 * (1 - (x / 0.65) ** 2)
        _add(s, _box((0.22, 0.40, 0.20), (x, 0, z), mat), f'ArchVoussoir_{i}')
    return s


def _make_general(name, material):
    s = trimesh.Scene()
    if 'Facade Corner' in name:
        _add(s, _box((1.0, 0.18, 2.8), (0, 0, 1.4), material), 'FacadeA')
        _add(s, _box((0.18, 1.0, 2.8), (-0.41, 0.41, 1.4), material), 'FacadeB')
    elif 'Entrance Canopy' in name:
        _add(s, _box((2.6, 1.45, 0.16), (0, 0.48, 2.75), 'MAT_BLACKENED_STEEL'), 'CanopyRoof')
        for x in (-1.10, 1.10):
            _add(s, _cylinder(0.04, 2.65, (x, 0.98, 1.325), 'MAT_BRASS_POLISHED', 12), f'CanopyPost_{x}')
        _add(s, _box((2.1, 0.04, 0.28), (0, -0.25, 2.58), 'MAT_SIGNAGE'), 'CanopySignBand')
    elif 'Parapet' in name:
        _add(s, _box((1.0, 0.28, 0.80), (0, 0, 0.40), material), 'Parapet')
        _add(s, _box((1.08, 0.34, 0.10), (0, 0, 0.85), 'MAT_STONE_LIGHT'), 'ParapetCap')
    elif 'Cornice' in name:
        for i, z in enumerate((0.08, 0.18, 0.30)):
            _add(s, _box((1.0 + i * 0.08, 0.24 + i * 0.04, 0.10), (0, 0, z), material), f'Cornice_{i}')
    elif 'Vent Grille' in name:
        _add(s, _box((0.95, 0.08, 0.72), (0, 0, 0.36), 'MAT_BLACKENED_STEEL'), 'VentFrame')
        for z in np.linspace(0.11, 0.61, 7):
            _add(s, _box((0.78, 0.04, 0.035), (0, -0.06, z), 'MAT_STAINLESS'), f'VentSlat_{z:.2f}')
    elif 'Hanging Sign Mount' in name:
        _add(s, _box((0.12, 0.12, 1.15), (-0.48, 0, 0.58), 'MAT_BLACKENED_STEEL'), 'MountPost')
        _add(s, _box((0.92, 0.08, 0.10), (-0.02, 0, 1.05), 'MAT_BLACKENED_STEEL'), 'Bracket')
        _add(s, _box((0.62, 0.07, 0.58), (0.18, 0, 0.64), 'MAT_SIGNAGE'), 'SignPlate')
    elif 'Baseboard' in name:
        _add(s, _box((1.0, 0.08, 0.14), (0, 0, 0.07), material), 'Baseboard')
    elif 'Crown Molding' in name:
        _add(s, _box((1.0, 0.12, 0.16), (0, 0, 0.08), material), 'Crown')
        _add(s, _box((0.96, 0.18, 0.06), (0, 0.03, 0.17), material), 'CrownLip')
    elif 'Corner Trim' in name:
        _add(s, _box((0.12, 0.12, 2.8), (0, 0, 1.4), material), 'CornerTrim')
    elif 'Door Transom' in name:
        _frame(s, 1.12, 0.55, 0.14, material, 'TransomFrame')
        _add(s, _box((0.86, 0.02, 0.31), (0, 0, 0.17), 'MAT_GLASS_CLEAR'), 'TransomGlass')
    else:
        _add(s, _box((1.0, 0.22, 1.0), (0, 0, 0.5), material), 'Body')
    return s


def build_asset(display_name: str, material: str) -> trimesh.Scene:
    name = display_name
    if any(k in name for k in ('Floor', 'Slab')):
        return _make_floor(name, material)
    if any(k in name for k in ('Interior Wall', 'Service Wall', 'Glass Partition', 'Half Wall', 'Wood Wall Panel')):
        return _make_wall(name, material)
    if 'Elevator' in name:
        return _make_elevator(name)
    if 'Door' in name and 'Transom' not in name:
        return _make_door(name, material)
    if 'Window' in name or 'Curtain Wall' in name:
        return _make_window(name, material)
    if 'Stair' in name:
        return _make_stair(name)
    if 'Railing' in name:
        return _make_railing()
    if 'Column' in name:
        return _make_column(name, material)
    if 'Archway' in name:
        return _make_archway(name, material)
    return _make_general(name, material)


def generate_batch(manifest_path: Path | str, output_dir: Path | str) -> list[Path]:
    manifest_path = Path(manifest_path)
    output_dir = Path(output_dir)
    data = json.loads(manifest_path.read_text(encoding='utf-8'))
    entries = _validated_entries(data)
    output_dir.mkdir(parents=True, exist_ok=True)
    outputs = []
    for row in entries:
        asset_id, display_name, _subcategory, material_family, _profile, _animation_set, _anchors = row
        scene = build_asset(display_name, material_family)
        scene.units = 'meters'
        out = output_dir / f'{asset_id}.glb'
        out.write_bytes(scene.export(file_type='glb'))
        outputs.append(out)
    return outputs


def generate_package(manifest_path: Path | str, output_dir: Path | str, source_path: str) -> list[Path]:
    manifest_path = Path(manifest_path)
    output_dir = Path(output_dir)
    outputs = generate_batch(manifest_path, output_dir)
    data = json.loads(manifest_path.read_text(encoding='utf-8'))
    rows = {row[0]: row for row in _validated_entries(data)}
    for glb_path in outputs:
        asset_id, _display_name, subcategory, material_family, profile, animation_set, _anchors = rows[glb_path.stem]
        tags = ['batch_01', 'architectural_diorama_realism', subcategory]
        if animation_set:
            tags.append(f'animation_binding:{animation_set}')
        metadata = {
            'schema': 1,
            'asset_id': asset_id,
            'asset_type': PROFILE_ASSET_TYPES[profile],
            'source': source_path,
            'units': 'meters',
            'lod_policy': 'lod_architecture',
            'collision_policy': 'simple_proxy',
            'cutaway_policy': 'normal',
            'material_slots': [material_family],
            'tags': tags,
            'dependencies': [],
            'milestone': 'batch_01',
            'source_revision': 1,
            'metadata_revision': 1,
        }
        glb_path.with_suffix('.asset.json').write_text(
            json.dumps(metadata, indent=2, sort_keys=True) + '\n', encoding='utf-8'
        )
    return outputs


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('manifest')
    parser.add_argument('output')
    parser.add_argument('--package', action='store_true')
    args = parser.parse_args()
    outputs = (
        generate_package(args.manifest, args.output, 'Tools/ArtGeneration/batch01_generate.py')
        if args.package
        else generate_batch(args.manifest, args.output)
    )
    for path in outputs:
        print(path)
