from __future__ import annotations

import numpy as np
import trimesh

from amenity_decor_factory import build_asset as build_legacy_amenity
from service_asset_factory import add, box, cyl
from service_asset_factory import gym as build_gym
from v2_asset_common import chair, luggage_or_accessibility, semantic_composite, table


def _semantic_gym(name: str, mat: str) -> trimesh.Scene:
    """Preserve detailed legacy gym geometry with V2 semantic root naming."""
    scene = trimesh.Scene()
    if 'Treadmill' in name:
        add(scene, box((1.35, 0.55, 0.12), (0, 0, 0.06), 'MAT_BLACKENED_STEEL'), 'PrimaryDeck')
        for i, x in enumerate((-0.20, 0.20)):
            add(scene, box((0.05, 0.05, 1.02), (x, 0.22, 0.57), 'MAT_BLACKENED_STEEL'), f'Handrail_{i}')
        add(scene, box((0.45, 0.12, 0.28), (0, 0.22, 1.12), 'MAT_ELECTRONICS'), 'Console')
        return scene
    if 'Elliptical' in name:
        add(scene, cyl(0.26, 0.06, (0, 0, 0.26), 'MAT_BLACKENED_STEEL', 24), 'PrimaryFlywheel')
        for i, x in enumerate((-0.20, 0.20)):
            add(scene, box((0.05, 0.05, 1.30), (x, 0, 0.70), 'MAT_BLACKENED_STEEL'), f'Handle_{i}')
        add(scene, box((0.60, 0.16, 0.05), (-0.22, -0.10, 0.18), 'MAT_BLACKENED_STEEL'), 'Pedal_Left')
        add(scene, box((0.60, 0.16, 0.05), (0.22, 0.10, 0.18), 'MAT_BLACKENED_STEEL'), 'Pedal_Right')
        return scene
    if 'Weight Bench' in name:
        add(scene, box((1.25, 0.38, 0.14), (0, 0, 0.48), 'MAT_UPHOLSTERY'), 'Seat')
        for i, x in enumerate((-0.48, 0.48)):
            add(scene, box((0.08, 0.32, 0.46), (x, 0, 0.23), 'MAT_BLACKENED_STEEL'), f'Leg_{i}')
        return scene
    return build_gym(name, mat)


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if subcategory in {'luggage', 'accessible'}:
        return luggage_or_accessibility(name, mat)
    if subcategory == 'gym':
        scene = _semantic_gym(name, mat)
        if len(scene.geometry) >= 2:
            return scene
    if 'Exterior Bench' in name or 'Pool Lounger' in name:
        return chair(name, mat)
    if 'Exterior Cafe Table' in name:
        return table(name, mat)
    if 'Exterior Cafe Chair' in name:
        return chair(name, mat)
    scene = build_legacy_amenity(name, mat)
    names = set(scene.graph.nodes_geometry)
    if len(scene.geometry) < 2 or names == {'Body'}:
        return semantic_composite(name, mat, asset_id)
    return scene
