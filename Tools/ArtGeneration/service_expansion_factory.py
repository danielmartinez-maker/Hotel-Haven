"""Purpose-built kitchen, laundry, receiving and plant assets for A2001–A2250."""

import math

import trimesh

from v2_asset_common import add, box, cyl, sphere


def _rot(mesh, angle, axis, point=None):
    mesh.apply_transform(trimesh.transformations.rotation_matrix(angle, axis, point=point))
    return mesh


def _legs(scene, width, depth, height, material='MAT_BLACKENED_STEEL', prefix='Leg'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.055, .055, height), (x * width * .42, y * depth * .38, height / 2), material),
            f'{prefix}_{index}')


def _wheels(scene, width, depth, radius=.09, prefix='Caster'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        center = (x * width * .40, y * depth * .40, radius)
        wheel = cyl(radius, .055, center, 'MAT_BLACKENED_STEEL', 16)
        _rot(wheel, math.pi / 2, [1, 0, 0], center)
        add(scene, wheel, f'{prefix}_{index}')


def _handle(scene, x, y, z, width, name='PushHandle', material='MAT_STAINLESS'):
    add(scene, cyl(.028, width, (x, y, z), material, 12), name)


# Batch 41: compact and medium-scale commercial kitchen machinery.
def _planetary_mixer(v, mat):
    scene = trimesh.Scene(); w, h = .64 + .05 * v, 1.02 + .06 * v
    add(scene, box((w, .55, .12), (0, 0, .06), 'MAT_STAINLESS'), 'MixerFoot')
    add(scene, box((w * .76, .40, h * .55), (0, .04, h * .29), mat), 'MixerPedestal')
    add(scene, box((w * .90, .48, .24), (0, .04, h * .66), 'MAT_STAINLESS'), 'MixerHead')
    add(scene, cyl(.22 + .015 * v, .32, (0, -.08, .34), 'MAT_STAINLESS', 24), 'MixingBowl')
    add(scene, cyl(.08, .30, (0, -.08, .57), 'MAT_BLACKENED_STEEL', 14), 'BeaterShaft')
    add(scene, box((.24, .045, .06), (w * .35, -.19, h * .59), 'MAT_BRASS_POLISHED'), 'SpeedDial')
    add(scene, box((.46, .18, .035), (0, -.18, .22), 'MAT_SIGNAGE'), 'MixerModelPlate')
    return scene


def _dough_sheeter(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.10 + .10 * v, .58 + .05 * v, .82 + .05 * v
    _legs(scene, w, d, .68, 'MAT_STAINLESS', 'SheeterLeg')
    add(scene, box((w, d, .10), (0, 0, .70), mat), 'SheeterDeck')
    for side, x in enumerate((-.22, .22)):
        roller = cyl(.075 + .006 * v, w * .78, (x, 0, .89), 'MAT_STAINLESS', 20)
        _rot(roller, math.pi / 2, [0, 1, 0], (x, 0, .89)); add(scene, roller, f'DoughRoller_{side}')
        add(scene, box((.06, .07, .18), (x, .02, .78), 'MAT_BLACKENED_STEEL'), f'RollerBearing_{side}')
    for side, x in enumerate((-.47, .47)):
        add(scene, box((.16, .36 + .025 * v, .06), (x, .31, .73), 'MAT_STAINLESS'), f'FeedTray_{side}')
    add(scene, cyl(.075, .06, (w * .43, .28, .77), 'MAT_BRASS_POLISHED', 12), 'ThicknessWheel')
    add(scene, box((.30, .035, .13), (0, -.31, .78), 'MAT_SIGNAGE'), 'SheeterLabel')
    return scene


def _counter_blender(v, mat):
    scene = trimesh.Scene(); w, h = .34 + .025 * v, .52 + .035 * v
    add(scene, box((w * 1.65, .38, .28), (0, 0, .14), mat), 'BlenderMotorBase')
    add(scene, box((w * 1.30, .34, .07), (0, -.02, .315), 'MAT_STAINLESS'), 'BlenderControlRail')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .09
        add(scene, cyl(.025, .035, (x, -.205, .34), 'MAT_BRASS_POLISHED', 10), f'BlendButton_{i}')
    add(scene, cyl(w * .72, h * .70, (0, .02, .28 + h * .35), 'MAT_GLASS_CLEAR', 20), 'BlenderJar')
    add(scene, cyl(w * .79, .055, (0, .02, .28 + h * .72), 'MAT_BLACKENED_STEEL', 18), 'BlenderLid')
    add(scene, cyl(.035, .20, (0, .02, .28 + h * .55), 'MAT_STAINLESS', 10), 'BlenderBladeShaft')
    add(scene, box((.24, .045, .08), (0, -.22, .18), 'MAT_SIGNAGE'), 'BlenderLogo')
    return scene


def _food_processor(v, mat):
    scene = trimesh.Scene(); w, h = .36 + .025 * v, .48 + .035 * v
    add(scene, box((w * 1.60, .38, .25), (0, 0, .125), mat), 'ProcessorBase')
    add(scene, cyl(w * .66, h * .60, (0, .02, .25 + h * .30), 'MAT_GLASS_CLEAR', 20), 'ProcessorBowl')
    add(scene, cyl(w * .72, .055, (0, .02, .25 + h * .61), 'MAT_STAINLESS', 20), 'ProcessorBowlRim')
    add(scene, box((.14, .12, .22), (w * .52, .02, .52 + h * .36), 'MAT_BLACKENED_STEEL'), 'FeedChute')
    add(scene, box((.06, .06, .16), (w * .52, .02, .70 + h * .36), 'MAT_STAINLESS'), 'FoodPusher')
    add(scene, cyl(.06, .05, (w * .40, -.20, .31), 'MAT_BRASS_POLISHED', 14), 'ProcessorDial')
    add(scene, box((.26, .035, .075), (0, -.20, .15), 'MAT_SIGNAGE'), 'ProcessorLabel')
    return scene


def _meat_slicer(v, mat):
    scene = trimesh.Scene(); w, h = .70 + .05 * v, .72 + .05 * v
    add(scene, box((w, .52, .12), (0, 0, .06), 'MAT_STAINLESS'), 'SlicerBase')
    add(scene, box((w * .80, .40, .34 + .02 * v), (0, .04, .29), mat), 'SlicerMotorHousing')
    blade = cyl(.27 + .012 * v, .055, (-w * .18, .10, .64), 'MAT_STAINLESS', 28)
    _rot(blade, math.pi / 2, [1, 0, 0], (-w * .18, .10, .64)); add(scene, blade, 'SlicerBlade')
    add(scene, box((.055, .08, .58), (-w * .18, .13, .64), 'MAT_BLACKENED_STEEL'), 'BladeGuardPost')
    add(scene, box((.36 + .025 * v, .45, .06), (w * .20, .05, .36), 'MAT_STAINLESS'), 'ProductCarriage')
    add(scene, box((.07, .07, .30), (w * .38, .22, .55), 'MAT_BRASS_POLISHED'), 'CarriageGrip')
    add(scene, box((.20, .035, .08), (0, -.27, .13), 'MAT_SIGNAGE'), 'SlicerLabel')
    return scene


def _counter_toaster(v, mat):
    scene = trimesh.Scene(); w, d, h = .54 + .04 * v, .36 + .03 * v, .30 + .02 * v
    add(scene, box((w, d, h), (0, 0, h / 2 + .10), mat), 'ToasterBody')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .60 / (1 + v // 2)
        add(scene, box((.045, d * .55, .022), (x, 0, h + .11), 'MAT_BLACKENED_STEEL'), f'ToastSlot_{i}')
    add(scene, box((.06, .05, .19), (w * .59, -.02, h * .48 + .08), 'MAT_STAINLESS'), 'ToastLever')
    add(scene, cyl(.045, .025, (-w * .40, -.19, h * .54 + .08), 'MAT_BRASS_POLISHED', 12), 'BrowningDial')
    add(scene, box((.30, .025, .07), (0, -.19, .16), 'MAT_SIGNAGE'), 'ToasterBrandPlate')
    return scene


def _proofing_cabinet(v, mat):
    scene = trimesh.Scene(); w, h = .80 + .06 * v, 1.58 + .10 * v
    add(scene, box((w, .62, h), (0, 0, h / 2), mat), 'ProofingCabinet')
    add(scene, box((w * .76, .04, h * .78), (0, -.33, h * .54), 'MAT_GLASS_CLEAR'), 'ProofingDoorWindow')
    for i in range(4 + v // 2):
        z = .26 + i * (h - .42) / (3 + v // 2)
        add(scene, box((w * .72, .43, .035), (0, 0, z), 'MAT_STAINLESS'), f'ProofingShelf_{i}')
    add(scene, box((.12, .06, .42), (w * .56, -.35, h * .66), 'MAT_ELECTRONICS'), 'ProofingController')
    for i in range(3 + v):
        add(scene, box((.025, .02, .045), (w * .56, -.39, h * .82 + i * .07), 'MAT_BRASS_POLISHED'), f'ControllerKey_{i}')
    add(scene, box((w * .60, .04, .09), (0, -.34, .12), 'MAT_SIGNAGE'), 'ProofingLabel')
    return scene


def _blast_chiller(v, mat):
    scene = trimesh.Scene(); w, d, h = .88 + .06 * v, .72 + .05 * v, 1.34 + .10 * v
    add(scene, box((w, d, h), (0, 0, h / 2 + .10), mat), 'BlastChillerBody')
    _wheels(scene, w, d, .10, 'ChillerCaster')
    add(scene, box((w * .72, .04, h * .70), (0, -d * .52, h * .52), 'MAT_STAINLESS'), 'ChillerDoor')
    add(scene, box((.13, .07, .36), (w * .58, -d * .56, h * .64), 'MAT_ELECTRONICS'), 'ChillerControlPanel')
    for i in range(4 + v):
        z = .30 + i * .15
        add(scene, box((w * .52, .025, .025), (0, -d * .55, z), 'MAT_BLACKENED_STEEL'), f'ChillerVent_{i}')
    add(scene, cyl(.035, .045, (w * .42, -d * .56, .42), 'MAT_BRASS_POLISHED', 10), 'ChillerDoorHandle')
    return scene


def _ice_maker(v, mat):
    scene = trimesh.Scene(); w, d, h = .76 + .06 * v, .64 + .05 * v, 1.16 + .08 * v
    add(scene, box((w, d, h * .68), (0, 0, h * .34 + .08), mat), 'IceMakerCabinet')
    add(scene, box((w * 1.02, d * 1.02, .16), (0, 0, h * .68 + .08), 'MAT_STAINLESS'), 'IceBin')
    add(scene, box((w * .84, d * .76, .30 + .02 * v), (0, 0, h * .68 + .31), 'MAT_BLACKENED_STEEL'), 'IceHopper')
    add(scene, box((.30, .06, .22), (0, -d * .53, .70), 'MAT_STAINLESS'), 'IceDispenserChute')
    add(scene, box((.22, .05, .08), (0, -d * .56, .54), 'MAT_ELECTRONICS'), 'IceTouchPad')
    for i in range(4 + v):
        add(scene, box((.035, .04, .03), ((i - (3 + v) / 2) * .10, -d * .54, .25), 'MAT_BLACKENED_STEEL'), f'CondenserVent_{i}')
    add(scene, cyl(.04, .32, (w * .55, d * .20, h * .43), 'MAT_STAINLESS', 12), 'WaterSupplyStub')
    return scene


def _induction_warmer(v, mat):
    scene = trimesh.Scene(); w, d = .70 + .06 * v, .50 + .04 * v
    add(scene, box((w, d, .14), (0, 0, .12), mat), 'InductionWarmerBase')
    add(scene, box((w * .92, d * .88, .045), (0, 0, .214), 'MAT_STONE_LIGHT'), 'CeramicCooktop')
    for i in range(1 + v // 2):
        x = (i - v / 4) * w * .42
        add(scene, cyl(.13 + .006 * v, .014, (x, 0, .244), 'MAT_BLACKENED_STEEL', 24), f'InductionZone_{i}')
    add(scene, box((.24, .045, .06), (w * .30, -d * .48, .22), 'MAT_ELECTRONICS'), 'WarmerDisplay')
    for i in range(2 + v // 2):
        add(scene, cyl(.025, .035, ((i - (1 + v // 2) / 2) * .09 + w * .30, -d * .50, .22), 'MAT_BRASS_POLISHED', 10), f'WarmerButton_{i}')
    _legs(scene, w, d, .10, 'MAT_STAINLESS', 'WarmerFoot')
    return scene


def _kitchen_machinery(family, v, mat):
    return (_planetary_mixer, _dough_sheeter, _counter_blender, _food_processor, _meat_slicer,
            _counter_toaster, _proofing_cabinet, _blast_chiller, _ice_maker, _induction_warmer)[family](v, mat)


# Batch 42: food preparation, dish staging and specialist kitchen storage.
def _ingredient_prep_table(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.35 + .12 * v, .72 + .06 * v, .88 + .04 * v
    _legs(scene, w, d, .76, 'MAT_STAINLESS', 'PrepTableLeg')
    add(scene, box((w, d, .12), (0, 0, .82), mat), 'PrepTableTop')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .54 / (1 + v // 2)
        add(scene, cyl(.12, .10, (x, -.02, .93), 'MAT_STAINLESS', 20), f'IngredientWell_{i}')
        add(scene, cyl(.13, .035, (x, -.02, .998), 'MAT_BRASS_POLISHED', 20), f'WellRim_{i}')
    add(scene, box((w * .86, .05, .08), (0, -d * .51, .80), 'MAT_SIGNAGE'), 'PrepStationLabel')
    add(scene, box((w * .80, .40, .06), (0, .02, .32), 'MAT_STAINLESS'), 'LowerStorageShelf')
    return scene


def _sheet_pan_rack(v, mat):
    scene = trimesh.Scene(); w, d, h = .72 + .05 * v, .66 + .05 * v, 1.70 + .10 * v
    for side, x in enumerate((-w * .43, w * .43)):
        add(scene, box((.055, .06, h), (x, 0, h / 2), mat), f'PanRackUpright_{side}')
    for i in range(6 + v):
        z = .18 + i * (h - .32) / (5 + v)
        add(scene, box((w, .035, .035), (0, 0, z), 'MAT_STAINLESS'), f'PanRail_{i}')
        add(scene, box((w * .82, d * .74, .035), (0, 0, z + .065), 'MAT_STAINLESS'), f'SheetPan_{i}')
    _wheels(scene, w, d, .08, 'PanRackCaster')
    add(scene, box((w * .90, .08, .07), (0, d * .36, h + .04), 'MAT_BRASS_POLISHED'), 'RackTopRail')
    return scene


def _hanging_pot_rack(v, mat):
    scene = trimesh.Scene(); w, d = 1.20 + .10 * v, .72 + .06 * v
    add(scene, box((w, d, .10), (0, 0, 2.45), mat), 'PotRackHeader')
    for side, x in enumerate((-.40, .40)):
        add(scene, cyl(.025, .72 + .04 * v, (x, 0, 2.04), 'MAT_STAINLESS', 10), f'PotRackHanger_{side}')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w / (3 + v)
        add(scene, cyl(.025, .45 + .03 * v, (x, -.10, 1.70), 'MAT_STAINLESS', 10), f'PotHook_{i}')
        pot = cyl(.12 + .008 * v, .17 + .012 * (i % 2), (x, -.10, 1.42), 'MAT_STAINLESS', 18)
        add(scene, pot, f'StockPot_{i}')
        add(scene, box((.18, .035, .035), (x, -.10, 1.33), 'MAT_BLACKENED_STEEL'), f'PotHandle_{i}')
    for side, x in enumerate((-.45, .45)):
        add(scene, box((.06, .08, .28), (x, 0, 2.56), 'MAT_STONE_LIGHT'), f'CeilingMount_{side}')
    return scene


def _plate_dispenser(v, mat):
    scene = trimesh.Scene(); w, h = .72 + .05 * v, .94 + .06 * v
    add(scene, box((w, .48, .12), (0, 0, .06), 'MAT_STAINLESS'), 'PlateDispenserBase')
    add(scene, box((w * .92, .38, h * .78), (0, 0, h * .48), mat), 'PlateDispenserBody')
    for side, x in enumerate((-.23, .23)):
        add(scene, cyl(.13 + .01 * v, .30 + .02 * v, (x, -.20, h * .80), 'MAT_CERAMIC_FIXTURE', 20), f'PlateStack_{side}')
        add(scene, cyl(.14 + .01 * v, .025, (x, -.20, h * .80 + .17 + .01 * v), 'MAT_STAINLESS', 20), f'SpringPlate_{side}')
    add(scene, box((w * .86, .045, .07), (0, -.25, .24), 'MAT_SIGNAGE'), 'PlateTypeLabel')
    add(scene, box((.06, .04, .38), (w * .52, -.26, .54), 'MAT_BRASS_POLISHED'), 'DispenserHandle')
    return scene


def _heated_plate_cabinet(v, mat):
    scene = trimesh.Scene(); w, d, h = .84 + .06 * v, .64 + .04 * v, 1.02 + .08 * v
    add(scene, box((w, d, h), (0, 0, h / 2), mat), 'PlateWarmerCabinet')
    add(scene, box((w * .78, .04, h * .68), (0, -d * .53, h * .52), 'MAT_STAINLESS'), 'WarmerDoor')
    for level in range(3 + v // 2):
        z = .28 + level * .20
        add(scene, box((w * .68, d * .32, .035), (0, -.04, z), 'MAT_STAINLESS'), f'PlateWarmerShelf_{level}')
    add(scene, box((.14, .05, .36), (w * .56, -d * .56, h * .62), 'MAT_ELECTRONICS'), 'ThermostatPanel')
    add(scene, cyl(.035, .035, (w * .56, -d * .60, h * .42), 'MAT_BRASS_POLISHED', 10), 'WarmerDoorPull')
    add(scene, box((w * .60, .04, .075), (0, -d * .54, .15), 'MAT_SIGNAGE'), 'WarmerIdentityPlate')
    return scene


def _glass_rack(v, mat):
    scene = trimesh.Scene(); w, d, h = .72 + .05 * v, .58 + .04 * v, 1.24 + .08 * v
    for side, x in enumerate((-w * .45, w * .45)):
        add(scene, box((.05, .05, h), (x, 0, h / 2), mat), f'GlassRackPost_{side}')
    for level in range(3 + v // 2):
        z = .30 + level * .29
        add(scene, box((w, d, .045), (0, 0, z), 'MAT_STAINLESS'), f'GlassRackShelf_{level}')
        for col in range(4 + v):
            x = (col - (3 + v) / 2) * w * .78 / (3 + v)
            add(scene, cyl(.045, .11, (x, 0, z + .10), 'MAT_GLASS_CLEAR', 12), f'GlassCup_{level}_{col}')
            add(scene, cyl(.065, .018, (x, 0, z + .04), 'MAT_BLACKENED_STEEL', 12), f'CupRing_{level}_{col}')
    _wheels(scene, w, d, .08, 'GlassRackCaster')
    return scene


def _cutlery_wash_station(v, mat):
    scene = trimesh.Scene(); w, d = .86 + .06 * v, .58 + .05 * v
    add(scene, box((w, d, .68), (0, 0, .36), mat), 'CutlerySanitizerBody')
    add(scene, box((w * .80, .04, .38), (0, -d * .53, .43), 'MAT_STAINLESS'), 'SanitizerDoor')
    add(scene, box((.42, .30, .12), (0, -.18, .78), 'MAT_STAINLESS'), 'CutleryBasket')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * .10
        add(scene, box((.035, .12, .26 + .02 * v), (x, -.20, .95), 'MAT_STAINLESS'), f'CutlerySlot_{i}')
    add(scene, box((.13, .05, .28), (w * .55, -d * .57, .60), 'MAT_ELECTRONICS'), 'SanitizerControl')
    add(scene, box((.20, .035, .07), (0, -d * .57, .20), 'MAT_SIGNAGE'), 'SanitizerLabel')
    add(scene, box((w * 1.02, .08, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'SanitizerPlinth')
    return scene


def _ingredient_hopper(v, mat):
    scene = trimesh.Scene(); w, d, h = .56 + .05 * v, .54 + .04 * v, 1.08 + .08 * v
    add(scene, box((w, d, .08), (0, 0, .04), 'MAT_STAINLESS'), 'HopperFoot')
    add(scene, box((w * .78, d * .74, h * .60), (0, 0, h * .38), mat), 'IngredientBin')
    add(scene, box((w * .86, d * .82, .06), (0, 0, h * .70), 'MAT_STAINLESS'), 'HopperRim')
    lid = box((w * .86, d * .82, .055), (0, 0, h * .75), 'MAT_BLACKENED_STEEL')
    _rot(lid, -.08 - .015 * v, [1, 0, 0], (0, 0, h * .75)); add(scene, lid, 'HopperLid')
    add(scene, box((.12, .10, .26), (0, -d * .45, h * .18), 'MAT_STAINLESS'), 'DispenseGate')
    add(scene, cyl(.03, .22, (w * .53, 0, h * .40), 'MAT_BRASS_POLISHED', 10), 'GateLever')
    _wheels(scene, w, d, .08, 'HopperCaster')
    return scene


def _knife_sanitizer(v, mat):
    scene = trimesh.Scene(); w, h = .62 + .05 * v, .82 + .06 * v
    add(scene, box((w, .28, h), (0, 0, h / 2), mat), 'KnifeSanitizerHousing')
    add(scene, box((w * .76, .035, h * .70), (0, -.16, h * .54), 'MAT_GLASS_CLEAR'), 'SanitizerWindow')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .64 / (3 + v)
        add(scene, box((.035, .06, .48 + .03 * v), (x, -.18, .50), 'MAT_STAINLESS'), f'KnifeSlot_{i}')
    add(scene, box((.11, .045, .27), (w * .55, -.17, h * .65), 'MAT_ELECTRONICS'), 'SanitizerSwitch')
    add(scene, box((w * .64, .035, .07), (0, -.17, .14), 'MAT_SIGNAGE'), 'KnifeSanitizerLabel')
    add(scene, box((w * 1.04, .34, .07), (0, 0, .035), 'MAT_STONE_LIGHT'), 'SanitizerFoot')
    return scene


def _spice_rail(v, mat):
    scene = trimesh.Scene(); w = .92 + .08 * v
    add(scene, box((w, .24, .075), (0, 0, .04), 'MAT_WOOD_WARM'), 'SpiceRailBase')
    add(scene, box((w, .045, .52), (0, .11, .32), mat), 'SpiceRailBack')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .86 / (3 + v)
        add(scene, cyl(.065, .23 + .015 * v, (x, -.02, .19), 'MAT_GLASS_CLEAR', 14), f'SpiceJar_{i}')
        add(scene, cyl(.068, .032, (x, -.02, .32 + .0075 * v), 'MAT_BRASS_POLISHED', 14), f'SpiceJarLid_{i}')
        add(scene, box((.10, .02, .055), (x, -.09, .13), 'MAT_SIGNAGE'), f'SpiceLabel_{i}')
    add(scene, box((w * .88, .05, .05), (0, -.13, .08), 'MAT_STAINLESS'), 'SpiceRailLip')
    return scene


def _kitchen_staging(family, v, mat):
    return (_ingredient_prep_table, _sheet_pan_rack, _hanging_pot_rack, _plate_dispenser,
            _heated_plate_cabinet, _glass_rack, _cutlery_wash_station, _ingredient_hopper,
            _knife_sanitizer, _spice_rail)[family](v, mat)


# Batch 43: commercial laundry equipment and linen handling stations.
def _commercial_washer(v, mat):
    scene = trimesh.Scene(); w, h = .92 + .08 * v, 1.18 + .10 * v
    add(scene, box((w, .78, h), (0, 0, h / 2), mat), 'WasherCabinet')
    washer_center = (0, -.405, .64)
    outer_ring = cyl(.28 + .015 * v, .055, washer_center, 'MAT_STAINLESS', 28)
    _rot(outer_ring, math.pi / 2, [1, 0, 0], washer_center); add(scene, outer_ring, 'WasherDoorOuterRing')
    window_center = (0, -.445, .64)
    washer_window = cyl(.22 + .012 * v, .06, window_center, 'MAT_GLASS_CLEAR', 28)
    _rot(washer_window, math.pi / 2, [1, 0, 0], window_center); add(scene, washer_window, 'WasherDoorWindow')
    drum_center = (0, -.48, .64)
    washer_drum = cyl(.17 + .010 * v, .065, drum_center, 'MAT_BLACKENED_STEEL', 24)
    _rot(washer_drum, math.pi / 2, [1, 0, 0], drum_center); add(scene, washer_drum, 'WasherDrum')
    add(scene, box((.18, .045, .38), (w * .36, -.415, h * .73), 'MAT_ELECTRONICS'), 'WasherControlPanel')
    for i in range(3 + v):
        add(scene, cyl(.022, .025, ((i - (2 + v) / 2) * .055 + w * .36, -.45, h * .82), 'MAT_BRASS_POLISHED', 10), f'WasherControl_{i}')
    add(scene, box((w * .52, .05, .085), (0, -.40, .14), 'MAT_SIGNAGE'), 'WasherRatingPlate')
    return scene


def _commercial_dryer(v, mat):
    scene = trimesh.Scene(); w, h = .88 + .08 * v, 1.26 + .10 * v
    add(scene, box((w, .76, h), (0, 0, h / 2), mat), 'DryerCabinet')
    add(scene, box((w * .72, .045, h * .58), (0, -.39, h * .48), 'MAT_STAINLESS'), 'DryerDoor')
    dryer_window_center = (0, -.425, h * .49)
    dryer_window = cyl(.21 + .014 * v, .045, dryer_window_center, 'MAT_GLASS_CLEAR', 26)
    _rot(dryer_window, math.pi / 2, [1, 0, 0], dryer_window_center); add(scene, dryer_window, 'DryerWindow')
    add(scene, box((w * .34, .045, .09), (0, -.42, h * .82), 'MAT_ELECTRONICS'), 'DryerControlStrip')
    for i in range(4 + v):
        z = .22 + i * .055
        add(scene, box((w * .72, .03, .018), (0, -.415, z), 'MAT_BLACKENED_STEEL'), f'DryerExhaustSlot_{i}')
    add(scene, cyl(.035, .045, (w * .35, -.43, h * .52), 'MAT_BRASS_POLISHED', 10), 'DryerDoorPull')
    add(scene, box((w * .60, .04, .06), (0, -.40, .12), 'MAT_SIGNAGE'), 'DryerIdentityPlate')
    return scene


def _roller_ironer(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.30 + .12 * v, .60 + .05 * v, .96 + .07 * v
    _legs(scene, w, d, .74, 'MAT_STAINLESS', 'IronerLeg')
    add(scene, box((w, d, .11), (0, 0, .74), mat), 'IronerDeck')
    for i, z in enumerate((.91, 1.12)):
        roller = cyl(.09 + .008 * v, w * .84, (0, 0, z), 'MAT_STAINLESS', 22)
        _rot(roller, math.pi / 2, [0, 1, 0], (0, 0, z)); add(scene, roller, f'IronerRoller_{i}')
    for side, x in enumerate((-.42, .42)):
        add(scene, box((.07, .12, .48), (x * w, 0, .96), 'MAT_BLACKENED_STEEL'), f'IronerSideFrame_{side}')
    add(scene, box((.13, .06, .34), (w * .51, -.25, .97), 'MAT_ELECTRONICS'), 'IronerControl')
    add(scene, box((w * .65, .04, .065), (0, -.32, .80), 'MAT_SIGNAGE'), 'IronerWarningPlate')
    return scene


def _linen_folding_table(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.48 + .12 * v, .78 + .06 * v, .90 + .05 * v
    _legs(scene, w, d, h - .13, 'MAT_STAINLESS', 'FoldingTableLeg')
    add(scene, box((w, d, .13), (0, 0, h - .065), mat), 'LinenFoldingTop')
    add(scene, box((w * .84, .07, .045), (0, -d * .50, h + .02), 'MAT_BRASS_POLISHED'), 'TableEdgeRail')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .84 / (3 + v)
        add(scene, box((.025, d * .72, .018), (x, 0, h + .002), 'MAT_STONE_LIGHT'), f'FoldGuide_{i}')
    add(scene, box((w * .64, .38, .06), (0, 0, .24), 'MAT_STAINLESS'), 'UnderTableShelf')
    add(scene, box((.26, .035, .08), (w * .35, -d * .52, .19), 'MAT_SIGNAGE'), 'FoldingStationLabel')
    return scene


def _steam_finisher(v, mat):
    scene = trimesh.Scene(); w, h = .78 + .06 * v, 1.68 + .10 * v
    add(scene, box((w, .60, h), (0, 0, h / 2), mat), 'SteamFinisherCabinet')
    add(scene, box((w * .70, .04, h * .72), (0, -.31, h * .52), 'MAT_GLASS_CLEAR'), 'FinisherDoorWindow')
    add(scene, cyl(.025, .48 + .03 * v, (0, .18, h * .76), 'MAT_STAINLESS', 12), 'GarmentSuspensionBar')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * .24
        add(scene, box((.035, .20, .05), (x, .12, h * .72), 'MAT_BRASS_POLISHED'), f'HangerHook_{i}')
    add(scene, box((.14, .05, .34), (w * .55, -.33, h * .67), 'MAT_ELECTRONICS'), 'SteamCyclePanel')
    add(scene, cyl(.075, .18, (-w * .32, .23, .16), 'MAT_STAINLESS', 14), 'CondensateTank')
    add(scene, box((w * .56, .04, .075), (0, -.31, .12), 'MAT_SIGNAGE'), 'FinisherLabel')
    return scene


def _boot_dryer(v, mat):
    scene = trimesh.Scene(); w, h = .78 + .06 * v, 1.18 + .08 * v
    add(scene, box((w, .48, .20), (0, 0, .10), 'MAT_STAINLESS'), 'BootDryerBase')
    add(scene, box((w * .92, .42, .46), (0, 0, .43), mat), 'BootDryerColumn')
    add(scene, box((.18, .12, .25), (0, -.08, .80), 'MAT_ELECTRONICS'), 'DryerAirManifold')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .72 / (3 + v)
        add(scene, cyl(.035, .44 + .03 * v, (x, -.12, .77), 'MAT_STAINLESS', 12), f'BootDryTube_{i}')
        add(scene, cyl(.025, .055, (x, -.12, 1.00), 'MAT_BLACKENED_STEEL', 10), f'TubeNozzle_{i}')
    add(scene, box((.16, .05, .30), (w * .55, -.24, .74), 'MAT_SIGNAGE'), 'BootDryerLabel')
    add(scene, box((w * 1.04, .54, .06), (0, 0, .03), 'MAT_STONE_LIGHT'), 'DryerFoot')
    return scene


def _garment_rack(v, mat):
    scene = trimesh.Scene(); w, h = 1.12 + .10 * v, 1.62 + .10 * v
    add(scene, box((w * 1.05, .48, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'GarmentRackFoot')
    for side, x in enumerate((-.43 * w, .43 * w)):
        add(scene, box((.055, .055, h), (x, 0, h / 2), mat), f'GarmentRackPost_{side}')
    add(scene, cyl(.035, w * .96, (0, 0, h), 'MAT_STAINLESS', 14), 'HangingRail')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .76 / (3 + v)
        add(scene, box((.07, .025, .23), (x, 0, h - .18), 'MAT_WOOD_WARM'), f'CoatHanger_{i}')
        add(scene, cyl(.016, .11, (x, 0, h - .06), 'MAT_BRASS_POLISHED', 8), f'HangerHook_{i}')
    add(scene, box((w * .82, .06, .10), (0, .20, .18), 'MAT_STAINLESS'), 'ShoeShelf')
    return scene


def _linen_scale(v, mat):
    scene = trimesh.Scene(); w, d = .66 + .05 * v, .58 + .04 * v
    add(scene, box((w, d, .12), (0, 0, .06), mat), 'LinenScaleBase')
    add(scene, box((w * .92, d * .88, .065), (0, 0, .152), 'MAT_STAINLESS'), 'WeighingPlatform')
    add(scene, box((.23, .20, .38), (0, d * .33, .37), 'MAT_ELECTRONICS'), 'ScaleDisplayHead')
    add(scene, box((.18, .04, .12), (0, d * .20, .39), 'MAT_GLASS_CLEAR'), 'ScaleDisplayWindow')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * .055
        add(scene, cyl(.018, .025, (x, d * .20, .29), 'MAT_BRASS_POLISHED', 10), f'ScaleKey_{i}')
    add(scene, box((.24, .035, .07), (0, -d * .50, .11), 'MAT_SIGNAGE'), 'ScaleCapacityLabel')
    return scene


def _uniform_issue_locker(v, mat):
    scene = trimesh.Scene(); w, h = 1.20 + .10 * v, 1.78 + .10 * v
    add(scene, box((w, .52, h), (0, 0, h / 2), mat), 'UniformLockerBank')
    cols = 2 + v // 2
    for i in range(cols):
        x = (i - (cols - 1) / 2) * w * .78 / cols
        add(scene, box((w * .70 / cols, .035, h * .74), (x, -.28, h * .50), 'MAT_STAINLESS'), f'IssueLockerDoor_{i}')
        add(scene, box((.10, .035, .16), (x, -.31, h * .72), 'MAT_SIGNAGE'), f'LockerIdentityTag_{i}')
        add(scene, cyl(.022, .025, (x + w * .22 / cols, -.32, h * .50), 'MAT_BRASS_POLISHED', 10), f'LockerLatch_{i}')
    add(scene, box((.22, .07, .38), (w * .56, -.31, h * .55), 'MAT_ELECTRONICS'), 'UniformIssueTerminal')
    add(scene, box((w * 1.04, .56, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'LockerPlinth')
    return scene


def _laundry_chute_intake(v, mat):
    scene = trimesh.Scene(); w, h = .82 + .06 * v, 1.42 + .10 * v
    add(scene, box((w, .18, h), (0, 0, h / 2), mat), 'ChuteIntakeFrame')
    add(scene, box((w * .64, .055, h * .52), (0, -.12, h * .48), 'MAT_BLACKENED_STEEL'), 'ChuteIntakeDoor')
    add(scene, box((w * .46, .045, .28 + .02 * v), (0, -.16, h * .65), 'MAT_STAINLESS'), 'ChuteDropSlot')
    add(scene, box((.14, .045, .35), (w * .55, -.15, h * .67), 'MAT_ELECTRONICS'), 'ChuteSafetySwitch')
    add(scene, box((w * .78, .035, .15), (0, -.13, h * .23), 'MAT_SIGNAGE'), 'ChuteRoomLabel')
    for i in range(3 + v):
        add(scene, cyl(.022, .025, ((i - (2 + v) / 2) * w * .55 / (2 + v), -.16, .24), 'MAT_BRASS_POLISHED', 10), f'IntakeBolt_{i}')
    add(scene, box((w * 1.08, .25, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'ChuteBaseSill')
    return scene


def _laundry_plant(family, v, mat):
    return (_commercial_washer, _commercial_dryer, _roller_ironer, _linen_folding_table,
            _steam_finisher, _boot_dryer, _garment_rack, _linen_scale,
            _uniform_issue_locker, _laundry_chute_intake)[family](v, mat)


# Batch 44: freight receiving, loading dock and internal goods movement.
def _pallet_jack(v, mat):
    scene = trimesh.Scene(); w, d, h = .56 + .04 * v, 1.24 + .10 * v, 1.10 + .08 * v
    add(scene, box((w * .65, d * .52, .09), (0, 0, .16), mat), 'JackPumpChassis')
    for side, x in enumerate((-.18, .18)):
        add(scene, box((.10, d * .78, .055), (x, -d * .28, .08), 'MAT_STAINLESS'), f'PalletFork_{side}')
        for rear in (0, 1):
            roller_center = (x, d * (.38 if rear else -.62), .07)
            roller = cyl(.07, .055, roller_center, 'MAT_BLACKENED_STEEL', 14)
            _rot(roller, math.pi / 2, [1, 0, 0], roller_center); add(scene, roller, f'ForkRoller_{side}_{rear}')
    add(scene, box((.06, .08, h * .72), (0, d * .43, h * .36), 'MAT_STAINLESS'), 'JackHandleStem')
    handle_center = (0, d * .45, h * .76)
    handle = cyl(.035, .42, handle_center, 'MAT_BLACKENED_STEEL', 12)
    _rot(handle, math.pi / 2, [0, 1, 0], handle_center); add(scene, handle, 'JackGrip')
    add(scene, cyl(.075, .08, (0, 0, .22), 'MAT_BRASS_POLISHED', 14), 'HydraulicPumpCap')
    add(scene, box((.16, .035, .09), (0, -d * .42, .18), 'MAT_SIGNAGE'), 'JackCapacityPlate')
    return scene


def _dock_leveler(v, mat):
    scene = trimesh.Scene(); w, d = 1.50 + .12 * v, 1.10 + .08 * v
    add(scene, box((w, d, .17), (0, 0, .085), mat), 'DockLevelerDeck')
    add(scene, box((w * .92, .28, .11), (0, -d * .54, .10), 'MAT_STAINLESS'), 'HingedLipPlate')
    add(scene, box((w * .84, .045, .035), (0, -d * .32, .18), 'MAT_BLACKENED_STEEL'), 'DeckGripStrip')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .82 / (4 + v)
        add(scene, box((.035, d * .72, .012), (x, 0, .178), 'MAT_STAINLESS'), f'AntiSlipRib_{i}')
    for side, x in enumerate((-.42 * w, .42 * w)):
        add(scene, cyl(.04, .07, (x, d * .30, .20), 'MAT_BRASS_POLISHED', 12), f'LevelerHinge_{side}')
    add(scene, box((.16, .13, .12), (w * .55, d * .32, .18), 'MAT_ELECTRONICS'), 'LevelerControlBox')
    return scene


def _dock_shelter(v, mat):
    scene = trimesh.Scene(); w, h = 1.85 + .12 * v, 2.20 + .12 * v
    for side, x in enumerate((-w / 2, w / 2)):
        add(scene, box((.12, .16, h), (x, 0, h / 2), mat), f'ShelterJamb_{side}')
        add(scene, box((.20, .22, h * .12), (x, 0, h * .93), 'MAT_BLACKENED_STEEL'), f'RainCanopyArm_{side}')
    add(scene, box((w + .16, .30, .18), (0, 0, h - .09), 'MAT_STONE_LIGHT'), 'DockShelterHeader')
    add(scene, box((w * .76, .09, h * .74), (0, -.05, h * .47), 'MAT_LINEN'), 'WeatherCurtain')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .70 / (3 + v)
        add(scene, box((.035, .08, h * .72), (x, -.11, h * .47), 'MAT_BLACKENED_STEEL'), f'CurtainRib_{i}')
    for side, x in enumerate((-w * .42, w * .42)):
        bumper_height = .34 + .025 * v
        add(scene, box((.12, .38, bumper_height), (x, -.14, bumper_height / 2), 'MAT_PLASTIC_RUBBER'), f'DockBumper_{side}')
    return scene


def _walkie_stacker(v, mat):
    scene = trimesh.Scene(); w, d, h = .72 + .06 * v, .90 + .07 * v, 1.86 + .12 * v
    add(scene, box((w, d * .72, .16), (0, 0, .12), mat), 'StackerChassis')
    _wheels(scene, w, d, .10, 'StackerCaster')
    for side, x in enumerate((-.24, .24)):
        add(scene, box((.055, .08, h * .78), (x, d * .30, h * .39), 'MAT_STAINLESS'), f'LiftMast_{side}')
    add(scene, box((.12, .07, h * .72), (0, d * .33, h * .38), 'MAT_BLACKENED_STEEL'), 'MastCrossbar')
    add(scene, box((w * .92, .42, .10), (0, -d * .45, .25), 'MAT_STAINLESS'), 'LoadForks')
    add(scene, box((.16, .12, .48), (-w * .55, d * .20, .74), 'MAT_ELECTRONICS'), 'StackerControlHandle')
    add(scene, box((w * .66, .06, .16), (0, -d * .51, .34), 'MAT_SIGNAGE'), 'StackerCapacityLabel')
    return scene


def _wire_roll_container(v, mat):
    scene = trimesh.Scene(); w, d, h = .94 + .08 * v, .72 + .06 * v, 1.48 + .10 * v
    add(scene, box((w, d, .08), (0, 0, .18), mat), 'RollContainerBase')
    _wheels(scene, w, d, .09, 'RollContainerCaster')
    for side, y in enumerate((-.48 * d, .48 * d)):
        add(scene, box((w, .035, h), (0, y, h / 2 + .22), 'MAT_STAINLESS'), f'WireSideRail_{side}')
        for i in range(5 + v):
            x = (i - (4 + v) / 2) * w * .92 / (4 + v)
            add(scene, cyl(.014, h * .88, (x, y, h * .46 + .22), 'MAT_BLACKENED_STEEL', 8), f'WireGrid_{side}_{i}')
    for side, x in enumerate((-.48 * w, .48 * w)):
        add(scene, box((.035, d * .96, h), (x, 0, h / 2 + .22), 'MAT_STAINLESS'), f'RollContainerEnd_{side}')
    add(scene, box((w * .36, .045, .18), (0, -.52 * d, .52), 'MAT_SIGNAGE'), 'RouteCardHolder')
    return scene


def _parcel_scale(v, mat):
    scene = trimesh.Scene(); w, d = .82 + .06 * v, .68 + .05 * v
    add(scene, box((w, d, .12), (0, 0, .06), mat), 'ParcelScaleBase')
    add(scene, box((w * .90, d * .86, .055), (0, 0, .147), 'MAT_STAINLESS'), 'ParcelWeighPlatform')
    add(scene, cyl(.045, .82 + .06 * v, (w * .48, d * .40, .56), 'MAT_STAINLESS', 12), 'DisplayStand')
    add(scene, box((.34, .12, .22), (w * .48, d * .40, 1.02 + .08 * v), 'MAT_ELECTRONICS'), 'ParcelScaleDisplay')
    add(scene, box((.23, .045, .10), (w * .48, d * .33, 1.02 + .08 * v), 'MAT_GLASS_CLEAR'), 'DisplayScreen')
    for i in range(3 + v):
        add(scene, cyl(.018, .028, (w * .48 + (i - (2 + v) / 2) * .07, d * .32, .34), 'MAT_BRASS_POLISHED', 10), f'ScalePresetKey_{i}')
    add(scene, box((w * .55, .04, .07), (0, -d * .50, .12), 'MAT_SIGNAGE'), 'ParcelScaleLabel')
    return scene


def _receiving_cage(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.20 + .10 * v, .82 + .07 * v, 1.70 + .10 * v
    add(scene, box((w, d, .12), (0, 0, .06), 'MAT_STAINLESS'), 'ReceivingCageBase')
    _wheels(scene, w, d, .08, 'CageCaster')
    for side, y in enumerate((-.48 * d, .48 * d)):
        add(scene, box((w, .045, h), (0, y, h / 2 + .12), mat), f'CageWall_{side}')
    for side, x in enumerate((-.48 * w, .48 * w)):
        add(scene, box((.045, d * .96, h), (x, 0, h / 2 + .12), 'MAT_STAINLESS'), f'CageEnd_{side}')
    for i in range(6 + v):
        x = (i - (5 + v) / 2) * w * .92 / (5 + v)
        add(scene, cyl(.016, h * .90, (x, -.50 * d, h * .46 + .12), 'MAT_BLACKENED_STEEL', 8), f'CageWire_{i}')
    add(scene, box((.42, .05, .36), (0, -.51 * d, h * .67), 'MAT_SIGNAGE'), 'SecuritySealPlate')
    add(scene, cyl(.03, .06, (w * .47, -.52 * d, .58), 'MAT_BRASS_POLISHED', 10), 'CageDoorLatch')
    return scene


def _drum_dolly(v, mat):
    scene = trimesh.Scene(); w, h = .64 + .05 * v, .52 + .04 * v
    add(scene, cyl(.28 + .015 * v, .12, (0, 0, .11), mat, 24), 'DrumDollyPlatform')
    add(scene, cyl(.22 + .012 * v, .045, (0, 0, .195), 'MAT_STAINLESS', 22), 'DrumSeatRing')
    for i, angle in enumerate((0, math.pi / 2, math.pi, 3 * math.pi / 2)):
        x, y = .28 * math.cos(angle), .28 * math.sin(angle)
        add(scene, box((.09, .09, .36 + .025 * v), (x, y, .28), 'MAT_BLACKENED_STEEL'), f'DrumRetainer_{i}')
        wheel = cyl(.085, .06, (x, y, .09), 'MAT_STAINLESS', 14)
        _rot(wheel, math.pi / 2, [1, 0, 0], (x, y, .09)); add(scene, wheel, f'DollyWheel_{i}')
    add(scene, box((w * .48, .08, .075), (0, 0, h), 'MAT_SIGNAGE'), 'DollyLoadBadge')
    return scene


def _cargo_hoist(v, mat):
    scene = trimesh.Scene(); w, h = .82 + .06 * v, 1.86 + .10 * v
    add(scene, box((w * 1.22, .58, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'HoistFoot')
    for side, x in enumerate((-.43 * w, .43 * w)):
        add(scene, box((.065, .08, h), (x, 0, h / 2), mat), f'HoistMast_{side}')
    add(scene, box((w, .12, .10), (0, 0, h + .03), 'MAT_STAINLESS'), 'HoistCrossBeam')
    add(scene, cyl(.16, .22, (0, 0, h - .10), 'MAT_BLACKENED_STEEL', 18), 'CableWinchDrum')
    add(scene, cyl(.018, .52 + .04 * v, (0, 0, h - .43), 'MAT_STAINLESS', 10), 'HoistCable')
    add(scene, cyl(.065, .10, (0, 0, h - .72 - .02 * v), 'MAT_BRASS_POLISHED', 12), 'CargoHook')
    add(scene, box((.16, .20, .38), (w * .62, .08, h * .56), 'MAT_ELECTRONICS'), 'HoistControlBox')
    return scene


def _mobile_dock_ramp(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.18 + .10 * v, 1.65 + .12 * v, .38 + .04 * v
    add(scene, box((w, d, .12), (0, 0, h), mat), 'MobileRampDeck')
    for side, x in enumerate((-.42 * w, .42 * w)):
        add(scene, box((.08, d * .82, h), (x, 0, h / 2), 'MAT_BLACKENED_STEEL'), f'RampSupport_{side}')
        add(scene, box((.16, .20, .12), (x, -d * .52, .06), 'MAT_STAINLESS'), f'RampFoot_{side}')
    add(scene, box((w * .92, .20, .10), (0, d * .47, h - .02), 'MAT_STAINLESS'), 'RampHingeLip')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .86 / (4 + v)
        add(scene, box((.026, d * .86, .018), (x, 0, h + .069), 'MAT_BLACKENED_STEEL'), f'RampGripBar_{i}')
    add(scene, box((.22, .06, .13), (w * .55, -d * .42, h * .62), 'MAT_SIGNAGE'), 'RampLoadLabel')
    return scene


def _freight_receiving(family, v, mat):
    return (_pallet_jack, _dock_leveler, _dock_shelter, _walkie_stacker, _wire_roll_container,
            _parcel_scale, _receiving_cage, _drum_dolly, _cargo_hoist, _mobile_dock_ramp)[family](v, mat)


# Batch 45: hotel plant room, life safety and mechanical service assemblies.
def _electrical_switchboard(v, mat):
    scene = trimesh.Scene(); w, h = 1.10 + .10 * v, 1.76 + .10 * v
    add(scene, box((w, .34, h), (0, 0, h / 2), mat), 'SwitchboardEnclosure')
    add(scene, box((w * .82, .045, h * .82), (0, -.19, h * .53), 'MAT_STAINLESS'), 'SwitchboardDoor')
    for col in range(3 + v // 2):
        x = (col - (2 + v // 2) / 2) * w * .58 / (2 + v // 2)
        for row in range(4 + v):
            z = .42 + row * .20
            add(scene, box((.07, .04, .12), (x, -.22, z), 'MAT_BLACKENED_STEEL'), f'CircuitBreaker_{col}_{row}')
            add(scene, cyl(.018, .025, (x + .03, -.245, z), 'MAT_BRASS_POLISHED', 8), f'BreakerStatus_{col}_{row}')
    add(scene, box((.18, .05, .34), (w * .56, -.21, h * .68), 'MAT_ELECTRONICS'), 'MainDisconnect')
    add(scene, box((w * .62, .04, .12), (0, -.21, .18), 'MAT_SIGNAGE'), 'SwitchboardHazardLabel')
    add(scene, box((w * 1.04, .42, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'SwitchboardPlinth')
    return scene


def _sprinkler_valve_station(v, mat):
    scene = trimesh.Scene(); w, h = .98 + .08 * v, 1.58 + .10 * v
    add(scene, box((w * .70, .25, .10), (0, 0, .05), 'MAT_STAINLESS'), 'ValveStationFoot')
    add(scene, cyl(.09, h * .76, (0, 0, h * .38 + .10), mat, 18), 'RiserMainPipe')
    for side, x in enumerate((-.34, .34)):
        branch_center = (x, 0, h * .57)
        branch = cyl(.055, w * .60, branch_center, 'MAT_STAINLESS', 16)
        _rot(branch, math.pi / 2, [0, 1, 0], branch_center); add(scene, branch, f'SprinklerBranch_{side}')
        add(scene, cyl(.12, .12, (x, 0, h * .57), 'MAT_BRASS_POLISHED', 18), f'ValveBody_{side}')
        add(scene, cyl(.025, .40 + .03 * v, (x, 0, h * .57 + .25), 'MAT_BLACKENED_STEEL', 10), f'ValveStem_{side}')
        add(scene, box((.12, .06, .05), (x, 0, h * .57 + .48), 'MAT_STAINLESS'), f'ValveHandle_{side}')
    add(scene, box((.30, .05, .22), (0, -.15, h * .83), 'MAT_SIGNAGE'), 'FireSystemIdentityPlate')
    add(scene, cyl(.04, .06, (0, -.17, h * .68), 'MAT_ELECTRONICS', 14), 'PressureGauge')
    add(scene, box((w * .96, .05, .08), (0, -.16, .14), 'MAT_STONE_LIGHT'), 'ValveFlowArrow')
    return scene


def _fire_hose_cabinet(v, mat):
    scene = trimesh.Scene(); w, h = .82 + .06 * v, 1.20 + .08 * v
    add(scene, box((w, .28, h), (0, 0, h / 2), mat), 'FireHoseCabinet')
    add(scene, box((w * .76, .04, h * .80), (0, -.16, h * .54), 'MAT_GLASS_CLEAR'), 'CabinetGlassDoor')
    reel = cyl(.28 + .012 * v, .08, (0, -.10, h * .50), 'MAT_STAINLESS', 24)
    _rot(reel, math.pi / 2, [1, 0, 0], (0, -.10, h * .50)); add(scene, reel, 'HoseReel')
    for i in range(5 + v):
        z = .32 + i * .06
        add(scene, cyl(.018, .035, (-.05, -.16, z), 'MAT_BRASS_POLISHED', 8), f'HoseCoil_{i}')
    add(scene, box((.16, .05, .18), (w * .53, -.17, h * .66), 'MAT_BLACKENED_STEEL'), 'HoseLatch')
    add(scene, box((w * .66, .04, .12), (0, -.17, .16), 'MAT_SIGNAGE'), 'FireHoseLabel')
    return scene


def _water_heater(v, mat):
    scene = trimesh.Scene(); h, r = 1.62 + .12 * v, .42 + .025 * v
    add(scene, cyl(r * 1.16, .10, (0, 0, .05), 'MAT_STONE_LIGHT', 22), 'HeaterFoot')
    add(scene, cyl(r, h * .82, (0, 0, .10 + h * .41), mat, 26), 'HotWaterTank')
    add(scene, cyl(r * 1.04, .075, (0, 0, .10 + h * .82), 'MAT_STAINLESS', 22), 'TankCrown')
    add(scene, cyl(.045, .34 + .03 * v, (0, 0, h + .28), 'MAT_STAINLESS', 12), 'FlueStub')
    hot_center = (r * .88, 0, h * .52)
    hot_pipe = cyl(.035, .42, hot_center, 'MAT_BRASS_POLISHED', 12)
    _rot(hot_pipe, math.pi / 2, [0, 1, 0], hot_center); add(scene, hot_pipe, 'HotWaterOutlet')
    cold_center = (-r * .88, 0, h * .30)
    cold_pipe = cyl(.035, .36, cold_center, 'MAT_STAINLESS', 12)
    _rot(cold_pipe, math.pi / 2, [0, 1, 0], cold_center); add(scene, cold_pipe, 'ColdWaterInlet')
    add(scene, box((.24, .05, .32), (0, -r * 1.02, h * .50), 'MAT_ELECTRONICS'), 'HeaterThermostat')
    add(scene, box((.34, .05, .10), (0, -r * 1.04, .21), 'MAT_SIGNAGE'), 'HeaterRatingPlate')
    return scene


def _circulation_pump(v, mat):
    scene = trimesh.Scene(); w, d = .96 + .08 * v, .56 + .04 * v
    add(scene, box((w, d, .12), (0, 0, .06), 'MAT_STONE_LIGHT'), 'PumpSkid')
    add(scene, box((w * .74, .28, .24), (0, 0, .24), mat), 'PumpMotor')
    add(scene, cyl(.18 + .01 * v, .24, (0, 0, .50), 'MAT_STAINLESS', 20), 'PumpVolute')
    add(scene, cyl(.055, .48 + .03 * v, (0, 0, .77), 'MAT_BLACKENED_STEEL', 12), 'PumpOutletPipe')
    add(scene, cyl(.055, .38, (0, 0, .22), 'MAT_STAINLESS', 12), 'PumpInletPipe')
    add(scene, cyl(.12, .08, (0, .18, .52), 'MAT_BRASS_POLISHED', 16), 'PumpCouplingGuard')
    add(scene, box((.20, .12, .14), (w * .55, 0, .36), 'MAT_ELECTRONICS'), 'PumpControlBox')
    add(scene, box((w * .64, .04, .075), (0, -d * .52, .13), 'MAT_SIGNAGE'), 'PumpFlowLabel')
    return scene


def _filter_vessels(v, mat):
    scene = trimesh.Scene(); h, r = 1.42 + .10 * v, .25 + .02 * v
    for index, x in enumerate((-.34, 0, .34)):
        add(scene, cyl(r, h * .78, (x, 0, h * .39 + .08), mat, 22), f'FilterCanister_{index}')
        add(scene, cyl(r * 1.08, .08, (x, 0, .12), 'MAT_STAINLESS', 18), f'FilterFootRing_{index}')
        add(scene, cyl(r * 1.04, .07, (x, 0, h * .80 + .08), 'MAT_BLACKENED_STEEL', 18), f'FilterCap_{index}')
        add(scene, cyl(.04, .52, (x, 0, h * .48), 'MAT_STAINLESS', 12), f'FilterPipe_{index}')
    manifold_center = (0, 0, h * .52)
    manifold = cyl(.045, 1.04 + .06 * v, manifold_center, 'MAT_STAINLESS', 14)
    _rot(manifold, math.pi / 2, [0, 1, 0], manifold_center); add(scene, manifold, 'FilterManifold')
    add(scene, box((.34, .06, .20), (0, -.30, h * .64), 'MAT_SIGNAGE'), 'FilterFlowTag')
    add(scene, box((1.08, .38, .07), (0, 0, .035), 'MAT_STONE_LIGHT'), 'FilterSkidBase')
    return scene


def _booster_manifold(v, mat):
    scene = trimesh.Scene(); w, h = 1.16 + .10 * v, 1.46 + .10 * v
    add(scene, box((w, .35, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'BoosterSkid')
    add(scene, cyl(.07, h * .72, (0, 0, h * .36 + .08), mat, 16), 'BoosterHeaderPipe')
    for i in range(3 + v // 2):
        x = (i - (2 + v // 2) / 2) * w * .58 / (2 + v // 2)
        add(scene, cyl(.13 + .008 * v, .25, (x, 0, .38), 'MAT_STAINLESS', 18), f'PressurePump_{i}')
        add(scene, cyl(.035, .42, (x, 0, .72), 'MAT_BLACKENED_STEEL', 10), f'PumpRiser_{i}')
        add(scene, cyl(.06, .08, (x, 0, .98), 'MAT_BRASS_POLISHED', 12), f'IsolationValve_{i}')
    add(scene, cyl(.045, .44 + .03 * v, (w * .46, 0, h * .58), 'MAT_STAINLESS', 12), 'GaugeStem')
    add(scene, sphere(.10, (w * .46, 0, h * .78), 'MAT_STAINLESS', 2), 'PressureGaugeFace')
    add(scene, box((.24, .05, .12), (0, -.21, .17), 'MAT_SIGNAGE'), 'BoosterStationLabel')
    return scene


def _caged_service_ladder(v, mat):
    scene = trimesh.Scene(); h, w = 2.24 + .14 * v, .70 + .05 * v
    for side, x in enumerate((-.32, .32)):
        add(scene, box((.055, .07, h), (x, 0, h / 2), mat), f'LadderStringer_{side}')
    for i in range(8 + v):
        z = .22 + i * (h - .34) / (7 + v)
        add(scene, box((w, .08, .045), (0, 0, z), 'MAT_STAINLESS'), f'LadderRung_{i}')
    for side, x in enumerate((-.45, .45)):
        rail = cyl(.022, h * .70, (x, .22, h * .70), 'MAT_BLACKENED_STEEL', 10)
        _rot(rail, .22, [1, 0, 0], (x, .22, h * .70)); add(scene, rail, f'SafetyCageRail_{side}')
    for i in range(4 + v):
        z = .78 + i * .30
        add(scene, box((w + .18, .035, .035), (0, .22, z), 'MAT_STAINLESS'), f'CageHoop_{i}')
    add(scene, box((w * 1.34, .28, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'LadderBasePlate')
    return scene


def _emergency_generator(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.26 + .10 * v, .72 + .06 * v, 1.14 + .08 * v
    add(scene, box((w, d, h), (0, 0, h / 2 + .12), mat), 'GeneratorEnclosure')
    add(scene, box((w * .76, .045, h * .74), (0, -d * .52, h * .52), 'MAT_STAINLESS'), 'GeneratorAccessDoor')
    for i in range(5 + v):
        z = .34 + i * .075
        add(scene, box((w * .70, .025, .025), (0, -d * .55, z), 'MAT_BLACKENED_STEEL'), f'GeneratorVent_{i}')
    add(scene, box((.28, .05, .26), (w * .57, -d * .55, h * .72), 'MAT_ELECTRONICS'), 'GeneratorStatusPanel')
    add(scene, cyl(.075, .32 + .03 * v, (w * .35, .20, h + .28), 'MAT_STAINLESS', 14), 'ExhaustStack')
    add(scene, box((w * 1.06, d * 1.04, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'GeneratorSkid')
    return scene


def _rooftop_exhaust_fan(v, mat):
    scene = trimesh.Scene(); w, h = .92 + .07 * v, 1.18 + .08 * v
    add(scene, box((w * 1.18, .82, .12), (0, 0, .06), 'MAT_STONE_LIGHT'), 'FanRoofCurb')
    add(scene, box((w, .68, h * .60), (0, 0, h * .40), mat), 'ExhaustFanHousing')
    add(scene, box((w * 1.06, .74, .09), (0, 0, h * .70), 'MAT_STAINLESS'), 'FanShroudRim')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .72 / (4 + v)
        blade = box((.055, .42 + .02 * v, .028), (x, 0, h * .70 + .055), 'MAT_BLACKENED_STEEL')
        _rot(blade, .08 + i * .03, [0, 0, 1], (x, 0, h * .70 + .055)); add(scene, blade, f'FanBlade_{i}')
    add(scene, cyl(.12, .12, (0, 0, h * .70 + .13), 'MAT_BRASS_POLISHED', 18), 'FanHub')
    for side, x in enumerate((-.38, .38)):
        add(scene, cyl(.03, .56, (x, 0, h * .30), 'MAT_STAINLESS', 12), f'FanMotorMount_{side}')
    return scene


def _plant_and_safety(family, v, mat):
    return (_electrical_switchboard, _sprinkler_valve_station, _fire_hose_cabinet, _water_heater,
            _circulation_pump, _filter_vessels, _booster_manifold, _caged_service_ladder,
            _emergency_generator, _rooftop_exhaust_fan)[family](v, mat)


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 2001 <= number <= 2250:
        raise ValueError(f'service expansion asset ID outside A2001–A2250: {asset_id}')
    batch_index = (number - 2001) // 50
    family, variant = divmod((number - 2001) % 50, 5)
    builders = (_kitchen_machinery, _kitchen_staging, _laundry_plant, _freight_receiving, _plant_and_safety)
    return builders[batch_index](family, variant, mat)
