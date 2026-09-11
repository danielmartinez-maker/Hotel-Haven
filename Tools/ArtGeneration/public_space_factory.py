from __future__ import annotations

import trimesh

from v2_asset_common import cabinet, cart, chair, desk, semantic_composite, sofa, table


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if any(k in name for k in ('Reception Desk', 'Concierge Desk', 'Business Center Desk')):
        return desk(name, mat)
    if 'Luggage Cart' in name:
        return cart(name, mat)
    if any(k in name for k in ('Sofa',)):
        return sofa(name, mat)
    if any(k in name for k in ('Chair', 'Bench', 'Ottoman')):
        return chair(name, mat)
    if any(k in name for k in ('Table', 'Workstation')):
        return table(name, mat)
    if any(k in name for k in ('Shelf', 'Rack', 'Locker', 'Display', 'Parcel')):
        return cabinet(name, mat)
    return semantic_composite(name, mat, asset_id)
