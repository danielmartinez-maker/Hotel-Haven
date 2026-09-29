"""Purpose-built architecture, finish, amenity and exterior collections for A1501–A1750."""

import math

import trimesh

from v2_asset_common import add, box, cyl, sphere


def _legs(scene, width, depth, height, material='MAT_BLACKENED_STEEL', prefix='Leg'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.06, .06, height),
                       (x * width * .42, y * depth * .38, height / 2), material),
            f'{prefix}_{index}')


def _rail(scene, width, z, depth=.08, material='MAT_WOOD_WARM', name='Rail'):
    add(scene, box((width, depth, .09), (0, 0, z), material), name)


def _wheel(scene, x, y, z, name):
    center = (x, y, z)
    wheel = cyl(.10, .055, center, 'MAT_BLACKENED_STEEL', 16)
    wheel.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [1, 0, 0], point=center))
    add(scene, wheel, name)


def _arch(pieces, scene, radius, center_z, mat, prefix='ArchBlock', y=.24):
    for index, angle in enumerate(pieces):
        x = radius * math.cos(angle)
        z = center_z + radius * math.sin(angle)
        block = box((.30, .18, .22), (x, y, z), mat)
        tangent_angle = angle - math.pi / 2
        block.apply_transform(trimesh.transformations.rotation_matrix(
            tangent_angle, [0, 1, 0], point=(x, y, z)))
        add(scene, block, f'{prefix}_{index}')


def _pilaster(v, mat):
    scene = trimesh.Scene()
    height, width = 2.25 + .10 * v, .42 + .035 * v
    add(scene, box((width * 1.55, .48, .16), (0, 0, .08), 'MAT_STONE_LIGHT'), 'PilasterBase')
    add(scene, box((width, .32, height), (0, 0, height / 2 + .16), mat), 'PilasterShaft')
    add(scene, box((width * 1.45, .46, .18), (0, 0, height + .25), 'MAT_STONE_LIGHT'), 'PilasterCapital')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width * .68 / (2 + v)
        add(scene, box((.028, .035, height * .78), (x, -.175, height * .53), 'MAT_BRASS_POLISHED'), f'Flute_{i}')
    return scene


def _balustrade(v, mat):
    scene = trimesh.Scene()
    width, height = 1.20 + .12 * v, .92 + .045 * v
    add(scene, box((width, .15, .11), (0, 0, height), mat), 'TopHandrail')
    add(scene, box((width, .12, .10), (0, 0, .16), 'MAT_WOOD_WARM'), 'BottomRail')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        add(scene, box((.08 + .006 * v, .08, height - .22), (x, 0, height / 2), mat), f'Baluster_{i}')
    for side, x in enumerate((-width * .48, width * .48)):
        add(scene, box((.16, .18, height + .18), (x, 0, (height + .18) / 2), 'MAT_STONE_LIGHT'), f'NewelPost_{side}')
        add(scene, sphere(.10 + .01 * v, (x, 0, height + .28), 'MAT_BRASS_POLISHED', 1), f'NewelCap_{side}')
    return scene


def _arched_door(v, mat):
    scene = trimesh.Scene()
    width, height = 1.35 + .10 * v, 2.30 + .09 * v
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.18, .30, height), (x, 0, height / 2), mat), f'DoorJamb_{side}')
        add(scene, box((.24, .38, .10), (x, 0, .05), 'MAT_STONE_LIGHT'), f'JambFoot_{side}')
    add(scene, box((width + .42, .38, .22), (0, 0, height - .03), 'MAT_STONE_LIGHT'), 'DoorHeader')
    add(scene, box((width * .80, .045, height * .76), (0, .17, height * .43), 'MAT_WOOD_WARM'), 'DoorLeaf')
    segments = 7 + v
    angles = [math.pi * i / (segments - 1) for i in range(segments)]
    _arch(angles, scene, width * .47, height - .16, 'MAT_STONE_LIGHT', 'ArchVoussoir', y=.25)
    for i in range(2 + v // 2):
        z = .42 + i * .32
        add(scene, box((.045, .055, .08), (width * .37, .21, z), 'MAT_BRASS_POLISHED'), f'DoorHinge_{i}')
    return scene


def _window_bay(v, mat):
    scene = trimesh.Scene()
    width, height = 1.50 + .13 * v, 1.65 + .09 * v
    add(scene, box((width + .26, .22, .18), (0, 0, .09), 'MAT_STONE_LIGHT'), 'WindowSill')
    add(scene, box((width + .18, .20, .16), (0, 0, height + .08), mat), 'WindowLintel')
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.12, .18, height), (x, 0, height / 2 + .17), mat), f'WindowJamb_{side}')
    add(scene, box((width * .90, .04, height * .88), (0, .10, height / 2 + .16), 'MAT_GLASS_CLEAR'), 'Glazing')
    add(scene, box((.045, .08, height * .88), (0, .15, height / 2 + .16), 'MAT_WOOD_WARM'), 'CenterMullion')
    for i in range(1 + v // 2):
        z = .62 + i * .34
        add(scene, box((width * .86, .07, .045), (0, .15, z), 'MAT_WOOD_WARM'), f'WindowRail_{i}')
    return scene


def _wainscot(v, mat):
    scene = trimesh.Scene()
    width, height = 1.45 + .13 * v, 1.02 + .06 * v
    add(scene, box((width, .12, height), (0, 0, height / 2), mat), 'WainscotBacking')
    _rail(scene, width * 1.04, .12, .16, 'MAT_WOOD_WARM', 'BaseMolding')
    _rail(scene, width * 1.06, height + .10, .18, 'MAT_WOOD_WARM', 'ChairRail')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((width / (2 + v) * .82, .035, height * .72), (x, -.08, height * .53), mat), f'RecessedPanel_{i}')
        add(scene, box((.035, .04, height * .72), (x - width / (2 + v) * .43, -.11, height * .53), 'MAT_BRASS_POLISHED'), f'PanelBead_{i}')
    return scene


def _cornice(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.45 + .15 * v, .46 + .04 * v
    add(scene, box((width, depth, .12), (0, 0, .06), 'MAT_WOOD_WARM'), 'CorniceMount')
    add(scene, box((width, .12, .46 + .03 * v), (0, -depth * .36, .32 + .015 * v), mat), 'CorniceFace')
    add(scene, box((width * 1.04, depth, .11), (0, 0, .64 + .03 * v), 'MAT_STONE_LIGHT'), 'CeilingReturn')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        add(scene, box((.08, .035, .12), (x, -.19, .40), 'MAT_BRASS_POLISHED'), f'CorniceDentil_{i}')
    return scene


def _stair_landing(v, mat):
    scene = trimesh.Scene()
    width, run, rise = 1.20 + .12 * v, .92 + .09 * v, .18 + .018 * v
    count = 3 + v
    for i in range(count):
        y = -run * .38 + i * run * .76 / max(1, count - 1)
        z = rise * (i + 1) - rise / 2
        add(scene, box((width, run / count * 1.18, rise), (0, y, z), mat), f'StairTread_{i}')
        add(scene, box((width, .035, .035), (0, y - run / count * .45, z + rise / 2), 'MAT_STONE_LIGHT'), f'TreadNosing_{i}')
    add(scene, box((width * 1.10, .72, .16), (0, run * .42, rise * count - .08), 'MAT_STONE_LIGHT'), 'LandingPlatform')
    return scene


def _column_capital(v, mat):
    scene = trimesh.Scene()
    shaft = 2.15 + .09 * v
    add(scene, cyl(.32 + .025 * v, .16, (0, 0, .08), 'MAT_STONE_LIGHT', 24), 'ColumnBase')
    add(scene, cyl(.20 + .018 * v, shaft, (0, 0, .16 + shaft / 2), mat, 22), 'ColumnShaft')
    add(scene, cyl(.30 + .025 * v, .18, (0, 0, shaft + .25), 'MAT_STONE_LIGHT', 24), 'Abacus')
    add(scene, box((.82 + .08 * v, .62, .13), (0, 0, shaft + .40), mat), 'CapitalSlab')
    for i in range(3 + v):
        angle = i * math.pi * 2 / (3 + v)
        x, y = .26 * math.cos(angle), .26 * math.sin(angle)
        add(scene, sphere(.065 + .006 * v, (x, y, shaft + .31), 'MAT_BRASS_POLISHED', 1), f'CapitalRosette_{i}')
    return scene


def _wall_niche(v, mat):
    scene = trimesh.Scene()
    width, height = .84 + .08 * v, 1.70 + .10 * v
    add(scene, box((width, .10, height), (0, 0, height / 2), mat), 'NicheBack')
    for side, x in enumerate((-width * .48, width * .48)):
        add(scene, box((.10, .20, height), (x, .02, height / 2), 'MAT_WOOD_WARM'), f'NicheSide_{side}')
    for row, z in enumerate((.26, height * .48, height - .10)):
        add(scene, box((width * 1.04, .24, .11), (0, .02, z), 'MAT_STONE_LIGHT'), f'NicheShelf_{row}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, cyl(.055 + .01 * v, .22 + .03 * (i % 2), (x, -.08, .50 + .04 * v), 'MAT_CERAMIC_FIXTURE', 14), f'NicheVase_{i}')
    return scene


def _elevator_portal(v, mat):
    scene = trimesh.Scene()
    width, height = 1.65 + .12 * v, 2.45 + .08 * v
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.20, .32, height), (x, 0, height / 2), mat), f'PortalJamb_{side}')
        add(scene, box((.28, .38, .12), (x, 0, .06), 'MAT_STONE_LIGHT'), f'PortalFoot_{side}')
    add(scene, box((width + .28, .36, .22), (0, 0, height - .11), 'MAT_STONE_LIGHT'), 'PortalHeader')
    add(scene, box((width * .38, .05, height * .88), (0, .19, height * .48), 'MAT_BLACKENED_STEEL'), 'ElevatorDoors')
    add(scene, box((.08, .07, height * .86), (0, .23, height * .48), 'MAT_BRASS_POLISHED'), 'DoorReveal')
    add(scene, box((.12, .06, .38), (width * .60, .24, 1.48), 'MAT_ELECTRONICS'), 'CallPanel')
    for i in range(2 + v // 2):
        add(scene, sphere(.035, (width * .60, .28, 1.42 + .12 * i), 'MAT_EMISSIVE_WARM', 1), f'CallButton_{i}')
    return scene


def _architecture(family, v, mat):
    recipes = (_pilaster, _balustrade, _arched_door, _window_bay, _wainscot,
               _cornice, _stair_landing, _column_capital, _wall_niche, _elevator_portal)
    return recipes[family](v, mat)


def _lobby_armchair(v, mat):
    scene = trimesh.Scene()
    width = .76 + .065 * v
    add(scene, box((width, .68, .17), (0, 0, .47), mat), 'LoungeSeat')
    add(scene, box((width, .14, .72 + .045 * v), (0, .28, .88), mat), 'LoungeBack')
    for side, x in enumerate((-width * .55, width * .55)):
        add(scene, box((.11, .68, .39 + .02 * v), (x, 0, .65), mat), f'LoungeArm_{side}')
    _legs(scene, width, .68, .35, 'MAT_WOOD_WARM', 'ChairFoot')
    for i in range(1 + v // 2):
        add(scene, box((.17, .12, .14), ((i - v / 4) * .22, -.08, .61), 'MAT_LINEN'), f'LoungePillow_{i}')
    return scene


def _lobby_console(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.25 + .12 * v, .52 + .04 * v
    add(scene, box((width, depth, .10), (0, 0, .82), mat), 'ConsoleTop')
    add(scene, box((width * .78, depth * .66, .58), (0, 0, .43), mat), 'ConsoleBase')
    _legs(scene, width, depth, .32, 'MAT_BRASS_POLISHED', 'ConsoleLeg')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((.20, .025, .30), (x, depth / 2 + .02, .57), 'MAT_WOOD_WARM'), f'ConsoleDrawer_{i}')
        add(scene, box((.09, .03, .02), (x, depth / 2 + .045, .57), 'MAT_BRASS_POLISHED'), f'ConsolePull_{i}')
    add(scene, box((.36, .28, .03), (0, 0, .90), 'MAT_LINEN'), 'ConsoleRunner')
    return scene


def _coat_rack(v, mat):
    scene = trimesh.Scene()
    height = 1.72 + .10 * v
    add(scene, cyl(.28 + .025 * v, .07, (0, 0, .035), 'MAT_STONE_LIGHT', 22), 'RackBase')
    add(scene, cyl(.055, height - .08, (0, 0, height / 2), mat, 16), 'RackPost')
    add(scene, cyl(.28 + .035 * v, .07, (0, 0, height - .04), 'MAT_WOOD_WARM', 20), 'CrownHub')
    for i in range(4 + v):
        angle = i * 2 * math.pi / (4 + v)
        x, y = .35 * math.cos(angle), .35 * math.sin(angle)
        add(scene, box((.12, .07, .045), (x, y, height - .12), mat), f'CoatHook_{i}')
        add(scene, sphere(.045, (x * 1.22, y * 1.22, height - .06), 'MAT_BRASS_POLISHED', 1), f'HookCap_{i}')
    return scene


def _luggage_scale(v, mat):
    scene = trimesh.Scene()
    width, depth = .66 + .055 * v, .62 + .04 * v
    add(scene, box((width, depth, .12), (0, 0, .12), mat), 'WeighingPlatform')
    add(scene, box((width * .56, .055, 1.05 + .08 * v), (0, depth * .40, .68 + .04 * v), 'MAT_STAINLESS'), 'ScalePost')
    add(scene, cyl(.22 + .025 * v, .06, (0, depth * .40, 1.28 + .08 * v), 'MAT_WOOD_WARM', 22), 'DialHousing')
    add(scene, box((.04, .015, .16), (0, depth * .40 - .04, 1.30 + .08 * v), 'MAT_BRASS_POLISHED'), 'DialNeedle')
    add(scene, box((width * .72, .035, .07), (0, -depth * .48, .24), 'MAT_BLACKENED_STEEL'), 'ToeRail')
    return scene


def _umbrella_stand(v, mat):
    scene = trimesh.Scene()
    height, radius = .62 + .045 * v, .27 + .025 * v
    add(scene, cyl(radius + .06, .08, (0, 0, .04), 'MAT_STONE_LIGHT', 22), 'StandFoot')
    add(scene, cyl(radius, height, (0, 0, height / 2 + .08), mat, 20), 'UmbrellaReceptacle')
    add(scene, cyl(radius * .92, .06, (0, 0, height + .11), 'MAT_BRASS_POLISHED', 22), 'StandRim')
    for i in range(2 + v):
        angle = i * 2 * math.pi / (2 + v)
        x, y = radius * .38 * math.cos(angle), radius * .38 * math.sin(angle)
        add(scene, cyl(.022, .90 + .07 * v, (x, y, height + .55 + .035 * v), 'MAT_STAINLESS', 10), f'UmbrellaShaft_{i}')
        add(scene, sphere(.12 + .01 * v, (x, y, height + 1.03 + .035 * v), 'MAT_VEGETATION', 1), f'UmbrellaHandle_{i}')
    return scene


def _magazine_rack(v, mat):
    scene = trimesh.Scene()
    width, height = .72 + .06 * v, 1.12 + .08 * v
    add(scene, box((width, .08, height), (0, 0, height / 2 + .05), 'MAT_WOOD_WARM'), 'RackBack')
    for side, x in enumerate((-width * .48, width * .48)):
        add(scene, box((.06, .26, height), (x, 0, height / 2 + .05), mat), f'RackSide_{side}')
    for i in range(2 + v):
        z = .28 + i * (height - .20) / (1 + v)
        add(scene, box((width * .88, .34, .07), (0, .06, z), mat), f'MagazineShelf_{i}')
        for j in range(2 + v // 2):
            x = (j - (1 + v // 2) / 2) * width * .42 / (1 + v // 2)
            add(scene, box((.13, .04, .23), (x, .22, z + .15), 'MAT_SIGNAGE'), f'MagazineCover_{i}_{j}')
    return scene


def _fireplace(v, mat):
    scene = trimesh.Scene()
    width, height = 1.45 + .12 * v, 1.48 + .09 * v
    add(scene, box((width * 1.18, .48, .18), (0, 0, .09), 'MAT_STONE_LIGHT'), 'Hearth')
    add(scene, box((.20, .40, height), (-width * .42, 0, height / 2 + .18), mat), 'FireplacePier_L')
    add(scene, box((.20, .40, height), (width * .42, 0, height / 2 + .18), mat), 'FireplacePier_R')
    add(scene, box((width, .46, .22), (0, 0, height + .18), 'MAT_STONE_LIGHT'), 'Mantel')
    add(scene, box((width * .52, .08, height * .58), (0, -.12, height * .36), 'MAT_BLACKENED_STEEL'), 'Firebox')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * .22
        add(scene, cyl(.06, .30 + .035 * v, (x, -.18, .35), 'MAT_WOOD_WARM', 14), f'FireLog_{i}')
    return scene


def _lobby_cafe_table(v, mat):
    scene = trimesh.Scene()
    radius, height = .50 + .06 * v, .74
    add(scene, cyl(radius, .08, (0, 0, height), mat, 28), 'CafeTableTop')
    add(scene, cyl(.09 + .01 * v, .66, (0, 0, .40), 'MAT_BLACKENED_STEEL', 16), 'TableStem')
    add(scene, cyl(.32 + .025 * v, .08, (0, 0, .08), 'MAT_STONE_LIGHT', 24), 'TableBase')
    for i in range(2 + v // 2):
        angle = i * 2 * math.pi / (2 + v // 2)
        x, y = .28 * math.cos(angle), .28 * math.sin(angle)
        add(scene, sphere(.06, (x, y, height + .10), 'MAT_VEGETATION', 1), f'CafeFlower_{i}')
    return scene


def _parcel_shelf(v, mat):
    scene = trimesh.Scene()
    width, height = 1.0 + .10 * v, 1.62 + .10 * v
    add(scene, box((width, .40, .07), (0, 0, .035), 'MAT_BLACKENED_STEEL'), 'ShelfBase')
    for side, x in enumerate((-width * .46, width * .46)):
        add(scene, box((.06, .08, height), (x, 0, height / 2), mat), f'ShelfUpright_{side}')
    for level in range(2 + v):
        z = .30 + level * (height - .36) / (1 + v)
        add(scene, box((width * .92, .38, .06), (0, 0, z), mat), f'ParcelShelf_{level}')
        for i in range(2 + v // 2):
            x = (i - (1 + v // 2) / 2) * width * .54 / (1 + v // 2)
            add(scene, box((.18 + .01 * v, .20, .16 + .02 * (i % 2)), (x, -.02, z + .11), 'MAT_WOOD_WARM'), f'Parcel_{level}_{i}')
    return scene


def _retail_kiosk(v, mat):
    scene = trimesh.Scene()
    width, height = .82 + .08 * v, 1.55 + .10 * v
    add(scene, box((width, .52, .84), (0, 0, .44), mat), 'KioskCabinet')
    add(scene, box((width * 1.05, .56, .08), (0, 0, .90), 'MAT_WOOD_WARM'), 'KioskCounter')
    add(scene, box((width * .68, .08, .52 + .04 * v), (0, .17, height), 'MAT_ELECTRONICS'), 'KioskDisplay')
    add(scene, box((width * .58, .02, .42 + .03 * v), (0, .22, height), 'MAT_GLASS_CLEAR'), 'KioskScreen')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((.16, .18, .18), (x, -.12, 1.02), 'MAT_CERAMIC_FIXTURE'), f'GiftItem_{i}')
    return scene


def _public(family, v, mat):
    recipes = (_lobby_armchair, _lobby_console, _coat_rack, _luggage_scale, _umbrella_stand,
               _magazine_rack, _fireplace, _lobby_cafe_table, _parcel_shelf, _retail_kiosk)
    return recipes[family](v, mat)


def _floor_tile(v, mat):
    scene = trimesh.Scene()
    size = 1.10 + .08 * v
    add(scene, box((size, size, .06), (0, 0, .03), mat), 'TileSubstrate')
    count = 2 + v // 2
    for i in range(1, count):
        x = -size / 2 + i * size / count
        add(scene, box((.014, size, .012), (x, 0, .066), 'MAT_STONE_LIGHT'), f'GroutLine_X_{i}')
        add(scene, box((size, .014, .012), (0, x, .066), 'MAT_STONE_LIGHT'), f'GroutLine_Y_{i}')
    add(scene, box((size * .90, size * .90, .008), (0, 0, .071), 'MAT_BRASS_POLISHED'), 'TileInset')
    add(scene, box((size, .025, .012), (0, -size / 2 + .0125, .072), 'MAT_STONE_LIGHT'), 'TileEdgeBorder')
    return scene


def _carpet_tile(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.08 + .09 * v, .96 + .07 * v
    add(scene, box((width, depth, .055), (0, 0, .0275), mat), 'CarpetTileBacking')
    for i in range(3 + v):
        y = -depth * .40 + i * depth * .80 / (2 + v)
        add(scene, box((width * .94, .018, .018), (0, y, .064), 'MAT_LINEN'), f'PileRib_{i}')
    add(scene, box((.025, depth, .02), (0, 0, .065), 'MAT_BLACKENED_STEEL'), 'TileSeam')
    return scene


def _plank_floor(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.20 + .09 * v, .90 + .07 * v
    add(scene, box((width, depth, .05), (0, 0, .025), 'MAT_WOOD_WARM'), 'FloorUnderlay')
    for i in range(3 + v):
        plank_width = width / (3 + v)
        x = -width / 2 + plank_width * (i + .5)
        add(scene, box((plank_width * .98, depth * .98, .045), (x, 0, .07), mat), f'WoodPlank_{i}')
        for seam in range(1 + v // 2):
            y = -depth * .42 + seam * depth * .84 / (1 + v // 2)
            add(scene, box((.025, .035, .012), (x + ((-1) ** seam) * plank_width * .25, y, .097), 'MAT_BRASS_POLISHED'), f'PlankJoint_{i}_{seam}')
    return scene


def _parquet_panel(v, mat):
    scene = trimesh.Scene()
    size = .96 + .08 * v
    add(scene, box((size, size, .06), (0, 0, .03), 'MAT_WOOD_WARM'), 'ParquetBacking')
    for i in range(2 + v):
        apex_y = size * (.12 - .06 * i)
        length = size * (.55 - .065 * i)
        half_projection = length / (2 * math.sqrt(2))
        line_width = .022 + .002 * v
        left_center = (-half_projection, apex_y - half_projection, .067)
        right_center = (half_projection, apex_y - half_projection, .067)
        left = box((length, line_width, .012), left_center, 'MAT_LINEN')
        left.apply_transform(trimesh.transformations.rotation_matrix(
            math.pi / 4, [0, 0, 1], point=left_center))
        left.apply_transform(trimesh.transformations.rotation_matrix(-math.pi / 4, [0, 0, 1]))
        add(scene, left, f'ParquetChevron_{i}')
        right = box((length, line_width, .012), right_center, 'MAT_STONE_LIGHT')
        right.apply_transform(trimesh.transformations.rotation_matrix(
            -math.pi / 4, [0, 0, 1], point=right_center))
        right.apply_transform(trimesh.transformations.rotation_matrix(-math.pi / 4, [0, 0, 1]))
        add(scene, right, f'ParquetCross_{i}')
    return scene


def _marble_panel(v, mat):
    scene = trimesh.Scene()
    width, height = 1.04 + .09 * v, 1.60 + .12 * v
    add(scene, box((width, .10, height), (0, 0, height / 2), mat), 'MarbleSlab')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width * .20
        vein = box((width * (.20 + .015 * v), .025, .035), (x, .065, height * (.22 + .11 * (i % 4))), 'MAT_STONE_LIGHT')
        vein.apply_transform(trimesh.transformations.rotation_matrix(.10 * (i - v / 2), [0, 1, 0], point=(x, .065, height * (.22 + .11 * (i % 4)))))
        add(scene, vein, f'MarbleVein_{i}')
    add(scene, box((width * 1.04, .15, .08), (0, 0, height + .04), 'MAT_BRASS_POLISHED'), 'PanelCap')
    for side, x in enumerate((-width * .51, width * .51)):
        add(scene, box((.035, .13, height), (x, 0, height / 2), 'MAT_BRASS_POLISHED'), f'MarbleEdgeTrim_{side}')
    return scene


def _acoustic_panel(v, mat):
    scene = trimesh.Scene()
    width, height = 1.05 + .10 * v, 1.30 + .10 * v
    add(scene, box((width, .10, height), (0, 0, height / 2), mat), 'AcousticCore')
    add(scene, box((width * 1.04, .07, height * 1.04), (0, .04, height * 1.04 / 2), 'MAT_WOOD_WARM'), 'PanelFrame')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        add(scene, box((.04, .04, height * .88), (x, .095, height / 2), 'MAT_STONE_LIGHT'), f'AcousticSlat_{i}')
    return scene


def _fabric_panel(v, mat):
    scene = trimesh.Scene()
    width, height = .88 + .08 * v, 1.42 + .11 * v
    add(scene, box((width, .12, height), (0, 0, height / 2), mat), 'UpholsteredPanel')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, cyl(.025, .08, (x, .08, height * .30), 'MAT_BRASS_POLISHED', 10), f'TuftButtonLow_{i}')
        add(scene, cyl(.025, .08, (x, .08, height * .70), 'MAT_BRASS_POLISHED', 10), f'TuftButtonHigh_{i}')
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.05, .08, height + .08), (x, 0, (height + .08) / 2), 'MAT_WOOD_WARM'), f'PanelTrim_{side}')
    return scene


def _paint_board(v, mat):
    scene = trimesh.Scene()
    width, height = .96 + .08 * v, 1.28 + .09 * v
    add(scene, box((width, .08, height), (0, 0, height / 2), 'MAT_WOOD_WARM'), 'PaintSampleBoard')
    add(scene, box((width * .84, .03, height * .70), (0, .06, height * .58), mat), 'PaintFinishSample')
    for i in range(2 + v):
        z = .18 + i * .11
        add(scene, box((width * .54, .025, .025), (0, .08, z), 'MAT_BRASS_POLISHED'), f'SwatchLabelRule_{i}')
    add(scene, box((width * .58, .025, .045), (0, .08, .12), 'MAT_SIGNAGE'), 'PaintFinishLabel')
    return scene


def _ceiling_panel(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.10 + .09 * v, .90 + .07 * v
    add(scene, box((width, depth, .09), (0, 0, .045), mat), 'CeilingPanelCore')
    add(scene, box((width * .90, depth * .90, .035), (0, 0, .105), 'MAT_WOOD_WARM'), 'CofferedInset')
    for i, sign in enumerate((-1, 1)):
        add(scene, box((.07, depth, .08), (sign * width * .46, 0, .06), 'MAT_BRASS_POLISHED'), f'CeilingBorderX_{i}')
        add(scene, box((width, .07, .08), (0, sign * depth * .46, .06), 'MAT_BRASS_POLISHED'), f'CeilingBorderY_{i}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, sphere(.035, (x, 0, .14), 'MAT_EMISSIVE_WARM', 1), f'RecessedLight_{i}')
    return scene


def _metal_trim(v, mat):
    scene = trimesh.Scene()
    length, height = 1.30 + .11 * v, .36 + .04 * v
    add(scene, box((length, .12, .10), (0, 0, .05), 'MAT_BLACKENED_STEEL'), 'TrimBase')
    add(scene, box((length, .06, .08), (0, .03, .15), mat), 'TrimProfile')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * length / (2 + v)
        add(scene, cyl(.045, .055, (x, -.08, .16), 'MAT_BRASS_POLISHED', 12), f'MountClip_{i}')
    add(scene, box((length * .90, .04, .045), (0, 0, height), 'MAT_STAINLESS'), 'CapRail')
    return scene


def _finish(family, v, mat):
    recipes = (_floor_tile, _carpet_tile, _plank_floor, _parquet_panel, _marble_panel,
               _acoustic_panel, _fabric_panel, _paint_board, _ceiling_panel, _metal_trim)
    return recipes[family](v, mat)


def _yoga_rack(v, mat):
    scene = trimesh.Scene()
    width = .85 + .08 * v
    add(scene, box((width, .42, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'YogaRackBase')
    for side, x in enumerate((-width * .44, width * .44)):
        add(scene, box((.05, .05, 1.15 + .08 * v), (x, 0, .62 + .04 * v), mat), f'RackPost_{side}')
    for row, z in enumerate((.40, .80, 1.20)):
        for i in range(2 + v):
            x = (i - (1 + v) / 2) * width / (2 + v)
            center = (x, 0, z)
            roll = cyl(.11 + .01 * v, .42 + .035 * v, center, 'MAT_LINEN', 18)
            roll.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [0, 1, 0], point=center))
            add(scene, roll, f'RolledYogaMat_{row}_{i}')
    return scene


def _spa_reception(v, mat):
    scene = trimesh.Scene()
    width = 1.18 + .11 * v
    add(scene, box((width, .64, .78), (0, 0, .42), mat), 'SpaReceptionBase')
    add(scene, box((width * 1.05, .70, .08), (0, 0, .84), 'MAT_STONE_LIGHT'), 'SpaCounter')
    add(scene, box((width * .52, .05, .44 + .035 * v), (0, .24, 1.18), 'MAT_SIGNAGE'), 'SpaCrestPanel')
    add(scene, box((width * .44, .02, .34 + .03 * v), (0, .275, 1.18), 'MAT_WOOD_WARM'), 'CrestInset')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((.16, .12, .07), (x, -.12, .91), 'MAT_CERAMIC_FIXTURE'), f'TeaServiceCup_{i}')
    return scene


def _massage_chair(v, mat):
    scene = trimesh.Scene()
    width = .72 + .06 * v
    add(scene, box((width, .52, .14), (0, 0, .42), mat), 'TreatmentSeat')
    add(scene, box((width, .18, .72 + .05 * v), (0, .20, .84), mat), 'TreatmentBack')
    add(scene, box((width * .82, .42, .12), (0, -.43, .34), mat), 'LegRest')
    for side, x in enumerate((-width * .60, width * .60)):
        add(scene, box((.10, .48, .16), (x, -.02, .65), mat), f'ArmRest_{side}')
        add(scene, box((.08, .10, .32), (x, .14, 1.08 + .025 * v), 'MAT_BLACKENED_STEEL'), f'BackSupport_{side}')
    add(scene, box((.34 + .025 * v, .22, .12), (0, .18, 1.31 + .04 * v), 'MAT_LINEN'), 'HeadCushion')
    _legs(scene, width, .62, .30, 'MAT_BLACKENED_STEEL', 'ChairFoot')
    return scene


def _treatment_trolley(v, mat):
    scene = trimesh.Scene()
    width, depth = .72 + .07 * v, .50 + .04 * v
    for level, z in enumerate((.34, .78, 1.20)):
        add(scene, box((width, depth, .07), (0, 0, z), mat), f'TreatmentShelf_{level}')
    for side, x in enumerate((-width * .44, width * .44)):
        add(scene, cyl(.035, 1.28 + .08 * v, (x, 0, .68 + .04 * v), 'MAT_STAINLESS', 12), f'TrolleyPost_{side}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, cyl(.09, .08, (x, 0, .88), 'MAT_CERAMIC_FIXTURE', 14), f'TowelCanister_{i}')
    for i, x in enumerate((-width * .42, width * .42)):
        _wheel(scene, x, depth * .38, .12, f'TrolleyWheel_{i}')
    return scene


def _pool_ladder(v, mat):
    scene = trimesh.Scene()
    height, spread = 1.25 + .08 * v, .48 + .04 * v
    for side, x in enumerate((-spread / 2, spread / 2)):
        add(scene, cyl(.045, height, (x, 0, height / 2), mat, 14), f'LadderRail_{side}')
        add(scene, cyl(.075, .10, (x, 0, .05), 'MAT_STONE_LIGHT', 18), f'LadderFoot_{side}')
    for i in range(3 + v):
        z = .24 + i * (height - .32) / (2 + v)
        center = (0, 0, z)
        step = cyl(.035, spread, center, 'MAT_STAINLESS', 12)
        step.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [0, 1, 0], point=center))
        add(scene, step, f'LadderStep_{i}')
    add(scene, box((spread * 1.16, .62, .07), (0, .22, .035), 'MAT_STONE_LIGHT'), 'PoolDeckPlate')
    return scene


def _lifeguard_chair(v, mat):
    scene = trimesh.Scene()
    width, height = .58 + .05 * v, 1.65 + .10 * v
    add(scene, box((width, .48, .12), (0, 0, height), mat), 'GuardSeat')
    add(scene, box((width, .10, .56 + .04 * v), (0, .20, height + .30), mat), 'GuardBack')
    for side, x in enumerate((-width * .40, width * .40)):
        add(scene, box((.07, .07, height), (x, .16, height / 2), 'MAT_STAINLESS'), f'ChairTower_{side}')
        add(scene, box((.07, .72, .10), (x, -.02, height + .12), 'MAT_WOOD_WARM'), f'ArmSupport_{side}')
    for i in range(3 + v):
        z = .22 + i * (height - .30) / (2 + v)
        add(scene, box((.45, .08, .055), (width * .55, .18, z), 'MAT_STAINLESS'), f'AccessLadderStep_{i}')
    add(scene, box((.48 + .035 * v, .10, .055), (0, -.33, height + .46), 'MAT_SIGNAGE'), 'LifeguardSign')
    return scene


def _diving_board(v, mat):
    scene = trimesh.Scene()
    width, height, length = .70 + .06 * v, 1.20 + .10 * v, 1.55 + .12 * v
    add(scene, box((width * 1.25, .84, .16), (0, 0, .08), 'MAT_STONE_LIGHT'), 'DivingTowerBase')
    add(scene, box((width * .74, .66, height), (0, 0, height / 2 + .16), mat), 'TowerPillar')
    add(scene, box((width, length, .12), (0, -length * .35, height + .22), mat), 'DivingBoard')
    for i, z in enumerate((.45, .78, 1.11)):
        add(scene, box((width * .86, .46, .07), (0, .36, z), 'MAT_STAINLESS'), f'TowerStep_{i}')
    add(scene, box((width * 1.04, .10, .08), (0, -length * .80, height + .29), 'MAT_BRASS_POLISHED'), 'BoardEdgeStrip')
    return scene


def _pool_towel_station(v, mat):
    scene = trimesh.Scene()
    width, height = .78 + .08 * v, 1.38 + .10 * v
    add(scene, box((width, .46, height * .54), (0, 0, height * .27), mat), 'TowelCabinet')
    add(scene, box((width * 1.06, .50, .08), (0, 0, height * .56), 'MAT_STONE_LIGHT'), 'TowelCounter')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        for layer in range(2 + v // 2):
            add(scene, box((.20 + .01 * v, .24, .09), (x, .05, height * .61 + layer * .10), 'MAT_LINEN'), f'FoldedTowel_{i}_{layer}')
    add(scene, box((width * .76, .05, .32), (0, .25, height + .04), 'MAT_SIGNAGE'), 'PoolSign')
    return scene


def _refreshment_cred(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.12 + .10 * v, .56 + .04 * v
    add(scene, box((width, depth, .82), (0, 0, .42), mat), 'RefreshmentCredenza')
    add(scene, box((width * 1.05, depth * 1.06, .08), (0, 0, .86), 'MAT_STONE_LIGHT'), 'CredenzaTop')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, cyl(.095, .22 + .02 * v, (x, .07, 1.02), 'MAT_GLASS_CLEAR', 16), f'WaterCarafe_{i}')
        add(scene, cyl(.07, .055, (x, -.12, .92), 'MAT_CERAMIC_FIXTURE', 16), f'RefreshmentCup_{i}')
    add(scene, box((width * .90, .035, .08), (0, depth / 2 + .02, .54), 'MAT_BRASS_POLISHED'), 'CredenzaFrontRail')
    return scene


def _projector_cart(v, mat):
    scene = trimesh.Scene()
    width, depth = .68 + .06 * v, .56 + .045 * v
    add(scene, box((width, depth, .08), (0, 0, .48), mat), 'ProjectorShelf')
    add(scene, box((width * .92, depth * .92, .06), (0, 0, 1.02), 'MAT_STAINLESS'), 'ProjectorTopShelf')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        add(scene, box((.05, .05, .96), (x * width * .44, y * depth * .42, .78), 'MAT_STAINLESS'), f'CartPost_{i}')
        _wheel(scene, x * width * .44, y * depth * .42, .12, f'CartWheel_{i}')
    add(scene, box((.42 + .04 * v, .34, .20 + .02 * v), (0, .03, 1.18), mat), 'ProjectorBody')
    add(scene, cyl(.11 + .01 * v, .07, (0, .22, 1.18), 'MAT_GLASS_CLEAR', 18), 'ProjectorLens')
    add(scene, box((.20, .04, .06), (0, depth / 2 + .03, .66), 'MAT_WOOD_WARM'), 'CartHandle')
    return scene


def _amenity(family, v, mat):
    recipes = (_yoga_rack, _spa_reception, _massage_chair, _treatment_trolley, _pool_ladder,
               _lifeguard_chair, _diving_board, _pool_towel_station, _refreshment_cred, _projector_cart)
    return recipes[family](v, mat)


def _garden_fountain(v, mat):
    scene = trimesh.Scene()
    radius = .62 + .08 * v
    add(scene, cyl(radius, .20, (0, 0, .10), mat, 26), 'FountainBasin')
    add(scene, cyl(radius * .82, .06, (0, 0, .22), 'MAT_STONE_LIGHT', 26), 'FountainWater')
    add(scene, cyl(.17 + .015 * v, .70 + .06 * v, (0, 0, .58), 'MAT_STONE_LIGHT', 20), 'FountainPedestal')
    add(scene, cyl(.30 + .02 * v, .11, (0, 0, .98 + .06 * v), mat, 22), 'FountainBowl')
    add(scene, sphere(.11 + .01 * v, (0, 0, 1.22 + .06 * v), 'MAT_STONE_LIGHT', 2), 'FountainFinial')
    for i in range(2 + v):
        angle = i * 2 * math.pi / (2 + v)
        x, y = radius * .70 * math.cos(angle), radius * .70 * math.sin(angle)
        add(scene, sphere(.07, (x, y, .20), 'MAT_GLASS_CLEAR', 1), f'WaterJet_{i}')
    return scene


def _patio_table(v, mat):
    scene = trimesh.Scene()
    width, depth, top = .92 + .09 * v, .78 + .08 * v, .74
    add(scene, box((width, depth, .10), (0, 0, top), mat), 'PatioTableTop')
    _legs(scene, width, depth, top - .05, 'MAT_BLACKENED_STEEL', 'TableLeg')
    add(scene, box((width * .82, depth * .82, .06), (0, 0, top * .48), 'MAT_STAINLESS'), 'TableBrace')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * width / (2 + v // 2)
        add(scene, cyl(.06, .035, (x, 0, top + .07), 'MAT_CERAMIC_FIXTURE', 12), f'OutdoorCup_{i}')
    return scene


def _garden_chair(v, mat):
    scene = trimesh.Scene()
    width, depth = .52 + .04 * v, .56 + .04 * v
    add(scene, box((width, depth, .09), (0, 0, .46), mat), 'GardenSeat')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        add(scene, box((.035, depth, .055), (x, 0, .76), mat), f'BackSlat_{i}')
    _legs(scene, width, depth, .43, 'MAT_BLACKENED_STEEL', 'ChairLeg')
    for side, x in enumerate((-width * .50, width * .50)):
        add(scene, box((.05, depth * .88, .06), (x, 0, .72), 'MAT_WOOD_WARM'), f'ArmRail_{side}')
    return scene


def _path_bollard(v, mat):
    scene = trimesh.Scene()
    height, radius = .72 + .08 * v, .15 + .012 * v
    add(scene, cyl(radius * 1.55, .10, (0, 0, .05), 'MAT_STONE_LIGHT', 20), 'BollardFoot')
    add(scene, cyl(radius, height, (0, 0, height / 2 + .10), mat, 20), 'BollardPost')
    add(scene, cyl(radius * 1.25, .14, (0, 0, height + .17), 'MAT_STONE_LIGHT', 20), 'BollardCap')
    add(scene, cyl(radius * .86, .18, (0, 0, height * .72), 'MAT_EMISSIVE_WARM', 18), 'BollardLightBand')
    for i in range(2 + v // 2):
        z = .24 + i * .16
        add(scene, box((radius * 1.8, .025, .025), (0, -radius * .90, z), 'MAT_BRASS_POLISHED'), f'BollardRing_{i}')
    return scene


def _garden_gate(v, mat):
    scene = trimesh.Scene()
    width, height = 1.15 + .12 * v, 1.65 + .10 * v
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.14, .18, height), (x, 0, height / 2), 'MAT_STONE_LIGHT'), f'GatePost_{side}')
        add(scene, box((.20, .22, .11), (x, 0, .055), 'MAT_STONE_LIGHT'), f'GateFoot_{side}')
    for row, z in enumerate((.18, height - .16)):
        add(scene, box((width, .10, .10), (0, 0, z), mat), f'GateRail_{row}')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        add(scene, box((.055, .065, height - .36), (x, 0, height / 2), mat), f'GatePicket_{i}')
        add(scene, sphere(.045, (x, 0, height - .12), 'MAT_BRASS_POLISHED', 1), f'GateFinial_{i}')
    add(scene, box((.12, .045, .27), (width * .32, -.07, .95), 'MAT_BRASS_POLISHED'), 'GateLatch')
    return scene


def _garden_trellis(v, mat):
    scene = trimesh.Scene()
    width, height = 1.25 + .12 * v, 1.80 + .11 * v
    for side, x in enumerate((-width * .45, width * .45)):
        add(scene, box((.09, .10, height), (x, 0, height / 2), mat), f'TrellisPost_{side}')
        add(scene, box((.16, .18, .08), (x, 0, .04), 'MAT_STONE_LIGHT'), f'TrellisFoot_{side}')
    for row, z in enumerate((.34, .74, 1.14, 1.54)):
        add(scene, box((width, .07, .055), (0, 0, z), mat), f'TrellisCrossbar_{row}')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        add(scene, box((.045, .055, height * .90), (x, 0, height / 2), mat), f'TrellisLattice_{i}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, sphere(.13 + .01 * v, (x, -.07, .48 + .35 * (i % 2)), 'MAT_VEGETATION', 1), f'ClimbingLeaf_{i}')
    return scene


def _outdoor_shower(v, mat):
    scene = trimesh.Scene()
    height, width = 2.05 + .10 * v, .92 + .08 * v
    add(scene, box((width, width, .12), (0, 0, .06), 'MAT_STONE_LIGHT'), 'ShowerDeck')
    for side, x in enumerate((-width * .42, width * .42)):
        add(scene, cyl(.055, height, (x, 0, height / 2 + .12), mat, 14), f'ShowerPost_{side}')
    add(scene, box((width * .92, .10, .09), (0, 0, height + .08), mat), 'ShowerHeader')
    add(scene, cyl(.16 + .01 * v, .06, (0, .18, height - .05), 'MAT_STAINLESS', 20), 'OutdoorShowerHead')
    add(scene, cyl(.022, .64, (width * .30, .12, 1.30), 'MAT_STAINLESS', 12), 'RiserPipe')
    add(scene, box((.08, .045, .08), (width * .30, .12, 1.06), 'MAT_BRASS_POLISHED'), 'WaterControl')
    return scene


def _flowerbed(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.32 + .13 * v, .76 + .08 * v
    add(scene, box((width, depth, .22), (0, 0, .11), mat), 'RaisedFlowerBed')
    add(scene, box((width * .90, depth * .84, .08), (0, 0, .26), 'MAT_STONE_LIGHT'), 'SoilSurface')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * width / (2 + v)
        z = .58 + .04 * (i % 2)
        add(scene, cyl(.018, .44, (x, 0, .45), 'MAT_VEGETATION', 8), f'FlowerStem_{i}')
        add(scene, sphere(.14 + .008 * v, (x, 0, z), 'MAT_VEGETATION', 1), f'FlowerHead_{i}')
        add(scene, sphere(.045, (x, -.10, .43), 'MAT_LINEN', 1), f'FlowerBud_{i}')
    return scene


def _garden_bench(v, mat):
    scene = trimesh.Scene()
    width = 1.18 + .12 * v
    add(scene, box((width, .54, .12), (0, 0, .48), mat), 'GardenBenchSeat')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * width / (3 + v)
        add(scene, box((.045, .10, .44), (x, .25, .82), mat), f'BenchBackSlat_{i}')
    for side, x in enumerate((-width * .42, width * .42)):
        add(scene, box((.10, .12, .46), (x, 0, .27), 'MAT_BLACKENED_STEEL'), f'BenchSupport_{side}')
    add(scene, box((width * 1.08, .12, .075), (0, .26, 1.07), 'MAT_WOOD_WARM'), 'BenchTopRail')
    return scene


def _walkway_arch(v, mat):
    scene = trimesh.Scene()
    width, height = 1.38 + .12 * v, 2.18 + .10 * v
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.22, .26, height), (x, 0, height / 2), mat), f'WalkwayPier_{side}')
        add(scene, box((.34, .38, .12), (x, 0, .06), 'MAT_STONE_LIGHT'), f'WalkwayFoot_{side}')
    add(scene, box((width + .24, .30, .24), (0, 0, height - .02), 'MAT_STONE_LIGHT'), 'WalkwayLintel')
    segments = 7 + v
    angles = [math.pi * i / (segments - 1) for i in range(segments)]
    _arch(angles, scene, width * .48, height - .18, 'MAT_STONE_LIGHT', 'WalkwayArchStone', y=.24)
    add(scene, sphere(.10 + .01 * v, (0, 0, height + .18), 'MAT_BRASS_POLISHED', 1), 'ArchCrest')
    return scene


def _exterior(family, v, mat):
    recipes = (_garden_fountain, _patio_table, _garden_chair, _path_bollard, _garden_gate,
               _garden_trellis, _outdoor_shower, _flowerbed, _garden_bench, _walkway_arch)
    return recipes[family](v, mat)


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 1501 <= number <= 1750:
        raise ValueError(f'property expansion asset ID outside A1501–A1750: {asset_id}')
    if number <= 1550:
        family, variant = divmod(number - 1501, 5)
        return _architecture(family, variant, mat)
    if number <= 1600:
        family, variant = divmod(number - 1551, 5)
        return _public(family, variant, mat)
    if number <= 1650:
        family, variant = divmod(number - 1601, 5)
        return _finish(family, variant, mat)
    if number <= 1700:
        family, variant = divmod(number - 1651, 5)
        return _amenity(family, variant, mat)
    family, variant = divmod(number - 1701, 5)
    return _exterior(family, variant, mat)
