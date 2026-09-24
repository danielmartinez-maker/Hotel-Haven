from __future__ import annotations

import trimesh

from v2_asset_common import add, box, cabinet, cyl, elevator_or_door, semantic_composite, stair


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if 'Elevator' in name:
        if any(k in name for k in ('Indicator', 'Panel', 'Threshold', 'Frame')) and 'Door' not in name and 'Gate' not in name:
            return semantic_composite(name, mat, asset_id)
        return elevator_or_door(name, mat)
    if 'Stair' in name:
        return stair(name, mat)
    if any(k in name for k in ('Handrail', 'Balustrade', 'Wall Protection Rail')):
        scene = trimesh.Scene()
        glass = 'Glass' in name
        if glass:
            add(scene, box((1.55, 0.035, 0.78), (0, 0, 0.62), 'MAT_GLASS_CLEAR'), 'PrimaryPanel')
        for i, x in enumerate((-0.72, 0.0, 0.72)):
            add(scene, box((0.04, 0.04, 0.92), (x, 0, 0.46), 'MAT_BLACKENED_STEEL'), f'Post_{i}')
        add(scene, box((1.55, 0.06, 0.07), (0, 0, 0.96), mat), 'Handrail')
        add(scene, box((1.55, 0.10, 0.05), (0, 0, 0.025), 'MAT_BLACKENED_STEEL'), 'Base')
        return scene
    if any(k in name for k in ('Door', 'Gate', 'Hatch')):
        return elevator_or_door(name, mat)
    if 'Column Round' in name:
        scene = trimesh.Scene()
        add(scene, cyl(0.28, 2.65, (0, 0, 1.325), mat, 28), 'PrimaryStructure')
        add(scene, cyl(0.36, 0.10, (0, 0, 0.05), 'MAT_STONE_LIGHT', 28), 'Base')
        add(scene, cyl(0.33, 0.10, (0, 0, 2.60), 'MAT_STONE_LIGHT', 28), 'Capital')
        return scene
    if 'Column' in name:
        scene = trimesh.Scene()
        add(scene, box((0.52, 0.52, 2.65), (0, 0, 1.325), mat), 'PrimaryStructure')
        add(scene, box((0.64, 0.64, 0.10), (0, 0, 0.05), 'MAT_STONE_LIGHT'), 'Base')
        add(scene, box((0.60, 0.60, 0.10), (0, 0, 2.60), 'MAT_STONE_LIGHT'), 'Capital')
        return scene
    if any(k in name for k in ('Panel', 'Cabinet')):
        return cabinet(name, mat)
    if any(k in name for k in ('Wall', 'Divider', 'Bulkhead', 'Transition Arch')):
        scene = trimesh.Scene()
        w, d, h = 1.80, 0.14, 2.55
        if 'Half Height' in name:
            h = 1.15
        add(scene, box((w, d, h), (0, 0, h / 2), mat), 'PrimaryStructure')
        add(scene, box((w, d + 0.04, 0.10), (0, 0, 0.05), 'MAT_STONE_LIGHT'), 'Base')
        add(scene, box((w, d + 0.025, 0.08), (0, 0, h - 0.04), 'MAT_WOOD_WARM'), 'RoleDetail')
        if 'Glass' in name:
            add(scene, box((w * 0.76, 0.035, h * 0.70), (0, -d / 2 - 0.02, h * 0.55), 'MAT_GLASS_CLEAR'), 'GlassPanel')
        return scene
    return semantic_composite(name, mat, asset_id)
