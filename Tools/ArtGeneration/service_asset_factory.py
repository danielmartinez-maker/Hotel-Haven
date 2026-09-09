from __future__ import annotations

import numpy as np
import trimesh

PALETTE = {
    'MAT_WOOD_WARM': ((0.42, 0.22, 0.10, 1), 0.0, 0.52),
    'MAT_WOOD_DARK': ((0.22, 0.12, 0.08, 1), 0.0, 0.48),
    'MAT_UPHOLSTERY': ((0.42, 0.31, 0.25, 1), 0.0, 0.84),
    'MAT_STAINLESS': ((0.58, 0.61, 0.62, 1), 0.85, 0.42),
    'MAT_SERVICE_PAINT': ((0.24, 0.30, 0.34, 1), 0.0, 0.68),
    'MAT_PLASTIC_RUBBER': ((0.18, 0.18, 0.18, 1), 0.0, 0.64),
    'MAT_GLASS_CLEAR': ((0.34, 0.55, 0.64, 0.42), 0.0, 0.10),
    'MAT_BLACKENED_STEEL': ((0.10, 0.12, 0.13, 1), 0.78, 0.50),
    'MAT_SIGNAGE': ((0.16, 0.17, 0.17, 1), 0.25, 0.44),
    'MAT_PAVING': ((0.42, 0.42, 0.39, 1), 0.0, 0.78),
    'MAT_ELECTRONICS': ((0.08, 0.09, 0.10, 1), 0.12, 0.36),
    'MAT_LINEN': ((0.88, 0.86, 0.80, 1), 0.0, 0.88),
    'MAT_BRASS_POLISHED': ((0.62, 0.38, 0.10, 1), 0.88, 0.30),
}


def material(name: str):
    rgba, metallic, roughness = PALETTE.get(name, PALETTE['MAT_SERVICE_PAINT'])
    encoded = np.clip(np.rint(np.asarray(rgba, dtype=float) * 255.0), 0, 255).astype(np.uint8)
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=encoded,
        metallicFactor=metallic,
        roughnessFactor=roughness,
    )


def box(extents, center=(0, 0, 0), mat='MAT_SERVICE_PAINT'):
    mesh = trimesh.creation.box(extents=extents)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def cyl(radius, height, center=(0, 0, 0), mat='MAT_STAINLESS', sections=18):
    mesh = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def add(scene, mesh, name):
    scene.add_geometry(mesh, node_name=name, geom_name=name)


def chair(name, mat):
    scene = trimesh.Scene()
    add(scene, box((0.48, 0.50, 0.10), (0, 0, 0.48), mat), 'Seat')
    add(scene, box((0.48, 0.10, 0.58), (0, 0.20, 0.76), mat), 'Back')
    for i, (x, y) in enumerate(((-0.19, -0.19), (0.19, -0.19), (-0.19, 0.19), (0.19, 0.19))):
        add(scene, box((0.045, 0.045, 0.43), (x, y, 0.215), 'MAT_WOOD_DARK'), f'Leg_{i}')
    if 'Upholstered' in name:
        add(scene, box((0.43, 0.42, 0.055), (0, -0.01, 0.555), 'MAT_UPHOLSTERY'), 'SeatCushion')
        add(scene, box((0.42, 0.055, 0.48), (0, 0.135, 0.80), 'MAT_UPHOLSTERY'), 'BackCushion')
    if 'Stool' in name:
        add(scene, cyl(0.20, 0.05, (0, 0, 0.74), mat, 20), 'StoolSeatAccent')
    return scene


def table_like(name, mat):
    scene = trimesh.Scene()
    w, d, h = 1.20, 0.65, 0.90
    if 'Cocktail Table' in name:
        w, d, h = 0.72, 0.72, 1.08
    if 'Folding Table' in name:
        w, d, h = 1.50, 0.72, 0.86
    if 'Workbench' in name:
        w, d, h = 1.55, 0.72, 0.92
    add(scene, box((w, d, 0.075), (0, 0, h), mat), 'Top')
    for i, (x, y) in enumerate(((-w * 0.42, -d * 0.36), (w * 0.42, -d * 0.36), (-w * 0.42, d * 0.36), (w * 0.42, d * 0.36))):
        add(scene, box((0.06, 0.06, h - 0.04), (x, y, (h - 0.04) / 2), 'MAT_BLACKENED_STEEL'), f'Leg_{i}')
    if 'Workbench' in name:
        add(scene, box((w, 0.06, 0.78), (0, d / 2 - 0.03, 1.28), 'MAT_SERVICE_PAINT'), 'Backboard')
        add(scene, box((0.44, 0.18, 0.10), (-0.40, -0.10, 1.04), 'MAT_STAINLESS'), 'BenchVice')
    return scene


def host_stand(mat):
    scene = trimesh.Scene()
    add(scene, box((0.86, 0.48, 1.05), (0, 0, 0.525), mat), 'HostBody')
    add(scene, box((0.94, 0.54, 0.07), (0, 0, 1.085), 'MAT_WOOD_DARK'), 'HostTop')
    add(scene, box((0.32, 0.04, 0.22), (0.20, -0.255, 1.18), 'MAT_ELECTRONICS'), 'ReservationScreen')
    add(scene, box((0.18, 0.12, 0.025), (-0.25, -0.20, 1.13), 'MAT_SIGNAGE'), 'ReservationBook')
    return scene


def counter(name, mat):
    scene = trimesh.Scene()
    w, d, h = 1.45, 0.68, 0.92
    if 'Corner' in name:
        w = 1.20
    add(scene, box((w, d, h), (0, 0, h / 2), mat), 'CounterBody')
    top_mat = 'MAT_STAINLESS' if any(k in name for k in ('Kitchen', 'Service', 'Prep', 'Buffet')) else mat
    add(scene, box((w + 0.08, d + 0.08, 0.07), (0, 0, h + 0.035), top_mat), 'CounterTop')
    if 'Corner' in name:
        add(scene, box((0.58, 0.58, h), (w * 0.43, d * 0.43, h / 2), mat), 'CornerReturn')
    if 'Sneeze Guard' in name:
        scene = trimesh.Scene()
        add(scene, box((1.40, 0.025, 0.50), (0, 0, 0.90), 'MAT_GLASS_CLEAR'), 'Glass')
        for i, x in enumerate((-0.62, 0.62)):
            add(scene, cyl(0.018, 0.90, (x, 0, 0.45), 'MAT_STAINLESS', 10), f'Post_{i}')
    if 'Hot Well' in name or 'Cold Well' in name or 'Ice Bin' in name:
        for i, x in enumerate((-0.42, 0, 0.42)):
            add(scene, box((0.34, 0.42, 0.08), (x, -0.05, h + 0.075), 'MAT_STAINLESS'), f'Well_{i}')
        if 'Hot Well' in name:
            add(scene, box((1.05, 0.05, 0.05), (0, 0.20, 1.34), 'MAT_BRASS_POLISHED'), 'HeatRail')
        elif 'Cold Well' in name:
            add(scene, box((1.05, 0.05, 0.12), (0, 0.20, 1.05), 'MAT_GLASS_CLEAR'), 'ColdPanGuard')
    return scene


def appliance(name, mat):
    scene = trimesh.Scene()
    w, d, h = 0.72, 0.70, 1.45
    if any(k in name for k in ('Refrigerator', 'Freezer', 'Warmer Cabinet')):
        w, d, h = 0.78, 0.72, 1.90
    if any(k in name for k in ('Oven', 'Dishwasher', 'Washer', 'Dryer')):
        w, d, h = 0.82, 0.75, 1.05
    if any(k in name for k in ('Range', 'Griddle', 'Fryer')):
        w, d, h = 0.84, 0.72, 0.90
    if 'Salamander' in name:
        w, d, h = 0.86, 0.42, 0.46
    add(scene, box((w, d, h), (0, 0, h / 2), mat), 'Body')
    add(scene, box((w * 0.72, 0.025, h * 0.30), (0, -d / 2 - 0.014, h * 0.58), 'MAT_ELECTRONICS'), 'ControlOrDoor')
    if any(k in name for k in ('Washer', 'Dryer')):
        add(scene, cyl(0.22, 0.025, (0, -d / 2 - 0.032, 0.54), 'MAT_GLASS_CLEAR', 24), 'DrumWindow')
        if 'Washer' in name:
            add(scene, box((0.22, 0.035, 0.09), (-0.22, -d / 2 - 0.035, 0.90), 'MAT_ELECTRONICS'), 'DetergentDrawer')
        else:
            for i, x in enumerate(np.linspace(-0.22, 0.22, 5)):
                add(scene, box((0.045, 0.02, 0.18), (x, -d / 2 - 0.04, 0.90), 'MAT_BLACKENED_STEEL'), f'VentSlat_{i}')
            add(scene, box((0.54, 0.025, 0.22), (0, -d / 2 - 0.035, 0.90), 'MAT_BLACKENED_STEEL'), 'VentGrille')
    if 'Range' in name:
        for i, (x, y) in enumerate(((-0.22, -0.16), (0.22, -0.16), (-0.22, 0.16), (0.22, 0.16))):
            add(scene, cyl(0.11, 0.025, (x, y, h + 0.02), 'MAT_BLACKENED_STEEL', 20), f'Burner_{i}')
    if 'Griddle' in name:
        add(scene, box((0.68, 0.50, 0.035), (0, -0.02, h + 0.03), 'MAT_BLACKENED_STEEL'), 'GriddlePlate')
    if 'Fryer' in name:
        for i, x in enumerate((-0.20, 0.20)):
            add(scene, box((0.30, 0.42, 0.12), (x, -0.03, h + 0.08), 'MAT_STAINLESS'), f'FryerBasket_{i}')
            add(scene, box((0.04, 0.28, 0.04), (x, 0.30, h + 0.15), 'MAT_BLACKENED_STEEL'), f'BasketHandle_{i}')
    if 'Oven' in name:
        add(scene, box((0.58, 0.05, 0.055), (0, -d / 2 - 0.05, 0.74), 'MAT_BLACKENED_STEEL'), 'OvenHandle')
    if 'Dishwasher' in name:
        add(scene, box((0.46, 0.05, 0.06), (0, -d / 2 - 0.05, 0.82), 'MAT_BLACKENED_STEEL'), 'DishwasherHandle')
        add(scene, box((0.54, 0.025, 0.10), (0, -d / 2 - 0.04, 0.95), 'MAT_ELECTRONICS'), 'WashControlPanel')
    if 'Sink' in name:
        scene = counter(name, 'MAT_STAINLESS')
        for i, x in enumerate((-0.42, 0, 0.42)):
            add(scene, box((0.34, 0.42, 0.12), (x, -0.04, 1.00), 'MAT_STAINLESS'), f'Basin_{i}')
        add(scene, cyl(0.025, 0.40, (0, 0.16, 1.18), 'MAT_STAINLESS', 12), 'FaucetRiser')
    return scene


def display_or_dispenser(name, mat):
    scene = trimesh.Scene()
    if 'Display Case' in name:
        add(scene, box((1.05, 0.55, 0.78), (0, 0, 0.39), 'MAT_WOOD_WARM'), 'Base')
        add(scene, box((1.00, 0.48, 0.70), (0, 0, 1.10), 'MAT_GLASS_CLEAR'), 'GlassCase')
        add(scene, box((0.92, 0.40, 0.025), (0, 0, 0.92), 'MAT_STAINLESS'), 'DisplayShelf')
        return scene
    if any(k in name for k in ('Cereal', 'Juice', 'Detergent')):
        add(scene, box((0.58, 0.34, 0.88), (0, 0, 0.44), mat), 'DispenserBody')
        for i, x in enumerate((-0.16, 0.16)):
            add(scene, cyl(0.09, 0.34, (x, -0.06, 1.05), 'MAT_GLASS_CLEAR', 16), f'Canister_{i}')
        if 'Juice' in name:
            add(scene, box((0.34, 0.08, 0.08), (0, -0.22, 0.78), 'MAT_ELECTRONICS'), 'TapPanel')
        return scene
    if 'Coffee Urn' in name:
        add(scene, cyl(0.22, 0.55, (0, 0, 0.28), 'MAT_STAINLESS', 24), 'Urn')
        add(scene, cyl(0.12, 0.12, (0, 0, 0.61), 'MAT_BLACKENED_STEEL', 16), 'Lid')
        add(scene, box((0.05, 0.16, 0.05), (0, -0.18, 0.25), 'MAT_BLACKENED_STEEL'), 'Spigot')
        return scene
    if 'Beer Tap' in name:
        add(scene, cyl(0.09, 0.55, (0, 0, 0.28), 'MAT_STAINLESS', 18), 'Tower')
        for i, x in enumerate((-0.13, 0.13)):
            add(scene, box((0.04, 0.16, 0.04), (x, -0.10, 0.55), 'MAT_BLACKENED_STEEL'), f'Tap_{i}')
        return scene
    if 'Wine Rack' in name or 'Bottle Display' in name:
        add(scene, box((1.05, 0.30, 1.45), (0, 0, 0.725), mat), 'RackFrame')
        for z in (0.35, 0.70, 1.05):
            add(scene, box((0.90, 0.25, 0.035), (0, 0, z), mat), f'Shelf_{z}')
        for j, z in enumerate((0.44, 0.79, 1.14)):
            for i, x in enumerate(np.linspace(-0.34, 0.34, 4)):
                add(scene, cyl(0.035, 0.24, (x, 0, z), 'MAT_GLASS_CLEAR', 12), f'Bottle_{j}_{i}')
        return scene
    return counter(name, mat)


def shelf_or_cabinet(name, mat):
    scene = trimesh.Scene()
    w, d, h = 1.10, 0.48, 1.80
    if 'Single' in name:
        w = 0.72
    if 'Double' in name:
        w = 1.45
    if 'Pegboard' in name:
        d = 0.08
    add(scene, box((w, d, 0.07), (0, 0, 0.035), mat), 'Base')
    for i, x in enumerate((-w / 2 + 0.035, w / 2 - 0.035)):
        add(scene, box((0.07, 0.07, h), (x, 0, h / 2), mat), f'Upright_{i}')
    for i, z in enumerate((0.38, 0.76, 1.14, 1.52)):
        add(scene, box((w * 0.94, d * 0.90, 0.045), (0, 0, z), mat), f'Shelf_{i}')
    if 'Cabinet' in name or 'Locker' in name:
        add(scene, box((w, d, h), (0, 0, h / 2), mat), 'CabinetBody')
        for i, x in enumerate(np.linspace(-w * 0.35, w * 0.35, 3)):
            add(scene, box((0.025, 0.03, 0.18), (x, -d / 2 - 0.02, 1.0), 'MAT_BLACKENED_STEEL'), f'Handle_{i}')
    return scene


def cart(name, mat):
    scene = trimesh.Scene()
    w, length, h = 1.00, 1.30, 0.82
    if 'Compact' in name:
        w, length = 0.78, 1.05
    if 'Flatbed' in name or 'Platform Dolly' in name:
        h = 0.22
    if 'Hand Truck' in name:
        add(scene, box((0.46, 0.12, 0.06), (0, -0.12, 0.03), mat), 'ToePlate')
        for i, x in enumerate((-0.18, 0.18)):
            add(scene, box((0.05, 0.05, 1.25), (x, 0, 0.625), mat), f'Rail_{i}')
        add(scene, box((0.48, 0.05, 0.05), (0, 0, 1.22), mat), 'Handlebar')
        wheel_pos = ((-0.22, 0.06, 0.10), (0.22, 0.06, 0.10))
    else:
        add(scene, box((w, length, 0.09), (0, 0, 0.22), mat), 'Base')
        if h > 0.3:
            add(scene, box((w * 0.92, length * 0.82, h), (0, 0, 0.22 + h / 2), mat), 'Body')
        wheel_pos = ((-w * 0.42, -length * 0.40, 0.10), (w * 0.42, -length * 0.40, 0.10), (-w * 0.42, length * 0.40, 0.10), (w * 0.42, length * 0.40, 0.10))
    for i, (x, y, z) in enumerate(wheel_pos):
        wheel = cyl(0.10, 0.05, (x, y, z), 'MAT_BLACKENED_STEEL', 16)
        wheel.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0], [x, y, z]))
        add(scene, wheel, f'MOV_Wheel_{i}')
    if 'Housekeeping' in name:
        add(scene, box((w * 0.82, 0.18, 0.40), (0, -length * 0.28, 0.85), 'MAT_LINEN'), 'LinenStack')
        add(scene, box((0.16, length * 0.55, 0.42), (w * 0.48, 0, 0.72), 'MAT_PLASTIC_RUBBER'), 'SupplyCaddy')
    if 'Room Service' in name:
        add(scene, box((w * 0.70, length * 0.55, 0.08), (0, 0, 0.92), 'MAT_WOOD_WARM'), 'ServiceTop')
    return scene


def small_cleaning(name, mat):
    scene = trimesh.Scene()
    if 'Vacuum' in name or 'Floor Buffer' in name:
        add(scene, cyl(0.20, 0.18, (0, 0, 0.09), mat, 20), 'Base')
        add(scene, box((0.08, 0.08, 0.95), (0, 0.04, 0.565), 'MAT_BLACKENED_STEEL'), 'Handle')
        if 'Canister' in name:
            add(scene, cyl(0.16, 0.35, (0.24, 0, 0.175), mat, 18), 'Canister')
        return scene
    if 'Mop Bucket' in name:
        add(scene, box((0.48, 0.34, 0.38), (0, 0, 0.19), mat), 'Bucket')
        add(scene, box((0.20, 0.30, 0.24), (0.18, 0, 0.48), 'MAT_BLACKENED_STEEL'), 'Wringer')
        return scene
    if 'Caddy' in name or 'Toolbox' in name:
        add(scene, box((0.48, 0.26, 0.26), (0, 0, 0.13), mat), 'Box')
        add(scene, box((0.25, 0.04, 0.20), (0, 0, 0.36), 'MAT_BLACKENED_STEEL'), 'Handle')
        return scene
    if any(k in name for k in ('Hamper', 'Sorting Bin', 'Trash Bin', 'Recycling Bin', 'Compactor Bin')):
        h = 0.68 if 'Compactor' not in name else 1.10
        w = 0.52 if 'Compactor' not in name else 1.10
        add(scene, box((w, 0.48, h), (0, 0, h / 2), mat), 'Bin')
        add(scene, box((w * 0.95, 0.44, 0.06), (0, 0, h + 0.03), 'MAT_BLACKENED_STEEL'), 'Lid')
        return scene
    if 'Wet Floor Sign' in name:
        add(scene, box((0.36, 0.06, 0.70), (-0.12, 0, 0.35), 'MAT_SIGNAGE'), 'PanelA')
        add(scene, box((0.36, 0.06, 0.70), (0.12, 0, 0.35), 'MAT_SIGNAGE'), 'PanelB')
        return scene
    return shelf_or_cabinet(name, mat)


def ladder(name, mat):
    scene = trimesh.Scene()
    h = 1.65 if 'Portable' in name else 1.20
    w = 0.48
    for i, x in enumerate((-w / 2, w / 2)):
        add(scene, box((0.045, 0.05, h), (x, 0, h / 2), mat), f'Rail_{i}')
    for i, z in enumerate(np.linspace(0.20, h - 0.18, 6)):
        add(scene, box((w, 0.12, 0.045), (0, 0, z), mat), f'Step_{i}')
    return scene


def dock_or_pallet(name, mat):
    scene = trimesh.Scene()
    if 'Pallet' in name:
        for i, y in enumerate((-0.38, 0, 0.38)):
            add(scene, box((1.10, 0.16, 0.08), (0, y, 0.18), 'MAT_WOOD_WARM'), f'Slat_{i}')
        for i, x in enumerate((-0.43, 0, 0.43)):
            add(scene, box((0.14, 0.92, 0.14), (x, 0, 0.07), 'MAT_WOOD_DARK'), f'Runner_{i}')
    elif 'Ramp' in name:
        ramp = box((1.40, 1.80, 0.12), (0, 0, 0.30), mat)
        ramp.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-12), [1, 0, 0], [0, 0, 0.30]))
        add(scene, ramp, 'Ramp')
        add(scene, box((1.40, 0.12, 0.18), (0, -0.88, 0.09), 'MAT_BLACKENED_STEEL'), 'RampFoot')
    elif 'Bumper' in name:
        add(scene, box((0.50, 0.22, 0.55), (0, 0, 0.275), 'MAT_PLASTIC_RUBBER'), 'Bumper')
    return scene


def security(name, mat):
    scene = trimesh.Scene()
    if 'Camera Dome' in name:
        add(scene, cyl(0.16, 0.08, (0, 0, 0.04), 'MAT_STAINLESS', 24), 'Mount')
        add(scene, cyl(0.12, 0.12, (0, 0, 0.12), 'MAT_GLASS_CLEAR', 24), 'Dome')
        return scene
    if 'Camera Bullet' in name:
        body = cyl(0.10, 0.42, (0, 0, 0.22), 'MAT_STAINLESS', 18)
        body.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0], [0, 0, 0.22]))
        add(scene, body, 'CameraBody')
        add(scene, box((0.04, 0.04, 0.35), (0, 0.12, 0.175), 'MAT_BLACKENED_STEEL'), 'Bracket')
        return scene
    if 'Monitor Desk' in name:
        add(scene, box((1.30, 0.65, 0.72), (0, 0, 0.36), 'MAT_WOOD_WARM'), 'Desk')
        for i, x in enumerate((-0.38, 0, 0.38)):
            add(scene, box((0.32, 0.04, 0.26), (x, -0.24, 0.92), 'MAT_ELECTRONICS'), f'Monitor_{i}')
            add(scene, box((0.05, 0.08, 0.18), (x, -0.18, 0.79), 'MAT_BLACKENED_STEEL'), f'MonitorStand_{i}')
        return scene
    if 'Time Clock' in name:
        add(scene, box((0.38, 0.20, 0.55), (0, 0, 0.275), 'MAT_ELECTRONICS'), 'TimeClockBody')
        add(scene, box((0.24, 0.025, 0.16), (0, -0.115, 0.39), 'MAT_GLASS_CLEAR'), 'Screen')
        return scene
    if 'First Aid' in name:
        add(scene, box((0.55, 0.20, 0.75), (0, 0, 0.375), mat), 'Cabinet')
        add(scene, box((0.08, 0.025, 0.38), (0, -0.115, 0.39), 'MAT_SIGNAGE'), 'CrossVertical')
        add(scene, box((0.38, 0.025, 0.08), (0, -0.115, 0.39), 'MAT_SIGNAGE'), 'CrossHorizontal')
        return scene
    if 'Fire Extinguisher' in name:
        add(scene, cyl(0.12, 0.55, (0, 0, 0.275), mat, 20), 'Cylinder')
        add(scene, box((0.18, 0.06, 0.08), (0, 0, 0.58), 'MAT_BLACKENED_STEEL'), 'Valve')
        return scene
    return appliance(name, mat)


def gym(name, mat):
    scene = trimesh.Scene()
    if 'Weight Bench' in name:
        add(scene, box((1.25, 0.38, 0.14), (0, 0, 0.48), 'MAT_UPHOLSTERY'), 'Bench')
        for i, x in enumerate((-0.48, 0.48)):
            add(scene, box((0.08, 0.32, 0.46), (x, 0, 0.23), 'MAT_BLACKENED_STEEL'), f'Leg_{i}')
        return scene
    if 'Treadmill' in name:
        add(scene, box((1.35, 0.55, 0.12), (0, 0, 0.06), 'MAT_BLACKENED_STEEL'), 'Deck')
        for i, x in enumerate((-0.20, 0.20)):
            add(scene, box((0.05, 0.05, 1.02), (x, 0.22, 0.57), 'MAT_BLACKENED_STEEL'), f'Handrail_{i}')
        add(scene, box((0.45, 0.12, 0.28), (0, 0.22, 1.12), 'MAT_ELECTRONICS'), 'Console')
        return scene
    if 'Stationary Bike' in name:
        add(scene, cyl(0.34, 0.06, (0, 0, 0.34), 'MAT_BLACKENED_STEEL', 24), 'MOV_Flywheel')
        add(scene, box((0.06, 0.06, 0.80), (0, 0, 0.68), 'MAT_BLACKENED_STEEL'), 'Frame')
        add(scene, box((0.24, 0.20, 0.08), (0, 0, 1.10), 'MAT_UPHOLSTERY'), 'Seat')
        add(scene, box((0.62, 0.05, 0.05), (0, 0, 1.18), 'MAT_BLACKENED_STEEL'), 'Handlebar')
        add(scene, box((0.30, 0.08, 0.035), (-0.18, 0, 0.34), 'MAT_BLACKENED_STEEL'), 'Pedal_Left')
        add(scene, box((0.30, 0.08, 0.035), (0.18, 0, 0.34), 'MAT_BLACKENED_STEEL'), 'Pedal_Right')
        return scene
    if 'Elliptical' in name:
        add(scene, cyl(0.26, 0.06, (0, 0, 0.26), 'MAT_BLACKENED_STEEL', 24), 'MOV_Flywheel')
        for i, x in enumerate((-0.20, 0.20)):
            add(scene, box((0.05, 0.05, 1.30), (x, 0, 0.70), 'MAT_BLACKENED_STEEL'), f'Handle_{i}')
        add(scene, box((0.60, 0.16, 0.05), (-0.22, -0.10, 0.18), 'MAT_BLACKENED_STEEL'), 'Pedal_Left')
        add(scene, box((0.60, 0.16, 0.05), (0.22, 0.10, 0.18), 'MAT_BLACKENED_STEEL'), 'Pedal_Right')
        return scene
    if 'Rowing' in name:
        add(scene, box((1.75, 0.10, 0.10), (0, 0, 0.10), 'MAT_BLACKENED_STEEL'), 'Rail')
        add(scene, box((0.36, 0.26, 0.10), (-0.40, 0, 0.26), 'MAT_UPHOLSTERY'), 'Seat')
        add(scene, cyl(0.28, 0.10, (0.68, 0, 0.28), 'MAT_BLACKENED_STEEL', 24), 'MOV_Flywheel')
        add(scene, box((0.45, 0.05, 0.05), (0.20, 0, 0.58), 'MAT_BLACKENED_STEEL'), 'PullHandle')
        return scene
    return scene


def build_asset(name: str, mat: str):
    if 'Host Stand' in name:
        return host_stand(mat)
    if 'Chair' in name or 'Stool' in name or name.endswith('Bench'):
        return chair(name, mat)
    if any(k in name for k in ('Table', 'Workbench')):
        return table_like(name, mat)
    if any(k in name for k in ('Counter', 'Prep Station', 'Service Station', 'Buffet', 'Hot Well', 'Cold Well', 'Ice Bin', 'Sneeze Guard')):
        return counter(name, mat)
    if any(k in name for k in ('Refrigerator', 'Freezer', 'Range', 'Oven', 'Griddle', 'Fryer', 'Salamander', 'Dishwasher', 'Sink', 'Warmer Cabinet', 'Hot Box', 'Washer', 'Dryer', 'Ironing Press')):
        return appliance(name, mat)
    if any(k in name for k in ('Dispenser', 'Display Case', 'Wine Rack', 'Bottle Display', 'Beer Tap', 'Coffee Urn')):
        return display_or_dispenser(name, mat)
    if any(k in name for k in ('Shelf', 'Cabinet', 'Rack', 'Locker')):
        return shelf_or_cabinet(name, mat)
    if any(k in name for k in ('Cart', 'Trolley', 'Dolly', 'Hand Truck')):
        return cart(name, mat)
    if any(k in name for k in ('Vacuum', 'Mop Bucket', 'Caddy', 'Hamper', 'Bin', 'Floor Buffer', 'Wet Floor Sign', 'Toolbox')):
        return small_cleaning(name, mat)
    if 'Ladder' in name:
        return ladder(name, mat)
    if any(k in name for k in ('Loading Dock', 'Pallet')):
        return dock_or_pallet(name, mat)
    if any(k in name for k in ('Security', 'Time Clock', 'First Aid', 'Fire Extinguisher')):
        return security(name, mat)
    if any(k in name for k in ('Treadmill', 'Elliptical', 'Stationary Bike', 'Rowing Machine', 'Weight Bench')):
        return gym(name, mat)
    scene = trimesh.Scene()
    add(scene, box((0.65, 0.45, 0.72), (0, 0, 0.36), mat), 'Body')
    return scene
