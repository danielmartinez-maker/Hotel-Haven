from __future__ import annotations

import trimesh

from amenity_decor_factory import build_asset as build_legacy_amenity
from service_asset_factory import gym as build_gym
from v2_asset_common import chair, luggage_or_accessibility, semantic_composite, table


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if subcategory in {'luggage', 'accessible'}:
        return luggage_or_accessibility(name, mat)
    if subcategory == 'gym':
        scene = build_gym(name, mat)
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
