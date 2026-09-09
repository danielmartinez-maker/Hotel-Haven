from __future__ import annotations

SEMANTIC_NODE_REQUIREMENTS = {
    'HH_A352': ('Yoga Mat Rack', {'YogaMat_0', 'YogaMat_1'}),
    'HH_A357': ('Spa Treatment Chair', {'Pedestal', 'Footrest'}),
    'HH_A364': ('Pool Lounge Chair', {'PoolFrame'}),
    'HH_A405': ('Directional Sign Ceiling', {'CeilingMount'}),
    'HH_A441': ('Planter Exterior Rectangular', {'PlanterBox'}),
    'HH_A443': ('Hedge Corner', {'HedgeLeg_A', 'HedgeLeg_B'}),
    'HH_A448': ('Bike Rack', {'RackLoop_0'}),
    'HH_A451': ('Guest Businessman', {'Accessory_Briefcase'}),
    'HH_A452': ('Guest Businesswoman', {'Accessory_Briefcase'}),
    'HH_A459': ('Guest Family Parent A', {'Accessory_FamilyTote'}),
    'HH_A460': ('Guest Family Parent B', {'Accessory_FamilyTote'}),
    'HH_A461': ('Guest Child Boy', {'Accessory_ChildBackpack'}),
    'HH_A462': ('Guest Child Girl', {'Accessory_ChildBackpack'}),
    'HH_A495': ('Maintenance Technician Man', {'Accessory_ToolPouch'}),
    'HH_A496': ('Maintenance Technician Woman', {'Accessory_ToolPouch'}),
    'HH_A497': ('Security Officer Man', {'Accessory_Radio'}),
    'HH_A498': ('Security Officer Woman', {'Accessory_Radio'}),
}

DISTINCT_VARIANT_GROUPS = {
    'wall_clocks': ('HH_A392', 'HH_A393'),
    'room_number_plaques': ('HH_A402', 'HH_A403'),
    'directional_signs': ('HH_A404', 'HH_A405'),
    'exterior_planters': ('HH_A440', 'HH_A441'),
    'hedges': ('HH_A442', 'HH_A443'),
    'hotel_facade_modules': ('HH_A426', 'HH_A427', 'HH_A428', 'HH_A429'),
}


def _asset_number(asset_id: str) -> int | None:
    try:
        return int(asset_id.rsplit('A', 1)[1])
    except (IndexError, ValueError):
        return None


def semantic_contract_failures(asset_id: str, nodes) -> list[str]:
    node_set = set(map(str, nodes))
    failures = []
    requirement = SEMANTIC_NODE_REQUIREMENTS.get(asset_id)
    if requirement:
        display_name, required = requirement
        missing = sorted(required - node_set)
        if missing:
            failures.append(
                f'{asset_id} {display_name}: missing semantic geometry nodes {missing}'
            )

    number = _asset_number(asset_id)
    if number is not None and 351 <= number <= 450 and node_set == {'Body'}:
        display_name = requirement[0] if requirement else 'Batch 08/09 asset'
        failures.append(
            f'{asset_id} {display_name}: generic fallback Body mesh is not release-quality semantic geometry'
        )
    return failures


def variant_signature_failures(signatures: dict[str, tuple]) -> list[str]:
    failures = []
    for group_name, asset_ids in DISTINCT_VARIANT_GROUPS.items():
        present = [(asset_id, signatures[asset_id]) for asset_id in asset_ids if asset_id in signatures]
        if len(present) < 2:
            continue
        for i, (left_id, left_signature) in enumerate(present):
            for right_id, right_signature in present[i + 1:]:
                if left_signature == right_signature:
                    failures.append(
                        f'{group_name}: {left_id} and {right_id} share the same release geometry signature'
                    )
    return failures
