"""Purpose-built hotel architecture, guestroom, public, decor, and exterior assets A3701–A3900."""

import math

import trimesh

from hotel_continuation_factory import (
    MAT_BRASS, MAT_CERAMIC, MAT_ELECTRONICS, MAT_GLASS, MAT_LABEL,
    MAT_LINEN, MAT_STAINLESS, MAT_STEEL, MAT_STONE, MAT_UPHOLSTERY,
    MAT_WOOD, _casters, _floor_feet, _label, _part, _post,
)
from v2_asset_common import add, cyl, sphere


SCALE = (.90, .95, 1.00, 1.05, 1.10)


def _architecture(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = 1.32 * scale, .82 * scale

    if kind == 0:  # ballroom stage lighting portal
        height = 2.48 + .055 * variant
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'TrussFoot_{i}', (.22, .24, .14), (x, 0, .07), MAT_STONE)
            _part(scene, f'TrussColumn_{i}', (.12, .14, height), (x, 0, height / 2), material)
        _part(scene, 'LightingTrussMainBeam', (width, .18, .18), (0, 0, height + .08), MAT_STEEL)
        _part(scene, 'TrussFrontFace', (width * .92, .06, .12), (0, -.12, height + .18), MAT_BRASS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .78 / (3 + variant)
            _part(scene, f'StageSpotHousing_{i}', (.12, .15, .13),
                  (x, -.10, height - .06), MAT_ELECTRONICS)
            _part(scene, f'SpotBarnDoor_{i}', (.18, .07, .04), (x, -.16, height - .14), MAT_STEEL)
        _label(scene, 'TrussRiggingLoadCard', .34 * width, -.18, .30)
    elif kind == 1:  # rooftop intake louver bank
        height = 1.88 + .045 * variant
        _part(scene, 'IntakeBankBase', (width, depth * .74, .16), (0, 0, .08), MAT_STONE)
        _part(scene, 'IntakeBankBacking', (width * .92, .13, height), (0, .15, height / 2 + .08), MAT_STEEL)
        for i, x in enumerate((-.45 * width, .45 * width)):
            _part(scene, f'IntakeBankEndFrame_{i}', (.09, .18, height), (x, 0, height / 2 + .08), material)
        for i in range(5 + variant):
            z = .32 + i * .27
            _part(scene, f'IntakeLouverBlade_{i}', (width * .78, .22, .075), (0, -.02, z), material)
            _part(scene, f'LouverActuatorTab_{i}', (.08, .08, .06), (.39 * width, -.12, z), MAT_BRASS)
        _part(scene, 'IntakeServicePanel', (.24, .06, .32), (-.32 * width, -.12, .43), MAT_STAINLESS)
        _label(scene, 'IntakeAirflowDirection', .34 * width, -.15, .25)
    elif kind == 2:  # ballroom floor expansion joint
        _part(scene, 'ExpansionJointStoneBed', (width, depth, .16), (0, 0, .08), MAT_STONE)
        _part(scene, 'JointCenterCover', (width * .88, .16, .075), (0, 0, .20), MAT_STAINLESS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .78 / (3 + variant)
            _part(scene, f'JointCoverSegment_{i}', (width * .78 / (4 + variant), .23, .045),
                  (x, 0, .27), material)
            _part(scene, f'JointAnchorBolt_{i}', (.045, .045, .035), (x, -.14, .305), MAT_BRASS)
        for side, y in enumerate((-.34 * depth, .34 * depth)):
            _part(scene, f'FloorTileBand_{side}', (width, .18, .035), (0, y, .18), MAT_CERAMIC)
        _label(scene, 'ExpansionJointInspectionMark', .36 * width, -.38 * depth, .24)
    elif kind == 3:  # courtyard glass door jamb module
        height = 2.42 + .06 * variant
        _part(scene, 'DoorThresholdStone', (width, .38, .15), (0, 0, .075), MAT_STONE)
        for i, x in enumerate((-.44 * width, .44 * width)):
            _part(scene, f'GlassDoorJamb_{i}', (.13, .17, height), (x, 0, height / 2), material)
            _part(scene, f'JambShadowLine_{i}', (.035, .19, height * .90),
                  (x - .07, -.04, height / 2), MAT_STEEL)
        _part(scene, 'DoorGlassLeaf', (width * .53, .045, height * .84), (-.14 * width, .08, height * .45), MAT_GLASS)
        _part(scene, 'DoorPullBar', (.045, .07, .60), (.04 * width, -.02, 1.05), MAT_BRASS)
        _part(scene, 'DoorCloserHeader', (width * .60, .12, .10), (0, .02, height - .10), MAT_STAINLESS)
        _part(scene, 'DoorSafetyStripe', (width * .42, .02, .065), (-.14 * width, .045, 1.05), MAT_LABEL)
        _label(scene, 'DoorAccessSymbol', .34 * width, -.12, .27)
    elif kind == 4:  # accessible corridor ramp handrail return
        run = 1.30 * scale
        _part(scene, 'RampLandingBase', (run, depth * .72, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'RampSurface', (run * .90, depth * .62, .12), (0, 0, .20), material)
        for i, x in enumerate((-.44 * run, .44 * run)):
            _post(scene, f'RampRailPost_{i}', x, 0, .88 + .025 * variant, .035, MAT_STEEL)
            _part(scene, f'RailFootPlate_{i}', (.17, .19, .055), (x, 0, .27), MAT_STAINLESS)
        _part(scene, 'RampTopHandrail', (run * .94, .075, .075), (0, 0, .91 + .025 * variant), MAT_WOOD)
        _part(scene, 'RampLowerHandrail', (run * .90, .055, .055), (0, -.06, .62), MAT_STEEL)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * run * .82 / (4 + variant)
            _part(scene, f'RampAntiSlipGroove_{i}', (.035, depth * .54, .018), (x, 0, .27), MAT_BRASS)
        _label(scene, 'RampMaximumSlopePlate', .33 * run, -.38 * depth, .32)
    elif kind == 5:  # ballroom stage front and access steps
        stage_h = .42 + .025 * variant
        _part(scene, 'StageDeck', (width, depth * .72, .14), (0, 0, stage_h), material)
        _part(scene, 'StageFrontFascia', (width * .98, .08, stage_h), (0, -.37 * depth, stage_h / 2), MAT_WOOD)
        _part(scene, 'StageAccessStepLower', (width * .36, .52, .17), (-.31 * width, -.42 * depth, .085), MAT_STONE)
        _part(scene, 'StageAccessStepUpper', (width * .36, .38, .15), (-.31 * width, -.29 * depth, .25), MAT_STONE)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .78 / (3 + variant)
            _part(scene, f'FasciaAcousticGrille_{i}', (.14, .035, .18),
                  (x, -.42 * depth, stage_h / 2), MAT_STEEL)
        _part(scene, 'StageEdgeReveal', (width * .96, .045, .045), (0, -.42 * depth, stage_h + .10), MAT_BRASS)
        _label(scene, 'StageLiftPointLabel', .38 * width, -.45 * depth, .25)
    elif kind == 6:  # roof drain overflow pan assembly
        _part(scene, 'RoofOverflowPan', (width, depth, .17), (0, 0, .085), MAT_STAINLESS)
        _part(scene, 'DrainSumpBasin', (.44, .36, .28), (0, .06, .30), MAT_STONE)
        _part(scene, 'PrimaryDrainCup', (.26, .24, .08), (0, .06, .48), MAT_STEEL)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * width * .78 / (4 + variant)
            _part(scene, f'OverflowPanRib_{i}', (.035, depth * .82, .055), (x, 0, .20), material)
        for side, x in enumerate((-.34 * width, .34 * width)):
            _part(scene, f'OverflowOutlet_{side}', (.16, .18, .13), (x, .30 * depth, .27), MAT_STAINLESS)
        _part(scene, 'DrainInspectionLid', (.26, .05, .20), (.36 * width, -.39 * depth, .32), MAT_STEEL)
        _label(scene, 'RoofWaterFlowArrow', -.36 * width, -.40 * depth, .29)
    elif kind == 7:  # guest corridor corner protection bay
        height = 1.82 + .045 * variant
        _part(scene, 'CornerGuardWallA', (.16, depth, height), (-.18, 0, height / 2), MAT_STONE)
        _part(scene, 'CornerGuardWallB', (depth, .16, height), (0, .18, height / 2), MAT_STONE)
        for i in range(3 + variant):
            z = .36 + i * .35
            _part(scene, f'CornerWallBumperA_{i}', (.08, depth * .72, .13), (-.08, 0, z), material)
            _part(scene, f'CornerWallBumperB_{i}', (depth * .72, .08, .13), (0, .08, z), material)
        _part(scene, 'CornerRevealCap', (.22, .22, .16), (-.05, .05, height - .04), MAT_WOOD)
        _label(scene, 'CornerGuardFireRating', -.25, -.42 * depth, .30)
    elif kind == 8:  # service riser access panel and pipe chase
        height = 2.24 + .05 * variant
        _part(scene, 'RiserBackplate', (width, .12, height), (0, .08, height / 2), MAT_STONE)
        _part(scene, 'RiserAccessDoor', (width * .62, .055, height * .74), (0, -.02, height * .48), material)
        for i, x in enumerate((-.40 * width, .40 * width)):
            _part(scene, f'RiserPipeColumn_{i}', (.095, .11, height * .92), (x, -.10, height / 2), MAT_STAINLESS)
            _part(scene, f'RiserPipeCoupling_{i}', (.15, .16, .12), (x, -.10, .62), MAT_BRASS)
        for i in range(3 + variant):
            z = .50 + i * .42
            _part(scene, f'RiserDoorVent_{i}', (width * .42, .025, .045), (0, -.065, z), MAT_STEEL)
        _part(scene, 'RiserLatch', (.055, .06, .26), (.28 * width, -.08, 1.02), MAT_STAINLESS)
        _label(scene, 'RiserServiceSchedule', -.31 * width, -.10, .30)
    else:  # night arrival wind screen and curb
        _part(scene, 'ArrivalWindScreenFoot', (width, depth * .66, .16), (0, 0, .08), MAT_STONE)
        height = 2.12 + .045 * variant
        _part(scene, 'ArrivalScreenGlass', (width * .78, .045, height * .90), (0, .02, height * .52), MAT_GLASS)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'ArrivalScreenPost_{i}', (.08, .12, height), (x, 0, height / 2), material)
        _part(scene, 'ArrivalCanopyBlade', (width * .96, depth * .85, .10), (0, 0, height + .08), MAT_STEEL)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .80 / (3 + variant)
            _part(scene, f'CanopyBladeRib_{i}', (.035, depth * .72, .045), (x, 0, height + .16), MAT_BRASS)
        _label(scene, 'NightEntryAccessibilityMarker', .34 * width, -.23, .27)
    return scene


def _guestroom(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = 1.08 * scale, .70 * scale

    if kind == 0:  # guest bath linen hamper bench
        seat = .48 + .025 * variant
        _part(scene, 'LinenHamperCase', (width * .84, depth * .72, .34), (0, 0, .25), material)
        _part(scene, 'LinenHamperFront', (width * .72, .05, .22), (0, -.40 * depth, .28), MAT_LINEN)
        _part(scene, 'HamperVentPanel', (width * .48, .035, .16), (0, -.44 * depth, .27), MAT_STEEL)
        _part(scene, 'BathBenchSeat', (width, depth, .12), (0, 0, seat), MAT_UPHOLSTERY)
        _floor_feet(scene, width * .88, depth * .68, .15, 'BathBenchFoot', MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .62 / (2 + variant)
            _part(scene, f'HamperVentSlot_{i}', (.045, .025, .12), (x, -.46 * depth, .28), MAT_BRASS)
        _label(scene, 'BathBenchLaundrySymbol', .34 * width, -.40 * depth, .19)
    elif kind == 1:  # long-stay kitchenette pantry tower
        height = 1.86 + .04 * variant
        _part(scene, 'KitchenettePlinth', (width * .76, depth * .76, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'PantryTowerCase', (width * .70, depth * .70, height), (0, 0, .12 + height / 2), material)
        _part(scene, 'PantryDoorUpper', (width * .64, .045, height * .48), (0, -.38 * depth, 1.38), MAT_WOOD)
        _part(scene, 'PantryDrawerLower', (width * .62, .05, .32), (0, -.39 * depth, .40), MAT_LINEN)
        _part(scene, 'MicrowaveShelf', (width * .66, depth * .56, .07), (0, 0, .97), MAT_STONE)
        _part(scene, 'MicrowaveOven', (.42, .32, .28), (-.10 * width, .02, 1.15), MAT_STAINLESS)
        _part(scene, 'PantryHandle', (.04, .05, .31), (.24 * width, -.42 * depth, 1.36), MAT_BRASS)
        for i in range(3 + variant):
            z = .54 + i * .15
            _part(scene, f'PantryShelfPin_{i}', (.065, .045, .035), (.31 * width, -.10, z), MAT_STEEL)
        _label(scene, 'KitchenetteCircuitNotice', -.29 * width, -.40 * depth, .24)
    elif kind == 2:  # suitcase valet with fold-out shelf
        height = .88 + .025 * variant
        _part(scene, 'SuitcaseValetBase', (width * .90, depth * .60, .12), (0, 0, .06), MAT_STONE)
        for i, x in enumerate((-.36 * width, .36 * width)):
            _part(scene, f'ValetFrameLeg_{i}', (.07, .08, height), (x, 0, height / 2), MAT_STEEL)
        _part(scene, 'SuitcaseCanvasSlats', (width * .84, depth * .54, .10), (0, 0, height * .62), MAT_LINEN)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .72 / (3 + variant)
            _part(scene, f'ValetSupportStrap_{i}', (.035, depth * .48, .055), (x, 0, height * .62), MAT_WOOD)
        _part(scene, 'FoldoutToiletryShelf', (width * .62, depth * .36, .065), (0, .33 * depth, height + .09), material)
        _part(scene, 'ValetBagRetainer', (width * .82, .06, .07), (0, -.24 * depth, height * .68), MAT_BRASS)
        _label(scene, 'ValetAssemblyLabel', .34 * width, -.32 * depth, .22)
    elif kind == 3:  # bedside reading trough and book light
        height = .56 + .025 * variant
        _part(scene, 'ReadingTroughBase', (width * .80, depth * .64, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'ReadingTroughCase', (width * .70, depth * .58, height), (0, 0, .12 + height / 2), material)
        _part(scene, 'BookPocketFront', (width * .62, .05, .30), (0, -.32 * depth, .36), MAT_WOOD)
        _part(scene, 'PocketLedge', (width * .70, depth * .42, .08), (0, .04, height + .14), MAT_UPHOLSTERY)
        _post(scene, 'ReadingLampStem', -.27 * width, .10, 1.42 + .025 * variant, .022, MAT_BRASS)
        _part(scene, 'ReadingLampShade', (.18, .16, .12), (-.27 * width, .10, 1.44 + .025 * variant), MAT_ELECTRONICS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .48 / (2 + variant)
            _part(scene, f'BookDivider_{i}', (.045, depth * .50, .25), (x, .02, .48), MAT_STEEL)
        _floor_feet(scene, width * .68, depth * .54, .13, 'ReadingTroughFoot', MAT_WOOD)
        _label(scene, 'ReadingTroughPowerGuide', .34 * width, -.37 * depth, .20)
    elif kind == 4:  # child travel crib with conversion rail
        length = 1.22 + .035 * variant
        _part(scene, 'TravelCribMattress', (width * .82, length, .12), (0, 0, .40), MAT_LINEN)
        _part(scene, 'CribBaseFrame', (width * .88, length * .95, .11), (0, 0, .27), MAT_WOOD)
        for side, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'CribEndPanel_{side}', (.10, length, .74), (x, 0, .80), material)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * width * .70 / (4 + variant)
            _part(scene, f'CribSafetySlat_{i}', (.035, .04, .55), (x, -.44 * length, .78), MAT_STEEL)
        _part(scene, 'CribConversionRail', (width * .72, .08, .08), (0, .40 * length, 1.22), MAT_BRASS)
        _casters(scene, width * .84, length * .88, 'CribCaster')
        _label(scene, 'CribWeightLimitPatch', .35 * width, -.45 * length, .25)
    elif kind == 5:  # accessible bath stool and grab rail set
        seat_h = .48 + .02 * variant
        _part(scene, 'BathStoolSeat', (width * .72, depth * .70, .10), (0, 0, seat_h), material)
        _floor_feet(scene, width * .66, depth * .64, seat_h - .09, 'BathStoolLeg', MAT_STAINLESS)
        for i, x in enumerate((-.37 * width, .37 * width)):
            _part(scene, f'StoolArmrest_{i}', (.08, depth * .60, .08), (x, 0, seat_h + .16), MAT_UPHOLSTERY)
            _post(scene, f'ArmrestSupport_{i}', x, 0, seat_h + .13, .025, MAT_STEEL)
        _part(scene, 'StoolBackSupport', (width * .68, .07, .46), (0, .27 * depth, seat_h + .28), MAT_LINEN)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .54 / (3 + variant)
            _part(scene, f'StoolDrainageSlot_{i}', (.025, .12, .018), (x, 0, seat_h + .055), MAT_STEEL)
        _label(scene, 'BathStoolLoadRating', .33 * width, -.39 * depth, .20)
    elif kind == 6:  # guestroom lounge reading chair
        seat_h = .48 + .02 * variant
        _part(scene, 'LoungeChairSeat', (width * .88, depth * .80, .16), (0, 0, seat_h), MAT_UPHOLSTERY)
        _part(scene, 'LoungeChairBack', (width * .88, .14, .78), (0, .30 * depth, .92), material)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'LoungeChairArm_{i}', (.14, depth * .68, .24), (x, 0, seat_h + .20), MAT_WOOD)
        _floor_feet(scene, width * .76, depth * .74, seat_h - .12, 'LoungeChairFoot', MAT_STEEL)
        _part(scene, 'ChairSideBookPocket', (.27, .06, .28), (.40 * width, -.36 * depth, .60), MAT_LINEN)
        for i in range(3 + variant):
            _part(scene, f'ChairUpholsteryTuft_{i}', (.06, .035, .06),
                  ((i - (2 + variant) / 2) * .14, .20 * depth, 1.04), MAT_BRASS)
        _label(scene, 'LoungeChairCareTag', -.34 * width, -.39 * depth, .20)
    elif kind == 7:  # suite ottoman with lift-up tray
        seat_h = .46 + .025 * variant
        _part(scene, 'OttomanBasePlinth', (width * .82, depth * .82, .10), (0, 0, .05), MAT_STONE)
        _part(scene, 'OttomanStorageBody', (width * .76, depth * .76, seat_h - .08),
              (0, 0, .10 + (seat_h - .08) / 2), material)
        _part(scene, 'OttomanCushionLid', (width * .90, depth * .90, .14), (0, 0, seat_h + .03), MAT_UPHOLSTERY)
        for i, x in enumerate((-.31 * width, .31 * width)):
            _part(scene, f'TrayLiftHinge_{i}', (.08, .08, .07), (x, .30 * depth, seat_h + .12), MAT_BRASS)
        _part(scene, 'OttomanTrayPanel', (width * .68, depth * .46, .065), (0, .18 * depth, seat_h + .20), MAT_WOOD)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .54 / (2 + variant)
            _part(scene, f'OttomanStorageStitch_{i}', (.025, .04, .23), (x, -.39 * depth, .27), MAT_LINEN)
        _label(scene, 'OttomanStorageInstruction', .33 * width, -.39 * depth, .18)
    elif kind == 8:  # heated bath towel rail with shelf
        height = 1.30 + .04 * variant
        _part(scene, 'TowelWarmerBase', (width * .68, depth * .55, .12), (0, 0, .06), MAT_STONE)
        for i, x in enumerate((-.30 * width, .30 * width)):
            _part(scene, f'TowelWarmerUpright_{i}', (.06, .08, height), (x, 0, height / 2 + .12), material)
        for i in range(4 + variant):
            z = .42 + i * .21
            _part(scene, f'HeatedTowelRail_{i}', (width * .60, .055, .055), (0, -.04, z), MAT_STAINLESS)
        _part(scene, 'LinenShelf', (width * .82, depth * .62, .08), (0, .06, height + .13), MAT_WOOD)
        _part(scene, 'WarmerControlPanel', (.18, .05, .20), (.34 * width, -.10, 1.04), MAT_ELECTRONICS)
        _label(scene, 'TowelWarmerTimerGuide', .34 * width, -.34 * depth, .24)
    else:  # room service tray return cart
        height = 1.08 + .025 * variant
        _part(scene, 'TrayReturnCartLowerShelf', (width, depth * .72, .10), (0, 0, .28), material)
        _part(scene, 'TrayReturnCartUpperShelf', (width * .94, depth * .70, .10), (0, 0, height), MAT_STAINLESS)
        for i, x in enumerate((-.42 * width, .42 * width)):
            _part(scene, f'TrayCartUpright_{i}', (.06, .08, height - .14), (x, 0, height / 2 + .14), MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .70 / (2 + variant)
            _part(scene, f'TrayRackDivider_{i}', (.045, depth * .62, .34), (x, 0, .62), MAT_WOOD)
        _casters(scene, width, depth, 'TrayCartCaster')
        _part(scene, 'CartHandle', (.07, .28, .07), (0, .18, height + .08), MAT_BRASS)
        _label(scene, 'TrayReturnRoomNumber', .35 * width, -.35 * depth, .22)
    return scene


def _public(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = 1.10 * scale, .72 * scale

    if kind == 0:  # spa reception service podium
        height = 1.02 + .025 * variant
        _part(scene, 'SpaReceptionBase', (width * .76, depth * .68, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'SpaReceptionBody', (width * .66, depth * .58, height), (0, 0, .12 + height / 2), material)
        _part(scene, 'GuestWelcomeCounter', (width, depth * .70, .10), (0, 0, height + .17), MAT_WOOD)
        _part(scene, 'SpaAppointmentScreen', (.30, .07, .24), (.24 * width, -.29 * depth, height + .32), MAT_ELECTRONICS)
        _part(scene, 'AppointmentCardTray', (.34, .25, .06), (-.29 * width, -.06, height + .14), MAT_STAINLESS)
        _part(scene, 'ReceptionPrivacyWing', (.055, depth * .60, .42), (.41 * width, .04, height + .32), MAT_LINEN)
        _label(scene, 'SpaReceptionQuietZone', .34 * width, -.39 * depth, .23)
    elif kind == 1:  # lobby meeting table and conversation stools
        table_h = .72 + .02 * variant
        _part(scene, 'MeetingTableTop', (width, depth * .86, .10), (0, 0, table_h), material)
        _floor_feet(scene, width * .86, depth * .74, table_h - .09, 'MeetingTableLeg', MAT_STEEL)
        for i, x in enumerate((-.44 * width, .44 * width)):
            _part(scene, f'ConversationStoolSeat_{i}', (.30, .30, .09), (x, -.62 * depth, .44), MAT_UPHOLSTERY)
            _post(scene, f'ConversationStoolPedestal_{i}', x, -.62 * depth, .40, .045, MAT_WOOD)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .62 / (2 + variant)
            _part(scene, f'TablePowerGrommet_{i}', (.10, .09, .035), (x, 0, table_h + .06), MAT_ELECTRONICS)
        _label(scene, 'MeetingTableDataPortGuide', .36 * width, -.38 * depth, .22)
    elif kind == 2:  # conference badge print check-in kiosk
        height = 1.56 + .035 * variant
        _part(scene, 'BadgeKioskBase', (width * .68, depth * .60, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'BadgeKioskMast', (.18, .18, height), (0, 0, height / 2 + .08), MAT_STEEL)
        _part(scene, 'BadgePrinterShelf', (width * .86, depth * .60, .09), (0, -.03, .78), material)
        _part(scene, 'BadgePrinter', (.34, .32, .25), (-.20 * width, .02, .96), MAT_STAINLESS)
        _part(scene, 'BadgeScanScreen', (.36, .07, .42), (0, -.12, height * .75), MAT_ELECTRONICS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .58 / (2 + variant)
            _part(scene, f'BadgeStockSlot_{i}', (.13, .18, .055), (x, .17 * depth, .86), MAT_LABEL)
        _label(scene, 'BadgeKioskHelpSymbol', .34 * width, -.14, .31)
    elif kind == 3:  # public reading room magazine stand
        height = 1.52 + .03 * variant
        _part(scene, 'MagazineStandBase', (width * .68, depth * .58, .13), (0, 0, .065), MAT_STONE)
        for i, y in enumerate((-.22 * depth, 0, .22 * depth)):
            _part(scene, f'MagazineRackShelf_{i}', (width * .82, depth * .30, .08),
                  (0, y, .42 + i * .40), material)
            for slot in range(3 + variant):
                x = (slot - (2 + variant) / 2) * width * .68 / (2 + variant)
                _part(scene, f'PeriodicalPocket_{i}_{slot}', (.18, .06, .30),
                      (x, y - .08, .60 + i * .40), MAT_LINEN)
        for i, x in enumerate((-.39 * width, .39 * width)):
            _part(scene, f'MagazineRackUpright_{i}', (.07, depth * .58, height),
                  (x, 0, height / 2), MAT_STEEL)
        _label(scene, 'ReadingRoomReturnNotice', .34 * width, -.34 * depth, .26)
    elif kind == 4:  # stroller and mobility parking rack
        _part(scene, 'MobilityRackBase', (width, depth * .72, .13), (0, 0, .065), MAT_STONE)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .82 / (3 + variant)
            _part(scene, f'StrollerWheelChannel_{i}', (.14, depth * .58, .10), (x, 0, .18), material)
            _part(scene, f'HandleHook_{i}', (.08, .10, .16), (x, .20 * depth, .94), MAT_BRASS)
            _post(scene, f'RackPost_{i}', x, .20 * depth, .89, .024, MAT_STEEL)
        _part(scene, 'MobilityRackOverheadRail', (width * .92, .08, .08), (0, .20 * depth, .92), MAT_WOOD)
        _part(scene, 'AccessibleParkingNotice', (.30, .06, .24), (0, -.25 * depth, .70), MAT_LABEL)
        _label(scene, 'RackUseInstructions', .34 * width, -.34 * depth, .23)
    elif kind == 5:  # banquet water refill island
        height = .90 + .025 * variant
        _part(scene, 'WaterIslandBase', (width, depth * .76, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'WaterIslandCabinet', (width * .90, depth * .66, height), (0, 0, .13 + height / 2), material)
        _part(scene, 'RefillCountertop', (width * 1.06, depth * .80, .10), (0, 0, height + .18), MAT_STAINLESS)
        for i, x in enumerate((-.28 * width, 0, .28 * width)):
            _post(scene, f'WaterDispenserTap_{i}', x, -.24 * depth, height + .54, .025, MAT_BRASS)
            _part(scene, f'TapBackplate_{i}', (.17, .045, .24), (x, -.22 * depth, height + .38), MAT_STEEL)
        _part(scene, 'WaterDrainGrate', (width * .46, .25, .04), (0, -.26 * depth, height + .11), MAT_STEEL)
        _floor_feet(scene, width * .86, depth * .62, .12, 'WaterIslandFoot', MAT_STEEL)
        _label(scene, 'WaterStationFilterDate', .34 * width, -.38 * depth, .25)
    elif kind == 6:  # lobby parcel pickup counter
        height = .94 + .025 * variant
        _part(scene, 'ParcelCounterBase', (width * .88, depth * .64, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'ParcelCounterBody', (width * .80, depth * .58, height), (0, 0, .13 + height / 2), material)
        _part(scene, 'PickupCounterTop', (width, depth * .76, .10), (0, 0, height + .18), MAT_WOOD)
        _part(scene, 'ParcelScanPad', (.25, .24, .045), (-.26 * width, -.22 * depth, height + .25), MAT_ELECTRONICS)
        _part(scene, 'PackageScale', (.34, .30, .08), (.23 * width, -.10 * depth, height + .23), MAT_STAINLESS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .64 / (2 + variant)
            _part(scene, f'PickupOrderSlot_{i}', (.14, .07, .18), (x, .30 * depth, height + .12), MAT_LABEL)
        _label(scene, 'ParcelPickupHoursCard', .34 * width, -.38 * depth, .22)
    else:  # public event room portable registration table
        height = .82 + .02 * variant
        _part(scene, 'RegistrationTableTop', (width, depth * .86, .10), (0, 0, height), material)
        _floor_feet(scene, width * .90, depth * .78, height - .09, 'RegistrationTableLeg', MAT_STEEL)
        _part(scene, 'RegistrationPrivacyPanel', (width * .78, .06, .36), (0, .32 * depth, height + .20), MAT_LINEN)
        _part(scene, 'GuestCheckInTablet', (.28, .06, .21), (-.29 * width, -.36 * depth, height + .17), MAT_ELECTRONICS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .66 / (2 + variant)
            _part(scene, f'RegistrationPencilCup_{i}', (.10, .10, .15), (x, -.06, height + .12), MAT_CERAMIC)
        _part(scene, 'TablefrontEventSkirt', (width * .82, .035, .28), (0, -.43 * depth, .52), MAT_WOOD)
        _label(scene, 'RegistrationQueueArrow', .35 * width, -.42 * depth, .24)
    return scene


def _decor(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width = .60 * scale

    if kind == 0:  # ceramic vessel trio on plinth
        _part(scene, 'VesselDisplayBase', (width * 1.30, .50 * scale, .10), (0, 0, .05), MAT_STONE)
        for i, x in enumerate((-.30 * width, 0, .30 * width)):
            size = .20 + .025 * ((i + variant) % 3)
            _part(scene, f'CeramicVesselBody_{i}', (size, size, .38 + .035 * variant),
                  (x, 0, .31 + .015 * variant), material)
            _part(scene, f'VesselNeck_{i}', (size * .42, size * .42, .12),
                  (x, 0, .56 + .02 * variant), MAT_CERAMIC)
        _part(scene, 'VesselDisplayCard', (.22, .025, .12), (0, -.28 * scale, .18), MAT_LABEL)
        _label(scene, 'VesselCollectionNumber', .38 * width, -.31 * scale, .20)
    elif kind == 1:  # brass and marble abstract sculpture
        _part(scene, 'SculpturePlinth', (width * .68, width * .68, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'AbstractSculptureCore', (.20 * scale, .18 * scale, .56 + .03 * variant),
              (0, 0, .42), material)
        for i, x in enumerate((-.18 * scale, .18 * scale)):
            add(scene, cyl(.085 + .01 * variant, .12, (x, 0, .77 + .03 * variant), MAT_BRASS, 16),
                f'SculptureRing_{i}')
        _part(scene, 'SculptureCrossbar', (.48 * scale, .08, .09), (0, 0, .88 + .03 * variant), MAT_STEEL)
        _part(scene, 'SculptureMakerPlate', (.24, .025, .10), (.25 * scale, -.20 * scale, .25), MAT_LABEL)
        _label(scene, 'SculptureInventoryCode', -.30 * scale, -.25 * scale, .18)
    elif kind == 2:  # hotel crest relief plaque
        _part(scene, 'CrestPlaqueBacking', (width, .08, .58 * scale), (0, .03, 1.62), MAT_WOOD)
        _part(scene, 'CrestPlaqueFrame', (width * 1.08, .06, .66 * scale), (0, -.03, 1.62), MAT_BRASS)
        _part(scene, 'HotelShieldRelief', (.34 * scale, .05, .42 * scale), (0, -.08, 1.63), material)
        for side, x in enumerate((-.23 * scale, .23 * scale)):
            _part(scene, f'CrestLaurelBranch_{side}', (.10, .035, .36 * scale),
                  (x, -.10, 1.63), MAT_STEEL)
        _part(scene, 'CrestRibbon', (.44 * scale, .06, .10), (0, -.13, 1.38), MAT_LINEN)
        _label(scene, 'CrestPlaqueEdition', .32 * scale, -.11, 1.26)
    elif kind == 3:  # framed heritage photograph triptych
        for i, x in enumerate((-.33 * scale, 0, .33 * scale)):
            _part(scene, f'PhotoFrame_{i}', (.28 * scale, .07, .44 * scale),
                  (x, 0, 1.70 + .025 * variant), material)
            _part(scene, f'HeritagePhoto_{i}', (.21 * scale, .035, .34 * scale),
                  (x, -.05, 1.70 + .025 * variant), MAT_LABEL)
            _part(scene, f'PhotoMat_{i}', (.24 * scale, .025, .38 * scale),
                  (x, -.03, 1.70 + .025 * variant), MAT_LINEN)
        _part(scene, 'TriptychMountingRail', (width * 1.18, .09, .07), (0, .02, 2.02), MAT_STEEL)
        for i, x in enumerate((-.33 * scale, 0, .33 * scale)):
            _part(scene, f'FrameHanger_{i}', (.06, .04, .08), (x, 0, 2.04), MAT_BRASS)
        _label(scene, 'TriptychArchiveCredit', .31 * scale, -.06, 1.34)
    elif kind == 4:  # seasonal flower arrangement bowl
        _part(scene, 'SeasonalDisplayTray', (width * 1.20, .48 * scale, .08), (0, 0, .04), MAT_WOOD)
        _part(scene, 'FlowerBowlFoot', (.32 * scale, .32 * scale, .12), (0, 0, .14), MAT_STONE)
        _part(scene, 'FlowerBowl', (.48 * scale, .42 * scale, .24), (0, 0, .29), material)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * .10 * scale
            y = .10 * scale * ((i % 2) - .5)
            _part(scene, f'ArrangementStem_{i}', (.035, .035, .42), (x, y, .58), MAT_STEEL)
            add(scene, sphere(.085 + .008 * variant, (x, y, .82), MAT_LINEN, subdivisions=1),
                f'ArrangementBloom_{i}')
        _part(scene, 'FlowerCareCard', (.24, .025, .11), (.32 * scale, -.22 * scale, .19), MAT_LABEL)
        _label(scene, 'SeasonalArrangementID', -.32 * scale, -.25 * scale, .18)
    elif kind == 5:  # concierge desk bookend set
        _part(scene, 'BookendDisplayShelf', (width * 1.12, .44 * scale, .07), (0, 0, .035), MAT_WOOD)
        for side, x in enumerate((-.28 * scale, .28 * scale)):
            _part(scene, f'BookendFoot_{side}', (.18 * scale, .32 * scale, .06), (x, 0, .10), MAT_BRASS)
            _part(scene, f'BookendUpright_{side}', (.06, .32 * scale, .34 + .025 * variant),
                  (x, 0, .30), material)
            _part(scene, f'BookendFinial_{side}', (.13, .13, .12), (x, 0, .53 + .025 * variant), MAT_STONE)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * .105 * scale
            _part(scene, f'ConciergeBook_{i}', (.09, .28 * scale, .35), (x, 0, .27), MAT_LINEN)
        _label(scene, 'BookendSetAssetTag', .34 * scale, -.25 * scale, .16)
    elif kind == 6:  # sculptural glass bowl on brass stand
        _part(scene, 'GlassBowlStandBase', (.42 * scale, .42 * scale, .08), (0, 0, .04), MAT_STONE)
        _post(scene, 'GlassBowlStandStem', 0, 0, .48 + .025 * variant, .04, MAT_BRASS)
        _part(scene, 'BowlStandCollar', (.20 * scale, .20 * scale, .08),
              (0, 0, .47 + .025 * variant), MAT_BRASS)
        _part(scene, 'SculpturalGlassBowl', (.58 * scale, .50 * scale, .24),
              (0, 0, .68 + .025 * variant), MAT_GLASS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * .12 * scale
            _part(scene, f'BowlCutFacet_{i}', (.055, .025, .09), (x, -.18 * scale, .68), material)
        _label(scene, 'GlassBowlCareCode', .34 * scale, -.27 * scale, .22)
    elif kind == 7:  # antique compass globe desk display
        _part(scene, 'CompassDisplayBase', (.52 * scale, .52 * scale, .10), (0, 0, .05), MAT_WOOD)
        _post(scene, 'CompassDisplayStem', 0, 0, .48, .035, MAT_BRASS)
        add(scene, sphere(.25 * scale, (0, 0, .78), material, subdivisions=2), 'CompassGlobe')
        _part(scene, 'CompassEquatorBand', (.53 * scale, .07, .045), (0, 0, .78), MAT_BRASS)
        for i, (x, y) in enumerate(((0, -.26), (.26, 0), (0, .26), (-.26, 0))):
            _part(scene, f'CompassDirectionMark_{i}', (.08, .035, .08),
                  (x * scale, y * scale, .78), MAT_LABEL)
        _part(scene, 'CompassNameplate', (.24, .025, .10), (.24 * scale, -.24 * scale, .18), MAT_STEEL)
        _label(scene, 'CompassDisplayIndex', -.32 * scale, -.25 * scale, .16)
    elif kind == 8:  # paired lobby floor lanterns
        for i, x in enumerate((-.27 * scale, .27 * scale)):
            _part(scene, f'LanternFoot_{i}', (.32 * scale, .32 * scale, .10), (x, 0, .05), MAT_STONE)
            _part(scene, f'LanternColumn_{i}', (.18 * scale, .18 * scale, .76 + .025 * variant),
                  (x, 0, .48), material)
            _part(scene, f'LanternGlass_{i}', (.24 * scale, .24 * scale, .30),
                  (x, 0, 1.02 + .025 * variant), MAT_GLASS)
            _part(scene, f'LanternCap_{i}', (.32 * scale, .32 * scale, .10),
                  (x, 0, 1.22 + .025 * variant), MAT_BRASS)
            _part(scene, f'LanternLight_{i}', (.09, .09, .18), (x, 0, 1.02 + .025 * variant), MAT_ELECTRONICS)
        _part(scene, 'LanternPairConnectingBase', (width * .92, .25, .06), (0, 0, .13), MAT_WOOD)
        _label(scene, 'LanternPairElectricalTag', .34 * scale, -.24 * scale, .20)
    else:  # miniature hotel model in glass vitrine
        _part(scene, 'VitrineDisplayFoot', (width * 1.10, .68 * scale, .09), (0, 0, .045), MAT_STONE)
        _part(scene, 'VitrineGlassCase', (width, .56 * scale, .50 + .025 * variant),
              (0, 0, .37 + .025 * variant), MAT_GLASS)
        _part(scene, 'MiniHotelMainBlock', (.50 * scale, .34 * scale, .42), (0, 0, .34), MAT_WOOD)
        _part(scene, 'HotelModelRoof', (.56 * scale, .39 * scale, .08), (0, 0, .59), material)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * .10 * scale
            _part(scene, f'MiniHotelWindow_{i}', (.06, .025, .12), (x, -.18 * scale, .43), MAT_LABEL)
        _part(scene, 'VitrineMakerCard', (.25, .025, .10), (.31 * scale, -.27 * scale, .18), MAT_LINEN)
        _label(scene, 'MiniHotelScaleMarker', -.34 * scale, -.27 * scale, .17)
    return scene


def _exterior(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = .86 * scale, .66 * scale

    if kind == 0:  # courtyard cast drinking fountain
        _part(scene, 'FountainBasePlinth', (width * .76, depth * .76, .16), (0, 0, .08), MAT_STONE)
        _part(scene, 'FountainPedestal', (.42 * scale, .42 * scale, .56 + .025 * variant),
              (0, 0, .44), material)
        _part(scene, 'FountainBowl', (.78 * scale, .64 * scale, .18), (0, 0, .83 + .025 * variant), MAT_STONE)
        _part(scene, 'FountainBasinInset', (.58 * scale, .46 * scale, .07),
              (0, 0, .94 + .025 * variant), MAT_CERAMIC)
        _post(scene, 'FountainSpout', 0, -.23 * depth, 1.13 + .025 * variant, .025, MAT_STAINLESS)
        _part(scene, 'FountainSpoutLip', (.18, .12, .06), (0, -.23 * depth, 1.22 + .025 * variant), MAT_STAINLESS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .48 / (2 + variant)
            _part(scene, f'FountainBowlRelief_{i}', (.07, .025, .08), (x, -.31 * depth, .81), MAT_BRASS)
        _label(scene, 'FountainWaterQualityPlate', .32 * width, -.35 * depth, .25)
    else:  # arrival curb delineator marker set
        _part(scene, 'CurbMarkerBasePlate', (width, depth * .70, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'DelineatorRail', (width * .90, .10, .10), (0, 0, .22), MAT_STEEL)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .76 / (3 + variant)
            _part(scene, f'ArrivalDelineatorPost_{i}', (.07, .07, .42 + .02 * variant),
                  (x, 0, .48), material)
            _part(scene, f'ReflectorBand_{i}', (.09, .085, .09), (x, -.005, .55), MAT_LABEL)
        _part(scene, 'CurbWheelStop', (width * .72, .20, .16), (0, -.25 * depth, .08), MAT_STONE)
        _label(scene, 'ArrivalLaneDirectionalArrow', .34 * width, -.37 * depth, .28)
    return scene


def build_asset(name, subcategory, material, asset_id, profile):
    try:
        number = int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'hotel catalog completion asset ID outside A3701–A3900: {asset_id}') from exc
    if not 3701 <= number <= 3900:
        raise ValueError(f'hotel catalog completion asset ID outside A3701–A3900: {asset_id}')

    if number <= 3750:
        kind, variant = divmod(number - 3701, 5)
        return _architecture(kind, variant, material)
    if number <= 3800:
        kind, variant = divmod(number - 3751, 5)
        return _guestroom(kind, variant, material)
    if number <= 3840:
        kind, variant = divmod(number - 3801, 5)
        return _public(kind, variant, material)
    if number <= 3890:
        kind, variant = divmod(number - 3841, 5)
        return _decor(kind, variant, material)
    kind, variant = divmod(number - 3891, 5)
    return _exterior(kind, variant, material)
