from __future__ import annotations

from pathlib import Path

import numpy as np
import trimesh

# These assets are visually floor-standing even when their coarse production
# profile is shared with wall/counter-mounted furniture. The post-process is
# idempotent: it only adds support geometry when the generated mesh still
# floats more than the QA tolerance above Z=0.
FLOOR_SUPPORT_TARGETS = {
    'HH_A111', 'HH_A112', 'HH_A113', 'HH_A114', 'HH_A115', 'HH_A116',
    'HH_A120', 'HH_A134', 'HH_A135',
    'HH_A194', 'HH_A195', 'HH_A196',
    'HH_A249', 'HH_A250',
    'HH_A347', 'HH_A348', 'HH_A349',
    'HH_A353', 'HH_A357', 'HH_A360', 'HH_A362', 'HH_A363', 'HH_A364',
}

STEEL_SUPPORTS = {
    'HH_A347', 'HH_A348', 'HH_A349', 'HH_A353',
}


def _material(asset_id: str):
    if asset_id in STEEL_SUPPORTS:
        name = 'MAT_BLACKENED_STEEL'
        rgba = (26, 31, 33, 255)
        metallic, roughness = 0.78, 0.50
    else:
        name = 'MAT_WOOD_DARK'
        rgba = (56, 31, 20, 255)
        metallic, roughness = 0.0, 0.48
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=np.asarray(rgba, dtype=np.uint8),
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def _box(extents, center, asset_id: str):
    mesh = trimesh.creation.box(extents=extents)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=_material(asset_id))
    return mesh


def _add_supports(scene: trimesh.Scene, asset_id: str, bounds: np.ndarray) -> None:
    minimum, maximum = bounds
    width = max(float(maximum[0] - minimum[0]), 0.20)
    depth = max(float(maximum[1] - minimum[1]), 0.20)
    floor_gap = float(minimum[2])

    if asset_id == 'HH_A353':
        # The procedural exercise-ball asset is intentionally sparse; give it
        # an explicit low rack base and uprights so the silhouette reads as a
        # rack rather than four disconnected spheres.
        base_depth = max(depth * 0.90, 0.32)
        scene.add_geometry(
            _box((width * 0.92, base_depth, 0.08), (0.0, 0.0, 0.04), asset_id),
            node_name='QC_FloorRackBase',
            geom_name='QC_FloorRackBase',
        )
        upright_h = max(float(maximum[2]) * 0.92, 0.75)
        for i, x in enumerate((-width * 0.38, width * 0.38)):
            scene.add_geometry(
                _box((0.05, 0.05, upright_h), (x, 0.0, upright_h / 2.0), asset_id),
                node_name=f'QC_FloorRackUpright_{i}',
                geom_name=f'QC_FloorRackUpright_{i}',
            )
        return

    support_h = max(floor_gap, 0.04)
    support_w = min(0.09, max(0.05, min(width, depth) * 0.09))
    x_positions = (minimum[0] + width * 0.18, maximum[0] - width * 0.18)
    y_positions = (minimum[1] + depth * 0.18, maximum[1] - depth * 0.18)
    for i, (x, y) in enumerate(
        ((x_positions[0], y_positions[0]),
         (x_positions[1], y_positions[0]),
         (x_positions[0], y_positions[1]),
         (x_positions[1], y_positions[1]))
    ):
        scene.add_geometry(
            _box((support_w, support_w, support_h), (float(x), float(y), support_h / 2.0), asset_id),
            node_name=f'QC_FloorSupport_{i}',
            geom_name=f'QC_FloorSupport_{i}',
        )


def harden_floor_supports(exports_root: Path, tolerance: float = 0.12) -> list[str]:
    hardened: list[str] = []
    for asset_id in sorted(FLOOR_SUPPORT_TARGETS):
        matches = list(exports_root.glob(f'Batch*/{asset_id}.glb'))
        if len(matches) != 1:
            raise RuntimeError(f'{asset_id}: expected one generated GLB, found {len(matches)}')
        path = matches[0]
        scene = trimesh.load(path, force='scene')
        bounds = np.asarray(scene.bounds, dtype=float)
        if bounds.shape != (2, 3) or not np.isfinite(bounds).all():
            raise RuntimeError(f'{asset_id}: invalid bounds during floor support hardening')
        if float(bounds[0, 2]) <= tolerance:
            continue
        _add_supports(scene, asset_id, bounds)
        path.write_bytes(scene.export(file_type='glb'))
        hardened.append(asset_id)
    return hardened
