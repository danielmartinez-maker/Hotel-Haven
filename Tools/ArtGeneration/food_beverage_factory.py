from __future__ import annotations

import trimesh

from v2_asset_common import cabinet, cart, chair, desk, semantic_composite, table


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if any(k in name for k in ('Table', 'High Top')):
        return table(name, mat)
    if 'Room Service Cart' in name or 'Chair Cart' in name:
        return cart(name, mat)
    if any(k in name for k in ('Chair', 'Stool', 'Booth')):
        return chair(name, mat)
    if any(k in name for k in ('Host Stand', 'Service Station', 'Counter', 'Buffet')):
        return desk(name, mat)
    if any(k in name for k in ('Cabinet', 'Shelf', 'Rack', 'Display Case')):
        return cabinet(name, mat)
    return semantic_composite(name, mat, asset_id)
