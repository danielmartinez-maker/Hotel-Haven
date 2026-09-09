from __future__ import annotations

import trimesh

from service_asset_factory import build_asset as build_legacy_service
from v2_asset_common import cart, semantic_composite


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if any(k in name for k in ('Cart', 'Trolley', 'Hand Truck')):
        return cart(name, mat)
    scene = build_legacy_service(name, mat)
    names = set(scene.graph.nodes_geometry)
    if len(scene.geometry) < 2 or names == {'Body'}:
        return semantic_composite(name, mat, asset_id)
    return scene
