"""Purpose-built guest, staff, amenity and lobby assets for A1051–A1250."""

import trimesh

from character_v2_factory import build_asset as build_character
from v2_asset_common import add, box, cyl, sphere


def _legs(scene, width, depth, height, material='MAT_BLACKENED_STEEL'):
    for i, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.055, .055, height),
                       (x * width * .42, y * depth * .38, height / 2), material), f'Leg_{i}')


def _bed(v, mat):
    scene = trimesh.Scene()
    width, length, height = .78 + .09 * v, 1.72 + .12 * v, .36 + .025 * v
    add(scene, box((width, length, .16), (0, 0, height), 'MAT_WOOD_WARM'), 'BedFrame')
    add(scene, box((width * .96, length * .96, .20), (0, 0, height + .18), mat), 'Mattress')
    add(scene, box((width * 1.04, .11, .66 + .035 * v),
                   (0, length * .47, height + .48), 'MAT_WOOD_WARM'), 'Headboard')
    _legs(scene, width, length, height - .06)
    for i in range(1 + v // 2):
        x = (i - v / 4) * width * .28
        add(scene, box((width * .27, .32, .09), (x, length * .27, height + .33),
                       'MAT_LINEN'), f'Pillow_{i}')
    add(scene, box((width * .58, .22, .07), (0, -length * .34, height + .32),
                   'MAT_LINEN'), 'FoldedCover')
    return scene


def _sauna_bench(v, mat):
    scene = trimesh.Scene()
    width, depth, seat_z = 1.0 + .14 * v, .40 + .04 * v, .48 + .025 * v
    _legs(scene, width, depth, seat_z - .05, 'MAT_WOOD_DARK')
    for i in range(3 + v):
        y = -.16 + i * .08
        add(scene, box((width, .065, .055), (0, y, seat_z), mat), f'SeatSlat_{i}')
    for side, x in enumerate((-.44, .44)):
        add(scene, box((.07, .07, .58 + .03 * v), (x, depth * .38, .79), mat), f'BackPost_{side}')
    for row, z in enumerate((.73, .98 + .03 * v)):
        add(scene, box((width * .90, .055, .07), (0, depth * .38, z), mat), f'BackRail_{row}')
    if v >= 2:
        add(scene, box((width * .72, .27, .15), (0, -.35, .18), mat), 'LowerStep')
    return scene


def _towel_warmer(v, mat):
    scene = trimesh.Scene()
    width, height = .72 + .08 * v, 1.25 + .11 * v
    add(scene, box((width, .48, height), (0, 0, height / 2 + .07), mat), 'CabinetBody')
    add(scene, box((width * .88, .04, height * .80), (0, -.26, height * .55),
                   'MAT_CERAMIC_FIXTURE'), 'DoorPair')
    add(scene, box((width * .90, .54, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'Plinth')
    for i in range(2 + v):
        z = .28 + i * (height - .42) / (1 + v)
        add(scene, box((width * .78, .025, .035), (0, -.285, z), 'MAT_BRASS_POLISHED'),
                   f'DoorPull_{i}')
    for i in range(2 + v):
        add(scene, box((.28, .035, .045),
                       (0, .255, .35 + i * .11), 'MAT_STAINLESS'), f'Vent_{i}')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * .12
        add(scene, cyl(.075, .23, (x, -.02, height + .13), 'MAT_LINEN', 16), f'TowelRoll_{i}')
    return scene


def _treadmill(v, mat):
    scene = trimesh.Scene()
    length, width = 1.30 + .13 * v, .56 + .045 * v
    add(scene, box((length, width, .13), (0, .02, .22), mat), 'Deck')
    add(scene, box((length * .88, width * .78, .035), (0, .02, .305), 'MAT_ELECTRONICS'), 'RunningBelt')
    for i, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.10, .10, .15),
                       (x * length * .40, .02 + y * width * .37, .075),
                       'MAT_BLACKENED_STEEL'), f'DeckSupport_{i}')
    for i, x in enumerate((-.42, .42)):
        add(scene, box((.08, .08, .93 + .04 * v), (x * width, .48, .74), mat), f'ConsolePost_{i}')
        add(scene, box((.07, .76, .07), (x * width, .15, .43), 'MAT_BLACKENED_STEEL'), f'SideRail_{i}')
    add(scene, box((.50 + .05 * v, .09, .30), (0, .48, 1.28 + .04 * v),
                   'MAT_ELECTRONICS'), 'ControlConsole')
    add(scene, box((.30 + .03 * v, .018, .16), (0, .425, 1.29 + .04 * v),
                   'MAT_GLASS_CLEAR'), 'ConsoleDisplay')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width * .20
        add(scene, box((.025, .035, .045), (x, .418, 1.32 + .04 * v),
                       'MAT_EMISSIVE_WARM'), f'ConsoleButton_{i}')
    for i, x in enumerate((-.44, .44)):
        add(scene, cyl(.10, .045, (x * width, -.55, .17), 'MAT_BLACKENED_STEEL', 14), f'Roller_{i}')
    return scene


def _cycle(v, mat):
    scene = trimesh.Scene()
    spread = .38 + .035 * v
    add(scene, box((1.0 + .10 * v, .12, .08), (0, 0, .06), mat), 'StabilizerBase')
    add(scene, cyl(.27 + .025 * v, .07, (0, -.34, .34), mat, 24), 'Flywheel')
    add(scene, box((.07, .07, .52), (-spread, .02, .30), mat), 'RearFrame')
    add(scene, box((.07, .07, .64), (spread, -.18, .34), mat), 'FrontFrame')
    add(scene, box((spread * 2, .08, .07), (0, -.08, .60), mat), 'FrameBrace')
    add(scene, box((.32, .20, .08), (-spread, .04, .65), 'MAT_UPHOLSTERY'), 'Seat')
    add(scene, box((.07, .07, .36), (spread, -.18, .73), mat), 'HandlePost')
    add(scene, box((.46 + .04 * v, .065, .055), (spread, -.18, .92), mat), 'Handlebar')
    add(scene, box((.28, .10, .18), (spread, -.18, 1.16), 'MAT_ELECTRONICS'), 'Display')
    add(scene, box((.22, .018, .12), (spread, -.235, 1.17), 'MAT_GLASS_CLEAR'), 'DisplayGlass')
    for side, x in enumerate((-.16, .16)):
        add(scene, box((.055, .22, .04), (x, -.34, .28), 'MAT_BLACKENED_STEEL'), f'Pedal_{side}')
    return scene


def _strength_station(v, mat):
    scene = trimesh.Scene()
    width, height = .82 + .07 * v, 1.75 + .10 * v
    add(scene, box((width, .64, .07), (0, 0, .035), mat), 'BaseFrame')
    for i, x in enumerate((-width * .42, width * .42)):
        add(scene, box((.07, .09, height), (x, .20, height / 2), mat), f'WeightPost_{i}')
    add(scene, box((width, .10, .08), (0, .20, height - .04), mat), 'TopBeam')
    for i in range(3 + v):
        z = .20 + i * .12
        add(scene, box((.35, .28, .075), (0, .20, z), 'MAT_STAINLESS'), f'WeightPlate_{i}')
    add(scene, box((.045, .045, .62), (0, -.05, .70), 'MAT_BLACKENED_STEEL'), 'Cable')
    add(scene, box((.42, .10, .055), (0, -.05, .40), 'MAT_BRASS_POLISHED'), 'PullBar')
    add(scene, box((.44 + .035 * v, .34, .10), (0, -.30, .45), 'MAT_UPHOLSTERY'), 'TrainingSeat')
    for i, x in enumerate((-.24, .24)):
        add(scene, box((.07, .08, .62), (x, -.30, .78), 'MAT_BLACKENED_STEEL'), f'SeatSupport_{i}')
    return scene


def _pool_lounger(v, mat):
    scene = trimesh.Scene()
    width, length = .66 + .045 * v, 1.62 + .10 * v
    frame_z = .24 + .018 * v
    add(scene, box((width, length, .11), (0, 0, frame_z), 'MAT_BLACKENED_STEEL'), 'LoungerFrame')
    for i, (x, y) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        add(scene, box((.05, .05, frame_z - .06),
                       (x * width * .42, y * length * .42, (frame_z - .06) / 2),
                       'MAT_STAINLESS'), f'Support_{i}')
    add(scene, box((width * .95, length * .54, .10), (0, -length * .22, frame_z + .11), mat), 'SeatCushion')
    back_z = frame_z + .46 + .035 * v
    back = box((width * .95, .13, .72 + .04 * v), (0, length * .30, back_z), mat)
    back.apply_transform(trimesh.transformations.rotation_matrix(
        -.10 - .035 * v, [1, 0, 0], [0, length * .30, back_z]))
    add(scene, back, 'AdjustableBack')
    for i in range(1 + v // 2):
        x = (i - v / 4) * width * .32
        add(scene, box((width * .24, .20, .055), (x, -length * .32, frame_z + .18),
                       'MAT_LINEN'), f'Headrest_{i}')
    return scene


def _umbrella(v, mat):
    scene = trimesh.Scene()
    radius, height, count = .70 + .10 * v, 2.05 + .12 * v, 4 + v
    add(scene, cyl(.30 + .035 * v, .09, (0, 0, .045), 'MAT_STONE_LIGHT', 20), 'WeightedBase')
    add(scene, cyl(.045, height, (0, 0, height / 2), 'MAT_STAINLESS', 18), 'CenterPole')
    add(scene, cyl(radius * .12, .06, (0, 0, height), 'MAT_BLACKENED_STEEL', 14), 'Hub')
    for i in range(count):
        angle = i * 6.283185307179586 / count
        x, y = radius * .46 * __import__('math').cos(angle), radius * .46 * __import__('math').sin(angle)
        panel = box((radius * .72, radius * .72, .075), (x, y, height - .04), mat)
        panel.apply_transform(trimesh.transformations.rotation_matrix(angle, [0, 0, 1]))
        add(scene, panel, f'CanopyPanel_{i}')
        rib = box((radius * .82, .028, .028), (x, y, height - .095), 'MAT_BLACKENED_STEEL')
        rib.apply_transform(trimesh.transformations.rotation_matrix(angle, [0, 0, 1]))
        add(scene, rib, f'Rib_{i}')
    return scene


def _lectern(v, mat):
    scene = trimesh.Scene()
    height, width = 1.05 + .09 * v, .62 + .07 * v
    add(scene, box((.66 + .06 * v, .52, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'Base')
    add(scene, box((width * .58, .32, height * .72), (0, .02, height * .36), mat), 'Pedestal')
    top = box((width, .46, .08), (0, -.05, height), mat)
    top.apply_transform(trimesh.transformations.rotation_matrix(-.12 - .025 * v, [1, 0, 0], [0, -.05, height]))
    add(scene, top, 'ReadingSurface')
    add(scene, box((width * .90, .07, .09), (0, -.29, height + .02), 'MAT_WOOD_DARK'), 'PaperStop')
    add(scene, cyl(.018, .38 + .04 * v, (width * .32, .03, height + .22),
                   'MAT_BLACKENED_STEEL', 12), 'MicrophoneStem')
    add(scene, sphere(.035, (width * .32, -.02, height + .43 + .04 * v),
                      'MAT_BLACKENED_STEEL', 1), 'MicrophoneHead')
    add(scene, box((.18, .025, .08), (0, -.171, height * .54), 'MAT_BRASS_POLISHED'), 'Nameplate')
    return scene


def _stage(v, mat):
    scene = trimesh.Scene()
    width, depth, height = 1.55 + .16 * v, 1.20 + .12 * v, .20 + .06 * v
    add(scene, box((width, depth, height), (0, 0, height / 2), mat), 'StageDeck')
    add(scene, box((width * .96, depth * .94, .04), (0, 0, height + .02), 'MAT_WOOD_DARK'), 'StageSurface')
    for i, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.08, .08, height), (x * width * .42, y * depth * .40, height / 2),
                       'MAT_BLACKENED_STEEL'), f'Support_{i}')
    add(scene, box((width, .045, .11), (0, -depth / 2 - .015, height * .60), 'MAT_BRASS_POLISHED'), 'FrontFascia')
    for i in range(2 + v):
        x = width * ((i + .5) / (2 + v) - .5) * .88
        add(scene, box((.035, .035, .035), (x, depth / 2 + .035, .035), 'MAT_STAINLESS'), f'Joiner_{i}')
    if v >= 2:
        for i in range(1 + v // 2):
            add(scene, box((.48 + .04 * v, .24, .12),
                           (0, -depth / 2 - .12 - .22 * i, .06 + .12 * i), mat), f'AccessStep_{i}')
    return scene


def _settee(v, mat):
    scene = trimesh.Scene()
    seats, width = 1 + v // 2, .92 + .15 * v
    add(scene, box((width, .64, .18), (0, 0, .47), 'MAT_WOOD_DARK'), 'SeatBase')
    add(scene, box((width, .13, .66), (0, .25, .80), mat), 'BackFrame')
    for i in range(seats):
        x = (i - (seats - 1) / 2) * width / seats
        add(scene, box((width / seats * .90, .52, .11), (x, -.015, .62), mat), f'SeatCushion_{i}')
        add(scene, box((width / seats * .90, .09, .45), (x, .20, .89), 'MAT_UPHOLSTERY'), f'BackCushion_{i}')
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.12, .68, .42 + .04 * v), (x, 0, .65), mat), f'Arm_{side}')
    _legs(scene, width, .64, .36)
    return scene


def _sofa(v, mat):
    scene = trimesh.Scene()
    seats = 2 + v // 2
    width = 1.28 + .18 * v
    add(scene, box((width, .76, .20), (0, 0, .45), 'MAT_WOOD_DARK'), 'SofaBase')
    add(scene, box((width, .14, .70), (0, .30, .85), mat), 'SofaBack')
    for i in range(seats):
        x = (i - (seats - 1) / 2) * width / seats
        add(scene, box((width / seats * .88, .62, .12), (x, -.03, .61), mat), f'Seat_{i}')
        add(scene, box((width / seats * .88, .11, .52), (x, .20, .91), 'MAT_UPHOLSTERY'), f'BackCushion_{i}')
    for side, x in enumerate((-width / 2, width / 2)):
        add(scene, box((.14, .72, .44), (x, 0, .67), mat), f'Arm_{side}')
    _legs(scene, width, .76, .34)
    return scene


def _stanchion(v, mat):
    scene = trimesh.Scene()
    poles = 3 + v
    xs = [((i / (poles - 1)) - .5) * (1.25 + .12 * v) for i in range(poles)]
    for i, x in enumerate(xs):
        add(scene, cyl(.16 + .012 * v, .07, (x, 0, .035), 'MAT_STONE_LIGHT', 18), f'Base_{i}')
        add(scene, cyl(.035, .90 + .035 * v, (x, 0, .52), mat, 16), f'Post_{i}')
        add(scene, sphere(.055, (x, 0, 1.00 + .035 * v), mat, 1), f'Finial_{i}')
    for i in range(poles - 1):
        length = xs[i + 1] - xs[i]
        add(scene, box((length, .045, .055), ((xs[i] + xs[i + 1]) / 2, 0, .78), 'MAT_SIGNAGE'), f'Rope_{i}')
    return scene


def _kiosk(v, mat):
    scene = trimesh.Scene()
    height = 1.48 + .12 * v
    add(scene, box((.58 + .06 * v, .48, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'Base')
    add(scene, box((.26, .26, .72 + .05 * v), (0, .02, .43 + .025 * v), mat), 'Pedestal')
    add(scene, box((.72 + .07 * v, .13, .72 + .05 * v), (0, 0, height), 'MAT_ELECTRONICS'), 'DisplayHousing')
    add(scene, box((.61 + .05 * v, .018, .52 + .04 * v), (0, -.075, height), 'MAT_GLASS_CLEAR'), 'TouchDisplay')
    add(scene, box((.62, .025, .08), (0, -.09, height - .33), 'MAT_EMISSIVE_WARM'), 'StatusBar')
    add(scene, box((.78 + .07 * v, .15, .08), (0, 0, height + .39 + .04 * v), mat), 'DisplayHeader')
    return scene


def _desk(v, mat):
    scene = trimesh.Scene()
    width, depth = 1.55 + .15 * v, .68 + .05 * v
    add(scene, box((width, depth, .13), (0, 0, .91), mat), 'Countertop')
    add(scene, box((width * .88, depth * .60, .72), (0, depth * .07, .40), mat), 'DeskBase')
    add(scene, box((width * .90, .035, .43), (0, -depth / 2 - .02, .63), 'MAT_WOOD_DARK'), 'FrontPanel')
    add(scene, box((width * .92, .07, .11), (0, 0, 1.03), 'MAT_BRASS_POLISHED'), 'CounterRail')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * width * .66 / (1 + v)
        add(scene, box((.08, .10, .32), (x, .02, .72), 'MAT_BLACKENED_STEEL'), f'Pilaster_{i}')
    add(scene, box((.36, .04, .22), (width * .20, -.16, 1.10), 'MAT_ELECTRONICS'), 'CheckInTerminal')
    add(scene, box((.28, .02, .16), (width * .20, -.19, 1.10), 'MAT_GLASS_CLEAR'), 'TerminalScreen')
    for i in range(1 + v // 2):
        add(scene, box((.24, .035, .02), (-width * .30 + i * .12, -.17, .74), 'MAT_BRASS_POLISHED'), f'DrawerPull_{i}')
    return scene


def _bell_cart(v, mat):
    scene = trimesh.Scene()
    width, depth, height = .72 + .08 * v, .92 + .10 * v, 1.38 + .08 * v
    add(scene, box((width, depth, .12), (0, 0, .16), 'MAT_BRASS_POLISHED'), 'Platform')
    for i, x in enumerate((-width * .43, width * .43)):
        add(scene, box((.07, .07, height), (x, depth * .40, height / 2), mat), f'FramePost_{i}')
    add(scene, box((width, .07, .07), (0, depth * .40, height), mat), 'TopRail')
    add(scene, box((width * .72, .025, .045), (0, depth * .40, .38), 'MAT_WOOD_WARM'), 'LowerRail')
    add(scene, box((width * .72, .025, .045), (0, depth * .40, .74), 'MAT_WOOD_WARM'), 'MiddleRail')
    add(scene, box((width * .28, .08, .10), (0, -depth * .48, .30), 'MAT_BLACKENED_STEEL'), 'TowHandle')
    for i, x in enumerate((-width * .38, width * .38)):
        wheel_radius = .12 + .01 * v
        center = (x, depth * .40, wheel_radius)
        wheel = cyl(wheel_radius, .055, center, 'MAT_BLACKENED_STEEL', 18)
        wheel.apply_transform(trimesh.transformations.rotation_matrix(
            1.57079632679, [1, 0, 0], point=center))
        add(scene, wheel, f'MOV_Wheel_{i}')
    add(scene, box((width * .64, .035, .12), (0, -depth * .51, 1.10), 'MAT_SIGNAGE'), 'HotelCrest')
    return scene


def _planter(v, mat):
    scene = trimesh.Scene()
    width, depth, height = .68 + .09 * v, .52 + .06 * v, .48 + .035 * v
    add(scene, box((width + .08, depth + .08, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'PlanterFoot')
    add(scene, box((width, depth, height), (0, 0, height / 2 + .07), mat), 'PlanterBody')
    add(scene, box((width + .07, depth + .07, .08), (0, 0, height + .10), 'MAT_BRASS_POLISHED'), 'PlanterRim')
    for i in range(2 + v):
        x = width * ((i + .5) / (2 + v) - .5) * .78
        z = height + .28 + .04 * (i % 2)
        add(scene, cyl(.035 + .004 * v, .36 + .03 * v, (x, 0, z), 'MAT_WOOD_DARK', 10), f'Stem_{i}')
        add(scene, sphere(.16 + .01 * v, (x, 0, z + .20), 'MAT_VEGETATION'), f'Foliage_{i}')
    return scene


def _sculpture(v, mat):
    scene = trimesh.Scene()
    width = .58 + .09 * v
    add(scene, box((.54 + .04 * v, .48, .14), (0, 0, .07), 'MAT_STONE_LIGHT'), 'Plinth')
    add(scene, cyl(.28 + .018 * v, .035, (0, 0, .17), 'MAT_STAINLESS', 20), 'SculptureCollar')
    for i in range(2 + v):
        z = .30 + i * .19
        radius = .17 + .025 * ((i + v) % 3)
        form = sphere(radius, (0, 0, z), mat)
        form.apply_scale([1.0 + .04 * v, .76 + .04 * (i % 2), 1.0 + .06 * ((v + i) % 2)])
        add(scene, form, f'SculptedForm_{i}')
    spine_height = .82 + .06 * v
    add(scene, cyl(.025 + .006 * v, spine_height,
                   (width * .32, 0, .06 + spine_height / 2), 'MAT_BRASS_POLISHED', 12), 'AccentSpine')
    return scene


def _shop_shelf(v, mat):
    scene = trimesh.Scene()
    width, height = .92 + .08 * v, 1.25 + .10 * v
    add(scene, box((width, .38, .07), (0, 0, .035), 'MAT_BLACKENED_STEEL'), 'Base')
    for i, x in enumerate((-width * .44, width * .44)):
        add(scene, box((.06, .07, height), (x, 0, height / 2), mat), f'Upright_{i}')
    shelf_count = 2 + v
    for i in range(shelf_count):
        z = .28 + i * (height - .34) / (shelf_count - 1)
        add(scene, box((width * .92, .34, .055), (0, 0, z), mat), f'Shelf_{i}')
        for j in range(2 + v // 2):
            x = (j - (1 + v // 2) / 2) * width * .58 / (1 + v // 2)
            add(scene, box((.14 + .01 * v, .18, .18 + .015 * (j % 2)),
                           (x, -.035, z + .12), 'MAT_CERAMIC_FIXTURE' if i % 2 else 'MAT_SIGNAGE'),
                       f'Merchandise_{i}_{j}')
    return scene


def _floor_lamp(v, mat):
    scene = trimesh.Scene()
    height, radius = 1.58 + .12 * v, .16 + .018 * v
    add(scene, cyl(radius, .09, (0, 0, .045), 'MAT_BRASS_POLISHED', 20), 'WeightedBase')
    add(scene, cyl(.035 + .004 * v, height * .76, (0, 0, .09 + height * .38), mat, 16), 'LampStem')
    add(scene, cyl(.26 + .025 * v, .26 + .02 * v, (0, 0, height * .83), 'MAT_LINEN', 24), 'LampShade')
    add(scene, sphere(.07 + .008 * v, (0, 0, height * .70), 'MAT_EMISSIVE_WARM', 1), 'LightDiffuser')
    for i in range(2 + v):
        angle = i * 6.283185307179586 / (2 + v)
        x = .075 * __import__('math').cos(angle)
        y = .075 * __import__('math').sin(angle)
        add(scene, cyl(.012, .08, (x, y, height * .68), 'MAT_BRASS_POLISHED', 8), f'ShadeRib_{i}')
    return scene


AMENITY_BUILDERS = (
    _bed, _sauna_bench, _towel_warmer, _treadmill, _cycle,
    _strength_station, _pool_lounger, _umbrella, _lectern, _stage,
)

PUBLIC_BUILDERS = (
    _settee, _sofa, _stanchion, _kiosk, _desk,
    _bell_cart, _planter, _sculpture, _shop_shelf, _floor_lamp,
)


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 1051 <= number <= 1250:
        raise ValueError(f'extended hotel asset ID outside A1051–A1250: {asset_id}')
    if number <= 1150:
        return build_character(name, subcategory, mat, asset_id, profile)
    if number <= 1200:
        family, variant = divmod(number - 1151, 5)
        return AMENITY_BUILDERS[family](variant, mat)
    family, variant = divmod(number - 1201, 5)
    return PUBLIC_BUILDERS[family](variant, mat)
