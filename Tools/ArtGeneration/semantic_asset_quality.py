from __future__ import annotations

SEMANTIC_NODE_REQUIREMENTS = {
    'HH_A121': (
        'Standard Nightstand',
        {'CaseTop', 'CasePlinth', 'Drawer_0_0', 'DrawerPull_0_0', 'NightstandLowerReveal'},
    ),
    'HH_A141': (
        'Standard Wardrobe',
        {'Door_Left', 'Door_Right', 'Handle_Left', 'Handle_Right',
         'WardrobeTopCap', 'WardrobePlinth', 'WardrobeDoorGap'},
    ),
    'HH_A186': (
        'Reception Desk Single',
        {'Countertop', 'Monitor_0', 'ReceptionToeKick',
         'ReceptionFrontRail', 'ReceptionPilaster_0', 'ReceptionPilaster_1'},
    ),
    'HH_A302': (
        'Cleaning Supply Cabinet',
        {'CabinetDoor_0', 'CabinetDoor_1', 'CabinetHandle_0', 'CabinetHandle_1',
         'CabinetDoorGap', 'CabinetPlinth', 'CleaningLabel', 'CleaningShelf'},
    ),
    'HH_A338': (
        'Staff Locker Bank',
        {'LockerDoor_0', 'LockerDoor_1', 'LockerDoor_2',
         'LockerHandle_0', 'LockerHandle_1', 'LockerHandle_2',
         'LockerVent_0_0', 'LockerVent_1_0', 'LockerVent_2_0',
         'CabinetPlinth', 'LockerToeReveal'},
    ),
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
    **{f'character_expansion_{group}': tuple(f'HH_A{i:03d}' for i in range(1051 + group * 5, 1056 + group * 5))
       for group in range(20)},
    **{f'amenity_expansion_{group}': tuple(f'HH_A{i:03d}' for i in range(1151 + group * 5, 1156 + group * 5))
       for group in range(10)},
    **{f'public_expansion_{group}': tuple(f'HH_A{i:03d}' for i in range(1201 + group * 5, 1206 + group * 5))
       for group in range(10)},
    **{f'catalog_character_{group}': tuple(f'HH_A{i:04d}' for i in range(1251 + group * 5, 1256 + group * 5))
       for group in range(6)},
    **{f'catalog_guestroom_{group}': tuple(f'HH_A{i:04d}' for i in range(1281 + group * 5, 1286 + group * 5))
       for group in range(14)},
    **{f'catalog_restaurant_{group}': tuple(f'HH_A{i:04d}' for i in range(1351 + group * 5, 1356 + group * 5))
       for group in range(10)},
    **{f'catalog_housekeeping_{group}': tuple(f'HH_A{i:04d}' for i in range(1401 + group * 5, 1406 + group * 5))
       for group in range(10)},
    **{f'catalog_decor_{group}': tuple(f'HH_A{i:04d}' for i in range(1451 + group * 5, 1456 + group * 5))
       for group in range(10)},
    **{f'property_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(1501 + group * 5, 1506 + group * 5))
       for group in range(10)},
    **{f'property_public_{group}': tuple(f'HH_A{i:04d}' for i in range(1551 + group * 5, 1556 + group * 5))
       for group in range(10)},
    **{f'property_finish_{group}': tuple(f'HH_A{i:04d}' for i in range(1601 + group * 5, 1606 + group * 5))
       for group in range(10)},
    **{f'property_amenities_{group}': tuple(f'HH_A{i:04d}' for i in range(1651 + group * 5, 1656 + group * 5))
       for group in range(10)},
    **{f'property_exterior_{group}': tuple(f'HH_A{i:04d}' for i in range(1701 + group * 5, 1706 + group * 5))
       for group in range(10)},
    **{f'operations_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(1751 + group * 5, 1756 + group * 5))
       for group in range(10)},
    **{f'operations_public_{group}': tuple(f'HH_A{i:04d}' for i in range(1801 + group * 5, 1806 + group * 5))
       for group in range(10)},
    **{f'operations_restaurant_{group}': tuple(f'HH_A{i:04d}' for i in range(1851 + group * 5, 1856 + group * 5))
       for group in range(10)},
    **{f'operations_housekeeping_{group}': tuple(f'HH_A{i:04d}' for i in range(1901 + group * 5, 1906 + group * 5))
       for group in range(10)},
    **{f'operations_amenities_{group}': tuple(f'HH_A{i:04d}' for i in range(1951 + group * 5, 1956 + group * 5))
       for group in range(10)},
    **{f'service_kitchen_machinery_{group}': tuple(f'HH_A{i:04d}' for i in range(2001 + group * 5, 2006 + group * 5))
       for group in range(10)},
    **{f'service_kitchen_staging_{group}': tuple(f'HH_A{i:04d}' for i in range(2051 + group * 5, 2056 + group * 5))
       for group in range(10)},
    **{f'service_laundry_{group}': tuple(f'HH_A{i:04d}' for i in range(2101 + group * 5, 2106 + group * 5))
       for group in range(10)},
    **{f'service_receiving_{group}': tuple(f'HH_A{i:04d}' for i in range(2151 + group * 5, 2156 + group * 5))
       for group in range(10)},
    **{f'service_plant_{group}': tuple(f'HH_A{i:04d}' for i in range(2201 + group * 5, 2206 + group * 5))
       for group in range(10)},
    **{f'experience_accessible_guest_{group}': tuple(f'HH_A{i:04d}' for i in range(2251 + group * 5, 2256 + group * 5))
       for group in range(10)},
    **{f'experience_heritage_decor_{group}': tuple(f'HH_A{i:04d}' for i in range(2301 + group * 5, 2306 + group * 5))
       for group in range(10)},
    **{f'experience_grounds_{group}': tuple(f'HH_A{i:04d}' for i in range(2351 + group * 5, 2356 + group * 5))
       for group in range(10)},
    **{f'experience_wellness_{group}': tuple(f'HH_A{i:04d}' for i in range(2401 + group * 5, 2406 + group * 5))
       for group in range(10)},
    **{f'experience_finish_{group}': tuple(f'HH_A{i:04d}' for i in range(2451 + group * 5, 2456 + group * 5))
       for group in range(10)},
    **{f'destination_events_{group}': tuple(f'HH_A{i:04d}' for i in range(2501 + group * 5, 2506 + group * 5))
       for group in range(10)},
    **{f'destination_decor_{group}': tuple(f'HH_A{i:04d}' for i in range(2551 + group * 5, 2556 + group * 5))
       for group in range(10)},
    **{f'destination_exterior_{group}': tuple(f'HH_A{i:04d}' for i in range(2601 + group * 5, 2606 + group * 5))
       for group in range(10)},
    **{f'destination_finish_{group}': tuple(f'HH_A{i:04d}' for i in range(2651 + group * 5, 2656 + group * 5))
       for group in range(10)},
    **{f'destination_restaurant_{group}': tuple(f'HH_A{i:04d}' for i in range(2701 + group * 5, 2706 + group * 5))
       for group in range(10)},
    **{f'core_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(2751 + group * 5, 2756 + group * 5))
       for group in range(10)},
    **{f'core_guestroom_{group}': tuple(f'HH_A{i:04d}' for i in range(2801 + group * 5, 2806 + group * 5))
       for group in range(10)},
    **{f'core_public_{group}': tuple(f'HH_A{i:04d}' for i in range(2851 + group * 5, 2856 + group * 5))
       for group in range(10)},
    **{f'core_housekeeping_{group}': tuple(f'HH_A{i:04d}' for i in range(2901 + group * 5, 2906 + group * 5))
       for group in range(10)},
    **{f'core_restaurant_{group}': tuple(f'HH_A{i:04d}' for i in range(2951 + group * 5, 2956 + group * 5))
       for group in range(10)},
    **{f'continuation_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(3001 + group * 5, 3006 + group * 5))
       for group in range(10)},
    **{f'continuation_guestroom_{group}': tuple(f'HH_A{i:04d}' for i in range(3051 + group * 5, 3056 + group * 5))
       for group in range(10)},
    **{f'continuation_public_{group}': tuple(f'HH_A{i:04d}' for i in range(3101 + group * 5, 3106 + group * 5))
       for group in range(10)},
    **{f'continuation_restaurant_{group}': tuple(f'HH_A{i:04d}' for i in range(3151 + group * 5, 3156 + group * 5))
       for group in range(10)},
    **{f'continuation_housekeeping_{group}': tuple(f'HH_A{i:04d}' for i in range(3201 + group * 5, 3206 + group * 5))
       for group in range(9)},
    'continuation_finish_0': tuple(f'HH_A{i:04d}' for i in range(3246, 3251)),
    **{f'completion_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(3251 + group * 5, 3256 + group * 5))
       for group in range(10)},
    **{f'completion_guestroom_{group}': tuple(f'HH_A{i:04d}' for i in range(3301 + group * 5, 3306 + group * 5))
       for group in range(10)},
    **{f'completion_public_{group}': tuple(f'HH_A{i:04d}' for i in range(3351 + group * 5, 3356 + group * 5))
       for group in range(10)},
    **{f'completion_amenity_{group}': tuple(f'HH_A{i:04d}' for i in range(3401 + group * 5, 3406 + group * 5))
       for group in range(10)},
    **{f'completion_decor_{group}': tuple(f'HH_A{i:04d}' for i in range(3451 + group * 5, 3456 + group * 5))
       for group in range(10)},
    **{f'finalization_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(3501 + group * 5, 3506 + group * 5))
       for group in range(10)},
    **{f'finalization_guestroom_{group}': tuple(f'HH_A{i:04d}' for i in range(3551 + group * 5, 3556 + group * 5))
       for group in range(10)},
    **{f'finalization_public_{group}': tuple(f'HH_A{i:04d}' for i in range(3601 + group * 5, 3606 + group * 5))
       for group in range(10)},
    **{f'finalization_amenity_{group}': tuple(f'HH_A{i:04d}' for i in range(3651 + group * 5, 3656 + group * 5))
       for group in range(9)},
    'finalization_decor_0': tuple(f'HH_A{i:04d}' for i in range(3696, 3701)),
    **{f'catalog_completion_architecture_{group}': tuple(f'HH_A{i:04d}' for i in range(3701 + group * 5, 3706 + group * 5))
       for group in range(10)},
    **{f'catalog_completion_guestroom_{group}': tuple(f'HH_A{i:04d}' for i in range(3751 + group * 5, 3756 + group * 5))
       for group in range(10)},
    **{f'catalog_completion_public_{group}': tuple(f'HH_A{i:04d}' for i in range(3801 + group * 5, 3806 + group * 5))
       for group in range(8)},
    **{f'catalog_completion_decor_{group}': tuple(f'HH_A{i:04d}' for i in range(3841 + group * 5, 3846 + group * 5))
       for group in range(10)},
    **{f'catalog_completion_exterior_{group}': tuple(f'HH_A{i:04d}' for i in range(3891 + group * 5, 3896 + group * 5))
       for group in range(2)},
    **{f'curated_hotel_{group}': tuple(f'HH_A{i:03d}' for i in range(851 + group * 5, 856 + group * 5))
       for group in range(40)},
    **{f'corridor_module_{family}': tuple(f'HH_A{i:03d}' for i in range(801 + family * 5, 806 + family * 5))
       for family in range(10)},
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
