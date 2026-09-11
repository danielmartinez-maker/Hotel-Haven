from __future__ import annotations

import trimesh

from service_asset_factory import add as legacy_add
from service_asset_factory import box as legacy_box
from service_asset_factory import build_asset as build_legacy_service
from v2_asset_common import cart, semantic_composite


def _semantic_hamper(mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    legacy_add(scene, legacy_box((0.52, 0.48, 0.68), (0, 0, 0.34), mat), 'PrimaryBin')
    legacy_add(
        scene,
        legacy_box((0.52 * 0.95, 0.44, 0.06), (0, 0, 0.71), 'MAT_BLACKENED_STEEL'),
        'Lid',
    )
    return scene


def _semantic_caddy(mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    legacy_add(scene, legacy_box((0.48, 0.26, 0.26), (0, 0, 0.13), mat), 'PrimaryBox')
    legacy_add(
        scene,
        legacy_box((0.25, 0.04, 0.20), (0, 0, 0.36), 'MAT_BLACKENED_STEEL'),
        'Handle',
    )
    return scene


def _semantic_mop_bucket(mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    legacy_add(scene, legacy_box((0.48, 0.34, 0.38), (0, 0, 0.19), mat), 'PrimaryBucket')
    legacy_add(
        scene,
        legacy_box((0.20, 0.30, 0.24), (0.18, 0, 0.48), 'MAT_BLACKENED_STEEL'),
        'Wringer',
    )
    return scene


def _semantic_ladder(name: str, mat: str) -> trimesh.Scene:
    scene = trimesh.Scene()
    h = 1.65 if 'Portable' in name else 1.20
    w = 0.48
    for i, x in enumerate((-w / 2, w / 2)):
        node = 'PrimaryRail' if i == 0 else 'Rail_1'
        legacy_add(scene, legacy_box((0.045, 0.05, h), (x, 0, h / 2), mat), node)
    step_start = 0.20
    step_end = h - 0.18
    for i in range(6):
        z = step_start + (step_end - step_start) * i / 5
        legacy_add(scene, legacy_box((w, 0.12, 0.045), (0, 0, z), mat), f'Step_{i}')
    return scene


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if any(k in name for k in ('Cart', 'Trolley', 'Hand Truck')):
        return cart(name, mat)
    if 'Hamper' in name:
        return _semantic_hamper(mat)
    if 'Caddy' in name or 'Toolbox' in name:
        return _semantic_caddy(mat)
    if 'Mop Bucket' in name:
        return _semantic_mop_bucket(mat)
    if 'Ladder' in name:
        return _semantic_ladder(name, mat)
    scene = build_legacy_service(name, mat)
    names = set(scene.graph.nodes_geometry)
    if len(scene.geometry) < 2 or names == {'Body'}:
        return semantic_composite(name, mat, asset_id)
    return scene
