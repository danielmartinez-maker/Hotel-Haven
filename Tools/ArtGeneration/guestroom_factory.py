from __future__ import annotations

import trimesh

from v2_asset_common import (
    bathroom_detail,
    bathroom_fixture,
    bed,
    cabinet,
    chair,
    desk,
    lamp,
    semantic_composite,
    sofa,
    table,
    wall_feature,
)


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if 'Bed Frame' in name:
        return bed(name, mat)
    if any(k in name for k in ('Chair', 'Shower Seat')):
        return chair(name, mat)
    if 'Sofa' in name:
        return sofa(name, mat)
    if 'Lamp' in name:
        return lamp(name, mat)
    if 'Wall Art' in name or 'Mirror' in name:
        return wall_feature(name, mat)
    if any(k in name for k in (
        'Shower Tray', 'Shower Head', 'Towel Rail', 'Towel Shelf',
        'Amenity Tray', 'Tissue Box', 'Hair Dryer', 'Grab Bar', 'Toilet Rail',
    )):
        return bathroom_detail(name, mat)
    if any(k in name for k in ('Desk', 'Vanity')):
        if 'Vanity' in name:
            return bathroom_fixture(name, mat)
        return desk(name, mat)
    if any(k in name for k in ('Table', 'Nightstand')):
        return table(name, mat)
    if any(k in name for k in ('Wardrobe', 'Cabinet', 'Console', 'Safe', 'Coffee Station')):
        return cabinet(name, mat)
    if subcategory in {'bathroom', 'accessible'} and any(k in name for k in ('Sink', 'Toilet', 'Bathtub', 'Shower')):
        return bathroom_fixture(name, mat)
    return semantic_composite(name, mat, asset_id)
