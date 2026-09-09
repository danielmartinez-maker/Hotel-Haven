from __future__ import annotations

import numpy as np
import trimesh

from service_asset_factory import add, box, chair, cyl, material, shelf_or_cabinet


def sph(radius, center=(0, 0, 0), mat='MAT_VEGETATION'):
    mesh = trimesh.creation.icosphere(subdivisions=2, radius=radius)
    mesh.apply_translation(center)
    mesh.visual = trimesh.visual.TextureVisuals(material=material(mat))
    return mesh


def rack(name, mat):
    scene = trimesh.Scene()
    add(scene, box((1.0, .42, .08), (0, 0, .04), mat), 'Base')
    for i, x in enumerate((-.44, .44)):
        add(scene, box((.05, .05, 1.4), (x, 0, .70), mat), f'Upright_{i}')
    for i, z in enumerate((.34, .68, 1.02, 1.34)):
        add(scene, box((.90, .36, .035), (0, 0, z), mat), f'Shelf_{i}')

    if 'Dumbbell' in name:
        for row, z in enumerate((.42, .76, 1.10)):
            for i, x in enumerate(np.linspace(-.33, .33, 4)):
                add(scene, cyl(.035, .24, (x, 0, z), 'MAT_BLACKENED_STEEL', 12), f'Weight_{row}_{i}')
    elif 'Yoga Mat' in name:
        for i, z in enumerate((.39, .73, 1.07)):
            add(scene, box((.72, .20, .11), (0, 0, z), 'MAT_UPHOLSTERY'), f'YogaMat_{i}')
        add(scene, box((.78, .05, .05), (0, -.19, 1.40), 'MAT_BRASS_POLISHED'), 'YogaRackAccent')
    elif 'Exercise Ball' in name:
        scene = trimesh.Scene()
        add(scene, box((.82, .48, .07), (0, 0, .035), mat), 'BallRackBase')
        for i, (x, z) in enumerate(((-.28, .35), (.28, .35), (-.28, .88), (.28, .88))):
            add(scene, sph(.22, (x, 0, z), 'MAT_UPHOLSTERY'), f'Ball_{i}')
        for i, x in enumerate((-.34, .34)):
            add(scene, box((.045, .045, 1.12), (x, 0, .56), mat), f'BallRackUpright_{i}')
    elif 'Product Shelf' in name:
        for row, z in enumerate((.42, .76, 1.10)):
            for i, x in enumerate((-.29, -.10, .10, .29)):
                add(scene, box((.12, .10, .18), (x, -.06, z), 'MAT_CERAMIC_FIXTURE'), f'Product_{row}_{i}')
    return scene


def mirror_wall():
    scene = trimesh.Scene()
    add(scene, box((2.4, .04, 1.8), (0, 0, .9), 'MAT_GLASS_CLEAR'), 'Mirror')
    add(scene, box((2.5, .08, .07), (0, 0, 1.82), 'MAT_BLACKENED_STEEL'), 'TopFrame')
    add(scene, box((2.5, .08, .07), (0, 0, .035), 'MAT_BLACKENED_STEEL'), 'BottomFrame')
    return scene


def water_station():
    scene = trimesh.Scene()
    add(scene, box((.52, .42, 1.02), (0, 0, .51), 'MAT_ELECTRONICS'), 'Body')
    add(scene, cyl(.16, .34, (0, 0, 1.20), 'MAT_GLASS_CLEAR', 20), 'Bottle')
    add(scene, box((.22, .04, .12), (0, -.23, .76), 'MAT_STAINLESS'), 'DispensePanel')
    return scene


def lounger(name, mat):
    scene = trimesh.Scene()
    w, d = .68, 1.78

    if 'Treatment Chair' in name:
        add(scene, cyl(.25, .16, (0, 0, .08), 'MAT_STAINLESS', 24), 'Pedestal')
        add(scene, box((.62, .82, .15), (0, -.12, .55), mat), 'Seat')
        back = box((.62, .13, .78), (0, .39, .91), mat)
        back.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-16), [1, 0, 0], [0, .39, .91]))
        add(scene, back, 'Back')
        footrest = box((.54, .52, .10), (0, -.70, .39), mat)
        footrest.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(10), [1, 0, 0], [0, -.70, .39]))
        add(scene, footrest, 'Footrest')
        add(scene, box((.08, .66, .08), (0, -.47, .29), 'MAT_STAINLESS'), 'FootrestSupport')
        return scene

    add(scene, box((w, d, .14), (0, 0, .42), mat), 'Seat')
    back = box((w, .12, .76), (0, .72, .80), mat)
    back.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-18), [1, 0, 0], [0, .72, .80]))
    add(scene, back, 'Back')

    if 'Pool Lounge' in name:
        add(scene, box((.74, 1.66, .065), (0, 0, .19), 'MAT_BLACKENED_STEEL'), 'PoolFrame')
        for i, (x, y) in enumerate(((-.30, -.70), (.30, -.70), (-.30, .70), (.30, .70))):
            add(scene, box((.045, .045, .19), (x, y, .095), 'MAT_BLACKENED_STEEL'), f'PoolLeg_{i}')
    else:
        add(scene, box((.54, 1.26, .12), (0, -.05, .14), 'MAT_WOOD_DARK'), 'LoungerBase')
    return scene


def massage_table():
    scene = trimesh.Scene()
    add(scene, box((.72, 1.85, .15), (0, 0, .72), 'MAT_UPHOLSTERY'), 'Pad')
    for i, (x, y) in enumerate(((-.28, -.72), (.28, -.72), (-.28, .72), (.28, .72))):
        add(scene, box((.055, .055, .70), (x, y, .35), 'MAT_WOOD_DARK'), f'Leg_{i}')
    add(scene, cyl(.10, .05, (0, .82, .82), 'MAT_UPHOLSTERY', 18), 'FaceCradle')
    return scene


def bench(name, mat):
    scene = trimesh.Scene()
    width = 1.45
    add(scene, box((width, .52, .13), (0, 0, .47), mat), 'Seat')
    add(scene, box((width, .10, .55), (0, .21, .74), mat), 'Back')
    for i, x in enumerate((-.58, .58)):
        add(scene, box((.08, .38, .42), (x, 0, .21), mat), f'BenchSupport_{i}')
    if 'Sauna' in name:
        for i, x in enumerate(np.linspace(-.58, .58, 5)):
            add(scene, box((.055, .44, .05), (x, 0, .55), 'MAT_WOOD_WARM'), f'SaunaSlat_{i}')
    return scene


def conference_table(name, mat):
    scene = trimesh.Scene()
    width, depth, height = (2.8, 1.15, .76) if 'Large' in name else (1.7, .92, .76)
    if 'Round' in name:
        add(scene, cyl(.72, .08, (0, 0, height), 'MAT_WOOD_WARM', 28), 'Top')
        add(scene, cyl(.14, height, (0, 0, height / 2), 'MAT_BLACKENED_STEEL', 16), 'Pedestal')
        return scene
    add(scene, box((width, depth, .08), (0, 0, height), mat), 'Top')
    for i, x in enumerate((-width * .38, width * .38)):
        add(scene, box((.08, .55, .70), (x, 0, .35), 'MAT_BLACKENED_STEEL'), f'Leg_{i}')
    if 'Game Table' in name:
        add(scene, box((.72, .52, .015), (0, 0, height + .05), 'MAT_SIGNAGE'), 'GameSurface')
    return scene


def podium(name='Conference Podium'):
    scene = trimesh.Scene()
    add(scene, box((.66, .48, 1.08), (0, 0, .54), 'MAT_WOOD_WARM'), 'PodiumBody')
    add(scene, box((.72, .54, .08), (0, -.03, 1.12), 'MAT_WOOD_WARM'), 'PodiumTop')
    if 'Valet' in name:
        add(scene, box((.32, .035, .20), (0, -.29, .84), 'MAT_SIGNAGE'), 'ValetSign')
        for i, x in enumerate((-.18, 0, .18)):
            add(scene, cyl(.012, .08, (x, -.29, .62), 'MAT_BRASS_POLISHED', 10), f'KeyHook_{i}')
    return scene


def screen_or_board(name, mat):
    scene = trimesh.Scene()
    panel_mat = mat if 'Whiteboard' in name else 'MAT_ELECTRONICS'
    add(scene, box((1.9, .055, 1.15), (0, 0, .92), panel_mat), 'Panel')
    if 'Presentation Screen' in name:
        add(scene, box((2.02, .08, .08), (0, 0, 1.53), 'MAT_BLACKENED_STEEL'), 'ScreenHousing')
    else:
        add(scene, box((1.92, .09, .06), (0, 0, .31), 'MAT_BLACKENED_STEEL'), 'MarkerTray')
    return scene


def stage_or_floor(name, mat):
    scene = trimesh.Scene()
    height = .20 if 'Stage' in name else .06
    add(scene, box((1.8, 1.8, height), (0, 0, height / 2), mat), 'Module')
    if 'Stage' in name:
        add(scene, box((1.72, .05, .08), (0, -.88, .14), 'MAT_BLACKENED_STEEL'), 'StageTrim')
    else:
        for i, x in enumerate((-.45, .45)):
            add(scene, box((.02, 1.76, .01), (x, 0, .065), 'MAT_WOOD_DARK'), f'FloorJoint_{i}')
    return scene


def divider():
    scene = trimesh.Scene()
    for i, x in enumerate((-.62, 0, .62)):
        add(scene, box((.58, .08, 1.82), (x, 0, .91), 'MAT_WOOD_WARM'), f'Panel_{i}')
    for i, x in enumerate((-.70, .70)):
        add(scene, box((.10, .42, .06), (x, 0, .03), 'MAT_BLACKENED_STEEL'), f'Foot_{i}')
    return scene


def av_rack():
    scene = shelf_or_cabinet('AV Equipment Rack', 'MAT_BLACKENED_STEEL')
    for i, z in enumerate((.42, .62, .82, 1.02)):
        add(scene, box((.46, .025, .09), (0, -.23, z), 'MAT_ELECTRONICS'), f'AVUnit_{i}')
    return scene


def game_table():
    return conference_table('Game Table', 'MAT_WOOD_WARM')


def framed_art(name, mat):
    scene = trimesh.Scene()
    add(scene, box((.82, .055, .62), (0, 0, .31), 'MAT_WOOD_DARK'), 'Frame')
    add(scene, box((.70, .018, .50), (0, -.035, .31), mat), 'ArtPanel')
    if 'Landscape' in name:
        add(scene, box((.62, .010, .025), (0, -.052, .27), 'MAT_SIGNAGE'), 'LandscapeHorizon')
    elif 'Abstract' in name:
        for i, (x, z) in enumerate(((-.18, .23), (.02, .37), (.19, .19))):
            add(scene, box((.14, .010, .12), (x, -.052, z), 'MAT_BRASS_POLISHED'), f'AbstractBlock_{i}')
    elif 'Architectural' in name:
        for i, x in enumerate((-.20, 0, .20)):
            add(scene, box((.018, .010, .34), (x, -.052, .31), 'MAT_SIGNAGE'), f'ArchitecturalLine_{i}')
    elif 'Botanical' in name:
        for i, x in enumerate((-.16, 0, .16)):
            add(scene, box((.10, .010, .20), (x, -.052, .31 + .04 * (i - 1)), 'MAT_VEGETATION'), f'BotanicalLeaf_{i}')
    elif 'Local Culture' in name:
        for i, z in enumerate((.18, .30, .42)):
            add(scene, box((.52 - .10 * i, .010, .035), (0, -.052, z), 'MAT_BRASS_POLISHED'), f'CultureMotif_{i}')
    return scene


def sculpture(name, mat):
    scene = trimesh.Scene()
    add(scene, box((.38, .38, .16), (0, 0, .08), 'MAT_STONE_LIGHT'), 'Base')
    add(scene, sph(.23, (0, 0, .42), mat), 'Form')
    if 'Brass' in name:
        add(scene, cyl(.06, .42, (0, 0, .34), 'MAT_BRASS_POLISHED', 14), 'BrassSpine')
    return scene


def vase(name, mat):
    scene = trimesh.Scene()
    height = .62 if 'Tall' in name else .34
    add(scene, cyl(.16, height, (0, 0, height / 2), mat, 24), 'Vase')
    add(scene, cyl(.09, .06, (0, 0, height + .03), mat, 20), 'VaseNeck')
    return scene


def books_or_magazines(name):
    scene = trimesh.Scene()
    for i in range(4):
        width = .34 + .02 * i
        add(scene, box((width, .24, .045), (0, 0, .025 + .05 * i), 'MAT_SIGNAGE'), f'Item_{i}')
    return scene


def candles():
    scene = trimesh.Scene()
    for i, (x, height) in enumerate(((-.14, .28), (0, .38), (.14, .22))):
        add(scene, cyl(.045, height, (x, 0, height / 2), 'MAT_EMISSIVE_WARM', 16), f'Candle_{i}')
    return scene


def clock(name, mat):
    scene = trimesh.Scene()
    radius = .28 if 'Wall' in name else .16
    add(scene, cyl(radius, .045, (0, 0, radius), mat, 28), 'ClockBody')
    if 'Classic' in name:
        add(scene, cyl(radius + .035, .022, (0, .005, radius), 'MAT_BRASS_POLISHED', 28), 'ClassicRim')
        for i, angle in enumerate(np.linspace(0, 2 * np.pi, 12, endpoint=False)):
            x = .20 * np.cos(angle)
            z = radius + .20 * np.sin(angle)
            add(scene, box((.018, .010, .035), (x, -.032, z), 'MAT_BRASS_POLISHED'), f'HourMarker_{i}')
    elif 'Modern' in name:
        add(scene, box((.018, .010, .19), (0, -.032, radius + .065), 'MAT_BRASS_POLISHED'), 'MinuteHand')
        add(scene, box((.14, .010, .018), (.045, -.032, radius), 'MAT_BRASS_POLISHED'), 'HourHand')
    else:
        add(scene, box((.20, .12, .05), (0, 0, .025), 'MAT_WOOD_DARK'), 'TableClockBase')
    return scene


def plant(name):
    scene = trimesh.Scene()
    scale = .45 if 'Small' in name else .72 if 'Medium' in name else 1.05
    add(scene, cyl(.18 * scale, .30 * scale, (0, 0, .15 * scale), 'MAT_STONE_LIGHT', 20), 'Pot')
    add(scene, cyl(.035 * scale, .65 * scale, (0, 0, .50 * scale), 'MAT_WOOD_DARK', 12), 'Stem')
    for i, angle in enumerate(np.linspace(0, 2 * np.pi, 7, endpoint=False)):
        add(scene, sph(.16 * scale, (.18 * scale * np.cos(angle), .18 * scale * np.sin(angle), .78 * scale), 'MAT_VEGETATION'), f'Leaf_{i}')
    return scene


def flower_arrangement(name):
    scene = plant('Potted Plant Medium' if 'Table' in name else 'Potted Plant Large')
    for i, angle in enumerate(np.linspace(0, 2 * np.pi, 5, endpoint=False)):
        add(scene, sph(.06, (.16 * np.cos(angle), .16 * np.sin(angle), .70), 'MAT_EMISSIVE_WARM'), f'Flower_{i}')
    return scene


def sign(name, mat):
    scene = trimesh.Scene()
    width, height = (.62, .72) if 'Directory' in name else (.46, .22)

    if 'Menu Stand' in name:
        add(scene, box((.42, .035, .56), (0, 0, .98), mat), 'SignFace')
        add(scene, box((.05, .05, .72), (0, 0, .38), 'MAT_BLACKENED_STEEL'), 'StandPost')
        add(scene, box((.38, .28, .05), (0, 0, .025), 'MAT_BLACKENED_STEEL'), 'StandBase')
        return scene

    if 'Menu Board' in name:
        width, height = .80, .56
    add(scene, box((width, .035, height), (0, 0, height / 2), mat), 'SignFace')

    if 'Ceiling' in name:
        add(scene, box((.36, .06, .06), (0, 0, height + .15), 'MAT_BLACKENED_STEEL'), 'CeilingMount')
        for i, x in enumerate((-.14, .14)):
            add(scene, box((.018, .018, .18), (x, 0, height + .06), 'MAT_BLACKENED_STEEL'), f'CeilingStem_{i}')
    if 'Emergency Exit' in name:
        add(scene, box((width * .78, .012, height * .42), (0, -.025, height * .54), 'MAT_EMISSIVE_WARM'), 'ExitGlow')
    if 'No Smoking' in name:
        bar = box((width * .72, .012, .025), (0, -.025, height / 2), 'MAT_EMISSIVE_WARM')
        bar.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(35), [0, 1, 0], [0, -.025, height / 2]))
        add(scene, bar, 'ProhibitionBar')
    if 'Staff Only' in name:
        add(scene, box((width * .68, .012, .028), (0, -.025, height * .28), 'MAT_BRASS_POLISHED'), 'StaffStripe')
    if 'Room Number Plaque Brass' in name:
        add(scene, box((width + .05, .055, height + .05), (0, .01, height / 2), 'MAT_BRASS_POLISHED'), 'PlaqueFrame')
    if 'Room Number Plaque Modern' in name:
        add(scene, box((.035, .012, height * .66), (-width * .34, -.025, height / 2), 'MAT_BRASS_POLISHED'), 'ModernAccent')
    if 'Directional' in name:
        add(scene, box((width * .22, .012, .035), (width * .22, -.025, height / 2), 'MAT_BRASS_POLISHED'), 'DirectionArrow')
    return scene


def cup_or_glass(name, mat):
    scene = trimesh.Scene()
    height = .10 if 'Cup' in name else .14
    add(scene, cyl(.05, height, (0, 0, height / 2), mat, 18), 'Vessel')
    if 'Wine Glass' in name:
        add(scene, cyl(.012, .10, (0, 0, .05), mat, 10), 'Stem')
        add(scene, cyl(.045, .012, (0, 0, .006), mat, 16), 'Foot')
    elif 'Coffee Cup' in name:
        add(scene, box((.045, .018, .055), (.055, 0, .055), mat), 'Handle')
    return scene


def tray_or_bowl(name, mat):
    scene = trimesh.Scene()
    add(scene, cyl(.18, .035, (0, 0, .02), mat, 24), 'Base')
    if 'Fruit Bowl' in name:
        for i, x in enumerate((-.08, 0, .08)):
            add(scene, sph(.045, (x, 0, .08), 'MAT_VEGETATION'), f'Fruit_{i}')
    return scene


def towel_stack():
    scene = trimesh.Scene()
    for i in range(4):
        add(scene, box((.34, .24, .055), (0, 0, .03 + .06 * i), 'MAT_LINEN'), f'Towel_{i}')
    return scene


def bottle_set():
    scene = trimesh.Scene()
    for i, x in enumerate((-.12, 0, .12)):
        add(scene, cyl(.035, .16, (x, 0, .08), 'MAT_PLASTIC_RUBBER', 14), f'Bottle_{i}')
        add(scene, box((.045, .045, .025), (x, 0, .172), 'MAT_SIGNAGE'), f'Cap_{i}')
    return scene


def exterior_module(name, mat):
    scene = trimesh.Scene()
    if 'Entrance Module' in name:
        add(scene, box((3.2, .30, 3.0), (0, 0, 1.5), mat), 'Facade')
        add(scene, box((1.7, .08, 2.35), (0, -.19, 1.18), 'MAT_GLASS_CLEAR'), 'EntranceGlass')
        add(scene, box((2.0, .68, .10), (0, -.44, 2.55), 'MAT_BLACKENED_STEEL'), 'EntranceCanopy')
        return scene
    if 'Window Bay' in name:
        add(scene, box((2.6, .30, 3.0), (0, 0, 1.5), mat), 'Facade')
        for i, x in enumerate((-.75, 0, .75)):
            add(scene, box((.52, .05, 1.35), (x, -.18, 1.55), 'MAT_GLASS_CLEAR'), f'Window_{i}')
        return scene
    if 'Balcony' in name:
        add(scene, box((2.6, .30, 3.0), (0, 0, 1.5), mat), 'Facade')
        add(scene, box((1.6, .75, .12), (0, -.48, 1.18), 'MAT_STONE_LIGHT'), 'BalconySlab')
        add(scene, box((1.55, .05, .08), (0, -.82, 1.78), 'MAT_BLACKENED_STEEL'), 'Balustrade')
        for i, x in enumerate(np.linspace(-.70, .70, 5)):
            add(scene, box((.025, .025, .58), (x, -.82, 1.48), 'MAT_BLACKENED_STEEL'), f'Baluster_{i}')
        return scene
    if 'Corner Tower' in name:
        add(scene, box((1.8, 1.8, 3.2), (0, 0, 1.6), mat), 'Tower')
        add(scene, box((1.95, 1.95, .16), (0, 0, 3.28), 'MAT_STONE_LIGHT'), 'TowerCap')
        return scene
    if 'Steps' in name:
        for i in range(4):
            add(scene, box((1.8, 1.2 - .22 * i, .16), (0, .12 * i, .08 + .16 * i), mat), f'Step_{i}')
        return scene
    if 'Ramp' in name:
        ramp = box((1.4, 2.8, .14), (0, 0, .32), mat)
        ramp.apply_transform(trimesh.transformations.rotation_matrix(np.deg2rad(-8), [1, 0, 0], [0, 0, .32]))
        add(scene, ramp, 'Ramp')
        for i, x in enumerate((-.64, .64)):
            add(scene, box((.035, 2.7, .68), (x, 0, .34), 'MAT_BLACKENED_STEEL'), f'RampRail_{i}')
        return scene
    if 'Paver Module' in name:
        add(scene, box((1.0, 1.0, .08), (0, 0, .04), mat), 'Paver')
        for i, x in enumerate((-.25, .25)):
            add(scene, box((.012, .98, .008), (x, 0, .082), 'MAT_STONE_LIGHT'), f'PaverJoint_{i}')
        return scene
    if 'Road Curb' in name:
        add(scene, box((1.6, .34, .22), (0, 0, .11), mat), 'Curb')
        return scene
    add(scene, box((1.0, 1.0, .08), (0, 0, .04), mat), 'Module')
    return scene


def street_item(name, mat):
    scene = trimesh.Scene()
    if 'Street Lamp' in name:
        add(scene, cyl(.06, 2.7, (0, 0, 1.35), mat, 16), 'Pole')
        add(scene, sph(.16, (0, 0, 2.62), 'MAT_EMISSIVE_WARM'), 'Lamp')
        add(scene, cyl(.18, .08, (0, 0, .04), mat, 18), 'LampBase')
        return scene
    if 'Bollard' in name:
        add(scene, cyl(.10, .72, (0, 0, .36), mat, 18), 'Bollard')
        add(scene, cyl(.14, .06, (0, 0, .03), mat, 18), 'BollardBase')
        return scene
    if 'Bike Rack' in name:
        add(scene, box((1.25, .10, .08), (0, 0, .04), mat), 'RackBase')
        for i, x in enumerate(np.linspace(-.48, .48, 4)):
            add(scene, box((.035, .34, .58), (x, 0, .29), mat), f'RackLoop_{i}')
            add(scene, box((.18, .34, .035), (x, 0, .56), mat), f'RackLoopTop_{i}')
        return scene
    add(scene, box((1.1, .35, .75), (0, 0, .375), mat), 'StreetItem')
    return scene


def outdoor_seating(name, mat):
    if 'Bench' in name:
        scene = trimesh.Scene()
        add(scene, box((1.42, .48, .10), (0, 0, .48), mat), 'Seat')
        add(scene, box((1.42, .08, .54), (0, .20, .78), mat), 'Back')
        for i, x in enumerate((-.56, .56)):
            add(scene, box((.08, .38, .45), (x, 0, .225), 'MAT_BLACKENED_STEEL'), f'BenchLeg_{i}')
        return scene
    if 'Lounge Chair' in name:
        scene = lounger('Pool Lounge Chair', mat)
        add(scene, box((.76, .08, .08), (0, -.74, .25), 'MAT_BRASS_POLISHED'), 'OutdoorAccent')
        return scene
    return chair(name, mat)


def planter_exterior(name):
    scene = trimesh.Scene()
    if 'Rectangular' in name:
        add(scene, box((1.10, .46, .38), (0, 0, .19), 'MAT_STONE_LIGHT'), 'PlanterBox')
        for i, x in enumerate(np.linspace(-.40, .40, 5)):
            add(scene, sph(.16, (x, 0, .52 + .04 * (i % 2)), 'MAT_VEGETATION'), f'PlanterLeaf_{i}')
    else:
        add(scene, cyl(.34, .42, (0, 0, .21), 'MAT_STONE_LIGHT', 24), 'PlanterRound')
        add(scene, cyl(.045, .64, (0, 0, .65), 'MAT_WOOD_DARK', 12), 'PlanterStem')
        for i, angle in enumerate(np.linspace(0, 2 * np.pi, 7, endpoint=False)):
            add(scene, sph(.18, (.20 * np.cos(angle), .20 * np.sin(angle), .88), 'MAT_VEGETATION'), f'PlanterLeaf_{i}')
    return scene


def hedge(name):
    scene = trimesh.Scene()
    if 'Corner' in name:
        add(scene, box((1.25, .42, .75), (-.42, 0, .375), 'MAT_VEGETATION'), 'HedgeLeg_A')
        add(scene, box((.42, 1.25, .75), (.42, .42, .375), 'MAT_VEGETATION'), 'HedgeLeg_B')
    else:
        add(scene, box((1.6, .42, .75), (0, 0, .375), 'MAT_VEGETATION'), 'HedgeStraight')
    return scene


def tree(name):
    scene = trimesh.Scene()
    scale = .75 if 'Small' in name else 1.15
    add(scene, cyl(.10 * scale, 1.55 * scale, (0, 0, .78 * scale), 'MAT_WOOD_DARK', 14), 'Trunk')
    add(scene, sph(.55 * scale, (0, 0, 1.6 * scale), 'MAT_VEGETATION'), 'Canopy')
    for i, angle in enumerate(np.linspace(0, 2 * np.pi, 5, endpoint=False)):
        add(scene, sph(.24 * scale, (.36 * scale * np.cos(angle), .36 * scale * np.sin(angle), 1.48 * scale), 'MAT_VEGETATION'), f'CanopyLobe_{i}')
    return scene


def fountain(name):
    scene = trimesh.Scene()
    add(scene, cyl(.95, .20, (0, 0, .10), 'MAT_STONE_LIGHT', 32), 'Basin')
    add(scene, cyl(.26, .72, (0, 0, .46), 'MAT_STONE_LIGHT', 24), 'Pedestal')
    add(scene, cyl(.72, .025, (0, 0, .22), 'MAT_GLASS_CLEAR', 28), 'WaterSurface')
    add(scene, cyl(.045, .50, (0, 0, .92), 'MAT_GLASS_CLEAR', 12), 'WaterJet')
    return scene


def build_asset(name, mat):
    if any(key in name for key in ('Dumbbell Rack', 'Yoga Mat Rack', 'Exercise Ball Rack', 'Product Shelf')):
        return rack(name, mat)
    if 'Mirror Wall' in name:
        return mirror_wall()
    if 'Water Station' in name:
        return water_station()
    if 'Massage Table' in name:
        return massage_table()
    if any(key in name for key in ('Treatment Chair', 'Lounger', 'Pool Lounge')):
        return lounger(name, mat)
    if any(key in name for key in ('Sauna Bench', 'Steam Room Bench')):
        return bench(name, mat)
    if 'Towel Warmer' in name or 'Towel Station' in name:
        return shelf_or_cabinet(name, mat)
    if any(key in name for key in ('Conference Table', 'Banquet Table', 'Side Table')):
        return conference_table(name, mat)
    if 'Conference Chair' in name or 'Banquet Chair' in name:
        return chair(name, mat)
    if 'Podium' in name:
        return podium(name)
    if 'Presentation Screen' in name or 'Whiteboard' in name:
        return screen_or_board(name, mat)
    if 'Stage Module' in name or 'Dance Floor Module' in name:
        return stage_or_floor(name, mat)
    if 'Divider Panel' in name:
        return divider()
    if 'AV Equipment Rack' in name:
        return av_rack()
    if 'Game Table' in name:
        return game_table()
    if 'Framed Art' in name:
        return framed_art(name, mat)
    if 'Sculpture' in name:
        return sculpture(name, mat)
    if 'Vase' in name:
        return vase(name, mat)
    if any(key in name for key in ('Book Stack', 'Magazine Stack', 'Brochure Stack')):
        return books_or_magazines(name)
    if 'Candle' in name:
        return candles()
    if 'Clock' in name:
        return clock(name, mat)
    if 'Potted Plant' in name:
        return plant(name)
    if 'Flower Arrangement' in name:
        return flower_arrangement(name)
    if any(key in name for key in ('Sign', 'Plaque', 'Hanger', 'Door Tag', 'Menu Board')):
        return sign(name, mat)
    if 'Key Card' in name:
        scene = trimesh.Scene()
        add(scene, box((.086, .054, .003), (0, 0, .0015), mat), 'Card')
        add(scene, box((.018, .012, .001), (.024, -.028, .0035), 'MAT_BRASS_POLISHED'), 'CardChip')
        return scene
    if any(key in name for key in ('Coffee Cup', 'Water Glass', 'Wine Glass')):
        return cup_or_glass(name, mat)
    if 'Fruit Bowl' in name or 'Decorative Tray' in name:
        return tray_or_bowl(name, mat)
    if 'Folded Towel' in name:
        return towel_stack()
    if 'Toiletry Bottle' in name:
        return bottle_set()
    if 'Waste Basket' in name:
        scene = trimesh.Scene()
        add(scene, cyl(.16, .32, (0, 0, .16), mat, 18), 'Basket')
        add(scene, cyl(.17, .035, (0, 0, .31), 'MAT_BLACKENED_STEEL', 18), 'BasketRim')
        return scene
    if name.startswith('Hotel Facade') or any(key in name for key in ('Entrance Steps', 'Accessible Ramp', 'Paver Module', 'Road Curb')):
        return exterior_module(name, mat)
    if any(key in name for key in ('Street Lamp', 'Bollard', 'Bike Rack')):
        return street_item(name, mat)
    if any(key in name for key in ('Outdoor Bench', 'Outdoor Lounge Chair')):
        return outdoor_seating(name, mat)
    if 'Outdoor Cafe Table' in name:
        return conference_table(name, mat)
    if 'Outdoor Cafe Chair' in name:
        return chair(name, mat)
    if 'Planter Exterior' in name:
        return planter_exterior(name)
    if 'Hedge' in name:
        return hedge(name)
    if 'Ornamental Tree' in name:
        return tree(name)
    if 'Fountain Courtyard' in name:
        return fountain(name)
    if 'Water Feature Wall' in name:
        scene = trimesh.Scene()
        add(scene, box((1.6, .24, 2.1), (0, 0, 1.05), 'MAT_STONE_LIGHT'), 'Wall')
        add(scene, box((1.35, .025, 1.62), (0, -.14, 1.08), 'MAT_GLASS_CLEAR'), 'WaterPanel')
        add(scene, box((1.50, .40, .12), (0, -.10, .06), 'MAT_STONE_LIGHT'), 'CatchBasin')
        return scene

    scene = trimesh.Scene()
    add(scene, box((.55, .40, .55), (0, 0, .275), mat), 'Body')
    return scene
