"""Purpose-built architecture, guestroom, public, amenity, and decor assets A3501–A3700."""

import trimesh

from hotel_continuation_factory import (
    MAT_BRASS, MAT_CERAMIC, MAT_ELECTRONICS, MAT_GLASS, MAT_LABEL,
    MAT_LINEN, MAT_STAINLESS, MAT_STEEL, MAT_STONE, MAT_UPHOLSTERY,
    MAT_WOOD, _casters, _floor_feet, _label, _part, _post,
)
from v2_asset_common import add, cyl, sphere


VARIANT_SCALE = (.90, .95, 1.00, 1.05, 1.10)


def _case(scene, prefix, width, depth, height, body, top=MAT_STONE):
    _part(scene, f'{prefix}Plinth', (width * .94, depth * .90, .08),
          (0, 0, .04), MAT_STEEL)
    _part(scene, f'{prefix}Case', (width, depth, height),
          (0, 0, .08 + height / 2), body)
    _part(scene, f'{prefix}Top', (width * 1.04, depth * 1.03, .08),
          (0, 0, .12 + height), top)


def _architecture(kind, variant, material):
    scene = trimesh.Scene()
    scale = VARIANT_SCALE[variant]
    width, depth = 1.35 * scale, .92 * scale

    if kind == 0:  # ballroom folding partition rail
        height = 2.38 + .07 * variant
        _part(scene, 'PartitionFloorGuide', (width, .12, .08), (0, 0, .04), MAT_STONE)
        _part(scene, 'PartitionHeadTrack', (width, .16, .12), (0, 0, height + .12), MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .84 / (2 + variant)
            _part(scene, f'FoldingPartitionLeaf_{i}', (width / (3 + variant) * .86, .085, height),
                  (x, 0, height / 2 + .08), material)
            _part(scene, f'PartitionHingeRail_{i}', (.045, .11, height * .96),
                  (x + width / (3 + variant) * .40, -.04, height / 2 + .08), MAT_BRASS)
        _part(scene, 'PartitionPocketCover', (.20, .22, height * .72),
              (.43 * width, .06, height * .38), MAT_WOOD)
        _label(scene, 'PartitionStackDirection', -.41 * width, -.08, .22)
    elif kind == 1:  # atrium window mullion bay
        height = 2.36 + .08 * variant
        _part(scene, 'WindowStoneSill', (width * 1.04, .28, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'VisionGlassPanel', (width * .91, .045, height * .88),
              (0, .02, height * .53), MAT_GLASS)
        for i, x in enumerate((-.44 * width, 0, .44 * width)):
            _part(scene, f'WindowMullion_{i}', (.07, .13, height), (x, 0, height / 2), material)
        _part(scene, 'WindowHeadCap', (width * 1.02, .18, .13), (0, 0, height + .015), MAT_STEEL)
        _part(scene, 'SolarShadeCassette', (width * .90, .18, .12), (0, -.02, height - .20), MAT_WOOD)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .78 / (2 + variant)
            _part(scene, f'ShadeDropStrap_{i}', (.025, .025, .40), (x, -.05, height - .43), MAT_BRASS)
        _label(scene, 'WindowCleaningAnchorPlate', .34 * width, -.11, .25)
    elif kind == 2:  # grand stair landing return
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * .18 * scale
            z = .13 + i * .16
            _part(scene, f'StairTread_{i}', (.25, .48, .12), (x, 0, z), MAT_STONE)
            _part(scene, f'StairRiser_{i}', (.24, .08, .15), (x - .02, -.22, z - .07), material)
        rail_height = .98 + .04 * variant
        for i, x in enumerate((-.45 * width, .42 * width)):
            _post(scene, f'LandingNewel_{i}', x, 0, rail_height, .055, MAT_WOOD)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * width * .78 / (4 + variant)
            _post(scene, f'LandingBaluster_{i}', x, -.12, rail_height * .82, .018, MAT_STEEL)
        _part(scene, 'LandingTopHandrail', (width, .08, .08), (0, -.12, rail_height), MAT_WOOD)
        _part(scene, 'LandingReturnHandrail', (.08, depth * .65, .08),
              (.44 * width, .18, rail_height), MAT_WOOD)
        _label(scene, 'StairLandingFloorMarker', -.39 * width, -.28, .16)
    elif kind == 3:  # skylight lantern curb
        curb_w, curb_d = width * .92, depth * .86
        _part(scene, 'SkylightRoofCurb', (curb_w, curb_d, .34), (0, 0, .17), MAT_STONE)
        _part(scene, 'SkylightGlazing', (curb_w * .82, curb_d * .76, .08), (0, 0, .42), MAT_GLASS)
        for i, (x, y) in enumerate(((-.45, -.40), (-.45, .40), (.45, -.40), (.45, .40))):
            _part(scene, f'LanternFramePost_{i}', (.07, .07, .50 + .025 * variant),
                  (x * curb_w, y * curb_d, .70), material)
        _part(scene, 'LanternGlazingCap', (curb_w * .90, curb_d * .88, .075),
              (0, 0, 1.00 + .025 * variant), MAT_GLASS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * curb_w * .74 / (2 + variant)
            _part(scene, f'SkylightRafter_{i}', (.035, curb_d * .84, .045),
                  (x, 0, 1.04 + .025 * variant), MAT_STEEL)
        _label(scene, 'SkylightServiceTag', .39 * width, -.30 * depth, .28)
    elif kind == 4:  # ballroom acoustic wall relief
        height = 2.40 + .06 * variant
        _part(scene, 'AcousticWallBacking', (width, .14, height), (0, 0, height / 2), MAT_LINEN)
        _part(scene, 'AcousticWallBaseRail', (width * 1.04, .20, .12), (0, 0, .06), MAT_STONE)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * width * .84 / (4 + variant)
            rib_w = .045 + .008 * (i % 2)
            _part(scene, f'TimberAcousticRib_{i}', (rib_w, .21, height * .92),
                  (x, -.055, height * .52), material)
            _part(scene, f'RibClip_{i}', (rib_w * 1.6, .12, .06),
                  (x, -.08, .34 + .12 * (i % 2)), MAT_BRASS)
        _part(scene, 'WallPanelTopReveal', (width, .10, .07), (0, 0, height - .02), MAT_STEEL)
        _label(scene, 'AcousticRatingPlate', .35 * width, -.15, .26)
    elif kind == 5:  # lobby fireplace portal surround
        height = 2.45 + .06 * variant
        _part(scene, 'FireplaceHearth', (width * 1.05, .66, .17), (0, -.08, .085), MAT_STONE)
        _part(scene, 'FireplaceBackPanel', (width * .82, .15, height), (0, .12, height / 2), material)
        for i, x in enumerate((-.39 * width, .39 * width)):
            _part(scene, f'FireplacePilaster_{i}', (.16, .24, height), (x, -.02, height / 2), MAT_WOOD)
            _part(scene, f'PilasterCapital_{i}', (.24, .30, .18), (x, -.02, height - .08), MAT_BRASS)
        _part(scene, 'FireplaceMantel', (width * 1.08, .36, .16), (0, -.02, height + .08), MAT_WOOD)
        _part(scene, 'FireboxRecess', (width * .52, .08, .82), (0, -.02, .59), MAT_STEEL)
        for i in range(3 + variant):
            _part(scene, f'FireGlowLog_{i}', (.22, .09, .09),
                  ((i - (2 + variant) / 2) * .16, -.08, .30 + .025 * (i % 2)), MAT_BRASS)
        _label(scene, 'FireplaceFuelSpecification', .32 * width, -.24, .22)
    elif kind == 6:  # elevator lobby door surround
        height = 2.48 + .06 * variant
        _part(scene, 'ElevatorThresholdPlate', (width * .84, .34, .11), (0, -.02, .055), MAT_STAINLESS)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'ElevatorStoneJamb_{i}', (.18, .22, height), (x, 0, height / 2), material)
            _part(scene, f'ElevatorShadowReveal_{i}', (.045, .24, height * .94),
                  (x - .09, -.035, height / 2), MAT_STEEL)
        _part(scene, 'ElevatorHeader', (width, .22, .22), (0, 0, height - .11), material)
        _part(scene, 'ElevatorDoorPanel', (width * .55, .07, height * .82),
              (0, .12, height * .43), MAT_STAINLESS)
        _part(scene, 'ElevatorCallStation', (.12, .08, .42), (.36 * width, -.15, 1.10), MAT_ELECTRONICS)
        _part(scene, 'ElevatorFloorIndicator', (.22, .07, .12), (0, -.15, height - .30), MAT_LABEL)
        _label(scene, 'ElevatorInspectionID', -.34 * width, -.16, .23)
    elif kind == 7:  # porte-cochere canopy bay
        height = 2.58 + .06 * variant
        for i, x in enumerate((-.42 * width, .42 * width)):
            _part(scene, f'CanopyStoneFoot_{i}', (.22, .24, .14), (x, 0, .07), MAT_STONE)
            _part(scene, f'CanopyColumn_{i}', (.12, .14, height), (x, 0, height / 2), material)
            _part(scene, f'CanopyColumnBaseBand_{i}', (.19, .20, .13), (x, 0, .20), MAT_BRASS)
        _part(scene, 'CanopyFrontBeam', (width, .20, .17), (0, -.38 * depth, height + .06), MAT_STEEL)
        _part(scene, 'CanopyRearBeam', (width, .18, .15), (0, .38 * depth, height + .06), MAT_WOOD)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .82 / (3 + variant)
            _part(scene, f'CanopyRafter_{i}', (.055, depth * .82, .08), (x, 0, height + .17), MAT_STAINLESS)
        _part(scene, 'CanopyRainGutter', (width * .92, .10, .10), (0, -.43 * depth, height + .14), MAT_STAINLESS)
        _label(scene, 'VehicleClearanceMarker', -.30 * width, -.19, 2.20)
    elif kind == 8:  # service elevator landing door
        height = 2.58 + .05 * variant
        _part(scene, 'ServiceLiftSill', (width * .84, .36, .13), (0, 0, .065), MAT_STEEL)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'ServiceLiftJamb_{i}', (.15, .18, height), (x, 0, height / 2), MAT_STAINLESS)
        _part(scene, 'LiftHeadTrack', (width * .95, .20, .18), (0, 0, height - .09), material)
        _part(scene, 'LiftDoorLeft', (width * .36, .07, height * .83), (-.18 * width, .06, height * .43), MAT_STEEL)
        _part(scene, 'LiftDoorRight', (width * .36, .07, height * .83), (.18 * width, .06, height * .43), MAT_STEEL)
        _part(scene, 'LiftStatusBeacon', (.14, .13, .25), (.39 * width, -.10, 2.24), MAT_ELECTRONICS)
        _part(scene, 'LiftServiceNotice', (.28, .045, .20), (-.31 * width, -.11, 1.70), MAT_LABEL)
        _label(scene, 'LiftRatedLoadPlaque', .30 * width, -.13, .34)
    else:  # facade brise-soleil fin bay
        height = 2.48 + .07 * variant
        _part(scene, 'SunshadeWallBacking', (width, .12, height), (0, .06, height / 2), MAT_STONE)
        _part(scene, 'SunshadeSillTrack', (width * 1.06, .22, .12), (0, 0, .06), MAT_STEEL)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * width * .84 / (4 + variant)
            _part(scene, f'BriseSoleilFin_{i}', (.075, .25, height * .96),
                  (x, -.08, height / 2), material)
            _part(scene, f'FinPivotShoe_{i}', (.13, .18, .08), (x, -.08, .32), MAT_BRASS)
        _part(scene, 'SunshadeHeadCap', (width * 1.05, .18, .12), (0, 0, height + .06), MAT_STAINLESS)
        _label(scene, 'FacadeOrientationTag', .35 * width, -.16, .28)
    return scene


def _guestroom(kind, variant, material):
    scene = trimesh.Scene()
    scale = VARIANT_SCALE[variant]
    width, depth = 1.12 * scale, .72 * scale

    if kind == 0:  # window bench luggage nook
        seat = .49 + .025 * variant
        _part(scene, 'WindowBenchStorageBox', (width * .92, depth * .74, .37),
              (0, .04, .28), material)
        _part(scene, 'BenchFrontReveal', (width * .78, .04, .11), (0, -.24 * depth, .22), MAT_STEEL)
        _part(scene, 'WindowBenchCushion', (width, depth, .12), (0, 0, seat), MAT_UPHOLSTERY)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .80 / (2 + variant)
            _part(scene, f'BenchDrawerFront_{i}', (width * .80 / (3 + variant), .045, .19),
                  (x, -.40 * depth, .27), MAT_WOOD)
            _part(scene, f'BenchDrawerPull_{i}', (.09, .035, .035), (x, -.43 * depth, .28), MAT_BRASS)
        _floor_feet(scene, width * .90, depth * .80, .16, 'BenchFoot', MAT_STEEL)
        _label(scene, 'BenchSafeLoadLabel', .37 * width, -.40 * depth, .18)
    elif kind == 1:  # guestroom minibar credenza
        case_h = .84 + .025 * variant
        _case(scene, 'Minibar', width, depth, case_h, material, MAT_STONE)
        _part(scene, 'ChillerDoor', (width * .38, .045, .54), (-.28 * width, -.56 * depth, .43), MAT_STAINLESS)
        _part(scene, 'ChillerWindow', (width * .25, .025, .34), (-.28 * width, -.58 * depth, .47), MAT_GLASS)
        _part(scene, 'MinibarHandle', (.035, .05, .28), (-.06 * width, -.61 * depth, .44), MAT_BRASS)
        for i in range(3 + variant):
            x = (.08 + i * .07) * width
            _part(scene, f'MinibarBottle_{i}', (.07, .07, .24 + .015 * (i % 2)),
                  (x, -.10 * depth, .35), MAT_CERAMIC)
        _part(scene, 'MinibarGlassShelf', (width * .50, .08, .055), (.20 * width, .10, .50), MAT_GLASS)
        _label(scene, 'MinibarInventoryCard', .35 * width, -.42 * depth, .22)
    elif kind == 2:  # suite drop-leaf dining table
        table_h = .74 + .025 * variant
        _part(scene, 'DiningTableTop', (width, depth * .72, .09), (0, 0, table_h), material)
        _floor_feet(scene, width * .88, depth * .64, table_h - .08, 'DiningTableLeg', MAT_STEEL)
        _part(scene, 'DropLeafLeft', (width * .27, depth * .72, .065),
              (-.63 * width, 0, table_h - .02), MAT_WOOD)
        _part(scene, 'DropLeafRight', (width * .27, depth * .72, .065),
              (.63 * width, 0, table_h - .02), MAT_WOOD)
        for side, x in enumerate((-.53 * width, .53 * width)):
            _part(scene, f'LeafHinge_{side}', (.08, .05, .055), (x, -.04, table_h - .08), MAT_BRASS)
        _part(scene, 'DiningChairSeat', (.42 * width, .38 * depth, .09),
              (0, -.72 * depth, .46), MAT_UPHOLSTERY)
        _part(scene, 'DiningChairBack', (.42 * width, .06, .46),
              (0, -.84 * depth, .70), MAT_WOOD)
        _floor_feet(scene, width * .36, depth * .34, .43, 'DiningChairLeg', MAT_STEEL)
        _label(scene, 'TableLeafLockLabel', .39 * width, -.35 * depth, .20)
    elif kind == 3:  # in-room wardrobe shoe and accessory tower
        height = 1.74 + .05 * variant
        _part(scene, 'WardrobeTowerPlinth', (width * .82, depth * .82, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'WardrobeTowerCase', (width * .74, depth * .74, height),
              (0, 0, .12 + height / 2), material)
        _part(scene, 'WardrobeUpperDoor', (width * .69, .045, height * .42),
              (0, -.39 * depth, 1.43), MAT_WOOD)
        _part(scene, 'WardrobeShoeDrawer', (width * .66, .05, .27),
              (0, -.39 * depth, .38), MAT_LINEN)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .48 / (2 + variant)
            _part(scene, f'ShoeShelf_{i}', (width * .58, depth * .60, .045),
                  (0, .02, .55 + i * .20), MAT_STONE)
        _part(scene, 'AccessoryTray', (width * .60, depth * .35, .10),
              (0, -.02, 1.80 + .025 * variant), MAT_WOOD)
        _label(scene, 'WardrobeWeightNotice', .30 * width, -.42 * depth, .24)
    elif kind == 4:  # foot-of-bed blanket trunk
        height = .52 + .025 * variant
        _case(scene, 'BlanketTrunk', width, depth * .76, height, material, MAT_UPHOLSTERY)
        _part(scene, 'TrunkLidSeam', (width * .84, .035, .025),
              (0, -.40 * depth, height + .11), MAT_BRASS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .72 / (2 + variant)
            _part(scene, f'TrunkPanelStitch_{i}', (.035, .025, .22),
                  (x, -.39 * depth, .34), MAT_LINEN)
        _floor_feet(scene, width * .90, depth * .70, .17, 'TrunkFoot', MAT_STEEL)
        _part(scene, 'TrunkSideHandleLeft', (.10, .08, .06), (-.48 * width, 0, .33), MAT_BRASS)
        _part(scene, 'TrunkSideHandleRight', (.10, .08, .06), (.48 * width, 0, .33), MAT_BRASS)
        _label(scene, 'TrunkLinenInventoryTag', .36 * width, -.40 * depth, .20)
    elif kind == 5:  # luggage weight-scale bench
        _part(scene, 'LuggageScalePlatform', (width * .90, depth * .78, .13), (0, 0, .075), MAT_STONE)
        _part(scene, 'ScaleLoadSurface', (width * .84, depth * .72, .045), (0, 0, .16), MAT_STEEL)
        for i, x in enumerate((-.31 * width, .31 * width)):
            _part(scene, f'ScaleBenchSupport_{i}', (.12, depth * .64, .56), (x, 0, .45), MAT_WOOD)
        _part(scene, 'LuggageRestRail', (width * .90, .07, .08), (0, .30 * depth, .76), MAT_STAINLESS)
        _part(scene, 'WeightDisplay', (.25, .08, .16), (0, -.38 * depth, .48), MAT_ELECTRONICS)
        for i in range(2 + variant):
            x = (i - (1 + variant) / 2) * width * .56 / (1 + variant)
            _part(scene, f'ScaleLoadCell_{i}', (.055, .055, .12), (x, 0, .20), MAT_BRASS)
        _label(scene, 'ScaleUnitInstructions', .35 * width, -.41 * depth, .28)
    elif kind == 6:  # compact rollaway bed frame
        length = 1.74 + .035 * variant
        _part(scene, 'RollawayMattress', (width * .88, length, .16), (0, 0, .42), MAT_LINEN)
        _part(scene, 'RollawayFrameRailLeft', (.065, length * .94, .13), (-.46 * width, 0, .30), MAT_STEEL)
        _part(scene, 'RollawayFrameRailRight', (.065, length * .94, .13), (.46 * width, 0, .30), MAT_STEEL)
        for i, y in enumerate((-.38 * length, .38 * length)):
            _part(scene, f'RollawayEndBar_{i}', (width * .86, .06, .12), (0, y, .30), MAT_STEEL)
        _casters(scene, width, length, 'RollawayCaster')
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .74 / (2 + variant)
            _part(scene, f'MattressQuiltLine_{i}', (.018, length * .70, .012), (x, 0, .505), MAT_UPHOLSTERY)
        _label(scene, 'RollawayStorageCode', .35 * width, -.40 * length, .22)
    elif kind == 7:  # closet valet organizer
        height = 1.58 + .045 * variant
        _part(scene, 'ValetOrganizerBase', (width * .54, depth * .56, .11), (0, 0, .055), MAT_STONE)
        for i, x in enumerate((-.22 * width, .22 * width)):
            _part(scene, f'ValetUpright_{i}', (.06, .07, height), (x, 0, height / 2), MAT_WOOD)
        _part(scene, 'SuitHangerCrossbar', (width * .55, .065, .065), (0, 0, height - .08), MAT_BRASS)
        _part(scene, 'ValetTray', (width * .66, depth * .50, .09), (0, -.01, .40), material)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .52 / (2 + variant)
            _part(scene, f'ValetAccessoryHook_{i}', (.045, .05, .15), (x, -.06, 1.18), MAT_STEEL)
        _part(scene, 'ShoeShelf', (width * .60, depth * .52, .07), (0, .02, .20), MAT_WOOD)
        _label(scene, 'ValetCareLabel', .36 * width, -.35 * depth, .26)
    elif kind == 8:  # guestroom bedside device charging bench
        seat_h = .57 + .025 * variant
        _part(scene, 'ChargingBenchSeat', (width, depth * .70, .12), (0, 0, seat_h), MAT_UPHOLSTERY)
        _floor_feet(scene, width * .88, depth * .64, seat_h - .10, 'ChargingBenchLeg', MAT_WOOD)
        _part(scene, 'DeviceShelf', (width * .72, .18, .08), (0, .28 * depth, .93), material)
        _part(scene, 'USBPanel', (.24, .045, .12), (0, -.38 * depth, .86), MAT_ELECTRONICS)
        for i in range(2 + variant):
            x = (i - (1 + variant) / 2) * width * .56 / (1 + variant)
            _part(scene, f'ChargingPort_{i}', (.045, .025, .035), (x, -.41 * depth, .86), MAT_BRASS)
        _part(scene, 'CableConcealmentChannel', (width * .62, .06, .08), (0, .18 * depth, .78), MAT_STEEL)
        _label(scene, 'ChargingSafetyLabel', .36 * width, -.42 * depth, .30)
    else:  # in-room tea and coffee console
        height = .88 + .025 * variant
        _case(scene, 'TeaConsole', width, depth * .78, height, material, MAT_STONE)
        _part(scene, 'ConsoleCupRail', (width * .72, .06, .10), (0, -.32 * depth, height + .18), MAT_BRASS)
        _part(scene, 'CoffeeMachine', (.32, .28, .37), (-.25 * width, 0, height + .31), MAT_ELECTRONICS)
        _part(scene, 'TeaCanisterTray', (width * .34, depth * .34, .06), (.27 * width, 0, height + .12), MAT_WOOD)
        for i in range(2 + variant):
            x = (.17 + i * .09) * width
            _part(scene, f'TeaCanister_{i}', (.085, .085, .18),
                  (x, -.08 * depth, height + .24), MAT_CERAMIC)
        _part(scene, 'ConsoleWasteDrawer', (width * .46, .05, .18), (0, -.40 * depth, .32), MAT_LINEN)
        _label(scene, 'TeaConsoleAllergenCard', .36 * width, -.42 * depth, .20)
    return scene


def _public(kind, variant, material):
    scene = trimesh.Scene()
    scale = VARIANT_SCALE[variant]
    width, depth = 1.16 * scale, .74 * scale

    if kind == 0:  # lobby courier locker bank
        height = 1.78 + .04 * variant
        _part(scene, 'CourierLockerPlinth', (width, depth * .70, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'CourierLockerBack', (width * .95, .11, height), (0, .24 * depth, height / 2 + .12), MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .78 / (2 + variant)
            _part(scene, f'ParcelLockerDoor_{i}', (width * .78 / (3 + variant) * .90, .07, height * .80),
                  (x, -.18 * depth, height * .53), material)
            _part(scene, f'LockerDoorPull_{i}', (.035, .045, .16), (x + .10, -.23 * depth, .63), MAT_BRASS)
            _part(scene, f'LockerStatusLight_{i}', (.045, .04, .045), (x - .08, -.23 * depth, 1.48), MAT_ELECTRONICS)
        _part(scene, 'CourierTouchPanel', (.20, .07, .34), (.40 * width, -.22 * depth, 1.18), MAT_ELECTRONICS)
        _floor_feet(scene, width * .90, depth * .52, .12, 'LockerBankFoot', MAT_STEEL)
        _label(scene, 'CourierHelpLine', -.34 * width, -.23 * depth, .30)
    elif kind == 1:  # concierge queue podium
        height = 1.02 + .03 * variant
        _case(scene, 'ConciergeQueuePodium', width * .70, depth * .66, height, material, MAT_STONE)
        _part(scene, 'QueuePodiumGuestShelf', (width * .92, depth * .42, .10),
              (0, -.10, height + .15), MAT_WOOD)
        _part(scene, 'QueuePodiumScreen', (.34, .06, .30), (0, .16, height + .35), MAT_ELECTRONICS)
        _post(scene, 'QueueRailPostLeft', -.45 * width, 0, .93, .035, MAT_STAINLESS)
        _post(scene, 'QueueRailPostRight', .45 * width, 0, .93, .035, MAT_STAINLESS)
        _part(scene, 'QueueGuideCrossbar', (width * .92, .05, .06), (0, 0, .93), MAT_BRASS)
        _label(scene, 'PodiumServiceBellPlate', .29 * width, -.39 * depth, .40)
    elif kind == 2:  # ballroom coat check desk and ticket rail
        height = .96 + .025 * variant
        _case(scene, 'CoatCheckCounter', width, depth * .60, height, material, MAT_WOOD)
        _part(scene, 'TicketRail', (width * .90, .055, .08), (0, .28 * depth, height + .11), MAT_BRASS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .78 / (3 + variant)
            _part(scene, f'NumberedTicketHook_{i}', (.045, .07, .22), (x, .20 * depth, height + .26), MAT_STEEL)
            _part(scene, f'CoatToken_{i}', (.075, .035, .075), (x, -.36 * depth, height + .13), MAT_LABEL)
        _part(scene, 'CheckInTerminal', (.26, .22, .28), (.34 * width, -.02, height + .24), MAT_ELECTRONICS)
        _floor_feet(scene, width * .88, depth * .54, .14, 'CoatDeskFoot', MAT_STEEL)
        _label(scene, 'CoatCheckNumberRange', -.36 * width, -.34 * depth, .24)
    elif kind == 3:  # umbrella drying station
        _part(scene, 'UmbrellaStationBase', (width * .78, depth * .72, .16), (0, 0, .08), MAT_STONE)
        _part(scene, 'UmbrellaDripTray', (width * .68, depth * .60, .12), (0, 0, .22), MAT_STAINLESS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .56 / (3 + variant)
            _post(scene, f'UmbrellaSleeve_{i}', x, 0, .92 + .025 * variant, .045, material)
            _part(scene, f'SleeveDrainCap_{i}', (.13, .13, .06), (x, 0, .96 + .025 * variant), MAT_STEEL)
        _part(scene, 'UmbrellaSignPanel', (.40, .07, .28), (0, .25 * depth, 1.20), MAT_LABEL)
        _part(scene, 'DryingAirVent', (width * .72, .10, .11), (0, -.15 * depth, .35), MAT_ELECTRONICS)
        _label(scene, 'DripTrayCleanoutMark', .34 * width, -.31 * depth, .25)
    elif kind == 4:  # public restroom family care station
        height = .92 + .02 * variant
        _case(scene, 'FamilyCareBaseCabinet', width * .90, depth * .64, height, material, MAT_STONE)
        _part(scene, 'ChangingSurface', (width, depth * .92, .12), (0, 0, height + .10), MAT_UPHOLSTERY)
        _part(scene, 'CareStationPrivacyScreen', (width * .86, .06, .48),
              (0, .36 * depth, height + .37), MAT_LINEN)
        _part(scene, 'SafetyBeltAnchor', (.28, .06, .055), (0, -.38 * depth, height + .16), MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .68 / (2 + variant)
            _part(scene, f'DisposableLinerSlot_{i}', (.18, .045, .11), (x, -.39 * depth, .37), MAT_STAINLESS)
        _floor_feet(scene, width * .86, depth * .60, .12, 'FamilyCareFoot', MAT_STEEL)
        _label(scene, 'FamilyCareMaxLoad', .34 * width, -.40 * depth, .24)
    elif kind == 5:  # business center print counter
        height = .82 + .02 * variant
        _case(scene, 'PrintCounter', width, depth * .72, height, material, MAT_WOOD)
        _part(scene, 'MultifunctionPrinterBase', (.48, .40, .34), (-.22 * width, .05, height + .25), MAT_STEEL)
        _part(scene, 'PrinterScannerLid', (.48, .40, .07), (-.22 * width, .05, height + .46), MAT_STAINLESS)
        _part(scene, 'PaperOutputTray', (.32, .23, .035), (-.22 * width, -.10, height + .38), MAT_LINEN)
        _part(scene, 'PrintJobScreen', (.20, .06, .19), (.34 * width, -.38 * depth, height + .18), MAT_ELECTRONICS)
        for i in range(3 + variant):
            _part(scene, f'PaperReam_{i}', (.22, .29, .055),
                  ((i - (2 + variant) / 2) * .24, .26 * depth, height + .11), MAT_LABEL)
        _label(scene, 'PrinterPrivacyNotice', .36 * width, -.41 * depth, .26)
    elif kind == 6:  # luggage wrapping console
        height = .94 + .02 * variant
        _part(scene, 'WrappingTableTop', (width, depth * .86, .11), (0, 0, height), material)
        _floor_feet(scene, width * .90, depth * .80, height - .09, 'WrappingTableLeg', MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .74 / (2 + variant)
            _part(scene, f'RollerTrack_{i}', (.08, depth * .70, .065), (x, 0, height + .09), MAT_STAINLESS)
        _post(scene, 'FilmRollAxle', -.42 * width, .18, 1.48, .035, MAT_STEEL)
        _part(scene, 'LuggageFilmRoll', (.13, .40, .13), (-.42 * width, .18, 1.13), MAT_LINEN)
        _part(scene, 'WrappingInstructionPanel', (.28, .06, .30), (.37 * width, .25 * depth, 1.25), MAT_LABEL)
        _label(scene, 'WrappingRateCard', .36 * width, -.40 * depth, .30)
    elif kind == 7:  # lobby device charging bar
        height = .78 + .02 * variant
        _part(scene, 'ChargingBarTop', (width, depth * .55, .11), (0, 0, height), material)
        _floor_feet(scene, width * .90, depth * .48, height - .08, 'ChargingBarLeg', MAT_STEEL)
        _part(scene, 'GuestLeanRail', (width * .90, .08, .09), (0, .25 * depth, height - .22), MAT_WOOD)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .80 / (3 + variant)
            _part(scene, f'ChargingBay_{i}', (.14, .13, .06), (x, -.08, height + .085), MAT_ELECTRONICS)
            _part(scene, f'USBPortPair_{i}', (.075, .025, .045), (x, -.30 * depth, height + .09), MAT_BRASS)
        _part(scene, 'ChargingStatusDisplay', (.26, .045, .12), (0, .12, height + .18), MAT_LABEL)
        _label(scene, 'ChargingBarSafetyCode', .37 * width, -.31 * depth, .20)
    elif kind == 8:  # event directory check-in stand
        height = 1.56 + .04 * variant
        _part(scene, 'DirectoryStandBase', (width * .68, depth * .56, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'DirectoryStandMast', (.16, .18, height), (0, 0, height / 2 + .08), MAT_STEEL)
        _part(scene, 'EventDirectoryScreen', (width * .72, .08, .68), (0, -.08, height * .73), MAT_ELECTRONICS)
        _part(scene, 'CheckInTray', (width * .88, depth * .52, .10), (0, -.03, .87), material)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .60 / (2 + variant)
            _part(scene, f'EventBrochureSlot_{i}', (.14, .08, .24), (x, .20 * depth, .61), MAT_LINEN)
        _part(scene, 'DirectoryAccessibilityButton', (.11, .06, .12), (.35 * width, -.14, 1.14), MAT_BRASS)
        _label(scene, 'EventHelpDeskArrow', -.35 * width, -.14, .38)
    else:  # shuttle arrival luggage rack
        height = .84 + .02 * variant
        _part(scene, 'ShuttleLuggageRackBase', (width, depth * .78, .12), (0, 0, .06), MAT_STONE)
        for i, x in enumerate((-.40 * width, .40 * width)):
            _part(scene, f'RackEndFrame_{i}', (.08, depth * .72, height), (x, 0, height / 2), MAT_STEEL)
        for i in range(4 + variant):
            y = (i - (3 + variant) / 2) * depth * .68 / (3 + variant)
            _part(scene, f'LuggageRoller_{i}', (width * .76, .07, .06), (0, y, .22), MAT_STAINLESS)
        _part(scene, 'RackUpperHandrail', (width * .88, .07, .07), (0, 0, height), MAT_WOOD)
        _part(scene, 'ShuttleRoutePlaque', (.30, .06, .18), (0, -.42 * depth, height - .16), MAT_LABEL)
        _label(scene, 'RackBagWeightLimit', .34 * width, -.42 * depth, .25)
    return scene


def _amenity(kind, variant, material):
    scene = trimesh.Scene()
    scale = VARIANT_SCALE[variant]
    width, depth = .94 * scale, .68 * scale

    if kind == 0:  # pool towel return carousel
        _part(scene, 'TowelCarouselBase', (width * .76, depth * .76, .14), (0, 0, .07), MAT_STONE)
        _post(scene, 'CarouselCenterMast', 0, 0, 1.42 + .04 * variant, .055, MAT_STEEL)
        _part(scene, 'CarouselCanopy', (width * .92, depth * .90, .10),
              (0, 0, 1.45 + .04 * variant), material)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .70 / (3 + variant)
            _part(scene, f'TowelReturnBin_{i}', (width * .72 / (4 + variant), depth * .56, .36),
                  (x, 0, .35), MAT_LINEN)
            _part(scene, f'BinLabel_{i}', (.12, .035, .07), (x, -.30 * depth, .38), MAT_LABEL)
        _part(scene, 'CarouselRotationHub', (.22, .22, .18), (0, 0, 1.22), MAT_BRASS)
        _label(scene, 'PoolTowelReturnInstructions', .36 * width, -.34 * depth, .27)
    elif kind == 1:  # spa herbal tea trolley
        height = 1.10 + .025 * variant
        _part(scene, 'TeaTrolleyLowerShelf', (width, depth * .70, .09), (0, 0, .25), material)
        _part(scene, 'TeaTrolleyUpperShelf', (width * .94, depth * .68, .08), (0, 0, height), MAT_WOOD)
        for i, x in enumerate((-.40 * width, .40 * width)):
            _part(scene, f'TrolleyUpright_{i}', (.07, .07, height - .12), (x, 0, height / 2 + .12), MAT_STEEL)
            _part(scene, f'TrolleyPushHandle_{i}', (.07, .32, .07), (x, .12, height + .08), MAT_BRASS)
        _casters(scene, width, depth, 'TeaTrolleyCaster')
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .68 / (2 + variant)
            _part(scene, f'TeaCanister_{i}', (.12, .12, .24), (x, 0, height + .20), MAT_CERAMIC)
        _part(scene, 'InfusionKettle', (.20, .24, .29), (-.28 * width, .02, height + .24), MAT_STAINLESS)
        _label(scene, 'TrolleyHerbalInfusionCard', .34 * width, -.34 * depth, .23)
    elif kind == 2:  # fitness hydration island
        height = 1.20 + .03 * variant
        _case(scene, 'HydrationIsland', width, depth * .72, .72, material, MAT_STONE)
        _part(scene, 'BottleFillerNozzle', (.08, .13, .18), (0, -.38 * depth, .96), MAT_STAINLESS)
        _part(scene, 'BottleSensorPanel', (.18, .045, .20), (.28 * width, -.39 * depth, 1.04), MAT_ELECTRONICS)
        _part(scene, 'BottleFillerRecess', (.42, .05, .29), (0, -.39 * depth, .69), MAT_STEEL)
        _part(scene, 'HydrationDisplay', (width * .82, .07, .12), (0, .12, height + .12), MAT_LABEL)
        _part(scene, 'CupDispenser', (.16, .16, .33), (.37 * width, .08, 1.08), MAT_CERAMIC)
        _floor_feet(scene, width * .88, depth * .66, .12, 'HydrationFoot', MAT_STEEL)
        _label(scene, 'HydrationFilterChangeDate', .35 * width, -.40 * depth, .28)
    elif kind == 3:  # children reading nook with book cubbies
        _part(scene, 'ReadingNookBase', (width, depth, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'NookBenchCushion', (width * .88, depth * .72, .25), (0, 0, .34), MAT_UPHOLSTERY)
        _part(scene, 'NookBackCushion', (width * .88, .13, .62), (0, .38 * depth, .76), MAT_LINEN)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .72 / (2 + variant)
            _part(scene, f'BookCubbieDivider_{i}', (.05, depth * .60, .52), (x, -.05, .68), material)
            _part(scene, f'PictureBook_{i}', (.16, .07, .30), (x + .05, -.15, .61), MAT_LABEL)
        _part(scene, 'NookCanopyArch', (width * .92, .08, .10), (0, .15, 1.52 + .03 * variant), MAT_WOOD)
        _post(scene, 'NookCanopyPostLeft', -.42 * width, .15, 1.48 + .03 * variant, .035, MAT_BRASS)
        _post(scene, 'NookCanopyPostRight', .42 * width, .15, 1.48 + .03 * variant, .035, MAT_BRASS)
        _label(scene, 'ReadingNookAgeGuide', .35 * width, -.43 * depth, .22)
    elif kind == 4:  # rooftop viewing binocular stand
        _part(scene, 'ViewingStandBase', (width * .72, depth * .72, .16), (0, 0, .08), MAT_STONE)
        _post(scene, 'BinocularStandMast', 0, 0, 1.12 + .04 * variant, .055, MAT_STEEL)
        _part(scene, 'BinocularTiltYoke', (.36, .20, .12), (0, -.02, 1.16 + .04 * variant), MAT_BRASS)
        for i, x in enumerate((-.12, .12)):
            add(scene, cyl(.07 + .005 * variant, .22, (x, -.09, 1.30 + .04 * variant), MAT_STAINLESS, 16),
                f'ViewingTube_{i}')
            _part(scene, f'ObjectiveLens_{i}', (.12, .04, .12),
                  (x, -.22, 1.30 + .04 * variant), MAT_GLASS)
        _part(scene, 'ViewingCoinBox', (.22, .16, .16), (0, .12, .95), MAT_ELECTRONICS)
        _part(scene, 'ViewingDirectionPlate', (.48, .07, .16), (0, .16, 1.52 + .04 * variant), material)
        _label(scene, 'ViewingUseInstructions', .33 * width, -.34 * depth, .30)
    elif kind == 5:  # event presenter AV lectern
        height = 1.08 + .025 * variant
        _part(scene, 'LecternBasePlinth', (width * .68, depth * .72, .15), (0, 0, .075), MAT_STONE)
        _part(scene, 'LecternPedestal', (width * .54, depth * .44, height), (0, 0, .15 + height / 2), material)
        _part(scene, 'LecternReadingTop', (width * .90, depth * .70, .11), (0, -.02, height + .20), MAT_WOOD)
        _part(scene, 'LecternBookStop', (width * .76, .08, .12), (0, -.32 * depth, height + .29), MAT_BRASS)
        _part(scene, 'LecternMonitor', (.28, .08, .22), (0, .18, height + .36), MAT_ELECTRONICS)
        _post(scene, 'LecternGooseneckMic', -.28 * width, .08, height + .52, .018, MAT_STEEL)
        _part(scene, 'AVControlPad', (.22, .035, .10), (.30 * width, -.18, height + .27), MAT_ELECTRONICS)
        _label(scene, 'LecternSignalChannelLabel', .33 * width, -.30 * depth, .26)
    elif kind == 6:  # board game lounge table set
        table_h = .72 + .02 * variant
        _part(scene, 'GameTableTop', (width, depth, .10), (0, 0, table_h), material)
        _floor_feet(scene, width * .88, depth * .84, table_h - .09, 'GameTableLeg', MAT_WOOD)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .64 / (2 + variant)
            _part(scene, f'GameBoardInset_{i}', (width * .18, depth * .48, .025), (x, 0, table_h + .055), MAT_STONE)
        _part(scene, 'GamePieceCaddy', (width * .82, .12, .14), (0, .38 * depth, table_h + .12), MAT_STEEL)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .68 / (3 + variant)
            _part(scene, f'GamePieceSlot_{i}', (.09, .08, .06), (x, .38 * depth, table_h + .22), MAT_BRASS)
        _label(scene, 'GameReturnInstructions', .37 * width, -.40 * depth, .24)
    elif kind == 7:  # sauna aromatherapy stone basin
        _part(scene, 'AromaBasinFoot', (width * .72, depth * .72, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'AromaStoneBasin', (width * .65, depth * .65, .46), (0, 0, .35), material)
        _part(scene, 'AromaBasinRim', (width * .76, depth * .76, .10), (0, 0, .63), MAT_BRASS)
        _part(scene, 'AromaWaterSurface', (width * .54, depth * .54, .035), (0, 0, .69), MAT_GLASS)
        for i in range(4 + variant):
            angle_x = (i - (3 + variant) / 2) * width * .14 / (3 + variant)
            add(scene, sphere(.10 + .006 * variant, (angle_x, 0, .77), MAT_STONE, subdivisions=1),
                f'HeatedAromaStone_{i}')
        _part(scene, 'AromaOilBottleTray', (.26, .20, .07), (.36 * width, .12, .79), MAT_WOOD)
        _label(scene, 'AromaOilSafetyCard', .34 * width, -.38 * depth, .30)
    else:  # pet welcome care station
        height = .76 + .02 * variant
        _case(scene, 'PetCareCabinet', width, depth * .70, height, material, MAT_STONE)
        _part(scene, 'PetWashBowl', (.40, .30, .14), (-.24 * width, 0, height + .10), MAT_STAINLESS)
        _part(scene, 'PetRinseTap', (.08, .07, .30), (-.24 * width, .16, height + .29), MAT_STEEL)
        _part(scene, 'LeashHookBoard', (.40, .06, .30), (.30 * width, .16, height + .20), MAT_WOOD)
        for i in range(3 + variant):
            x = (.18 + i * .08) * width
            _part(scene, f'LeashHook_{i}', (.055, .08, .12), (x, .11, height + .22), MAT_BRASS)
        _part(scene, 'PetTowelDrawer', (width * .55, .05, .18), (0, -.39 * depth, .35), MAT_LINEN)
        _floor_feet(scene, width * .90, depth * .62, .12, 'PetStationFoot', MAT_STEEL)
        _label(scene, 'PetWaterUseNotice', .34 * width, -.40 * depth, .22)
    return scene


def _decor(variant, material):
    scene = trimesh.Scene()
    scale = VARIANT_SCALE[variant]
    width, height = 1.10 * scale, .72 * scale
    _part(scene, 'HeritageTextileBacking', (width, .10, height), (0, .03, 1.76), MAT_WOOD)
    _part(scene, 'WovenTextileField', (width * .84, .045, height * .82), (0, -.04, 1.76), material)
    _part(scene, 'HangingTopRail', (width * 1.08, .11, .10), (0, .03, 2.18), MAT_BRASS)
    _part(scene, 'HangingBottomWeight', (width * .90, .08, .07), (0, .02, 1.36), MAT_STEEL)
    for i in range(4 + variant):
        x = (i - (3 + variant) / 2) * width * .70 / (3 + variant)
        _part(scene, f'TextileWovenStripe_{i}', (.045, .03, height * .68),
              (x, -.07, 1.76), MAT_LINEN if i % 2 else MAT_BRASS)
    for i, x in enumerate((-.40 * width, .40 * width)):
        _part(scene, f'TapestryMountingLoop_{i}', (.07, .05, .11), (x, .02, 2.25), MAT_STEEL)
    _label(scene, 'TextileArtistProvenance', .34 * width, -.10, 1.24)
    return scene


def build_asset(name, subcategory, material, asset_id, profile):
    try:
        number = int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'hotel finalization asset ID outside A3501–A3700: {asset_id}') from exc
    if not 3501 <= number <= 3700:
        raise ValueError(f'hotel finalization asset ID outside A3501–A3700: {asset_id}')

    if number <= 3550:
        group, variant = divmod(number - 3501, 5)
        return _architecture(group, variant, material)
    if number <= 3600:
        group, variant = divmod(number - 3551, 5)
        return _guestroom(group, variant, material)
    if number <= 3650:
        group, variant = divmod(number - 3601, 5)
        return _public(group, variant, material)
    if number <= 3695:
        group, variant = divmod(number - 3651, 5)
        return _amenity(group, variant, material)
    return _decor(number - 3696, material)
