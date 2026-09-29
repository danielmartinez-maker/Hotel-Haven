from __future__ import annotations

import trimesh

from service_asset_factory import build_asset as build_legacy_service
from v2_asset_common import add, cart, cyl, semantic_composite


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if any(k in name for k in ('Cart', 'Trolley', 'Hand Truck')):
        return cart(name, mat)
    scene = build_legacy_service(name, mat)
    names = set(scene.graph.nodes_geometry)
    if len(scene.geometry) < 2 or names == {'Body'}:
        scene = semantic_composite(name, mat, asset_id)
    if asset_id == 'HH_A749':
        for i, x in enumerate((-0.27, 0.27)):
            for j, y in enumerate((-0.20, 0.20)):
                add(scene, cyl(0.055, 0.035, (x, y, 0.055), 'MAT_BLACKENED_STEEL', 14), f'MOV_Wheel_{2*i+j}')
    if asset_id in {'HH_A709', 'HH_A710', 'HH_A712'}:
        add(scene, cyl(0.12, 0.04, (0, -0.17, 0.05), 'MAT_BLACKENED_STEEL', 18), 'MOV_Brush')
    if asset_id == 'HH_A711':
        transform, geometry = scene.graph['Wringer']
        wringer = scene.geometry[geometry].copy()
        scene.delete_geometry(geometry)
        scene.add_geometry(wringer, node_name='MOV_Wringer', geom_name='MOV_Wringer', transform=transform)
    return scene
