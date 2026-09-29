"""Purpose-built room, restaurant, service and decor collections for A1251–A1500."""

import math

import trimesh

from character_v2_factory import build_asset as build_character
from v2_asset_common import add, box, cyl, sphere


VARIANTS = ('Compact', 'Standard', 'Extended', 'High Capacity', 'Grand')


def _legs(scene, width, depth, height, material='MAT_BLACKENED_STEEL', prefix='Leg'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.055, .055, height),
                       (x * width * .42, y * depth * .38, height / 2), material),
            f'{prefix}_{index}')


def _wheel(scene, x, y, z, radius=.10, name='Wheel'):
    center = (x, y, z)
    wheel = cyl(radius, .055, center, 'MAT_BLACKENED_STEEL', 16)
    wheel.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [1, 0, 0], point=center))
    add(scene, wheel, name)


def _round_panel(scene, radius, depth, center, material, name):
    panel = cyl(radius, depth, center, material, 24)
    panel.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [1, 0, 0], point=center))
    add(scene, panel, name)


def _bed(v, mat):
    scene = trimesh.Scene()
    width, length, z = 1.32 + .13 * v, 2.00 + .12 * v, .36 + .018 * v
    add(scene, box((width, length, .22), (0, 0, z), 'MAT_WOOD_WARM'), 'BedFrame')
    add(scene, box((width * .97, length * .96, .22), (0, 0, z + .22), mat), 'Mattress')
    add(scene, box((width * 1.04, .13, .78 + .035 * v),
                   (0, length * .46, z + .55), 'MAT_WOOD_DARK'), 'Headboard')
    _legs(scene, width, length, z - .11)
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * width * .26
        add(scene, box((width * .22, .34, .085), (x, length * .27, z + .36),
                       'MAT_LINEN'), f'Pillow_{i}')
    add(scene, box((width * .88, length * .25, .075),
                   (0, -length * .32, z + .37), 'MAT_LINEN'), 'BedRunner')
    return scene


def _casework(v, mat, style):
    scene = trimesh.Scene()
    width, depth = .86 + .09 * v, .48 + .025 * v
    height = 1.02 + .09 * v if style == 'dresser' else 1.88 + .13 * v
    add(scene, box((width * .96, depth * .92, .09), (0, 0, .045), 'MAT_BLACKENED_STEEL'), 'CasePlinth')
    add(scene, box((width, depth, height), (0, 0, .10 + height / 2), mat), 'CaseBody')
    if style == 'armoire':
        for side, x in enumerate((-.25, .25)):
            add(scene, box((width * .46, .025, height * .78),
                           (x * width, depth / 2 + .016, .22 + height * .50), 'MAT_WOOD_WARM'),
                f'DoorPanel_{side}')
            add(scene, cyl(.018, .04, (x * width + .12, depth / 2 + .045, .96),
                           'MAT_BRASS_POLISHED', 10), f'DoorHandle_{side}')
        add(scene, box((width * 1.04, depth * 1.04, .09), (0, 0, height + .15), 'MAT_WOOD_DARK'), 'Crown')
        add(scene, box((width * .72, .04, .08), (0, -depth / 2 - .03, height * .62), 'MAT_STAINLESS'), 'WardrobeRail')
    else:
        for i in range(3 + v // 2):
            z = .28 + i * ((height - .30) / (3 + v // 2))
            add(scene, box((width * .88, .025, .16), (0, depth / 2 + .016, z), 'MAT_WOOD_WARM'), f'DrawerFace_{i}')
            add(scene, box((.14 + .01 * v, .035, .025), (0, depth / 2 + .04, z), 'MAT_BRASS_POLISHED'), f'DrawerPull_{i}')
    return scene


def _desk(v, mat):
    scene = trimesh.Scene()
    width, depth, top_z = 1.10 + .12 * v, .58 + .055 * v, .76
    add(scene, box((width, depth, .08), (0, 0, top_z), mat), 'WorkSurface')
    _legs(scene, width, depth, top_z - .04, 'MAT_WOOD_DARK', 'DeskLeg')
    add(scene, box((width * .32, depth * .78, .42), (-width * .30, 0, .34), 'MAT_WOOD_WARM'), 'DrawerPedestal')
    for i in range(2 + v // 2):
        z = .25 + i * .13
        add(scene, box((width * .27, .025, .09), (-width * .30, depth * .41, z), mat), f'DeskDrawer_{i}')
        add(scene, box((.12, .035, .018), (-width * .30, depth * .44, z), 'MAT_BRASS_POLISHED'), f'DeskPull_{i}')
    add(scene, box((.30, .24, .035), (0, .13, top_z + .065), 'MAT_ELECTRONICS'), 'DeskTablet')
    return scene


def _task_chair(v, mat):
    scene = trimesh.Scene()
    width, height = .52 + .035 * v, .96 + .07 * v
    add(scene, cyl(.24 + .018 * v, .055, (0, 0, .11), 'MAT_BLACKENED_STEEL', 20), 'FiveStarChairBase')
    add(scene, cyl(.065, .36, (0, 0, .31), 'MAT_STAINLESS', 14), 'ChairPneumaticPost')
    add(scene, box((width, .48, .10), (0, 0, .50), mat), 'TaskChairSeat')
    add(scene, box((width * .92, .09, .48 + .05 * v), (0, .20, .82 + .03 * v), mat), 'TaskChairBack')
    for side, x in enumerate((-.34, .34)):
        add(scene, box((.055, .34, .045), (x, -.02, .70), 'MAT_BLACKENED_STEEL'), f'ArmSupport_{side}')
        add(scene, box((.095, .38, .055), (x, -.02, .73), 'MAT_UPHOLSTERY'), f'ArmPad_{side}')
    for i in range(5):
        angle = i * 2 * math.pi / 5
        x, y = .22 * math.cos(angle), .22 * math.sin(angle)
        add(scene, box((.17, .05, .035), (x * .5, y * .5, .09), 'MAT_BLACKENED_STEEL'), f'ChairSpoke_{i}')
        _wheel(scene, x, y, .055, .045, f'ChairCaster_{i}')
    return scene


def _bedside_table(v, mat):
    scene = trimesh.Scene()
    width, depth, height = .55 + .055 * v, .44 + .025 * v, .58 + .045 * v
    add(scene, box((width, depth, height), (0, 0, height / 2 + .05), mat), 'NightstandBody')
    add(scene, box((width * 1.06, depth * 1.06, .07), (0, 0, height + .09), 'MAT_WOOD_DARK'), 'NightstandTop')
    for i in range(1 + v // 2):
        z = .28 + i * .16
        add(scene, box((width * .82, .022, .13), (0, depth / 2 + .014, z), 'MAT_WOOD_WARM'), f'NightstandDrawer_{i}')
        add(scene, cyl(.022, .035, (0, depth / 2 + .035, z), 'MAT_BRASS_POLISHED', 10), f'NightstandPull_{i}')
    _legs(scene, width, depth, .22)
    return scene


def _media_console(v, mat):
    scene = trimesh.Scene()
    width, depth, height = 1.30 + .16 * v, .42 + .035 * v, .58 + .04 * v
    add(scene, box((width, depth, height), (0, 0, height / 2 + .08), mat), 'MediaConsoleBody')
    add(scene, box((width * 1.04, depth * 1.06, .08), (0, 0, height + .12), 'MAT_WOOD_DARK'), 'ConsoleTop')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((.025, depth * .75, height * .62), (x, 0, height * .50), 'MAT_BLACKENED_STEEL'), f'OpenBayDivider_{i}')
    add(scene, box((width * .86, .07, .85 + .06 * v), (0, depth * .22, height + .62), 'MAT_ELECTRONICS'), 'TelevisionPanel')
    add(scene, box((width * .80, .02, .72 + .05 * v), (0, depth * .22 + .05, height + .62), 'MAT_GLASS_CLEAR'), 'ScreenGlass')
    return scene


def _coffee_unit(v, mat):
    scene = trimesh.Scene()
    width, depth, height = .72 + .075 * v, .50 + .025 * v, 1.02 + .07 * v
    add(scene, box((width, depth, height * .62), (0, 0, height * .31), mat), 'CoffeeCabinet')
    add(scene, box((width * 1.03, depth * 1.03, .07), (0, 0, height * .65), 'MAT_STONE_LIGHT'), 'CoffeeCounter')
    add(scene, box((width * .52, depth * .50, .42 + .03 * v), (0, .02, height * .65 + .24), 'MAT_ELECTRONICS'), 'BrewerBody')
    add(scene, box((width * .44, .025, .28), (0, depth * .22, height * .66 + .24), 'MAT_GLASS_CLEAR'), 'BrewerWaterWindow')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .18
        add(scene, cyl(.06, .07, (x, depth * .50 + .08, height * .65 + .10), 'MAT_CERAMIC_FIXTURE', 14), f'CoffeeCup_{i}')
    return scene


def _luggage_rack(v, mat):
    scene = trimesh.Scene()
    width, depth = .62 + .065 * v, .76 + .055 * v
    for side, x in enumerate((-width * .45, width * .45)):
        add(scene, box((.045, .045, .48 + .035 * v), (x, 0, .28 + .02 * v), mat), f'RackUpright_{side}')
        add(scene, box((.05, depth, .045), (x, 0, .05), 'MAT_BLACKENED_STEEL'), f'RackFoot_{side}')
        add(scene, box((.09, depth * .84, .055), (x, 0, .50 + .035 * v), 'MAT_BRASS_POLISHED'), f'RackRail_{side}')
    for i in range(3 + v // 2):
        y = (i - (2 + v // 2) / 2) * depth / (3 + v // 2)
        add(scene, box((width * .72, .025, .035), (0, y, .15), 'MAT_LINEN'), f'LuggageStrap_{i}')
    add(scene, box((.10, .08, .12), (0, depth * .49, .58 + .035 * v), 'MAT_WOOD_DARK'), 'HandleGrip')
    return scene


def _mirror(v, mat):
    scene = trimesh.Scene()
    width, height = .56 + .045 * v, 1.48 + .10 * v
    add(scene, box((width + .10, .08, height + .08), (0, .02, (height + .08) / 2), 'MAT_WOOD_DARK'), 'MirrorFrame')
    add(scene, box((width, .025, height), (0, .065, height / 2), 'MAT_GLASS_CLEAR'), 'MirrorGlass')
    add(scene, box((width * 1.30, .24, .07), (0, .02, .035), 'MAT_BLACKENED_STEEL'), 'MirrorBase')
    for i in range(2 + v // 2):
        z = .24 + i * .20
        add(scene, box((.025, .035, .07), (width / 2 + .035, .075, z), mat), f'FramePeg_{i}')
    return scene


def _vanity(v, mat):
    scene = trimesh.Scene()
    width, depth, top_z = 1.25 + .12 * v, .58 + .045 * v, .82
    add(scene, box((width, depth, .65), (0, 0, .43), mat), 'VanityCabinet')
    add(scene, box((width * 1.04, depth * 1.08, .08), (0, 0, top_z), 'MAT_STONE_LIGHT'), 'VanityCounter')
    for i, x in enumerate((-.28, .28)):
        bowl = cyl(.18 + .012 * v, .085, (x * (1 + .07 * v), -.02, top_z + .08), 'MAT_CERAMIC_FIXTURE', 22)
        add(scene, bowl, f'WashBasin_{i}')
        add(scene, cyl(.018, .26, (x * (1 + .07 * v), .17, top_z + .22), 'MAT_STAINLESS', 10), f'Faucet_{i}')
        add(scene, box((.025, .23, .025), (x * (1 + .07 * v), .06, top_z + .18), 'MAT_STAINLESS'), f'Spout_{i}')
    add(scene, box((width * .78, .045, .70 + .04 * v), (0, depth * .43, top_z + .44), mat), 'VanityMirrorFrame')
    add(scene, box((width * .68, .025, .60 + .04 * v), (0, depth * .43 + .04, top_z + .44), 'MAT_GLASS_CLEAR'), 'VanityMirror')
    return scene


def _shower(v, mat):
    scene = trimesh.Scene()
    width, depth = .92 + .08 * v, .94 + .07 * v
    add(scene, box((width, depth, .09), (0, 0, .045), 'MAT_CERAMIC_FIXTURE'), 'ShowerTray')
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.035, depth, 1.95 + .10 * v), (x, 0, .99 + .05 * v), 'MAT_STAINLESS'), f'FrameRail_{side}')
        add(scene, box((.018, depth * .92, 1.82 + .08 * v), (x * .96, 0, .98 + .04 * v), 'MAT_GLASS_CLEAR'), f'GlassPanel_{side}')
    add(scene, box((width, .035, .06), (0, depth / 2, .12), 'MAT_STAINLESS'), 'DoorThreshold')
    add(scene, cyl(.018, .70 + .04 * v, (-width * .32, depth * .30, 1.56), 'MAT_STAINLESS', 12), 'ShowerRiser')
    add(scene, cyl(.14 + .01 * v, .045, (-width * .32, depth * .30, 1.95 + .08 * v), 'MAT_STAINLESS', 18), 'RainHead')
    add(scene, box((.035, .03, .34), (width * .35, depth / 2 + .03, 1.10), 'MAT_BRASS_POLISHED'), 'DoorHandle')
    return scene


def _bathtub(v, mat):
    scene = trimesh.Scene()
    width, length = .80 + .06 * v, 1.62 + .12 * v
    add(scene, box((width, length, .48), (0, 0, .34), mat), 'TubBody')
    add(scene, box((width * .86, length * .88, .06), (0, 0, .61), 'MAT_CERAMIC_FIXTURE'), 'TubRim')
    add(scene, box((width * .72, length * .76, .035), (0, 0, .57), 'MAT_GLASS_CLEAR'), 'TubBasin')
    for i, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.08, .08, .22), (x * width * .35, y * length * .38, .11), 'MAT_STAINLESS'), f'TubFoot_{i}')
    add(scene, cyl(.025, .32, (width * .35, -length * .42, .78), 'MAT_STAINLESS', 12), 'TubFaucet')
    add(scene, cyl(.09, .04, (width * .35, -length * .42, .91), 'MAT_STAINLESS', 16), 'TubSpout')
    return scene


def _guestroom(family, v, mat):
    recipes = (
        lambda: _bed(v, mat), lambda: _casework(v, mat, 'armoire'),
        lambda: _casework(v, mat, 'dresser'), lambda: _desk(v, mat),
        lambda: _task_chair(v, mat), lambda: _accent_chair(v, mat),
        lambda: _bedside_table(v, mat), lambda: _media_console(v, mat),
        lambda: _coffee_unit(v, mat), lambda: _luggage_rack(v, mat),
        lambda: _mirror(v, mat), lambda: _vanity(v, mat),
        lambda: _shower(v, mat), lambda: _bathtub(v, mat),
    )
    return recipes[family]()


def _accent_chair(variant, material):
    scene = trimesh.Scene()
    width = .72 + .065 * variant
    add(scene, box((width, .62, .13), (0, 0, .48), material), 'ChairSeat')
    add(scene, box((width, .12, .62 + .045 * variant), (0, .25, .83), material), 'ChairBack')
    for side, x in enumerate((-width * .56, width * .56)):
        add(scene, box((.09, .62, .28), (x, 0, .64), material), f'ChairArm_{side}')
    _legs(scene, width, .62, .38, 'MAT_WOOD_DARK', 'ChairLeg')
    for i in range(1 + variant // 2):
        x = (i - variant / 4) * width * .28
        add(scene, box((.18, .10, .12), (x, .12, .59), 'MAT_UPHOLSTERY'), f'AccentCushion_{i}')
    return scene


def _dining_chair(v, mat):
    scene = trimesh.Scene()
    width = .48 + .035 * v
    add(scene, box((width, .48, .10), (0, 0, .48), mat), 'DiningSeat')
    add(scene, box((width, .09, .50 + .035 * v), (0, .19, .77), mat), 'DiningBack')
    _legs(scene, width, .48, .43, 'MAT_WOOD_DARK', 'DiningLeg')
    for i in range(2 + v // 2):
        z = .67 + i * .12
        add(scene, box((width * .75, .035, .045), (0, .22, z), 'MAT_WOOD_WARM'), f'BackSlat_{i}')
    return scene


def _booth(v, mat):
    scene = trimesh.Scene()
    width = 1.20 + .12 * v
    add(scene, box((width, .72, .18), (0, 0, .47), mat), 'BanquetteSeat')
    add(scene, box((width, .15, .88 + .06 * v), (0, .30, .98), mat), 'HighBack')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * width / (2 + v // 2)
        add(scene, box((width / (2 + v // 2) * .88, .54, .12), (x, -.03, .62), 'MAT_UPHOLSTERY'), f'SeatCushion_{i}')
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.11, .74, .46), (x, 0, .70), 'MAT_WOOD_DARK'), f'BoothArm_{side}')
    _legs(scene, width, .72, .35)
    return scene


def _dining_table(v, mat, round_top=False):
    scene = trimesh.Scene()
    width = .82 + .11 * v
    depth = width if round_top else .78 + .10 * v
    if round_top:
        add(scene, cyl(width * .49, .09, (0, 0, .78), mat, 28), 'RoundTableTop')
        add(scene, cyl(.10 + .015 * v, .65, (0, 0, .40), 'MAT_WOOD_DARK', 16), 'PedestalColumn')
        add(scene, cyl(.34 + .03 * v, .08, (0, 0, .08), 'MAT_BLACKENED_STEEL', 22), 'PedestalFoot')
        add(scene, cyl(.16 + .015 * v, .04, (0, 0, .78), 'MAT_LINEN', 20), 'TableCenterPlate')
    else:
        add(scene, box((width, depth, .09), (0, 0, .78), mat), 'SquareTableTop')
        _legs(scene, width, depth, .74, 'MAT_WOOD_DARK', 'TableLeg')
        add(scene, box((width * .72, depth * .72, .045), (0, 0, .52), 'MAT_BLACKENED_STEEL'), 'TableApron')
    add(scene, box((.20 + .025 * v, .15, .035), (0, 0, .84), 'MAT_LINEN'), 'TableSetting')
    return scene


def _barstool(v, mat):
    scene = trimesh.Scene()
    height = .76 + .055 * v
    add(scene, cyl(.24 + .025 * v, .11, (0, 0, height), mat, 22), 'BarstoolSeat')
    add(scene, cyl(.065, height - .12, (0, 0, height / 2), 'MAT_BLACKENED_STEEL', 16), 'SupportPost')
    add(scene, cyl(.27 + .03 * v, .055, (0, 0, .045), 'MAT_BLACKENED_STEEL', 22), 'StoolBase')
    add(scene, cyl(.23 + .02 * v, .045, (0, 0, .34 + .035 * v), 'MAT_STAINLESS', 20), 'FootRing')
    for side, x in enumerate((-.17, .17)):
        add(scene, box((.12, .30, .045), (x, 0, .35 + .035 * v), 'MAT_BLACKENED_STEEL'), f'FootRest_{side}')
    if v >= 2:
        add(scene, box((.35, .08, .25), (0, .22, height + .13), mat), 'LowBackrest')
    return scene


def _buffet(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.20 + .12 * v, .70 + .04 * v
    add(scene, box((width, depth, .76), (0, 0, .40), mat), 'BuffetCabinet')
    add(scene, box((width * 1.04, depth * 1.06, .09), (0, 0, .82), 'MAT_STAINLESS'), 'BuffetCounter')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((width / (2 + v) * .82, depth * .58, .06), (x, 0, .90), 'MAT_STAINLESS'), f'ChafingPan_{i}')
        add(scene, box((width / (2 + v) * .82, .025, .25), (x, depth * .38, 1.05), 'MAT_GLASS_CLEAR'), f'SneezeGuard_{i}')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .22
        add(scene, cyl(.08, .15, (x, .14, .97), 'MAT_BRASS_POLISHED', 16), f'ServingLadle_{i}')
    return scene


def _beverage_tower(v, mat):
    scene = trimesh.Scene()
    width = .72 + .07 * v
    add(scene, box((width, .60, .76), (0, 0, .42), mat), 'BeverageBase')
    add(scene, box((width * 1.04, .64, .08), (0, 0, .84), 'MAT_STAINLESS'), 'ServiceTop')
    for i in range(1 + v):
        x = (i - v / 2) * width / (1 + v)
        add(scene, cyl(.12 + .008 * v, .38 + .035 * v, (x, .05, 1.10), 'MAT_GLASS_CLEAR', 18), f'DrinkReservoir_{i}')
        add(scene, box((.06, .09, .14), (x, .35, .92), 'MAT_STAINLESS'), f'Spigot_{i}')
        add(scene, cyl(.045, .04, (x, .43, .80), 'MAT_CERAMIC_FIXTURE', 14), f'DripTray_{i}')
    return scene


def _host_stand(v, mat):
    scene = trimesh.Scene()
    width, height = .70 + .08 * v, 1.12 + .08 * v
    add(scene, box((width, .56, height), (0, 0, height / 2), mat), 'HostStandBody')
    add(scene, box((width * 1.08, .62, .08), (0, 0, height + .04), 'MAT_WOOD_DARK'), 'HostCounter')
    add(scene, box((.38 + .035 * v, .04, .30), (0, .30, height + .22), 'MAT_ELECTRONICS'), 'ReservationDisplay')
    add(scene, box((.32, .018, .24), (0, .34, height + .22), 'MAT_GLASS_CLEAR'), 'DisplayGlass')
    add(scene, box((width * .82, .035, .11), (0, .30, .72), 'MAT_SIGNAGE'), 'HotelPlaque')
    for i in range(1 + v // 2):
        x = (i - v / 4) * width * .28
        add(scene, box((.10, .04, .12), (x, .30, .48), 'MAT_BRASS_POLISHED'), f'KeyHook_{i}')
    return scene


def _server_station(v, mat):
    scene = trimesh.Scene()
    width, depth, height = .90 + .09 * v, .56 + .04 * v, 1.10 + .08 * v
    add(scene, box((width, depth, height), (0, 0, height / 2), mat), 'SideStationBody')
    add(scene, box((width * 1.04, depth * 1.06, .08), (0, 0, height + .04), 'MAT_STAINLESS'), 'StationTop')
    for i in range(2 + v):
        z = .24 + i * (height - .36) / (1 + v)
        add(scene, box((width * .84, .025, .11), (0, depth / 2 + .016, z), 'MAT_WOOD_WARM'), f'ServiceDrawer_{i}')
        add(scene, box((.12, .035, .02), (0, depth / 2 + .04, z), 'MAT_BRASS_POLISHED'), f'DrawerPull_{i}')
    for i in range(2 + v // 2):
        add(scene, cyl(.10, .035, ((i - v / 2) * .20, .04, height + .11), 'MAT_CERAMIC_FIXTURE', 14), f'PlateStack_{i}')
    return scene


def _plate_cart(v, mat):
    scene = trimesh.Scene()
    width, depth = .70 + .06 * v, .72 + .05 * v
    add(scene, box((width, depth, .10), (0, 0, .46), mat), 'CartLowerShelf')
    add(scene, box((width, depth, .08), (0, 0, .98), mat), 'CartUpperShelf')
    for i, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.045, .045, .92), (x * width * .44, y * depth * .44, .70), 'MAT_STAINLESS'), f'CartUpright_{i}')
    for shelf, z in enumerate((.57, 1.08)):
        for i in range(2 + v):
            x = (i - (1 + v) / 2) * width / (2 + v)
            add(scene, cyl(.10 + .008 * v, .025, (x, 0, z), 'MAT_CERAMIC_FIXTURE', 16), f'ServicePlate_{shelf}_{i}')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        _wheel(scene, x * width * .44, y * depth * .44, .12, .10, f'CartWheel_{i}')
    return scene


def _restaurant(family, v, mat):
    recipes = (
        lambda: _dining_chair(v, mat), lambda: _booth(v, mat),
        lambda: _dining_table(v, mat, True), lambda: _dining_table(v, mat, False),
        lambda: _barstool(v, mat), lambda: _buffet(v, mat),
        lambda: _beverage_tower(v, mat), lambda: _host_stand(v, mat),
        lambda: _server_station(v, mat), lambda: _plate_cart(v, mat),
    )
    return recipes[family]()


def _linen_cart(v, mat):
    scene = trimesh.Scene()
    width, depth = .85 + .08 * v, .66 + .06 * v
    add(scene, box((width, depth, .09), (0, 0, .40), mat), 'LinenCartBase')
    for side, x in enumerate((-width * .46, width * .46)):
        add(scene, box((.05, .05, 1.25 + .08 * v), (x, depth * .38, 1.02 + .04 * v), 'MAT_STAINLESS'), f'CartFrame_{side}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((width / (2 + v) * .90, depth * .72, .42), (x, 0, .68), 'MAT_LINEN'), f'LinenBag_{i}')
        add(scene, box((width / (2 + v) * .84, .08, .045), (x, depth * .38, .92), 'MAT_WOOD_WARM'), f'BagHandle_{i}')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        _wheel(scene, x * width * .42, y * depth * .40, .12, .10, f'LinenWheel_{i}')
    return scene


def _supply_cart(v, mat):
    scene = trimesh.Scene()
    width, depth = .70 + .06 * v, .54 + .04 * v
    for level, z in enumerate((.24, .68, 1.12)):
        add(scene, box((width, depth, .07), (0, 0, z), mat), f'SupplyShelf_{level}')
    for i, x in enumerate((-width * .45, width * .45)):
        add(scene, box((.05, .05, 1.30 + .08 * v), (x, depth * .42, .70 + .04 * v), 'MAT_STAINLESS'), f'SupplyUpright_{i}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width / (2 + v)
        add(scene, box((.18, .22, .22), (x, .04, .42), 'MAT_CERAMIC_FIXTURE'), f'SupplyBottle_{i}')
    add(scene, box((.20, .08, .11), (0, depth * .52, .35), 'MAT_WOOD_WARM'), 'CartHandle')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        _wheel(scene, x * width * .44, y * depth * .42, .12, .10, f'SupplyWheel_{i}')
    return scene


def _laundry_machine(v, mat, dryer=False):
    scene = trimesh.Scene()
    width, depth, height = .82 + .06 * v, .74 + .05 * v, 1.18 + .09 * v
    add(scene, box((width, depth, height), (0, 0, height / 2 + .04), mat), 'DryerCabinet' if dryer else 'WasherCabinet')
    add(scene, box((width * 1.04, depth * 1.03, .07), (0, 0, height + .075), 'MAT_STAINLESS'), 'MachineTop')
    _round_panel(scene, .29 + .018 * v, .05, (0, depth / 2 + .025, .63), 'MAT_STAINLESS', 'DoorRing')
    _round_panel(scene, .225 + .014 * v, .055, (0, depth / 2 + .060, .63), 'MAT_GLASS_CLEAR', 'DoorWindow')
    add(scene, box((width * .80, .18, .18), (0, depth / 2 + .09, height - .22), 'MAT_ELECTRONICS'), 'ControlPanel')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * width * .25
        if dryer:
            add(scene, box((.09, .10, .06), (x, depth / 2 + .15, height - .24), 'MAT_BRASS_POLISHED'), f'ProgramButton_{i}')
        else:
            add(scene, cyl(.035, .04, (x, depth / 2 + .16, height - .20), 'MAT_STAINLESS', 12), f'CycleKnob_{i}')
    if dryer:
        for i in range(3 + v):
            add(scene, box((width * .55, .022, .035), (0, depth / 2 + .02, .26 + i * .12), 'MAT_BLACKENED_STEEL'), f'LintVent_{i}')
    return scene


def _vacuum(v, mat):
    scene = trimesh.Scene()
    width = .36 + .035 * v
    add(scene, box((width + .22, .35, .16), (0, 0, .13), 'MAT_BLACKENED_STEEL'), 'VacuumHead')
    add(scene, box((width, .28, .60 + .045 * v), (0, 0, .49), mat), 'VacuumBody')
    add(scene, cyl(.18 + .012 * v, .34 + .025 * v, (0, .12, .73), 'MAT_STAINLESS', 20), 'DustCanister')
    add(scene, cyl(.035, .82 + .05 * v, (0, .19, 1.45 + .025 * v), 'MAT_BLACKENED_STEEL', 12), 'HandleStem')
    add(scene, box((.46 + .04 * v, .07, .07), (0, .19, 1.87 + .05 * v), mat), 'HandleGrip')
    for i, x in enumerate((-.21, .21)):
        _wheel(scene, x, .12, .12, .10, f'VacuumWheel_{i}')
    return scene


def _floor_buffer(v, mat):
    scene = trimesh.Scene()
    radius = .32 + .045 * v
    add(scene, cyl(radius, .18, (0, 0, .13), mat, 24), 'PolisherHousing')
    add(scene, cyl(radius * .88, .05, (0, 0, .015), 'MAT_LINEN', 24), 'FloorPad')
    add(scene, box((.28 + .025 * v, .20, .34), (0, .12, .38), 'MAT_ELECTRONICS'), 'MotorCover')
    for side, x in enumerate((-.15, .15)):
        add(scene, box((.045, .045, .82 + .05 * v), (x, .17, .92 + .025 * v), 'MAT_STAINLESS'), f'BufferHandlePost_{side}')
    add(scene, box((.48 + .04 * v, .08, .07), (0, .17, 1.37 + .05 * v), mat), 'BufferHandle')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .11
        add(scene, box((.06, .035, .06), (x, .12, .53), 'MAT_BRASS_POLISHED'), f'BufferSwitch_{i}')
    return scene


def _mop_bucket(v, mat):
    scene = trimesh.Scene()
    width, depth = .54 + .055 * v, .46 + .04 * v
    add(scene, box((width, depth, .54 + .035 * v), (0, 0, .32 + .02 * v), mat), 'MopBucket')
    add(scene, box((width * .52, depth * .72, .34 + .025 * v), (width * .54, .02, .54 + .018 * v), 'MAT_STAINLESS'), 'WringerBasket')
    add(scene, box((.08, .08, .58 + .03 * v), (width * .55, depth * .12, .76 + .03 * v), 'MAT_STAINLESS'), 'MopHandle')
    add(scene, cyl(.10 + .01 * v, .28, (width * .55, depth * .12, 1.16 + .03 * v), 'MAT_LINEN', 14), 'MopHead')
    add(scene, box((width * .90, .08, .04), (0, 0, .63 + .035 * v), 'MAT_STAINLESS'), 'BucketRim')
    for i, x in enumerate((-width * .40, width * .40)):
        _wheel(scene, x, depth * .30, .10, .08, f'BucketWheel_{i}')
    return scene


def _utility_sink(v, mat):
    scene = trimesh.Scene()
    width, depth, top = .92 + .09 * v, .58 + .05 * v, .88
    add(scene, box((width, depth, .14), (0, 0, top), 'MAT_STAINLESS'), 'UtilityBasin')
    add(scene, box((width * .80, depth * .68, .05), (0, 0, top - .07), 'MAT_CERAMIC_FIXTURE'), 'BasinInset')
    for i, x in enumerate((-width * .42, width * .42)):
        add(scene, box((.06, .06, top - .10), (x, 0, (top - .10) / 2), 'MAT_STAINLESS'), f'SinkLeg_{i}')
    add(scene, box((width, .055, .48 + .035 * v), (0, depth * .48, 1.20 + .035 * v), mat), 'SinkBackSplash')
    add(scene, cyl(.022, .25, (0, .15, 1.06), 'MAT_STAINLESS', 10), 'SinkFaucet')
    add(scene, box((.34, .045, .035), (0, .11, .96), 'MAT_STAINLESS'), 'SinkSpout')
    return scene


def _chemical_cabinet(v, mat):
    scene = _casework(v, mat, 'armoire')
    width = .86 + .09 * v
    add(scene, box((width * .32, .03, .20), (0, .30, 1.20), 'MAT_SIGNAGE'), 'ChemicalHazardLabel')
    add(scene, box((width * .06, .035, .09), (width * .32, .32, 1.20), 'MAT_EMISSIVE_WARM'), 'HazardSymbol')
    for i in range(2 + v // 2):
        z = .56 + i * .28
        add(scene, box((width * .78, .04, .05), (0, .28, z), 'MAT_STAINLESS'), f'ChemicalShelf_{i}')
    return scene


def _tool_chest(v, mat):
    scene = trimesh.Scene()
    width, depth, height = .88 + .08 * v, .52 + .035 * v, .78 + .08 * v
    add(scene, box((width, depth, height), (0, 0, height / 2 + .18), mat), 'ToolChestBody')
    add(scene, box((width * 1.03, depth * 1.05, .07), (0, 0, height + .215), 'MAT_BLACKENED_STEEL'), 'ChestTop')
    for i in range(3 + v // 2):
        z = .32 + i * .18
        add(scene, box((width * .84, .025, .13), (0, depth / 2 + .015, z), 'MAT_WOOD_WARM'), f'ToolDrawer_{i}')
        add(scene, box((.20 + .01 * v, .035, .025), (0, depth / 2 + .045, z), 'MAT_BRASS_POLISHED'), f'DrawerHandle_{i}')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        _wheel(scene, x * width * .42, y * depth * .38, .12, .10, f'ChestCaster_{i}')
    return scene


def _housekeeping(family, v, mat):
    recipes = (
        lambda: _linen_cart(v, mat), lambda: _supply_cart(v, mat),
        lambda: _laundry_machine(v, mat, False), lambda: _laundry_machine(v, mat, True),
        lambda: _vacuum(v, mat), lambda: _floor_buffer(v, mat),
        lambda: _mop_bucket(v, mat), lambda: _utility_sink(v, mat),
        lambda: _chemical_cabinet(v, mat), lambda: _tool_chest(v, mat),
    )
    return recipes[family]()


def _wall_sconce(v, mat):
    scene = trimesh.Scene()
    width = .38 + .04 * v
    add(scene, box((width, .08, .58 + .045 * v), (0, 0, .98), 'MAT_WOOD_DARK'), 'WallPlate')
    add(scene, box((.07, .28 + .02 * v, .07), (0, .17, 1.08), 'MAT_BRASS_POLISHED'), 'SconceArm')
    add(scene, cyl(.22 + .018 * v, .27 + .025 * v, (0, .36, 1.30), mat, 22), 'LampShade')
    add(scene, sphere(.075 + .008 * v, (0, .36, 1.15), 'MAT_EMISSIVE_WARM', 1), 'SconceBulb')
    for i in range(2 + v // 2):
        z = .80 + i * .15
        add(scene, box((width * .65, .025, .03), (0, .05, z), 'MAT_BRASS_POLISHED'), f'PlateAccent_{i}')
    return scene


def _chandelier(v, mat):
    scene = trimesh.Scene()
    arms = 3 + v
    add(scene, cyl(.22 + .025 * v, .10, (0, 0, 2.15 + .08 * v), 'MAT_BRASS_POLISHED', 20), 'CeilingCanopy')
    add(scene, cyl(.035, .72 + .06 * v, (0, 0, 1.74 + .05 * v), mat, 14), 'DropRod')
    add(scene, sphere(.16 + .02 * v, (0, 0, 1.38 + .04 * v), 'MAT_BRASS_POLISHED', 1), 'CentralHub')
    for i in range(arms):
        angle = i * (2 * math.pi / arms)
        x, y = math.cos(angle), math.sin(angle)
        add(scene, cyl(.025, .54 + .04 * v, (.26 * x, .26 * y, 1.36 + .04 * v), 'MAT_BRASS_POLISHED', 10), f'ChandelierArm_{i}')
        add(scene, cyl(.13 + .012 * v, .12, (.48 * x, .48 * y, 1.08 + .035 * v), mat, 18), f'LampCup_{i}')
        add(scene, sphere(.055, (.48 * x, .48 * y, 1.18 + .035 * v), 'MAT_EMISSIVE_WARM', 1), f'Bulb_{i}')
    return scene


def _centerpiece(v, mat):
    scene = trimesh.Scene()
    radius = .24 + .04 * v
    add(scene, cyl(radius, .07, (0, 0, .10), 'MAT_STONE_LIGHT', 24), 'CenterpieceBase')
    add(scene, cyl(radius * .68, .10, (0, 0, .18), mat, 22), 'DecorativeBowl')
    for i in range(2 + v):
        angle = i * (2 * math.pi / (2 + v))
        x, y = radius * .42 * math.cos(angle), radius * .42 * math.sin(angle)
        if i % 2:
            add(scene, sphere(.095 + .008 * v, (x, y, .39 + .02 * v), 'MAT_VEGETATION', 1), f'Flower_{i}')
        else:
            add(scene, cyl(.025, .20 + .025 * v, (x, y, .32 + .02 * v), 'MAT_WOOD_DARK', 10), f'Candle_{i}')
            add(scene, sphere(.035, (x, y, .44 + .025 * v), 'MAT_EMISSIVE_WARM', 1), f'CandleFlame_{i}')
    return scene


def _framed_art(v, mat):
    scene = trimesh.Scene()
    width, height = 1.02 + .10 * v, .70 + .06 * v
    add(scene, box((width + .12, .08, height + .12), (0, 0, height / 2 + .08), 'MAT_WOOD_DARK'), 'ArtFrame')
    add(scene, box((width, .025, height), (0, .047, height / 2 + .08), mat), 'Canvas')
    add(scene, box((width * .74, .012, height * .08), (0, .068, height * .22 + .08), 'MAT_LINEN'), 'ArtworkLowerAccent')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * width * .30
        z = height * (.30 + .10 * (i % 3)) + .08
        add(scene, box((width * .20, .015, height * .20), (x, .066, z), 'MAT_BRASS_POLISHED'), f'ArtworkForm_{i}')
    return scene


def _clock(v, mat):
    scene = trimesh.Scene()
    radius = .34 + .045 * v
    _round_panel(scene, radius + .06, .10, (0, 0, 1.48), 'MAT_WOOD_WARM', 'ClockHousing')
    _round_panel(scene, radius, .035, (0, .065, 1.48), mat, 'ClockFace')
    add(scene, box((.025, .035, radius * .88), (0, .092, 1.48 + radius * .12), 'MAT_BLACKENED_STEEL'), 'HourHand')
    add(scene, box((radius * .62, .025, .018), (radius * .26, .095, 1.48), 'MAT_BRASS_POLISHED'), 'MinuteHand')
    for i in range(2 + v):
        angle = i * 2 * math.pi / (2 + v)
        x, z = radius * .82 * math.cos(angle), 1.48 + radius * .82 * math.sin(angle)
        add(scene, sphere(.025 + .004 * v, (x, .096, z), 'MAT_BRASS_POLISHED', 1), f'HourMarker_{i}')
    stand_height = .34 + .025 * v
    add(scene, box((.11, .045, stand_height), (0, 0, stand_height / 2), 'MAT_WOOD_DARK'), 'ClockStand')
    return scene


def _totem(v, mat):
    scene = trimesh.Scene()
    width, height = .52 + .055 * v, 1.72 + .12 * v
    add(scene, box((width * 1.26, .44, .12), (0, 0, .06), 'MAT_STONE_LIGHT'), 'TotemBase')
    add(scene, box((width, .18, height), (0, 0, height / 2 + .12), mat), 'WayfindingColumn')
    for i in range(2 + v):
        z = .50 + i * .34
        add(scene, box((width * 1.34, .12, .23), (0, .12, z), 'MAT_SIGNAGE'), f'DirectionPanel_{i}')
        add(scene, box((.20 + .02 * v, .025, .05), (-width * .28, .19, z), 'MAT_BRASS_POLISHED'), f'Arrow_{i}')
        add(scene, box((width * .40, .025, .035), (width * .16, .19, z), 'MAT_LINEN'), f'DestinationLabel_{i}')
    return scene


def _room_plaque(v, mat):
    scene = trimesh.Scene()
    width, height = .42 + .035 * v, .22 + .018 * v
    add(scene, box((width, .055, height), (0, 0, .08 + height / 2), 'MAT_WOOD_DARK'), 'PlaqueBacking')
    add(scene, box((width * .84, .025, height * .72), (0, .035, .08 + height / 2), mat), 'NumberField')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * width * .18
        add(scene, box((.025 + .003 * v, .018, height * .42), (x, .055, .08 + height / 2), 'MAT_BRASS_POLISHED'), f'RoomDigit_{i}')
    for side, x in enumerate((-width * .38, width * .38)):
        add(scene, cyl(.018, .02, (x, .055, .08 + height / 2), 'MAT_STAINLESS', 10), f'MountPin_{side}')
    return scene


def _entry_mat(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.15 + .14 * v, .72 + .10 * v
    add(scene, box((width, depth, .045), (0, 0, .0225), mat), 'EntryMatBody')
    for i in range(4 + v):
        y = -depth * .40 + i * depth * .80 / (3 + v)
        add(scene, box((width * .90, .025, .012), (0, y, .048), 'MAT_BLACKENED_STEEL'), f'GripRib_{i}')
    for side, x in enumerate((-width * .48, width * .48)):
        add(scene, box((.035, depth, .025), (x, 0, .055), 'MAT_BRASS_POLISHED'), f'Border_{side}')
    return scene


def _vase(v, mat):
    scene = trimesh.Scene()
    scale = 1 + .12 * v
    add(scene, cyl(.22 * scale, .09, (0, 0, .045), 'MAT_STONE_LIGHT', 20), 'VaseFoot')
    add(scene, sphere(.25 * scale, (0, 0, .34 * scale), mat, 2), 'VaseBody')
    add(scene, cyl(.12 * scale, .24 * scale, (0, 0, .63 * scale), mat, 18), 'VaseNeck')
    add(scene, cyl(.16 * scale, .055, (0, 0, .78 * scale), 'MAT_BRASS_POLISHED', 20), 'VaseRim')
    for i in range(2 + v):
        angle = i * 2 * math.pi / (2 + v)
        x, y = .08 * scale * math.cos(angle), .08 * scale * math.sin(angle)
        add(scene, cyl(.018, .34 * scale, (x, y, 1.00 * scale), 'MAT_VEGETATION', 8), f'FlowerStem_{i}')
        add(scene, sphere(.08, (x, y, 1.20 * scale), 'MAT_VEGETATION', 1), f'FlowerHead_{i}')
    return scene


def _decorative_screen(v, mat):
    scene = trimesh.Scene()
    panels = 2 + v // 2
    width, height = .46 + .04 * v, 1.56 + .10 * v
    total = width * panels
    for panel in range(panels):
        x = (panel - (panels - 1) / 2) * width
        add(scene, box((width, .06, height), (x, 0, height / 2 + .10), mat), f'ScreenPanel_{panel}')
        for i in range(3 + v):
            z = .24 + i * (height - .24) / (2 + v)
            add(scene, box((width * .82, .025, .035), (x, .045, z + .10), 'MAT_WOOD_WARM'), f'ScreenSlat_{panel}_{i}')
        add(scene, box((.055, .10, .14), (x, 0, .07), 'MAT_BLACKENED_STEEL'), f'ScreenFoot_{panel}')
    return scene


def _decor(family, v, mat):
    recipes = (
        lambda: _centerpiece(v, mat), lambda: _wall_sconce(v, mat),
        lambda: _chandelier(v, mat), lambda: _framed_art(v, mat),
        lambda: _clock(v, mat), lambda: _totem(v, mat),
        lambda: _room_plaque(v, mat), lambda: _entry_mat(v, mat),
        lambda: _vase(v, mat), lambda: _decorative_screen(v, mat),
    )
    return recipes[family]()


def _catalog_role(number):
    if number <= 1255:
        return 'Guest Honeymooner Woman', 'guest', 'ANSET_GUEST_LOCOMOTION'
    if number <= 1260:
        return 'Guest Accessible Traveler Man', 'guest', 'ANSET_GUEST_LOCOMOTION'
    if number <= 1265:
        return 'Hotel Spa Therapist Woman', 'staff', 'ANSET_HOUSEKEEPING'
    if number <= 1270:
        return 'Pool Lifeguard Attendant Man', 'staff', 'ANSET_SECURITY'
    if number <= 1275:
        return 'Conference Event Coordinator Woman', 'staff', 'ANSET_MANAGER'
    return 'Hotel Valet Attendant Man', 'staff', 'ANSET_BELL_SERVICE'


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 1251 <= number <= 1500:
        raise ValueError(f'catalog expansion asset ID outside A1251–A1500: {asset_id}')
    if number <= 1280:
        role_name, role, _animation = _catalog_role(number)
        return build_character(role_name, role, mat, asset_id, profile)
    if number <= 1350:
        family, variant = divmod(number - 1281, 5)
        return _guestroom(family, variant, mat)
    if number <= 1400:
        family, variant = divmod(number - 1351, 5)
        return _restaurant(family, variant, mat)
    if number <= 1450:
        family, variant = divmod(number - 1401, 5)
        return _housekeeping(family, variant, mat)
    family, variant = divmod(number - 1451, 5)
    return _decor(family, variant, mat)
