from __future__ import annotations

import trimesh

from character_factory import build_character
from v2_asset_common import add, box


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    index = max(0, int(asset_id.split('A')[-1]) - 1)
    scene = build_character(name, index)
    nodes = set(scene.graph.nodes_geometry)
    if 'Root' not in nodes:
        add(scene, box((0.018, 0.018, 0.018), (0, 0, 0.009), 'MAT_BLACKENED_STEEL'), 'Root')

    if any(k in name for k in ('Spa Therapist', 'Fitness Attendant', 'Pool Attendant')):
        add(scene, box((0.18, 0.06, 0.08), (0.18, -0.17, 1.05), 'MAT_SIGNAGE'), 'RoleDetail_Badge')
        add(scene, box((0.22, 0.08, 0.04), (-0.18, -0.17, 0.98), 'MAT_LINEN'), 'RoleDetail_Towel')
    elif 'Banquet Captain' in name:
        add(scene, box((0.16, 0.05, 0.08), (0.18, -0.18, 1.12), 'MAT_BRASS_POLISHED'), 'RoleDetail_Nameplate')
        add(scene, box((0.24, 0.05, 0.32), (-0.22, -0.18, 1.00), 'MAT_SIGNAGE'), 'RoleDetail_OrderPad')
    elif 'Valet Attendant' in name:
        add(scene, box((0.24, 0.06, 0.12), (0.20, -0.17, 1.06), 'MAT_BRASS_POLISHED'), 'RoleDetail_KeyWallet')
        add(scene, box((0.32, 0.06, 0.05), (0, -0.19, 1.42), 'MAT_SIGNAGE'), 'RoleDetail_CapBand')
    else:
        add(scene, box((0.16, 0.05, 0.08), (0.18, -0.17, 1.08), 'MAT_SIGNAGE'), 'RoleDetail_Accessory')
    return scene
