"""Purpose-built hotel operations, food service and event assets for A1751–A2000."""

import math

import trimesh

from v2_asset_common import add, box, cyl, sphere


def _rot(mesh, angle, axis, point=None):
    mesh.apply_transform(trimesh.transformations.rotation_matrix(angle, axis, point=point))
    return mesh


def _legs(scene, width, depth, height, material='MAT_BLACKENED_STEEL', prefix='Leg'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.06, .06, height), (x * width * .42, y * depth * .38, height / 2), material),
            f'{prefix}_{index}')


def _wheels(scene, width, depth, z=.10, prefix='Caster'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        center = (x * width * .40, y * depth * .40, z)
        wheel = cyl(.10, .055, center, 'MAT_BLACKENED_STEEL', 16)
        _rot(wheel, math.pi / 2, [1, 0, 0], center)
        add(scene, wheel, f'{prefix}_{index}')


def _fasteners(scene, width, y, z, count=2, material='MAT_BRASS_POLISHED', prefix='Fastener'):
    for i in range(count):
        x = (i - (count - 1) / 2) * width / max(1, count)
        add(scene, cyl(.022, .025, (x, y, z), material, 10), f'{prefix}_{i}')


def _fire_door(v, mat):
    scene = trimesh.Scene(); w, h = 1.18 + .06 * v, 2.24 + .08 * v
    for side, x in enumerate((-w / 2, w / 2)):
        add(scene, box((.13, .24, h), (x, 0, h / 2), 'MAT_STONE_LIGHT'), f'FireDoorJamb_{side}')
        add(scene, box((.18, .30, .09), (x, 0, .045), 'MAT_BLACKENED_STEEL'), f'FireDoorFoot_{side}')
    add(scene, box((w + .18, .28, .15), (0, 0, h - .075), 'MAT_STONE_LIGHT'), 'FireDoorHeader')
    add(scene, box((w * .80, .07, h - .30), (0, .12, (h - .30) / 2), mat), 'FireRatedLeaf')
    add(scene, box((w * .72, .07, .12), (0, .18, .92), 'MAT_STAINLESS'), 'PanicBar')
    add(scene, box((.28, .05, .08), (0, .16, h - .22), 'MAT_STAINLESS'), 'DoorCloser')
    add(scene, box((.20, .035, .14), (-w * .27, -.08, 1.42), 'MAT_SIGNAGE'), 'FireRatingPlaque')
    _fasteners(scene, .55, .18, .28 + .06 * v, 3, prefix='HingePin')
    return scene


def _louver_door(v, mat):
    scene = trimesh.Scene(); w, h = 1.08 + .06 * v, 2.08 + .07 * v
    add(scene, box((w + .22, .16, h + .16), (0, 0, (h + .16) / 2), 'MAT_STONE_LIGHT'), 'LouverDoorFrame')
    add(scene, box((w, .06, h), (0, -.02, h / 2), mat), 'LouverDoorLeaf')
    for i in range(5 + v):
        z = .48 + i * (.12 + .008 * v)
        louver = box((w * .62, .05, .045), (0, -.065, z), 'MAT_STAINLESS')
        _rot(louver, -.18, [1, 0, 0], (0, -.065, z)); add(scene, louver, f'DoorLouver_{i}')
    add(scene, box((.18, .05, .08), (w * .32, -.08, 1.05), 'MAT_BRASS_POLISHED'), 'LeverHandle')
    add(scene, box((.045, .04, .24), (w * .32, -.08, 1.05), 'MAT_STAINLESS'), 'HandleBackplate')
    return scene


def _transom_door(v, mat):
    scene = trimesh.Scene(); w, h = 1.28 + .07 * v, 2.42 + .06 * v
    for side, x in enumerate((-w / 2, w / 2)):
        add(scene, box((.12, .20, h), (x, 0, h / 2), mat), f'TransomJamb_{side}')
    add(scene, box((w, .08, h * .70), (0, .05, h * .35), 'MAT_WOOD_WARM'), 'TransomDoorLeaf')
    add(scene, box((w * .94, .045, h * .22), (0, .08, h * .83), 'MAT_GLASS_CLEAR'), 'GlazedTransom')
    add(scene, box((w + .18, .22, .11), (0, 0, h + .055), 'MAT_STONE_LIGHT'), 'TransomHeader')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w / (2 + v // 2)
        add(scene, box((.035, .06, h * .21), (x, .12, h * .83), 'MAT_BLACKENED_STEEL'), f'TransomMullion_{i}')
    add(scene, box((.05, .06, .30), (w * .35, -.02, 1.10), 'MAT_BRASS_POLISHED'), 'DoorPull')
    return scene


def _ceiling_hatch(v, mat):
    scene = trimesh.Scene(); w, d = .72 + .06 * v, .62 + .05 * v
    add(scene, box((w + .12, d + .12, .09), (0, 0, .045), 'MAT_STONE_LIGHT'), 'HatchCasing')
    add(scene, box((w, d, .09), (0, 0, .12), mat), 'HatchPanel')
    add(scene, box((w * .76, .035, .025), (0, -d * .28, .175), 'MAT_BLACKENED_STEEL'), 'HatchSeam')
    add(scene, box((.12, .07, .035), (w * .31, 0, .185), 'MAT_BRASS_POLISHED'), 'HatchLatch')
    for side, x in enumerate((-w * .40, w * .40)):
        add(scene, box((.06, .08, .035), (x, 0, .19), 'MAT_STAINLESS'), f'HatchHinge_{side}')
    add(scene, box((.24, .18, .025), (0, d * .34, .18), 'MAT_SIGNAGE'), 'HatchServiceLabel')
    _fasteners(scene, w, -.38, .09, 4, prefix='CasingScrew')
    return scene


def _hvac_grille(v, mat):
    scene = trimesh.Scene(); w, h = .86 + .08 * v, .52 + .05 * v
    add(scene, box((w, .08, h), (0, 0, h / 2), 'MAT_STONE_LIGHT'), 'ReturnGrilleBackplate')
    add(scene, box((w * .92, .045, h * .86), (0, -.05, h / 2), mat), 'GrilleRecess')
    for i in range(5 + v):
        z = h * .20 + i * h * .60 / (4 + v)
        add(scene, box((w * .80, .055, .025), (0, -.085, z), 'MAT_STAINLESS'), f'GrilleBlade_{i}')
    for side, x in enumerate((-w * .46, w * .46)):
        add(scene, box((.035, .06, h * .90), (x, -.04, h / 2), 'MAT_BRASS_POLISHED'), f'GrilleFrame_{side}')
    return scene


def _bumper_rail(v, mat):
    scene = trimesh.Scene(); length, z = 1.55 + .14 * v, .72 + .08 * v
    add(scene, box((length, .13, .18), (0, 0, z), mat), 'WallBumperRail')
    add(scene, box((length * .96, .035, .035), (0, -.085, z + .04), 'MAT_STAINLESS'), 'RailAccent')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * length / (2 + v)
        add(scene, box((.06, .20, .13), (x, .02, z - .22), 'MAT_BLACKENED_STEEL'), f'RailBracket_{i}')
        add(scene, cyl(.03, .025, (x, -.105, z - .22), 'MAT_BRASS_POLISHED', 10), f'BracketScrew_{i}')
    add(scene, box((.18, .15, .21), (-length * .50, 0, z), 'MAT_STONE_LIGHT'), 'RailEndCap')
    return scene


def _threshold(v, mat):
    scene = trimesh.Scene(); w, d = 1.30 + .10 * v, .32 + .03 * v
    add(scene, box((w, d, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'ThresholdBase')
    add(scene, box((w * .92, d * .70, .045), (0, 0, .102), mat), 'ThresholdWearStrip')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w / (4 + v)
        add(scene, box((.035, d * .70, .012), (x, 0, .132), 'MAT_BLACKENED_STEEL'), f'AntiSlipGroove_{i}')
    for side, x in enumerate((-w * .45, w * .45)):
        add(scene, cyl(.028, .025, (x, 0, .132), 'MAT_BRASS_POLISHED', 10), f'AnchorCap_{side}')
    return scene


def _elevator_reveal(v, mat):
    scene = trimesh.Scene(); w, h = 1.56 + .10 * v, 2.44 + .07 * v
    for side, x in enumerate((-w / 2, w / 2)):
        add(scene, box((.16, .22, h), (x, 0, h / 2), mat), f'LiftRevealJamb_{side}')
        add(scene, box((.06, .28, h * .94), (x * .84, -.02, h / 2), 'MAT_STAINLESS'), f'LiftTrim_{side}')
    add(scene, box((w + .25, .28, .18), (0, 0, h - .09), 'MAT_STONE_LIGHT'), 'LiftRevealHeader')
    add(scene, box((.42, .06, .10), (0, -.15, h - .05), 'MAT_BRASS_POLISHED'), 'FloorIndicator')
    add(scene, box((.12, .08, .46), (w * .61, -.04, 1.42), 'MAT_ELECTRONICS'), 'CallStation')
    for i in range(2 + v // 2):
        add(scene, sphere(.028, (w * .61, -.10, 1.34 + .12 * i), 'MAT_EMISSIVE_WARM', 1), f'CallButton_{i}')
    return scene


def _corner_guard(v, mat):
    scene = trimesh.Scene(); h, width = 1.55 + .12 * v, .16 + .015 * v
    add(scene, box((width, .10, h), (-width / 2, 0, h / 2), mat), 'CornerGuardFaceA')
    add(scene, box((.10, width, h), (0, width / 2, h / 2), mat), 'CornerGuardFaceB')
    add(scene, box((width * 1.25, width * 1.25, .10), (0, 0, h + .05), 'MAT_STONE_LIGHT'), 'GuardCrown')
    for i in range(2 + v):
        z = .28 + i * (h - .50) / max(1, 1 + v)
        add(scene, cyl(.022, .025, (-width / 2 - .015, -.06, z), 'MAT_STAINLESS', 10), f'GuardFastenerA_{i}')
        add(scene, cyl(.022, .025, (.06, width / 2 + .015, z), 'MAT_STAINLESS', 10), f'GuardFastenerB_{i}')
    return scene


def _acoustic_cloud(v, mat):
    scene = trimesh.Scene(); w, d = 1.10 + .12 * v, .82 + .08 * v
    add(scene, box((w, d, .10), (0, 0, 2.10), mat), 'AcousticCloudPanel')
    for side, x in enumerate((-.38, .38)):
        add(scene, cyl(.025, .55 + .04 * v, (x, 0, 2.42), 'MAT_STAINLESS', 10), f'SuspensionRod_{side}')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w / (3 + v)
        add(scene, box((.035, d * .84, .18), (x, 0, 2.24), 'MAT_WOOD_WARM'), f'AcousticBaffle_{i}')
    add(scene, box((w * .92, .035, .025), (0, -d * .40, 2.16), 'MAT_BRASS_POLISHED'), 'CloudEdgeTrim')
    return scene


def _architecture(family, v, mat):
    return (_fire_door, _louver_door, _transom_door, _ceiling_hatch, _hvac_grille,
            _bumper_rail, _threshold, _elevator_reveal, _corner_guard, _acoustic_cloud)[family](v, mat)


def _stanchion(v, mat):
    scene = trimesh.Scene(); radius, height = .22 + .015 * v, .98 + .06 * v
    add(scene, cyl(radius * 1.35, .10, (0, 0, .05), 'MAT_STONE_LIGHT', 24), 'StanchionFoot')
    add(scene, cyl(radius, .06, (0, 0, .12), 'MAT_BRASS_POLISHED', 24), 'BaseCollar')
    add(scene, cyl(.055, height, (0, 0, height / 2 + .12), mat, 18), 'QueuePost')
    add(scene, cyl(.12 + .012 * v, .08, (0, 0, height + .16), 'MAT_BRASS_POLISHED', 20), 'PostFinial')
    for i in range(2 + v // 2):
        angle = i * math.pi * 2 / (2 + v // 2)
        add(scene, sphere(.035, (.17 * math.cos(angle), .17 * math.sin(angle), .10), 'MAT_BLACKENED_STEEL', 1), f'FootPad_{i}')
    add(scene, box((.36, .08, .08), (.30, 0, height + .12), 'MAT_UPHOLSTERY'), 'RopeSocket')
    return scene


def _valet_cart(v, mat):
    scene = trimesh.Scene(); w, d, h = .72 + .06 * v, .82 + .06 * v, 1.06 + .08 * v
    add(scene, box((w, d, .12), (0, 0, .28), mat), 'ValetPlatform')
    _wheels(scene, w, d, .10, 'ValetCaster')
    for side, x in enumerate((-w * .43, w * .43)):
        add(scene, box((.055, .07, h), (x, d * .35, h / 2 + .20), 'MAT_BRASS_POLISHED'), f'CartUpright_{side}')
        add(scene, box((.10, d * .84, .045), (x, 0, .40), 'MAT_WOOD_WARM'), f'LuggageRail_{side}')
    add(scene, box((w * 1.10, .08, .07), (0, d * .39, h + .18), 'MAT_WOOD_WARM'), 'CartHandle')
    add(scene, box((w * .90, .04, .20), (0, -d * .44, .44), 'MAT_SIGNAGE'), 'ValetLogoPlate')
    return scene


def _writing_lectern(v, mat):
    scene = trimesh.Scene(); w, h = .74 + .06 * v, 1.18 + .08 * v
    add(scene, box((w * .72, .50, .12), (0, 0, .06), 'MAT_STONE_LIGHT'), 'LecternFoot')
    add(scene, box((.20, .24, h * .70), (0, .02, h * .42), mat), 'LecternPedestal')
    top = box((w, .56, .10), (0, -.02, h * .84), 'MAT_WOOD_WARM')
    _rot(top, -.12, [1, 0, 0], (0, -.02, h * .84)); add(scene, top, 'WritingSurface')
    add(scene, box((w * .88, .06, .06), (0, -.25, h * .79), 'MAT_BRASS_POLISHED'), 'PageLedge')
    add(scene, cyl(.028, .48, (w * .32, .05, h + .12), 'MAT_STAINLESS', 12), 'LecternLampStem')
    add(scene, sphere(.11, (w * .32, -.02, h + .38), 'MAT_EMISSIVE_WARM', 1), 'LecternLamp')
    add(scene, box((.22, .16, .05), (-w * .28, -.12, .15), 'MAT_SIGNAGE'), 'EventCard')
    return scene


def _hydration_station(v, mat):
    scene = trimesh.Scene(); w, d, h = .82 + .06 * v, .58 + .05 * v, 1.18 + .08 * v
    add(scene, box((w, d, h * .56), (0, 0, h * .28), mat), 'HydrationCabinet')
    add(scene, box((w * 1.06, d * 1.08, .08), (0, 0, h * .58), 'MAT_STONE_LIGHT'), 'StationCounter')
    add(scene, cyl(.18 + .012 * v, .52 + .03 * v, (0, .08, h * .58 + .32), 'MAT_GLASS_CLEAR', 20), 'WaterReservoir')
    add(scene, cyl(.20 + .012 * v, .06, (0, .08, h * .58 + .61), 'MAT_STAINLESS', 20), 'ReservoirLid')
    add(scene, box((.18, .05, .08), (0, -d * .52, h * .40), 'MAT_STAINLESS'), 'DispenserSpigot')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .22
        add(scene, cyl(.06, .07, (x, -d * .35, h * .64), 'MAT_CERAMIC_FIXTURE', 16), f'WaterCup_{i}')
    add(scene, box((w * .62, .04, .22), (0, -d * .53, .25), 'MAT_SIGNAGE'), 'WaterServiceLabel')
    return scene


def _coat_check(v, mat):
    scene = trimesh.Scene(); w, h = 1.05 + .10 * v, 1.65 + .10 * v
    add(scene, box((w * 1.10, .32, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'CoatRackBase')
    for side, x in enumerate((-w * .44, w * .44)):
        add(scene, box((.08, .08, h), (x, 0, h / 2 + .10), mat), f'CheckPost_{side}')
    add(scene, box((w, .10, .10), (0, 0, h + .10), 'MAT_WOOD_WARM'), 'TicketRail')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w / (3 + v)
        add(scene, box((.07, .12, .18), (x, -.02, h + .23), 'MAT_BRASS_POLISHED'), f'CoatHook_{i}')
        add(scene, box((.12, .05, .10), (x, -.09, h * .62), 'MAT_SIGNAGE'), f'NumberTag_{i}')
    add(scene, box((w * .82, .04, .20), (0, -.18, .22), 'MAT_WOOD_WARM'), 'TicketShelf')
    return scene


def _charging_tower(v, mat):
    scene = trimesh.Scene(); w, h = .44 + .04 * v, 1.52 + .10 * v
    add(scene, box((w * 1.50, .52, .09), (0, 0, .045), 'MAT_STONE_LIGHT'), 'TowerFoot')
    add(scene, box((w, .30, h * .66), (0, 0, h * .33 + .10), mat), 'ChargingColumn')
    add(scene, box((w * 1.18, .34, .32), (0, 0, h * .76), 'MAT_ELECTRONICS'), 'TouchDisplay')
    add(scene, box((w * .72, .025, .42), (0, -.18, h * .75), 'MAT_GLASS_CLEAR'), 'DisplayGlass')
    for i in range(2 + v):
        z = .48 + i * .12
        add(scene, box((.11, .035, .045), ((i - (1 + v) / 2) * .15, -.17, z), 'MAT_STAINLESS'), f'ChargingPort_{i}')
    add(scene, box((.10, .06, .32), (w * .43, .10, h * .42), 'MAT_BLACKENED_STEEL'), 'CableSpool')
    return scene


def _foyer_bench(v, mat):
    scene = trimesh.Scene(); w, d = 1.35 + .12 * v, .48 + .04 * v
    add(scene, box((w, d, .16), (0, 0, .52), mat), 'BenchSeat')
    add(scene, box((w * .96, .11, .52 + .04 * v), (0, d * .43, .85), mat), 'BenchBack')
    _legs(scene, w, d, .42, 'MAT_WOOD_WARM', 'BenchLeg')
    add(scene, box((w * .82, d * .78, .07), (0, 0, .23), 'MAT_BLACKENED_STEEL'), 'ShoeShelf')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .30
        add(scene, box((.24, .04, .12), (x, -.10, .67), 'MAT_LINEN'), f'BenchCushionSeam_{i}')
    add(scene, box((w * .72, .03, .06), (0, -d * .51, .42), 'MAT_BRASS_POLISHED'), 'BenchNameplate')
    return scene


def _parcel_locker(v, mat):
    scene = trimesh.Scene(); w, h = 1.20 + .10 * v, 1.55 + .12 * v
    add(scene, box((w, .48, h), (0, 0, h / 2), mat), 'ParcelLockerBody')
    add(scene, box((w * 1.04, .52, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'LockerPlinth')
    rows, cols = 2 + v // 2, 2 + v // 2
    for row in range(rows):
        for col in range(cols):
            x = (col - (cols - 1) / 2) * w * .72 / cols
            z = .30 + row * (h - .48) / rows
            add(scene, box((w * .66 / cols, .035, (h - .48) / rows * .74), (x, -.255, z), 'MAT_WOOD_WARM'), f'LockerDoor_{row}_{col}')
            add(scene, cyl(.018, .025, (x + w * .20 / cols, -.28, z), 'MAT_BRASS_POLISHED', 10), f'LockerLatch_{row}_{col}')
    add(scene, box((.18, .04, .44), (w * .42, -.27, 1.02), 'MAT_ELECTRONICS'), 'ParcelTouchPanel')
    return scene


def _bell_stand(v, mat):
    scene = trimesh.Scene(); w, d, h = .68 + .06 * v, .52 + .04 * v, .92 + .07 * v
    add(scene, box((w, d, .10), (0, 0, h), mat), 'BellCounter')
    add(scene, box((w * .72, d * .72, h - .12), (0, 0, h / 2), 'MAT_WOOD_WARM'), 'StandPedestal')
    _legs(scene, w, d, .28, 'MAT_BRASS_POLISHED', 'StandLeg')
    add(scene, cyl(.11, .035, (0, -.08, h + .08), 'MAT_BRASS_POLISHED', 20), 'ServiceBellBase')
    add(scene, sphere(.07, (0, -.08, h + .13), 'MAT_BRASS_POLISHED', 2), 'ServiceBellDome')
    add(scene, box((.32, .06, .18), (0, d * .42, h + .18), 'MAT_SIGNAGE'), 'BellStandSign')
    add(scene, box((.40, .28, .04), (0, 0, .10), 'MAT_STONE_LIGHT'), 'FootRest')
    return scene


def _magazine_display(v, mat):
    scene = trimesh.Scene(); w, h = .86 + .08 * v, 1.28 + .10 * v
    add(scene, box((w * 1.15, .42, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'DisplayFoot')
    add(scene, box((w, .08, h), (0, .16, h / 2 + .08), mat), 'DisplayBackboard')
    for i in range(3 + v // 2):
        z = .36 + i * .24
        add(scene, box((w * .84, .20, .16), (0, -.02, z), 'MAT_WOOD_WARM'), f'MagazinePocket_{i}')
        add(scene, box((w * .62, .025, .12), (0, -.13, z + .02), 'MAT_SIGNAGE'), f'MagazineCover_{i}')
    add(scene, box((w * .72, .04, .14), (0, -.05, h + .12), 'MAT_BRASS_POLISHED'), 'DisplayHeader')
    return scene


def _public(family, v, mat):
    return (_stanchion, _valet_cart, _writing_lectern, _hydration_station, _coat_check,
            _charging_tower, _foyer_bench, _parcel_locker, _bell_stand, _magazine_display)[family](v, mat)


def _chafing_dish(v, mat):
    scene = trimesh.Scene(); w, d = .72 + .06 * v, .50 + .04 * v
    _legs(scene, w, d, .34, 'MAT_STAINLESS', 'ChafeLeg')
    add(scene, box((w, d, .10), (0, 0, .38), 'MAT_STAINLESS'), 'ChafingBase')
    add(scene, box((w * .86, d * .82, .14), (0, 0, .50), mat), 'FoodPan')
    add(scene, box((w * .90, d * .86, .08), (0, 0, .65), 'MAT_STAINLESS'), 'LidRim')
    lid = box((w * .90, d * .86, .055), (0, d * .25, .82), 'MAT_STAINLESS')
    _rot(lid, -.22, [1, 0, 0], (0, d * .25, .82)); add(scene, lid, 'RaisedServingLid')
    add(scene, cyl(.10, .08, (0, -.04, .87), 'MAT_WOOD_WARM', 16), 'LidKnob')
    for i, x in enumerate((-.22, .22)):
        add(scene, cyl(.07, .12, (x, 0, .22), 'MAT_BLACKENED_STEEL', 16), f'FuelCanister_{i}')
    return scene


def _salad_guard(v, mat):
    scene = trimesh.Scene(); w, d = 1.30 + .12 * v, .68 + .06 * v
    add(scene, box((w, d, .13), (0, 0, .83), mat), 'SaladBarCounter')
    add(scene, box((w * .96, d * .92, .70), (0, 0, .40), 'MAT_WOOD_WARM'), 'ColdDisplayBase')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * w / (2 + v)
        add(scene, cyl(.13 + .01 * v, .08, (x, -.02, .93), 'MAT_STAINLESS', 20), f'ChilledWell_{i}')
    for side, x in enumerate((-.42, .42)):
        add(scene, box((.055, .06, .56 + .04 * v), (x * w, 0, 1.27), 'MAT_STAINLESS'), f'GlassSupport_{side}')
    add(scene, box((w * .92, .035, .62 + .04 * v), (0, .04, 1.55), 'MAT_GLASS_CLEAR'), 'SneezeGuard')
    add(scene, box((w * .90, .06, .05), (0, .02, 1.88), 'MAT_BRASS_POLISHED'), 'GuardCap')
    return scene


def _carving_station(v, mat):
    scene = trimesh.Scene(); w, d = .90 + .07 * v, .62 + .05 * v
    add(scene, box((w, d, .10), (0, 0, .78), mat), 'CarvingWorktop')
    add(scene, box((w * .78, d * .72, .68), (0, 0, .36), 'MAT_WOOD_WARM'), 'CarvingCabinet')
    _legs(scene, w, d, .14, 'MAT_BLACKENED_STEEL', 'StationFoot')
    add(scene, cyl(.24, .055, (-.18, 0, .89), 'MAT_STAINLESS', 24), 'CarvingPlatter')
    add(scene, box((.48, .04, .10), (.25, -.12, .91), 'MAT_STAINLESS'), 'CarvingKnife')
    add(scene, box((.06, .06, .56), (-.32, .12, 1.18), 'MAT_STAINLESS'), 'HeatLampStem')
    add(scene, sphere(.16, (-.32, .10, 1.48), 'MAT_EMISSIVE_WARM', 1), 'HeatLampShade')
    add(scene, box((w * .90, .035, .08), (0, -d * .53, .56), 'MAT_SIGNAGE'), 'CarvingLabel')
    return scene


def _beverage_urn(v, mat):
    scene = trimesh.Scene(); h, r = 1.08 + .08 * v, .25 + .02 * v
    add(scene, cyl(r * 1.20, .12, (0, 0, .06), 'MAT_STONE_LIGHT', 22), 'UrnFoot')
    add(scene, cyl(r, h * .72, (0, 0, .18 + h * .36), mat, 24), 'UrnBody')
    add(scene, cyl(r * 1.04, .10, (0, 0, .18 + h * .73), 'MAT_STAINLESS', 22), 'UrnShoulder')
    add(scene, cyl(r * .92, .07, (0, 0, h + .20), 'MAT_STAINLESS', 22), 'UrnLid')
    add(scene, sphere(.075, (0, 0, h + .27), 'MAT_BRASS_POLISHED', 1), 'LidKnob')
    add(scene, box((.17, .10, .08), (0, -r - .05, .48), 'MAT_STAINLESS'), 'UrnSpigot')
    add(scene, box((.07, .04, .18), (0, -r - .10, .42), 'MAT_BLACKENED_STEEL'), 'SpigotLever')
    add(scene, box((.08, .04, .40), (r * .86, 0, .58), 'MAT_GLASS_CLEAR'), 'WaterLevelSight')
    return scene


def _condiment_caddy(v, mat):
    scene = trimesh.Scene(); w, d = .65 + .05 * v, .38 + .03 * v
    add(scene, box((w, d, .10), (0, 0, .05), mat), 'CaddyTray')
    for side, y in enumerate((-.14, .14)):
        add(scene, box((w * .94, .035, .20), (0, y, .18), 'MAT_WOOD_WARM'), f'CaddyDivider_{side}')
    for i in range(3 + v // 2):
        x = (i - (2 + v // 2) / 2) * w * .74 / (2 + v // 2)
        add(scene, cyl(.06, .24 + .025 * (i % 2), (x, -.08, .28), mat, 16), f'CondimentJar_{i}')
        add(scene, cyl(.065, .035, (x, -.08, .42 + .025 * (i % 2)), 'MAT_BRASS_POLISHED', 16), f'JarLid_{i}')
    add(scene, box((.18, .16, .14), (w * .34, .10, .17), 'MAT_LINEN'), 'NapkinHolder')
    add(scene, box((w * .86, .04, .06), (0, -d * .50, .10), 'MAT_SIGNAGE'), 'CaddyLabel')
    return scene


def _service_trolley(v, mat):
    scene = trimesh.Scene(); w, d, h = .78 + .07 * v, .55 + .05 * v, 1.02 + .07 * v
    for level, z in enumerate((.22, .62, 1.02 + .07 * v)):
        add(scene, box((w, d, .07), (0, 0, z), mat if level == 2 else 'MAT_STAINLESS'), f'TrolleyShelf_{level}')
        add(scene, box((w * .94, .025, .09), (0, -d * .48, z + .07), 'MAT_BRASS_POLISHED'), f'ShelfLip_{level}')
    for side, x in enumerate((-w * .45, w * .45)):
        add(scene, box((.05, .05, h), (x, d * .38, h / 2 + .12), 'MAT_STAINLESS'), f'TrolleyUpright_{side}')
    _wheels(scene, w, d, .10, 'TrolleyCaster')
    add(scene, box((w * .88, .07, .06), (0, d * .44, h + .12), 'MAT_WOOD_WARM'), 'PushBar')
    return scene


def _wine_stand(v, mat):
    scene = trimesh.Scene(); h, r = .80 + .06 * v, .25 + .025 * v
    add(scene, cyl(r * 1.22, .07, (0, 0, .035), 'MAT_STONE_LIGHT', 20), 'WineStandFoot')
    add(scene, cyl(.045, h * .72, (0, 0, h * .36 + .09), mat, 16), 'WineStandPost')
    add(scene, cyl(r, .08, (0, 0, h * .76 + .10), 'MAT_BRASS_POLISHED', 22), 'BucketSupportRing')
    add(scene, cyl(r * .76, .42 + .03 * v, (0, 0, h + .05), 'MAT_STAINLESS', 20), 'WineBucket')
    add(scene, cyl(r * .82, .045, (0, 0, h + .27 + .03 * v), 'MAT_BRASS_POLISHED', 20), 'BucketRim')
    for i, angle in enumerate((0, 2 * math.pi / 3, 4 * math.pi / 3)):
        x, y = .28 * math.cos(angle), .28 * math.sin(angle)
        add(scene, box((.045, .045, .36 + .04 * v), (x, y, .20 + .02 * v), 'MAT_BLACKENED_STEEL'), f'TripodLeg_{i}')
    return scene


def _undercounter_cooler(v, mat):
    scene = trimesh.Scene(); w, h = .98 + .08 * v, .80 + .05 * v
    add(scene, box((w, .58, h), (0, 0, h / 2 + .08), mat), 'CoolerBody')
    add(scene, box((w * .96, .035, h * .78), (0, -.31, h * .53), 'MAT_STAINLESS'), 'CoolerDoor')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .82 / (1 + v // 2)
        add(scene, box((.025, .06, h * .66), (x, -.34, h * .52), 'MAT_BLACKENED_STEEL'), f'DoorSplit_{i}')
        add(scene, cyl(.025, .04, (x + .08, -.36, h * .56), 'MAT_BRASS_POLISHED', 10), f'CoolerHandle_{i}')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w / (3 + v)
        add(scene, box((.035, .025, .035), (x, -.34, .16), 'MAT_BLACKENED_STEEL'), f'CoolingVent_{i}')
    _legs(scene, w, .58, .12, 'MAT_BLACKENED_STEEL', 'CoolerFoot')
    return scene


def _host_podium(v, mat):
    scene = trimesh.Scene(); w, h = .68 + .06 * v, 1.16 + .08 * v
    add(scene, box((w * 1.16, .54, .10), (0, 0, h), 'MAT_WOOD_WARM'), 'HostCounter')
    add(scene, box((w, .42, h * .78), (0, .02, h * .45), mat), 'HostPodiumBody')
    panel = box((w * .86, .12, .42), (0, -.20, h + .23), 'MAT_SIGNAGE')
    _rot(panel, -.14, [1, 0, 0], (0, -.20, h + .23)); add(scene, panel, 'ReservationBookPanel')
    add(scene, box((w * .92, .045, .06), (0, -.29, h + .08), 'MAT_BRASS_POLISHED'), 'BookStop')
    add(scene, box((.18, .16, .12), (w * .30, .08, h * .56), 'MAT_WOOD_WARM'), 'StorageCubby')
    add(scene, box((w * .86, .04, .18), (0, -.23, .62), 'MAT_STAINLESS'), 'PodiumKickPlate')
    return scene


def _banquette(v, mat):
    scene = trimesh.Scene(); w, d = 1.38 + .12 * v, .66 + .05 * v
    add(scene, box((w, d, .54), (0, 0, .27), 'MAT_WOOD_WARM'), 'BanquetteBase')
    add(scene, box((w * .96, d * .70, .16), (0, -.03, .64), mat), 'BanquetteSeat')
    add(scene, box((w * .96, .14, .78 + .04 * v), (0, d * .40, 1.10), mat), 'BanquetteBack')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * w / (2 + v)
        add(scene, box((w * .90 / (2 + v), d * .55, .10), (x, -.04, .77), 'MAT_LINEN'), f'SeatCushion_{i}')
    add(scene, box((.12, d * .96, 1.18), (-w * .47, 0, .78), 'MAT_WOOD_WARM'), 'BanquetteEndPanel')
    return scene


def _restaurant(family, v, mat):
    return (_chafing_dish, _salad_guard, _carving_station, _beverage_urn, _condiment_caddy,
            _service_trolley, _wine_stand, _undercounter_cooler, _host_podium, _banquette)[family](v, mat)


def _linen_cart(v, mat):
    scene = trimesh.Scene(); w, d, h = .92 + .08 * v, .62 + .06 * v, 1.10 + .08 * v
    add(scene, box((w, d, .09), (0, 0, .26), 'MAT_STAINLESS'), 'LinenCartDeck')
    _wheels(scene, w, d, .10, 'LinenCartCaster')
    for side, x in enumerate((-w * .46, w * .46)):
        add(scene, box((.055, .055, h), (x, 0, h / 2 + .18), mat), f'LinenCartPost_{side}')
    add(scene, box((w * .82, d * .70, .64), (0, 0, .73), 'MAT_LINEN'), 'LinenBag')
    add(scene, box((w * .88, .08, .06), (0, d * .42, h + .18), 'MAT_WOOD_WARM'), 'CartHandle')
    add(scene, box((w * .56, .035, .16), (0, -d * .52, .38), 'MAT_SIGNAGE'), 'LinenRouteLabel')
    return scene


def _supply_trolley(v, mat):
    scene = trimesh.Scene(); w, d, h = .82 + .07 * v, .54 + .05 * v, 1.28 + .08 * v
    for level, z in enumerate((.24, .72, 1.20 + .08 * v)):
        add(scene, box((w, d, .07), (0, 0, z), 'MAT_STAINLESS'), f'SupplyShelf_{level}')
    for side, x in enumerate((-w * .46, w * .46)):
        add(scene, box((.05, .05, h), (x, 0, h / 2 + .15), mat), f'TrolleyFrame_{side}')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .25
        add(scene, box((.18, .20, .27), (x, 0, .43), 'MAT_PLASTIC_RUBBER'), f'SupplyBin_{i}')
        add(scene, cyl(.06, .20 + .02 * v, (x, 0, 1.00), 'MAT_CERAMIC_FIXTURE', 14), f'CleanerBottle_{i}')
    _wheels(scene, w, d, .10, 'SupplyTrolleyCaster')
    add(scene, box((.13, .08, .50), (w * .55, .08, .67), 'MAT_STAINLESS'), 'MopClip')
    return scene


def _tri_bag_sorter(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.26 + .09 * v, .60 + .05 * v, 1.10 + .07 * v
    for side, x in enumerate((-w * .46, w * .46)):
        add(scene, box((.055, .055, h), (x, 0, h / 2), mat), f'SorterPost_{side}')
    for i in range(3):
        x = (i - 1) * w * .28
        add(scene, box((w * .27, d * .74, .08), (x, 0, .16), 'MAT_STAINLESS'), f'BagSupport_{i}')
        add(scene, box((w * .24, d * .64, .60 + .04 * v), (x, 0, .52), 'MAT_LINEN'), f'LaundryBag_{i}')
        add(scene, box((.20, .035, .12), (x, -d * .36, .88), 'MAT_SIGNAGE'), f'SortLabel_{i}')
    add(scene, box((w, .06, .06), (0, d * .35, h), 'MAT_WOOD_WARM'), 'SorterTopRail')
    _wheels(scene, w, d, .09, 'SorterCaster')
    return scene


def _vacuum(v, mat):
    scene = trimesh.Scene(); body_w, h = .46 + .04 * v, .64 + .05 * v
    add(scene, box((body_w, .38, h * .65), (0, .12, h * .40), mat), 'VacuumCanister')
    for side, x in enumerate((-.22, .22)):
        wheel = cyl(.14 + .01 * v, .05, (x, .16, .18), 'MAT_BLACKENED_STEEL', 18)
        _rot(wheel, math.pi / 2, [1, 0, 0], (x, .16, .18)); add(scene, wheel, f'VacuumWheel_{side}')
    add(scene, box((.52, .48, .10), (0, -.18, .08), 'MAT_BLACKENED_STEEL'), 'VacuumFloorHead')
    add(scene, cyl(.045, .54, (0, -.18, .38), 'MAT_STAINLESS', 14), 'VacuumWand')
    hose = cyl(.035, .62, (.18, -.02, .69), 'MAT_PLASTIC_RUBBER', 14)
    _rot(hose, math.pi / 2, [0, 1, 0], (.18, -.02, .69)); add(scene, hose, 'FlexibleHose')
    add(scene, box((.10, .12, .44), (.18, .16, .92), 'MAT_STAINLESS'), 'VacuumHandle')
    add(scene, box((.20, .03, .08), (0, -.09, h * .48), 'MAT_SIGNAGE'), 'VacuumModelPlate')
    return scene


def _ironing_station(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.16 + .08 * v, .36 + .04 * v, .86 + .06 * v
    board = box((w, d, .10), (0, 0, h), mat)
    _rot(board, -.04, [0, 1, 0], (0, 0, h)); add(scene, board, 'IroningBoardTop')
    for side, x in enumerate((-.28, .28)):
        add(scene, box((.045, .045, h - .10), (x, 0, h / 2), 'MAT_STAINLESS'), f'FoldingLeg_{side}')
    add(scene, box((.12, .12, .24), (-w * .32, 0, h + .14), 'MAT_STAINLESS'), 'SteamIronBody')
    add(scene, box((.07, .19, .10), (-w * .32, .02, h + .31), 'MAT_BLACKENED_STEEL'), 'IronHandle')
    add(scene, cyl(.018, .34, (-w * .32, -.08, h + .01), 'MAT_PLASTIC_RUBBER', 10), 'IronCord')
    add(scene, box((w * .78, .04, .04), (0, -d * .52, h - .05), 'MAT_BRASS_POLISHED'), 'BoardEdgeTrim')
    return scene


def _tray_stand(v, mat):
    scene = trimesh.Scene(); w, d, h = .70 + .06 * v, .55 + .05 * v, .72 + .05 * v
    add(scene, box((w, d, .07), (0, 0, h), mat), 'ServiceTrayTop')
    for side, x in enumerate((-.25, .25)):
        leg = box((.045, .045, h - .08), (x, 0, h / 2), 'MAT_WOOD_WARM')
        _rot(leg, .25 if side == 0 else -.25, [0, 1, 0], (x, 0, h / 2)); add(scene, leg, f'FoldingLeg_{side}')
    add(scene, box((.05, d * .80, .045), (0, 0, .25), 'MAT_BLACKENED_STEEL'), 'CrossBrace')
    add(scene, box((w * .92, .05, .07), (0, -d * .45, h + .045), 'MAT_STONE_LIGHT'), 'TrayLip')
    for i in range(2 + v):
        x = (i - (1 + v) / 2) * w / (2 + v)
        add(scene, cyl(.025, .018, (x, .12, h + .05), 'MAT_BRASS_POLISHED', 10), f'TrayFastener_{i}')
    return scene


def _waste_station(v, mat):
    scene = trimesh.Scene(); w = .44 + .04 * v
    for i, color in enumerate(('MAT_BRASS_POLISHED', 'MAT_VEGETATION', 'MAT_STAINLESS')):
        x = (i - 1) * w * .88
        add(scene, box((w * .82, .46, 1.02 + .06 * v), (x, 0, .51 + .03 * v), mat), f'SortBin_{i}')
        add(scene, box((w * .88, .50, .08), (x, 0, 1.05 + .06 * v), 'MAT_BLACKENED_STEEL'), f'BinLid_{i}')
        add(scene, box((w * .58, .035, .18), (x, -.24, .68), color), f'WasteStreamBadge_{i}')
        add(scene, box((.16, .035, .08), (x, -.25, .34), 'MAT_SIGNAGE'), f'WasteLabel_{i}')
    add(scene, box((w * 2.90, .54, .06), (0, 0, .03), 'MAT_STONE_LIGHT'), 'SortingBase')
    return scene


def _bucket_wringer(v, mat):
    scene = trimesh.Scene(); w, d, h = .58 + .05 * v, .48 + .04 * v, .68 + .05 * v
    add(scene, box((w, d, .10), (0, 0, .05), mat), 'MopBucketBody')
    add(scene, box((w * .68, d * .76, .26), (0, 0, h * .67), 'MAT_STAINLESS'), 'WringerHousing')
    add(scene, box((w * .70, .06, .035), (0, -d * .42, h * .83), 'MAT_BLACKENED_STEEL'), 'WringerSlot')
    add(scene, cyl(.025, .50 + .03 * v, (w * .48, 0, h + .26), 'MAT_STAINLESS', 12), 'MopHandle')
    add(scene, box((.24, .035, .05), (w * .48, 0, h + .50), 'MAT_BLACKENED_STEEL'), 'MopGrip')
    _wheels(scene, w, d, .09, 'BucketCaster')
    add(scene, box((w * .82, .04, .14), (0, -d * .52, .36), 'MAT_SIGNAGE'), 'ChemicalWarningLabel')
    return scene


def _supply_shelf(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.18 + .08 * v, .48 + .04 * v, 1.70 + .10 * v
    for side, x in enumerate((-w * .46, w * .46)):
        add(scene, box((.055, .06, h), (x, 0, h / 2), mat), f'SupplyUpright_{side}')
    for i in range(4 + v // 2):
        z = .18 + i * (h - .26) / (3 + v // 2)
        add(scene, box((w, d, .06), (0, 0, z), 'MAT_STAINLESS'), f'SupplyShelf_{i}')
        add(scene, box((w * .86, .12, .22), (0, -.05, z + .17), 'MAT_PLASTIC_RUBBER'), f'SupplyCrate_{i}')
        add(scene, box((.24, .025, .08), (w * .34, -d * .52, z + .10), 'MAT_SIGNAGE'), f'ShelfLabel_{i}')
    add(scene, box((w * 1.08, d * 1.04, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'ShelfPlinth')
    return scene


def _tripod_worklight(v, mat):
    scene = trimesh.Scene(); h = 1.46 + .12 * v
    add(scene, cyl(.045, h * .62, (0, 0, h * .31), 'MAT_STAINLESS', 14), 'LightStandMast')
    for i, angle in enumerate((0, 2 * math.pi / 3, 4 * math.pi / 3)):
        x, y = .32 * math.cos(angle), .32 * math.sin(angle)
        leg = box((.05, .05, .72 + .05 * v), (x, y, .36 + .025 * v), mat)
        _rot(leg, -.18, [0, 1, 0], (x, y, .36 + .025 * v)); add(scene, leg, f'TripodLeg_{i}')
    add(scene, box((.50 + .04 * v, .20, .40 + .03 * v), (0, 0, h * .78), 'MAT_ELECTRONICS'), 'WorklightHousing')
    add(scene, box((.40 + .03 * v, .03, .30 + .02 * v), (0, -.12, h * .78), 'MAT_EMISSIVE_WARM'), 'WorklightLens')
    add(scene, cyl(.06, .20, (0, 0, h * .78), 'MAT_BLACKENED_STEEL', 12), 'TiltYoke')
    add(scene, box((.22, .09, .12), (0, 0, .08), 'MAT_BLACKENED_STEEL'), 'TripodFoot')
    return scene


def _housekeeping(family, v, mat):
    return (_linen_cart, _supply_trolley, _tri_bag_sorter, _vacuum, _ironing_station,
            _tray_stand, _waste_station, _bucket_wringer, _supply_shelf, _tripod_worklight)[family](v, mat)


def _stage_riser(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.60 + .12 * v, 1.00 + .08 * v, .42 + .04 * v
    add(scene, box((w, d, .14), (0, 0, h), mat), 'StageDeck')
    add(scene, box((w * 1.02, .10, h), (0, -d * .45, h / 2), 'MAT_LINEN'), 'StageSkirt')
    for side, x in enumerate((-w * .40, w * .40)):
        add(scene, box((.09, .09, h), (x, 0, h / 2), 'MAT_BLACKENED_STEEL'), f'StageLeg_{side}')
    add(scene, box((.62, .36, .18), (w * .34, -d * .62, .09), 'MAT_STONE_LIGHT'), 'StageStep')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * w / (2 + v)
        add(scene, box((.05, .08, .10), (x, d * .45, h + .12), 'MAT_BRASS_POLISHED'), f'DeckAnchor_{i}')
    return scene


def _portable_lectern(v, mat):
    scene = trimesh.Scene(); w, h = .72 + .06 * v, 1.22 + .09 * v
    add(scene, box((.62, .46, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'PortableLecternBase')
    add(scene, box((.20, .20, h * .68), (0, 0, h * .36), mat), 'LecternColumn')
    top = box((w, .52, .10), (0, -.04, h * .84), 'MAT_WOOD_WARM')
    _rot(top, -.15, [1, 0, 0], (0, -.04, h * .84)); add(scene, top, 'LecternTop')
    add(scene, box((w * .86, .05, .06), (0, -.28, h * .78), 'MAT_BRASS_POLISHED'), 'PaperRetainer')
    add(scene, cyl(.018, .34, (w * .30, .04, h + .15), 'MAT_STAINLESS', 10), 'MicrophoneStem')
    add(scene, sphere(.045, (w * .30, .02, h + .34), 'MAT_BLACKENED_STEEL', 1), 'MicrophoneHead')
    add(scene, box((.23, .025, .14), (0, -.10, .26), 'MAT_SIGNAGE'), 'LecternBadge')
    return scene


def _projection_screen(v, mat):
    scene = trimesh.Scene(); w, h = 1.52 + .12 * v, 1.12 + .09 * v
    add(scene, box((w, .045, h), (0, 0, h / 2 + .20), 'MAT_LINEN'), 'ProjectionSurface')
    add(scene, box((w + .12, .09, .08), (0, 0, h + .24), mat), 'ScreenHeaderRail')
    add(scene, box((w + .12, .08, .06), (0, 0, .22), mat), 'ScreenLowerRail')
    for side, x in enumerate((-.48, .48)):
        add(scene, box((.055, .055, h + .35), (x, 0, (h + .35) / 2), 'MAT_STAINLESS'), f'ScreenTripod_{side}')
        add(scene, box((.32, .38, .06), (x, 0, .03), 'MAT_BLACKENED_STEEL'), f'ScreenFoot_{side}')
    add(scene, box((.12, .10, .20), (w * .51, 0, .40), 'MAT_BRASS_POLISHED'), 'ScreenHeightClamp')
    return scene


def _event_divider(v, mat):
    scene = trimesh.Scene(); w, h = .72 + .06 * v, 1.60 + .12 * v
    for i in range(3 + v // 2):
        x = (i - (2 + v // 2) / 2) * w / (2 + v // 2)
        add(scene, box((w / (3 + v // 2) * .94, .08, h), (x, 0, h / 2 + .08), mat), f'DividerLeaf_{i}')
        add(scene, box((.03, .12, h * .96), (x + .03, .06, h / 2 + .08), 'MAT_BRASS_POLISHED'), f'LeafHinge_{i}')
        add(scene, cyl(.07, .03, (x, 0, .07), 'MAT_BLACKENED_STEEL', 12), f'DividerCaster_{i}')
    add(scene, box((w * 1.10, .12, .10), (0, 0, h + .13), 'MAT_WOOD_WARM'), 'DividerTopCap')
    return scene


def _folding_chair(v, mat):
    scene = trimesh.Scene(); w, h = .50 + .04 * v, .84 + .06 * v
    add(scene, box((w, .42, .10), (0, 0, .48), mat), 'FoldingChairSeat')
    add(scene, box((w * .92, .08, .44 + .03 * v), (0, .17, h), mat), 'FoldingChairBack')
    for side, x in enumerate((-.20, .20)):
        front = box((.04, .04, .48), (x, -.13, .25), 'MAT_STAINLESS')
        back = box((.04, .04, .50), (x, .14, .26), 'MAT_BLACKENED_STEEL')
        _rot(front, -.12, [0, 1, 0], (x, -.13, .25)); _rot(back, .16, [0, 1, 0], (x, .14, .26))
        add(scene, front, f'FoldingLegFront_{side}'); add(scene, back, f'FoldingLegRear_{side}')
    add(scene, box((.42, .06, .035), (0, .01, .22), 'MAT_STAINLESS'), 'SeatBrace')
    add(scene, cyl(.028, .045, (0, 0, .56), 'MAT_BRASS_POLISHED', 10), 'FoldPivot')
    add(scene, box((.28, .035, .08), (0, -.22, .10), 'MAT_WOOD_WARM'), 'FrontFootBar')
    return scene


def _power_table(v, mat):
    scene = trimesh.Scene(); w, d = 1.60 + .12 * v, .78 + .06 * v
    add(scene, box((w, d, .09), (0, 0, .76), mat), 'ConferenceTop')
    _legs(scene, w, d, .72, 'MAT_BLACKENED_STEEL', 'TableLeg')
    add(scene, box((.34 + .03 * v, .26, .045), (0, 0, .825), 'MAT_STAINLESS'), 'PowerModuleFrame')
    add(scene, box((.27 + .02 * v, .19, .025), (0, 0, .86), 'MAT_ELECTRONICS'), 'PowerModuleSurface')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .07
        add(scene, cyl(.018, .02, (x, -.02, .88), 'MAT_BRASS_POLISHED', 10), f'USBPort_{i}')
    add(scene, cyl(.035, .09, (w * .43, d * .34, .84), 'MAT_BLACKENED_STEEL', 12), 'CableGrommet')
    return scene


def _speaker_tower(v, mat):
    scene = trimesh.Scene(); w, h = .54 + .05 * v, 1.48 + .10 * v
    add(scene, box((w * 1.22, .58, .10), (0, 0, .05), 'MAT_BLACKENED_STEEL'), 'SpeakerBase')
    add(scene, box((w, .42, h), (0, 0, h / 2 + .10), mat), 'SpeakerCabinet')
    for i in range(4 + v):
        z = .32 + i * (h - .48) / (3 + v)
        radius = .11 + .01 * (i % 2)
        add(scene, cyl(radius, .035, (0, -.225, z), 'MAT_STAINLESS', 20), f'SpeakerDriver_{i}')
        add(scene, cyl(radius * .64, .04, (0, -.25, z), 'MAT_BLACKENED_STEEL', 18), f'DriverCone_{i}')
    add(scene, box((.10, .05, .52), (w * .58, 0, h * .55), 'MAT_STAINLESS'), 'SpeakerHandle')
    add(scene, box((w * .78, .035, .12), (0, -.24, h * .90), 'MAT_SIGNAGE'), 'SpeakerBadge')
    return scene


def _fitness_step(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.06 + .10 * v, .42 + .04 * v, .16 + .025 * v
    add(scene, box((w, d, h), (0, 0, h / 2), mat), 'StepPlatform')
    for side, x in enumerate((-w * .38, w * .38)):
        add(scene, box((w * .20, d * .92, .07 + .01 * v), (x, 0, .035 + .005 * v), 'MAT_BLACKENED_STEEL'), f'StepRiser_{side}')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w / (4 + v)
        add(scene, box((.025, d * .84, .018), (x, 0, h + .009), 'MAT_STONE_LIGHT'), f'GripRib_{i}')
    add(scene, box((w * .80, .025, .035), (0, -d * .52, h * .68), 'MAT_SIGNAGE'), 'StepModelLabel')
    add(scene, box((.08, .08, .05), (w * .40, d * .42, h + .025), 'MAT_BRASS_POLISHED'), 'LevelMarker')
    return scene


def _bolster_rack(v, mat):
    scene = trimesh.Scene(); w, h = .96 + .08 * v, 1.28 + .10 * v
    for side, x in enumerate((-w * .42, w * .42)):
        add(scene, box((.055, .08, h), (x, 0, h / 2), mat), f'BolsterRackUpright_{side}')
    for i in range(3 + v // 2):
        z = .22 + i * (h - .30) / (2 + v // 2)
        add(scene, box((w * .90, .28, .06), (0, 0, z), 'MAT_WOOD_WARM'), f'BolsterShelf_{i}')
        for j in range(2 + v // 2):
            x = (j - (1 + v // 2) / 2) * w * .58 / (1 + v // 2)
            bolster = cyl(.095 + .005 * v, .40 + .03 * v, (x, 0, z + .16), 'MAT_LINEN', 16)
            _rot(bolster, math.pi / 2, [0, 1, 0], (x, 0, z + .16)); add(scene, bolster, f'YogaBolster_{i}_{j}')
    add(scene, box((w * 1.04, .12, .08), (0, 0, h + .04), 'MAT_BRASS_POLISHED'), 'RackCrown')
    return scene


def _event_hydration(v, mat):
    scene = trimesh.Scene(); w, h = .70 + .06 * v, 1.26 + .10 * v
    add(scene, box((w * 1.35, .58, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'HydrationBase')
    add(scene, box((w, .38, h * .66), (0, 0, h * .33 + .10), mat), 'HydrationColumn')
    add(scene, box((w * 1.16, .44, .10), (0, 0, h * .70), 'MAT_STAINLESS'), 'BottleShelf')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .24
        add(scene, cyl(.065, .34 + .025 * v, (x, 0, h * .94), 'MAT_GLASS_CLEAR', 18), f'WaterBottle_{i}')
        add(scene, cyl(.07, .045, (x, 0, h * .94 + .19 + .0125 * v), 'MAT_BRASS_POLISHED', 16), f'BottleCap_{i}')
    add(scene, box((w * .74, .035, .18), (0, -.21, .42), 'MAT_SIGNAGE'), 'HydrationWayfinding')
    add(scene, box((.44, .32, .055), (0, -.05, .13), 'MAT_BLACKENED_STEEL'), 'DripTray')
    return scene


def _amenity(family, v, mat):
    return (_stage_riser, _portable_lectern, _projection_screen, _event_divider, _folding_chair,
            _power_table, _speaker_tower, _fitness_step, _bolster_rack, _event_hydration)[family](v, mat)


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 1751 <= number <= 2000:
        raise ValueError(f'operations expansion asset ID outside A1751–A2000: {asset_id}')
    batch_index = (number - 1751) // 50
    family, variant = divmod((number - 1751) % 50, 5)
    builders = (_architecture, _public, _restaurant, _housekeeping, _amenity)
    return builders[batch_index](family, variant, mat)
