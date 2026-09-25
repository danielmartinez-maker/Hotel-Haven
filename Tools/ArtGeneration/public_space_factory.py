from __future__ import annotations

import trimesh

from v2_asset_common import (
    brochure_stand,
    cabinet,
    cart,
    chair,
    desk,
    directory_kiosk,
    divider,
    lamp,
    ottoman,
    semantic_composite,
    sofa,
    table,
)


def build_asset(name: str, subcategory: str, mat: str, asset_id: str, profile: str) -> trimesh.Scene:
    if any(k in name for k in ('Reception Desk', 'Concierge Desk', 'Business Center Desk')):
        return desk(name, mat)
    if 'Luggage Cart' in name:
        return cart(name, mat)
    if 'Lamp' in name:
        return lamp(name, mat)
    if 'Decorative Divider' in name:
        return divider(name, mat)
    if 'Ottoman' in name:
        return ottoman(name, mat)
    if 'Brochure Stand' in name:
        return brochure_stand(name, mat)
    if 'Digital Directory Kiosk' in name:
        return directory_kiosk(name, mat)
    if 'Sofa' in name:
        return sofa(name, mat)
    if any(k in name for k in ('Chair', 'Bench')):
        return chair(name, mat)
    if any(k in name for k in ('Table', 'Workstation')):
        return table(name, mat)
    if any(k in name for k in ('Shelf', 'Rack', 'Locker', 'Display', 'Parcel')):
        return cabinet(name, mat)
    return semantic_composite(name, mat, asset_id)
