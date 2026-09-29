"""Purpose-built final-count catalog assets A3901–A4200."""

import math

import trimesh

from hotel_continuation_factory import (
    MAT_BRASS, MAT_CERAMIC, MAT_ELECTRONICS, MAT_GLASS, MAT_LABEL,
    MAT_LINEN, MAT_STAINLESS, MAT_STEEL, MAT_STONE, MAT_UPHOLSTERY,
    MAT_WOOD, _casters, _floor_feet, _label, _part, _post,
)
from v2_asset_common import add, cyl, sphere


SCALE = (.90, .95, 1.00, 1.05, 1.10)
MAT_VEGETATION = 'MAT_VEGETATION'


def _architecture(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = 1.34 * scale, .86 * scale

    if kind == 0:  # ballroom floor pocket and AV cable lid
        _part(scene, 'FloorPocketStoneBed', (width, depth, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'CableServiceChannel', (width * .78, .25, .12), (0, 0, .18), MAT_STEEL)
        for i in range(2 + variant):
            x = (i - (1 + variant) / 2) * width * .62 / (1 + variant)
            _part(scene, f'FloorPocketHatchLeaf_{i}', (width * .28, depth * .68, .045),
                  (x, 0, .27), material)
            _part(scene, f'HatchPullRing_{i}', (.07, .035, .035), (x, -.22 * depth, .31), MAT_BRASS)
        _label(scene, 'AVFloorPocketServiceMark', .32 * width, -.38 * depth, .25)
    elif kind == 1:  # atrium maintenance catwalk bay
        deck_z = .42 + .02 * variant
        _part(scene, 'CatwalkDeck', (width, depth * .60, .14), (0, 0, deck_z), material)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'CatwalkSupportPost_{i}', (.09, .10, deck_z), (x, 0, deck_z / 2), MAT_STEEL)
            _part(scene, f'GuardrailUpright_{i}', (.055, .07, .88), (x, -.22 * depth, 1.02), MAT_STAINLESS)
        _part(scene, 'CatwalkGuardTopRail', (width * .92, .07, .07), (0, -.22 * depth, 1.44), MAT_BRASS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .78 / (3 + variant)
            _part(scene, f'CatwalkDeckGripBar_{i}', (.035, depth * .50, .018), (x, 0, deck_z + .08), MAT_STEEL)
        _label(scene, 'CatwalkLoadLimitPlate', .35 * width, -.40 * depth, .30)
    elif kind == 2:  # service elevator threshold and door jamb
        height = 2.25 + .04 * variant
        _part(scene, 'ElevatorThresholdSill', (width, .36, .14), (0, 0, .07), MAT_STONE)
        for i, x in enumerate((-.44 * width, .44 * width)):
            _part(scene, f'ElevatorJamb_{i}', (.16, .18, height), (x, 0, height / 2), material)
            _part(scene, f'JambInsetRail_{i}', (.035, .20, height * .88),
                  (x + .09, -.02, height * .48), MAT_STAINLESS)
        _part(scene, 'ElevatorHeadTrack', (width * .94, .20, .14), (0, 0, height + .04), MAT_STEEL)
        _part(scene, 'ThresholdSafetyStripe', (width * .66, .025, .06), (0, -.19, .17), MAT_LABEL)
        _label(scene, 'ElevatorClearOpeningTag', .31 * width, -.14, .27)
    elif kind == 3:  # porte cochere canopy pier module
        height = 2.72 + .04 * variant
        _part(scene, 'CanopyPierFooting', (width * .66, depth * .68, .18), (0, 0, .09), MAT_STONE)
        _part(scene, 'CanopyPierColumn', (.38 * width, .38 * depth, height), (0, 0, height / 2 + .09), material)
        _part(scene, 'PierCapital', (width * .86, depth * .80, .20), (0, 0, height + .18), MAT_STONE)
        _part(scene, 'CanopyBeamSeat', (width * 1.22, .42, .16), (0, 0, height + .36), MAT_STEEL)
        for i in range(2 + variant):
            x = (i - (1 + variant) / 2) * width * .82 / (1 + variant)
            _part(scene, f'PierFacadeReveal_{i}', (.045, .045, height * .74),
                  (x, -.20 * depth, height * .53), MAT_BRASS)
        _label(scene, 'CanopyPierInspectionPlate', .35 * width, -.22 * depth, .32)
    elif kind == 4:  # fire curtain side guides and headbox
        height = 2.46 + .04 * variant
        _part(scene, 'CurtainGuideBase', (width, depth * .50, .12), (0, 0, .06), MAT_STONE)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'CurtainSideGuide_{i}', (.09, .12, height), (x, 0, height / 2), material)
            _part(scene, f'GuideSmokeSeal_{i}', (.025, .055, height * .88),
                  (x - .055, -.075, height / 2), MAT_LINEN)
        _part(scene, 'CurtainHeadbox', (width * .96, .20, .18), (0, 0, height + .05), MAT_STEEL)
        _part(scene, 'CurtainDropPanel', (width * .80, .035, height * .70),
              (0, .06, height * .42), MAT_LINEN)
        _label(scene, 'FireCurtainManualRelease', .35 * width, -.12, .36)
    elif kind == 5:  # loading dock leveler plate and bumper set
        _part(scene, 'DockLevelerPitBed', (width, depth, .18), (0, 0, .09), MAT_STONE)
        _part(scene, 'DockLevelerPlatform', (width * .84, depth * .62, .15),
              (0, .02, .28), material)
        _part(scene, 'DockApproachLip', (width * .82, .20, .10), (0, -.40 * depth, .36), MAT_STEEL)
        for i, x in enumerate((-.42 * width, .42 * width)):
            _part(scene, f'DockBumper_{i}', (.13, .16, .42), (x, -.48 * depth, .35), MAT_STEEL)
            _part(scene, f'LevelerHydraulicHousing_{i}', (.11, .15, .20), (x * .65, .03, .43), MAT_STAINLESS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .70 / (3 + variant)
            _part(scene, f'LevelerGripRib_{i}', (.035, depth * .55, .025), (x, 0, .37), MAT_LABEL)
        _label(scene, 'DockCapacityStencil', .34 * width, -.42 * depth, .39)
    elif kind == 6:  # mechanical pipe riser support bay
        height = 2.28 + .04 * variant
        _part(scene, 'RiserSupportBase', (width, depth * .60, .13), (0, 0, .065), MAT_STONE)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .72 / (2 + variant)
            _post(scene, f'RiserPipe_{i}', x, .06, height, .065, material)
            for level in (0.42, 1.18, 1.94):
                _part(scene, f'PipeClamp_{i}_{int(level * 100)}', (.14, .20, .055),
                      (x, 0, level), MAT_STAINLESS)
        for level in (.40, 1.16, 1.92):
            _part(scene, f'RiserCrossBrace_{int(level * 100)}', (width * .88, .065, .07),
                  (0, .10, level), MAT_STEEL)
        _label(scene, 'RiserFlowDirectionPlate', .34 * width, -.34 * depth, .27)
    elif kind == 7:  # skylight shade cassette and slat bay
        _part(scene, 'ShadeCassetteBase', (width, depth, .16), (0, 0, .08), MAT_STEEL)
        _part(scene, 'ShadeCassetteRoller', (.13, depth * .78, .13),
              (-.40 * width, 0, .23), MAT_STAINLESS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .76 / (3 + variant)
            _part(scene, f'SkylightShadeBlade_{i}', (.09, depth * .84, .055),
                  (x, 0, .34 + .025 * (i % 2)), material)
            _part(scene, f'ShadeBladePivot_{i}', (.035, depth * .90, .035),
                  (x, 0, .42), MAT_BRASS)
        _label(scene, 'ShadeManualOverrideMark', .35 * width, -.42 * depth, .27)
    elif kind == 8:  # facade cleaning davit socket pair
        _part(scene, 'DavitSocketRoofPad', (width, depth, .16), (0, 0, .08), MAT_STONE)
        for i, x in enumerate((-.32 * width, .32 * width)):
            _part(scene, f'DavitSocketHousing_{i}', (.34, .34, .34), (x, 0, .33), material)
            _part(scene, f'DavitSocketSleeve_{i}', (.15, .15, .48), (x, 0, .73), MAT_STAINLESS)
            _part(scene, f'DavitPin_{i}', (.22, .055, .055), (x, -.20, .52), MAT_BRASS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .76 / (3 + variant)
            _part(scene, f'RoofAnchorBolt_{i}', (.07, .07, .06), (x, .34 * depth, .19), MAT_STEEL)
        _label(scene, 'DavitLoadCertificationPlate', .34 * width, -.40 * depth, .23)
    elif kind == 9:  # smoke-sealed service door surround
        height = 2.38 + .04 * variant
        _part(scene, 'ServiceDoorThreshold', (width, .38, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'ServiceDoorLeaf', (width * .72, .08, height * .88),
              (0, .02, height * .47), material)
        for i, x in enumerate((-.42 * width, .42 * width)):
            _part(scene, f'DoorFrameJamb_{i}', (.10, .17, height), (x, 0, height / 2), MAT_STEEL)
            _part(scene, f'DoorSmokeSeal_{i}', (.025, .05, height * .86),
                  (x + .06, -.10, height / 2), MAT_LINEN)
        _part(scene, 'ServiceDoorHeader', (width * .94, .17, .12), (0, 0, height + .04), MAT_STEEL)
        _part(scene, 'PanicBar', (width * .48, .06, .055), (0, -.05, 1.02), MAT_BRASS)
        _label(scene, 'FireRatingDoorTag', .33 * width, -.11, .24)
    elif kind == 10:  # ballroom partition stack pocket
        height = 2.58 + .04 * variant
        _part(scene, 'PartitionPocketBase', (width, depth * .68, .15), (0, 0, .075), MAT_STONE)
        _part(scene, 'MOV_PocketPanel_Stack', (width * .38, .12, height),
              (.28 * width, 0, height / 2 + .08), material)
        for i in range(3 + variant):
            x = (-.38 + i * .115) * width
            _part(scene, f'PartitionLeafEdge_{i}', (.06, .16, height * .84),
                  (x, -.06, height * .45), MAT_STEEL)
            _part(scene, f'PartitionHingeCap_{i}', (.09, .18, .09), (x, -.06, .90), MAT_BRASS)
        _part(scene, 'PartitionHeadTrack', (width * .94, .18, .13), (0, 0, height + .10), MAT_STEEL)
        _label(scene, 'PartitionStackDirectionLabel', .34 * width, -.12, .27)
    elif kind == 11:  # acoustic ceiling raft with service lights
        z = 2.72 + .04 * variant
        _part(scene, 'AcousticRaftCore', (width, depth, .16), (0, 0, z), material)
        _part(scene, 'RaftShadowReveal', (width * .92, depth * .90, .07), (0, 0, z - .12), MAT_STEEL)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .78 / (3 + variant)
            _part(scene, f'RaftAcousticRib_{i}', (.045, depth * .80, .045),
                  (x, 0, z + .11), MAT_WOOD)
        for i, x in enumerate((-.38 * width, .38 * width)):
            _part(scene, f'RaftSuspensionStem_{i}', (.025, .025, .48),
                  (x, .23 * depth, z - .34), MAT_BRASS)
        _part(scene, 'RaftLinearLightLens', (width * .48, .06, .035), (0, -.10, z - .02), MAT_ELECTRONICS)
        _label(scene, 'RaftServiceAccessMarker', .33 * width, -.42 * depth, z - .10)
    elif kind == 12:  # accessible ramp landing guardrail return
        run = width * .92
        _part(scene, 'RampLandingBase', (run, depth * .78, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'RampInclinedSurface', (run * .92, depth * .66, .12), (0, 0, .19), material)
        for i, x in enumerate((-.44 * run, .44 * run)):
            _post(scene, f'RampGuardPost_{i}', x, -.16, 1.02 + .025 * variant, .032, MAT_STEEL)
            _part(scene, f'GuardPostFoot_{i}', (.16, .18, .055), (x, -.16, .28), MAT_STAINLESS)
        _part(scene, 'RampUpperHandrail', (run * .92, .07, .07), (0, -.16, 1.04 + .025 * variant), MAT_WOOD)
        _part(scene, 'RampLowerHandrail', (run * .90, .055, .055), (0, -.22, .75), MAT_STEEL)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * run * .72 / (3 + variant)
            _part(scene, f'RampSurfaceGrip_{i}', (.035, depth * .55, .018), (x, 0, .27), MAT_BRASS)
        _label(scene, 'RampSlopeCompliancePlate', .31 * run, -.40 * depth, .33)
    elif kind == 13:  # roof parapet access ladder bay
        height = 2.56 + .035 * variant
        _part(scene, 'LadderLandingPad', (width, depth * .70, .16), (0, 0, .08), MAT_STONE)
        for i, x in enumerate((-.26 * width, .26 * width)):
            _post(scene, f'LadderSideRail_{i}', x, .04, height, .045, material)
            _part(scene, f'LadderReturnBracket_{i}', (.10, .24, .08), (x, .08, height - .12), MAT_STEEL)
        for i in range(8 + variant):
            z = .32 + i * (height - .46) / (7 + variant)
            _part(scene, f'LadderRung_{i}', (.58 * width, .065, .065), (0, .04, z), MAT_STAINLESS)
        _part(scene, 'LadderBottomStandoff', (width * .78, .08, .10), (0, .12, .24), MAT_STEEL)
        _label(scene, 'LadderFallProtectionNotice', .32 * width, -.31 * depth, .28)
    elif kind == 14:  # breezeway glazed windbreak bay
        height = 2.32 + .04 * variant
        _part(scene, 'WindbreakStoneSill', (width, depth * .46, .16), (0, 0, .08), MAT_STONE)
        for i, x in enumerate((-.43 * width, 0, .43 * width)):
            _part(scene, f'WindbreakMullion_{i}', (.065, .15, height), (x, 0, height / 2 + .08), material)
        for i, x in enumerate((-.215 * width, .215 * width)):
            _part(scene, f'WindbreakGlassPanel_{i}', (width * .38, .045, height * .84),
                  (x, .03, height * .48), MAT_GLASS)
        _part(scene, 'WindbreakHeadCap', (width * .95, .18, .11), (0, 0, height + .10), MAT_STEEL)
        _label(scene, 'WindbreakTemperedGlassMark', .34 * width, -.12, .27)
    else:  # service chase access hatch and pipe frame
        height = 2.28 + .04 * variant
        _part(scene, 'ChaseHatchStoneBase', (width, depth * .70, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'ChaseHatchPanel', (width * .76, .10, height * .88),
              (0, .08, height * .47), material)
        for i, x in enumerate((-.44 * width, .44 * width)):
            _part(scene, f'ChaseFrameUpright_{i}', (.095, .15, height), (x, 0, height / 2), MAT_STEEL)
        for i in range(5 + variant):
            z = .32 + i * (height - .48) / (4 + variant)
            _part(scene, f'ChaseVentSlot_{i}', (width * .26, .025, .035), (0, -.01, z), MAT_LABEL)
        _part(scene, 'ChaseServiceLatch', (.07, .055, .20), (.30 * width, -.02, 1.05), MAT_BRASS)
        _label(scene, 'ChaseUtilityIdentification', .30 * width, -.10, .23)
    return scene


def _guestroom(kind, variant, material, capacity_tiers=False):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    tier_width = (.78, .90, 1.02, 1.14, 1.26)[variant] if capacity_tiers else 1.0
    width, depth = .94 * scale * tier_width, .66 * scale

    if kind == 0:  # bath hamper bench
        _part(scene, 'HamperBenchPlinth', (width * .94, depth * .90, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'HamperWovenBody', (width, depth * .74, .46), (0, 0, .34), material)
        _part(scene, 'BenchCushion', (width * 1.04, depth, .12), (0, 0, .64), MAT_UPHOLSTERY)
        _part(scene, 'HamperFrontAccess', (width * .62, .025, .27), (0, -.25 * depth, .33), MAT_LINEN)
        for i in range(3):
            _part(scene, f'WovenBand_{i}', (width * .88, .025, .025),
                  (0, -.26 * depth, .19 + i * .13), MAT_WOOD)
        _label(scene, 'LaundrySortMark', .34 * width, -.34 * depth, .26)
    elif kind == 1:  # long-stay kitchenette pantry tower
        height = 1.72 + .04 * variant
        _part(scene, 'PantryTowerPlinth', (width * .82, depth * .80, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'PantryTowerCase', (width * .78, depth * .72, height), (0, 0, height / 2 + .10), material)
        for i in range(3 + variant):
            z = .34 + i * .31
            _part(scene, f'PantryDoor_{i}', (width * .72, .035, .27),
                  (0, -.44 * depth, z), MAT_WOOD)
            _part(scene, f'PantryPull_{i}', (.035, .035, .13),
                  (.27 * width, -.49 * depth, z), MAT_BRASS)
        _part(scene, 'PantryCounterCap', (width, depth * .86, .08), (0, 0, height + .15), MAT_STONE)
        _label(scene, 'PantryInventoryTag', .34 * width, -.46 * depth, .26)
    elif kind == 2:  # suitcase valet foldout shelf
        for i, x in enumerate((-.38 * width, .38 * width)):
            _part(scene, f'ValetUpright_{i}', (.065, .075, .72), (x, 0, .37), material)
        _part(scene, 'ValetFoldoutLuggageShelf', (width * .78, depth * .78, .075),
              (0, 0, .73), MAT_WOOD)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * width * .66 / (4 + variant)
            _part(scene, f'ShelfSupportStrap_{i}', (.035, .05, .34), (x, .22 * depth, .48), MAT_STEEL)
        _part(scene, 'ValetHangingBar', (width * .72, .05, .05), (0, .24 * depth, .82), MAT_BRASS)
        _label(scene, 'LuggageWeightLimitMark', .34 * width, -.34 * depth, .24)
    elif kind == 3:  # bedside reading trough and light
        _part(scene, 'ReadingTroughCase', (width, depth * .75, .54), (0, 0, .36), material)
        _part(scene, 'TroughOpenBookWell', (width * .78, depth * .54, .04), (0, -.02, .66), MAT_WOOD)
        _part(scene, 'TroughFrontRail', (width * .88, .055, .12), (0, -.27 * depth, .72), MAT_STEEL)
        _post(scene, 'BookLightStem', -.30 * width, 0, 1.05 + .02 * variant, .018, MAT_BRASS)
        _part(scene, 'BookLightHead', (.19, .12, .07), (-.30 * width, -.02, 1.07 + .02 * variant), MAT_ELECTRONICS)
        _part(scene, 'USBChargingPlate', (.24, .035, .13), (.31 * width, -.27 * depth, .53), MAT_STAINLESS)
        _label(scene, 'ReadingLightControl', .32 * width, -.34 * depth, .24)
    elif kind == 4:  # child travel crib
        for i, (x, y) in enumerate(((-.42 * width, -.30 * depth), (-.42 * width, .30 * depth),
                                    (.42 * width, -.30 * depth), (.42 * width, .30 * depth))):
            _part(scene, f'CribFoot_{i}', (.08, .08, .15), (x, y, .075), MAT_STEEL)
        _part(scene, 'TravelCribMattress', (width * .88, depth * .72, .16), (0, 0, .30), MAT_UPHOLSTERY)
        for side, y in enumerate((-.34 * depth, .34 * depth)):
            _part(scene, f'CribSideRail_{side}', (width, .065, .54), (0, y, .63), material)
            for i in range(6 + variant):
                x = (i - (5 + variant) / 2) * width * .84 / (5 + variant)
                _part(scene, f'CribSlat_{side}_{i}', (.035, .055, .46), (x, y, .61), MAT_STEEL)
        for i, x in enumerate((-.43 * width, .43 * width)):
            _part(scene, f'CribEndRail_{i}', (.065, depth * .78, .48), (x, 0, .61), MAT_WOOD)
        _part(scene, 'CribFoldLock', (.10, .06, .075), (.40 * width, -.30 * depth, .36), MAT_BRASS)
        _label(scene, 'CribAssemblyLockMark', -.31 * width, -.34 * depth, .23)
    elif kind == 5:  # accessible bath stool and grab support
        _part(scene, 'BathStoolSeat', (.62 * width, .58 * depth, .09), (0, 0, .47), material)
        for i, (x, y) in enumerate(((-.25, -.20), (-.25, .20), (.25, -.20), (.25, .20))):
            _part(scene, f'StoolLeg_{i}', (.055, .055, .43), (x * width, y * depth, .22), MAT_STAINLESS)
        _part(scene, 'StoolLowerBrace', (.54 * width, .055, .05), (0, 0, .18), MAT_STEEL)
        _part(scene, 'BathGrabHandleStem', (.055, .055, .62), (.42 * width, 0, .77), MAT_BRASS)
        _part(scene, 'BathGrabHandleTop', (.44 * width, .055, .055), (.23 * width, 0, 1.08), MAT_BRASS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .38 / (2 + variant)
            _part(scene, f'StoolDrainSlot_{i}', (.035, .20, .018), (x, 0, .52), MAT_STEEL)
        _label(scene, 'BathStoolWeightMark', -.31 * width, -.34 * depth, .24)
    elif kind == 6:  # guestroom reading chair
        _part(scene, 'ReadingChairSeat', (.76 * width, .72 * depth, .14), (0, 0, .49), material)
        _part(scene, 'ReadingChairBack', (.74 * width, .13, .78), (0, .28 * depth, .94), MAT_UPHOLSTERY)
        for i, x in enumerate((-.36 * width, .36 * width)):
            _part(scene, f'ChairArm_{i}', (.13, depth * .68, .12),
                  (x, 0, .79), MAT_WOOD)
        _floor_feet(scene, width * .72, depth * .66, .46, 'ChairLeg', MAT_STEEL)
        _part(scene, 'LumbarCushion', (.54 * width, .12, .24), (0, .18 * depth, .76), MAT_LINEN)
        _label(scene, 'ChairUpholsteryCode', .31 * width, -.34 * depth, .24)
    elif kind == 7:  # suite storage ottoman with lift tray
        _part(scene, 'OttomanPlinth', (width * .90, depth * .82, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'OttomanStorageBody', (width, depth, .38), (0, 0, .29), material)
        _part(scene, 'OttomanLiftTray', (width * .94, depth * .90, .08), (0, 0, .54), MAT_WOOD)
        for i in range(2 + variant):
            x = (i - (1 + variant) / 2) * width * .65 / (1 + variant)
            _part(scene, f'OttomanHinge_{i}', (.08, .045, .04), (x, .35 * depth, .58), MAT_BRASS)
        _part(scene, 'OttomanFrontPull', (.16, .035, .06), (0, -.51 * depth, .32), MAT_STAINLESS)
        _label(scene, 'OttomanStorageCapacityTag', .31 * width, -.36 * depth, .23)
    elif kind == 8:  # heated bath towel rail and shelf
        _part(scene, 'TowelRailBaseShelf', (width, depth * .62, .09), (0, 0, .10), MAT_STONE)
        for i, x in enumerate((-.40 * width, .40 * width)):
            _post(scene, f'HeatedTowelRailUpright_{i}', x, 0, 1.08 + .025 * variant, .026, material)
        for i in range(4 + variant):
            z = .40 + i * .17
            _part(scene, f'HeatedTowelBar_{i}', (width * .78, .055, .045), (0, 0, z), MAT_STAINLESS)
        _part(scene, 'RailTemperatureController', (.15, .09, .20), (.36 * width, -.10, .43), MAT_ELECTRONICS)
        _label(scene, 'HeatedRailElectricalMark', -.32 * width, -.34 * depth, .24)
    elif kind == 9:  # room service tray return cart
        _part(scene, 'TrayReturnLowerShelf', (width, depth, .10), (0, 0, .26), material)
        _part(scene, 'TrayReturnUpperShelf', (width * .94, depth * .90, .075), (0, 0, .78), MAT_STAINLESS)
        for i, x in enumerate((-.42 * width, .42 * width)):
            _part(scene, f'TrayCartUpright_{i}', (.055, .055, .78), (x, .35 * depth, .52), MAT_STEEL)
            _part(scene, f'TrayCartHandle_{i}', (.07, .07, .22), (x, .40 * depth, .93), MAT_BRASS)
        _casters(scene, width, depth, 'TrayReturnCart', animated=True)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .70 / (2 + variant)
            _part(scene, f'TrayDrainGroove_{i}', (.035, depth * .72, .018), (x, 0, .83), MAT_STEEL)
        _label(scene, 'TrayReturnRoomNumberClip', .32 * width, -.40 * depth, .40)
    elif kind == 10:  # underbed shoe drawer organizer
        _part(scene, 'ShoeOrganizerBase', (width, depth, .12), (0, 0, .06), MAT_STEEL)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .72 / (2 + variant)
            _part(scene, f'ShoeCaddyBay_{i}', (width * .19, depth * .76, .32), (x, 0, .29), material)
            _part(scene, f'ShoeBayFrontLip_{i}', (width * .18, .035, .08), (x, -.40 * depth, .39), MAT_WOOD)
        _part(scene, 'OrganizerPullBar', (width * .72, .06, .06), (0, -.48 * depth, .49), MAT_BRASS)
        _label(scene, 'UnderbedStorageTag', .33 * width, -.45 * depth, .22)
    elif kind == 11:  # pet-friendly bowl and leash station
        _part(scene, 'PetStationBaseTray', (width, depth, .12), (0, 0, .06), material)
        for i, x in enumerate((-.25 * width, .25 * width)):
            add(scene, cyl(.15 * scale, .10, (x, -.08, .20), MAT_STAINLESS, 16), f'PetWaterBowl_{i}')
            _part(scene, f'BowlRubberFoot_{i}', (.32, .32, .035), (x, -.08, .125), MAT_STEEL)
        canister_count = variant + 1
        for i in range(canister_count):
            x = (i - (canister_count - 1) / 2) * width * .84 / max(canister_count - 1, 1)
            add(scene, cyl(.082 * scale, .30, (x, .20 * depth, .285), MAT_WOOD, 12),
                f'PetTreatCanister_{i}')
            add(scene, cyl(.088 * scale, .035, (x, .20 * depth, .4525), MAT_BRASS, 12),
                f'PetTreatCanisterLid_{i}')
        _post(scene, 'LeashStationPost', .39 * width, .19 * depth, .80, .025, MAT_BRASS)
        _part(scene, 'LeashHook', (.18, .045, .045), (.39 * width, .19 * depth, .74), MAT_STEEL)
        _label(scene, 'PetWelcomeRoomTag', -.30 * width, -.40 * depth, .25)
    elif kind == 12:  # baby changing dresser
        _part(scene, 'ChangingDresserPlinth', (width * .94, depth * .88, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'ChangingDresserCase', (width, depth * .82, .66), (0, 0, .43), material)
        _part(scene, 'ChangingPad', (width * .88, depth * .76, .11), (0, 0, .82), MAT_LINEN)
        drawer_count = 3 + variant
        for i in range(drawer_count):
            x = (i - (drawer_count - 1) / 2) * width * .78 / drawer_count
            _part(scene, f'DresserDrawerFront_{i}', (width * .72 / drawer_count, .045, .22),
                  (x, -.43 * depth, .31 + .20 * (i % 2)), MAT_WOOD)
            _part(scene, f'DresserDrawerPull_{i}', (.09, .045, .045),
                  (x, -.46 * depth, .32 + .20 * (i % 2)), MAT_BRASS)
        _part(scene, 'ChangingPadSafetyLip', (width * .86, .07, .08), (0, .37 * depth, .89), MAT_STONE)
        _label(scene, 'ChangingStationMaximumLoad', .32 * width, -.46 * depth, .24)
    elif kind == 13:  # convertible sleeper loveseat
        _part(scene, 'SleeperLoveseatBase', (width, depth, .28), (0, 0, .24), MAT_STEEL)
        _part(scene, 'SleeperSeatCushion', (width * .90, depth * .70, .22), (0, -.02, .49), material)
        _part(scene, 'SleeperBackCushion', (width * .91, .16, .56), (0, .28 * depth, .86), MAT_UPHOLSTERY)
        for i in range(variant + 1):
            x = (i - variant / 2) * width * .72 / (variant + 1)
            _part(scene, f'SleeperSeatModuleSeam_{i}', (.025, depth * .62, .018),
                  (x, -.02, .609), MAT_LINEN)
        for i, x in enumerate((-.44 * width, .44 * width)):
            _part(scene, f'LoveseatArm_{i}', (.15, depth * .82, .49), (x, 0, .58), MAT_WOOD)
        _part(scene, 'PulloutBedFrontPanel', (width * .76, .065, .15), (0, -.44 * depth, .31), MAT_STEEL)
        _label(scene, 'SleeperMechanismRelease', .31 * width, -.48 * depth, .29)
    elif kind == 14:  # bath towel bench with hamper drawer
        _part(scene, 'TowelBenchSeat', (width, depth, .12), (0, 0, .58), material)
        _part(scene, 'BenchHamperBody', (width * .88, depth * .82, .42), (0, 0, .31), MAT_WOOD)
        compartment_count = variant + 1
        for i in range(compartment_count):
            x = (i - (compartment_count - 1) / 2) * width * .90 / compartment_count
            _part(scene, f'HamperSortCompartment_{i}',
                  (width * .84 / compartment_count, .075, .34),
                  (x, -.455 * depth, .34), MAT_LINEN)
        for i, x in enumerate((-.35 * width, .35 * width)):
            _part(scene, f'HamperVentSlot_{i}', (.22, .025, .13), (x, -.42 * depth, .30), MAT_STEEL)
        _part(scene, 'BenchLaundryPull', (.20, .04, .05), (0, -.45 * depth, .46), MAT_BRASS)
        _label(scene, 'TowelBenchLinenMark', .32 * width, -.43 * depth, .24)
    elif kind == 15:  # rollaway guest cot
        bed_width, bed_length = .84 + .21 * variant, 1.90 + .10 * variant
        _part(scene, 'RollawayCotFrame', (bed_width, bed_length, .14), (0, 0, .46), material)
        _part(scene, 'RollawayCotMattress', (bed_width * .94, bed_length * .96, .16), (0, 0, .61), MAT_LINEN)
        for i in range(variant + 1):
            y = (i - variant / 2) * bed_length * .72 / (variant + 1)
            _part(scene, f'CotReinforcingCrossbar_{i}', (bed_width * .86, .045, .055),
                  (0, y, .355), MAT_STAINLESS)
        for i, x in enumerate((-.40 * bed_width, .40 * bed_width)):
            _part(scene, f'CotFoldLeg_{i}', (.07, bed_length * .74, .42), (x, 0, .22), MAT_STEEL)
        _casters(scene, bed_width, bed_length, 'CotCaster')
        _part(scene, 'CotFoldLatch', (.14, .055, .10), (.43 * bed_width, -.35 * bed_length, .44), MAT_BRASS)
        _label(scene, 'CotLinenSizeMark', .32 * bed_width, -.42 * bed_length, .28)
    elif kind == 16:  # minibar bottle and glass organizer
        _part(scene, 'MinibarOrganizerBase', (width, depth, .12), (0, 0, .06), material)
        _part(scene, 'BottleRackBack', (width * .86, .07, .52), (0, .27 * depth, .40), MAT_WOOD)
        bottle_count = 3 + variant
        for i in range(bottle_count):
            x = (i - (bottle_count - 1) / 2) * width * .76 / bottle_count
            add(scene, cyl(.085, .40, (x, .20 * depth, .32), MAT_GLASS, 16),
                f'MinibarBottle_{i}')
            _part(scene, f'BottleCradle_{i}', (.15, .34, .10), (x, .04, .21), MAT_STEEL)
            _part(scene, f'BottleNeckKeeper_{i}', (.08, .05, .13), (x, .24 * depth, .48), MAT_BRASS)
        _part(scene, 'GlasswareShelf', (width * .88, depth * .72, .065), (0, -.12, .61), MAT_STONE)
        _label(scene, 'MinibarStockCode', .32 * width, -.40 * depth, .28)
    elif kind == 17:  # guest lounge chair and side table
        _part(scene, 'LoungeChairSeat', (.66 * width, .65 * depth, .13), (-.17 * width, 0, .48), material)
        _part(scene, 'LoungeChairBack', (.64 * width, .12, .73), (-.17 * width, .24 * depth, .87), MAT_UPHOLSTERY)
        _floor_feet(scene, .62 * width, .58 * depth, .42, 'LoungeChairLeg', MAT_WOOD)
        _part(scene, 'SideTableTop', (.42 * width, .42 * depth, .07), (.43 * width, -.05, .58), MAT_WOOD)
        _part(scene, 'SideTableStem', (.07, .07, .49), (.43 * width, -.05, .30), MAT_STEEL)
        _part(scene, 'SideTableFoot', (.32, .32, .06), (.43 * width, -.05, .06), MAT_STONE)
        cushion_count = variant + 1
        for i in range(cushion_count):
            x = -.17 * width + (i - (cushion_count - 1) / 2) * width * .62 / cushion_count
            _part(scene, f'LoungeLumbarCushion_{i}',
                  (width * .78 / cushion_count, .18, .38), (x, .18 * depth, .84), MAT_LINEN)
        _label(scene, 'LoungeSeatFabricTag', .28 * width, -.37 * depth, .25)
    elif kind == 18:  # foldaway guest writing desk
        _part(scene, 'WritingDeskTop', (width, depth * .78, .09), (0, 0, .79), material)
        _part(scene, 'DeskWallCleat', (width * .84, .08, .12), (0, .28 * depth, .65), MAT_STEEL)
        divider_count = variant + 1
        for i in range(divider_count):
            x = (i - (divider_count - 1) / 2) * width * .72 / divider_count
            _part(scene, f'DeskOrganizerDivider_{i}', (.025, .17, .20),
                  (x, .19 * depth, .93), MAT_WOOD)
        for i, x in enumerate((-.38 * width, .38 * width)):
            _part(scene, f'FoldDeskBrace_{i}', (.07, depth * .70, .42), (x, .02, .42), MAT_STAINLESS)
            _part(scene, f'FoldDeskFoot_{i}', (.18, .30, .055), (x, -.25 * depth, .055), MAT_STEEL)
        _part(scene, 'DeskPenTray', (width * .34, .18, .05), (0, -.27 * depth, .87), MAT_WOOD)
        _label(scene, 'DeskLoadLimitTag', .31 * width, -.42 * depth, .29)
    elif kind == 19:  # accessible makeup vanity console
        _part(scene, 'VanityConsolePlinth', (width * .90, depth * .82, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'AccessibleVanityCase', (width, depth * .72, .62), (0, 0, .41), material)
        _part(scene, 'VanityCountertop', (width * 1.05, depth * .88, .09), (0, 0, .77), MAT_STONE)
        _part(scene, 'VanityMirrorPanel', (width * .66, .055, .64), (0, .27 * depth, 1.16), MAT_GLASS)
        light_count = variant + 1
        for i in range(light_count):
            x = (i - (light_count - 1) / 2) * width * .58 / light_count
            _part(scene, f'VanityMirrorBulb_{i}', (.11, .06, .24),
                  (x, .225 * depth, 1.45), MAT_LINEN)
        for i, x in enumerate((-.35 * width, .35 * width)):
            _part(scene, f'VanityDrawerFront_{i}', (.27 * width, .035, .16),
                  (x, -.38 * depth, .40), MAT_WOOD)
        _part(scene, 'VanityTaskLight', (.08, .07, .38), (.40 * width, .23 * depth, 1.14), MAT_ELECTRONICS)
        _label(scene, 'VanityClearKneeMark', .30 * width, -.41 * depth, .25)
    elif kind == 20:  # closet luggage shelf and shoe drawer
        height = 1.24 + .24 * variant
        _part(scene, 'ClosetValetBase', (width, depth, .10), (0, 0, .05), MAT_STONE)
        for i, x in enumerate((-.40 * width, .40 * width)):
            _part(scene, f'ClosetShelfPost_{i}', (.065, .075, height), (x, 0, height / 2 + .10), material)
        _part(scene, 'ClosetTopLuggageShelf', (width, depth * .86, .08), (0, 0, height + .14), MAT_WOOD)
        _part(scene, 'ClosetShoeDrawer', (width * .86, depth * .72, .28), (0, 0, .36), MAT_WOOD)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .72 / (3 + variant)
            _part(scene, f'ShelfAntiSlidePeg_{i}', (.05, .05, .14), (x, .34 * depth, height + .23), MAT_BRASS)
        _label(scene, 'ClosetShelfWeightCode', .31 * width, -.42 * depth, .25)
    elif kind == 21:  # robe valet with steam station hook rail
        _part(scene, 'ValetStandBase', (.54 * width, .48 * depth, .10), (0, 0, .05), MAT_STONE)
        _post(scene, 'ValetStandColumn', 0, 0, 1.52 + .03 * variant, .035, material)
        _part(scene, 'ValetShoulderBar', (.70 * width, .07, .10), (0, 0, 1.53 + .03 * variant), MAT_WOOD)
        hook_count = variant + 1
        for i in range(hook_count):
            x = (i - (hook_count - 1) / 2) * width * .78 / hook_count
            _part(scene, f'RobeHook_{i}', (.12, .12, .24),
                  (x, -.05, 1.40 + .03 * variant), MAT_BRASS)
        _part(scene, 'SteamIronRest', (.30, .24, .07), (.32 * width, 0, .22), MAT_STEEL)
        _part(scene, 'SteamerCordGuide', (.055, .055, .64), (.32 * width, .14 * depth, .54), MAT_ELECTRONICS)
        _label(scene, 'ValetSafetyNotice', -.31 * width, -.34 * depth, .24)
    elif kind == 22:  # bedside book shelf and reading lamp
        _part(scene, 'BookShelfPlinth', (width, depth * .82, .09), (0, 0, .045), MAT_STEEL)
        for i in range(3):
            z = .38 + i * .29
            _part(scene, f'BookShelfBoard_{i}', (width, depth * .74, .06), (0, .03, z), material)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * width * .66 / (3 + variant)
            _part(scene, f'BookSpine_{i}', (.12, .12, .32 + .04 * (i % 2)),
                  (x, -.16 * depth, .53 + .29 * (i % 2)), MAT_LINEN)
        _post(scene, 'ReadingLampStem', .39 * width, 0, 1.42, .022, MAT_BRASS)
        _part(scene, 'ReadingLampShade', (.22, .20, .12), (.39 * width, 0, 1.45), MAT_ELECTRONICS)
        _label(scene, 'LampTouchControl', .32 * width, -.36 * depth, .25)
    elif kind == 23:  # underbed storage drawer module
        _part(scene, 'UnderbedDrawerFrame', (width, depth, .34), (0, 0, .23), material)
        drawer_count = 2 + variant
        for i in range(drawer_count):
            x = (i - (drawer_count - 1) / 2) * width * .90 / drawer_count
            _part(scene, f'UnderbedDrawerFront_{i}', (width * .84 / drawer_count, .045, .27),
                  (x, -.50 * depth, .24), MAT_WOOD)
            _part(scene, f'DrawerRecessPull_{i}', (.10, .035, .065), (x, -.53 * depth, .25), MAT_BRASS)
        _casters(scene, width, depth, 'DrawerModuleCaster')
        _label(scene, 'UnderbedModuleRoomTag', .32 * width, -.45 * depth, .44)
    elif kind == 24:  # family room bunk set
        bed_width, bed_length = .84 + .17 * variant, 1.92 + .10 * variant
        _part(scene, 'BunkLowerMattress', (bed_width, bed_length, .16), (0, 0, .45), MAT_UPHOLSTERY)
        _part(scene, 'BunkUpperMattress', (bed_width, bed_length, .16), (0, 0, 1.62 + .025 * variant), material)
        for i, (x, y) in enumerate(((-.48 * bed_width, -.48 * bed_length),
                                    (-.48 * bed_width, .48 * bed_length),
                                    (.48 * bed_width, -.48 * bed_length),
                                    (.48 * bed_width, .48 * bed_length))):
            _part(scene, f'BunkBedPost_{i}', (.09, .09, 2.12 + .025 * variant),
                  (x, y, 1.06 + .0125 * variant), MAT_WOOD)
        for i, x in enumerate((-.48 * bed_width, .48 * bed_width)):
            _part(scene, f'UpperBunkSafetyRail_{i}', (.07, bed_length * .82, .30),
                  (x, 0, 1.91 + .025 * variant), MAT_STEEL)
        for i in range(5 + variant):
            z = .50 + i * .23
            _part(scene, f'BunkLadderRung_{i}', (.44, .06, .06), (.48 * bed_width, -.42 * bed_length, z), MAT_BRASS)
        _label(scene, 'UpperBunkCapacityMark', .28 * width, -.41 * depth, .28)
        if variant == 4:
            trundle_width, trundle_length = bed_width * .82, bed_length * .68
            _part(scene, 'GrandBunkTrundleFrame', (trundle_width, trundle_length, .10),
                  (0, 0, .18), MAT_STEEL)
            _part(scene, 'GrandBunkTrundleMattress',
                  (trundle_width * .94, trundle_length * .94, .12), (0, 0, .29), MAT_LINEN)
    elif kind == 25:  # suite bar cabinet and glass rail
        _part(scene, 'SuiteBarPlinth', (width, depth, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'SuiteBarCabinet', (width, depth * .82, .68), (0, 0, .44), material)
        _part(scene, 'BarStoneTop', (width * 1.06, depth * .90, .09), (0, 0, .83), MAT_STONE)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * width * .72 / (2 + variant)
            _part(scene, f'BottleDivider_{i}', (.085, depth * .48, .40),
                  (x, .12, 1.00), MAT_GLASS)
        for i, x in enumerate((-.32 * width, .32 * width)):
            _part(scene, f'BarDrawerFront_{i}', (.30 * width, .035, .17),
                  (x, -.43 * depth, .45), MAT_WOOD)
        _label(scene, 'SuiteBarInventoryTag', .32 * width, -.45 * depth, .24)
    elif kind == 26:  # room service tray stand
        _part(scene, 'TrayStandTop', (width, depth * .84, .09), (0, 0, .78), material)
        for i in range(variant + 1):
            z = .18 + i * .105
            _part(scene, f'TrayStandLowerRackShelf_{i}',
                  (width * .72, depth * .62, .045), (0, .06, z), MAT_WOOD)
        for i, x in enumerate((-.38 * width, .38 * width)):
            _part(scene, f'TrayStandLeg_{i}', (.07, .07, .72), (x, .25 * depth, .39), MAT_STEEL)
        _part(scene, 'TrayRetainingFrontLip', (width * .90, .045, .09), (0, -.38 * depth, .86), MAT_WOOD)
        _part(scene, 'TrayRetainingSideLeft', (.045, depth * .70, .09), (-.46 * width, 0, .86), MAT_WOOD)
        _part(scene, 'TrayRetainingSideRight', (.045, depth * .70, .09), (.46 * width, 0, .86), MAT_WOOD)
        _floor_feet(scene, width * .74, depth * .62, .12, 'TrayStandFoot', MAT_STAINLESS)
        _label(scene, 'TrayStandRoomServiceMark', .30 * width, -.40 * depth, .26)
    elif kind == 27:  # window bench with side storage
        _part(scene, 'WindowBenchStorageCase', (width, depth, .43), (0, 0, .30), material)
        seat_count = variant + 1
        for i in range(seat_count):
            x = (i - (seat_count - 1) / 2) * width * 1.04 / seat_count
            _part(scene, f'WindowBenchSeatCushion_{i}',
                  (width * .98 / seat_count, depth * 1.03, .12), (x, 0, .58), MAT_UPHOLSTERY)
        for i, x in enumerate((-.28 * width, .28 * width)):
            _part(scene, f'BenchDrawerFront_{i}', (.34 * width, .035, .24),
                  (x, -.51 * depth, .31), MAT_WOOD)
            _part(scene, f'BenchDrawerPull_{i}', (.11, .035, .04), (x, -.54 * depth, .31), MAT_BRASS)
        _part(scene, 'BenchBackSill', (width, .09, .22), (0, .48 * depth, .72), MAT_STONE)
        _label(scene, 'WindowBenchUpholsteryTag', .30 * width, -.42 * depth, .24)
    elif kind == 28:  # guest linen chest with folded blanket tray
        _part(scene, 'LinenChestPlinth', (width * .92, depth * .86, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'LinenChestCase', (width, depth, .62), (0, 0, .41), material)
        _part(scene, 'ChestTopLid', (width * 1.04, depth * 1.03, .08), (0, 0, .77), MAT_WOOD)
        panel_count = 3 + variant
        for i in range(panel_count):
            x = (i - (panel_count - 1) / 2) * width * .90 / panel_count
            _part(scene, f'LinenChestFrontPanel_{i}', (width * .84 / panel_count, .045, .42),
                  (x, -.51 * depth, .42), MAT_LINEN)
        _part(scene, 'LinenChestLockPlate', (.12, .035, .12), (.38 * width, -.53 * depth, .55), MAT_BRASS)
        _label(scene, 'LinenChestInventoryCode', .31 * width, -.44 * depth, .25)
    elif kind == 29:  # guest ironing board cabinet
        # Tier this cabinet by height so the silhouette stays legible in a
        # shared preview frame as the other completion pieces grow in width.
        width = .94 * scale
        height = 1.24 + .25 * variant
        _part(scene, 'IroningCabinetBase', (width * .70, depth * .74, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'IroningCabinetBody', (width * .68, depth * .70, height),
              (0, 0, height / 2 + .10), material)
        board_depth = .28 + .11 * variant
        board_y = -.36 * depth - (board_depth - .25) / 2
        _part(scene, 'FoldoutIroningBoard', (width * .88, board_depth, .09),
              (0, board_y, .75), MAT_LINEN)
        if variant == 4:
            _part(scene, 'SleevePressingWing', (width * .28, .30, .09),
                  (.52 * width, board_y, .80), MAT_LINEN)
        _part(scene, 'IroningBoardSupportArm', (.065, depth * .66, .34), (0, -.18, .54), MAT_STEEL)
        _part(scene, 'SteamIronRestPlate', (.30, .24, .06), (.30 * width, -.04, .86), MAT_STAINLESS)
        vent_count = 4 + variant
        for i in range(vent_count):
            z = .32 + i * (height - .48) / (vent_count - 1)
            _part(scene, f'IroningCabinetVent_{i}', (.58 * width, .035, .075),
                  (0, -.36 * depth, z), MAT_LABEL)
        _label(scene, 'IroningSafetyNotice', .30 * width, -.40 * depth, .27)
    else:  # room safe and charging drawer console
        _part(scene, 'SafeConsolePlinth', (width, depth * .86, .10), (0, 0, .05), MAT_STEEL)
        _part(scene, 'GuestSafeCase', (width * .82, depth * .72, .55), (0, 0, .38), material)
        _part(scene, 'GuestSafeDoor', (width * .68, .05, .44), (0, -.45 * depth, .40), MAT_STAINLESS)
        _part(scene, 'SafeKeypad', (.14, .04, .18), (.26 * width, -.49 * depth, .43), MAT_ELECTRONICS)
        _part(scene, 'ChargingDrawer', (width, depth * .82, .19), (0, 0, .78), MAT_WOOD)
        _part(scene, 'WirelessChargingPad', (.30, .22, .025), (-.28 * width, -.10, .89), MAT_LABEL)
        port_count = variant + 1
        for i in range(port_count):
            x = (i - (port_count - 1) / 2) * width * .72 / port_count
            _part(scene, f'GuestDeviceChargingPort_{i}', (.14, .05, .11),
                  (x, -.49 * depth, .78), MAT_ELECTRONICS)
        _label(scene, 'SafeGuestInstructions', .31 * width, -.38 * depth, .24)
    return scene


def _decor(kind, variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]

    if kind == 0:  # carved stone storybook sculpture
        _part(scene, 'StorybookSculptureFoot', (.70 * scale, .44 * scale, .10), (0, 0, .05), MAT_STONE)
        _part(scene, 'StorybookLowerVolume', (.46 * scale, .16, .42), (0, 0, .31), material)
        _part(scene, 'StorybookUpperVolume', (.38 * scale, .14, .37), (.02, -.02, .68), MAT_WOOD)
        _part(scene, 'StorybookPageBlock', (.33 * scale, .025, .56), (0, -.10, .57), MAT_LINEN)
        _part(scene, 'StorybookSpineBand', (.045, .16, .78), (-.20 * scale, 0, .48), MAT_BRASS)
        _label(scene, 'StorybookArtistPlaque', .32 * scale, -.12, .20)
    elif kind == 1:  # lobby anniversary recognition tower
        _part(scene, 'RecognitionTowerPlinth', (.70 * scale, .56 * scale, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'AnniversaryMedallion', (.52 * scale, .10, .58 + .025 * variant),
              (0, 0, .47), material)
        for i in range(3 + variant):
            z = .34 + i * .14
            _part(scene, f'MedallionReliefBand_{i}', (.40 * scale, .035, .035), (0, -.06, z), MAT_BRASS)
        _part(scene, 'RecognitionHeader', (.42 * scale, .08, .15), (0, 0, .86 + .025 * variant), MAT_WOOD)
        _label(scene, 'AnniversaryDatePlate', .30 * scale, -.10, .22)
    elif kind == 2:  # heritage room key display case
        _part(scene, 'KeyCaseFoot', (.76 * scale, .34 * scale, .10), (0, 0, .05), MAT_WOOD)
        _part(scene, 'KeyCaseBackboard', (.68 * scale, .09, .72), (0, .02, .49), material)
        _part(scene, 'KeyCaseGlassFront', (.68 * scale, .035, .68), (0, -.055, .50), MAT_GLASS)
        for i in range(4 + variant):
            x = (i - (3 + variant) / 2) * .105 * scale
            _part(scene, f'HeritageRoomKey_{i}', (.055, .035, .25 + .02 * (i % 2)),
                  (x, -.09, .43 + .08 * (i % 2)), MAT_BRASS)
        _part(scene, 'KeyCaseHeaderRail', (.73 * scale, .07, .10), (0, 0, .88), MAT_STONE)
        _label(scene, 'KeyCaseCuratorTag', .31 * scale, -.10, .18)
    elif kind == 3:  # coastal glass orb cluster
        _part(scene, 'OrbClusterBase', (.62 * scale, .52 * scale, .09), (0, 0, .045), MAT_STONE)
        for i, (x, y, z, r) in enumerate(((-.18, 0, .32, .17), (.02, .02, .48, .22), (.21, 0, .30, .15))):
            add(scene, sphere(r * scale, (x * scale, y, z), material, subdivisions=2), f'CoastalOrb_{i}')
            _part(scene, f'OrbCradle_{i}', (.15 * scale, .14, .08), (x * scale, 0, z - r * .74), MAT_BRASS)
        _part(scene, 'OrbClusterNameplate', (.27 * scale, .025, .09), (.30 * scale, -.24 * scale, .16), MAT_STEEL)
        _label(scene, 'OrbClusterCareMark', -.31 * scale, -.23 * scale, .17)
    elif kind == 4:  # ballroom table number easel
        _part(scene, 'TableNumberEaselFoot', (.42 * scale, .28 * scale, .08), (0, 0, .04), MAT_STONE)
        _part(scene, 'EaselSupportBack', (.06, .09, .48), (0, .07, .31), MAT_WOOD)
        _part(scene, 'EaselNumberCard', (.40 * scale, .035, .42), (0, -.02, .50), material)
        for i in range(2 + variant):
            z = .38 + i * .12
            _part(scene, f'CardFoilRule_{i}', (.28 * scale, .02, .025), (0, -.045, z), MAT_BRASS)
        _part(scene, 'EaselCardRetainer', (.46 * scale, .06, .06), (0, -.04, .27), MAT_STEEL)
        _label(scene, 'EaselPropertyMark', .30 * scale, -.14, .17)
    elif kind == 5:  # curator artifact dome on stepped base
        _part(scene, 'ArtifactDomeBase', (.64 * scale, .64 * scale, .12), (0, 0, .06), MAT_WOOD)
        _part(scene, 'ArtifactDomeGlass', (.46 * scale, .46 * scale, .52 + .02 * variant),
              (0, 0, .40 + .02 * variant), MAT_GLASS)
        _part(scene, 'ArtifactDisplayBlock', (.18 * scale, .18 * scale, .25), (0, 0, .28), material)
        _part(scene, 'DomeBrassFinial', (.10, .10, .10), (0, 0, .70 + .02 * variant), MAT_BRASS)
        _part(scene, 'DomeBaseEdgeBand', (.68 * scale, .68 * scale, .035), (0, 0, .14), MAT_BRASS)
        _label(scene, 'ArtifactAccessionCode', .31 * scale, -.27 * scale, .18)
    elif kind == 6:  # ceramic desk fountain bowl
        _part(scene, 'FountainDeskBase', (.58 * scale, .50 * scale, .10), (0, 0, .05), MAT_STONE)
        _part(scene, 'DeskFountainPedestal', (.20 * scale, .20 * scale, .44), (0, 0, .31), material)
        _part(scene, 'DeskFountainBasin', (.67 * scale, .56 * scale, .15), (0, 0, .60), MAT_CERAMIC)
        _part(scene, 'BasinWaterInset', (.48 * scale, .38 * scale, .025), (0, 0, .69), MAT_GLASS)
        _post(scene, 'FountainCenterSpout', 0, 0, .86 + .02 * variant, .025, MAT_BRASS)
        _part(scene, 'FountainPumpCover', (.15, .13, .07), (.23 * scale, 0, .26), MAT_STEEL)
        _label(scene, 'FountainCareInstruction', .31 * scale, -.25 * scale, .18)
    elif kind == 7:  # miniature grand stair model
        _part(scene, 'StairModelFoot', (.72 * scale, .56 * scale, .08), (0, 0, .04), MAT_WOOD)
        for i in range(5 + variant):
            x = (i - (4 + variant) / 2) * .09 * scale
            _part(scene, f'MiniatureStairTread_{i}', (.20 * scale, .30 * scale, .055),
                  (x, 0, .12 + i * .075), material)
            _part(scene, f'MiniatureStairRiser_{i}', (.19 * scale, .04, .07),
                  (x - .01, -.13 * scale, .095 + i * .075), MAT_STONE)
        _part(scene, 'StairModelHandrail', (.035, .31 * scale, .035), (.26 * scale, 0, .65), MAT_BRASS)
        _label(scene, 'StairModelScalePlate', -.30 * scale, -.25 * scale, .16)
    elif kind == 8:  # banquet candle cluster on brass tray
        _part(scene, 'CandleClusterTray', (.72 * scale, .56 * scale, .07), (0, 0, .035), MAT_BRASS)
        for i in range(3 + variant):
            x = (i - (2 + variant) / 2) * .16 * scale
            height = .36 + .08 * (i % 3) + .018 * variant
            _part(scene, f'BanquetCandle_{i}', (.11 * scale, .11 * scale, height), (x, 0, .08 + height / 2), material)
            _part(scene, f'CandleBrassCup_{i}', (.15 * scale, .15 * scale, .07), (x, 0, .10), MAT_BRASS)
        _part(scene, 'CandleSnufferRest', (.16, .12, .04), (.30 * scale, .17 * scale, .10), MAT_STEEL)
        _label(scene, 'CandleSafetyCard', -.30 * scale, -.22 * scale, .14)
    else:  # pressed botanical frame set
        _part(scene, 'BotanicalFrameFoot', (.72 * scale, .28 * scale, .09), (0, 0, .045), MAT_WOOD)
        for i in range(2 + variant):
            x = (i - (1 + variant) / 2) * .23 * scale
            _part(scene, f'BotanicalFrameBorder_{i}', (.36 * scale, .07, .54), (x, 0, .39), material)
            _part(scene, f'PressedLeafPanel_{i}', (.27 * scale, .025, .43), (x, -.045, .39), MAT_LINEN)
            _part(scene, f'BotanicalStem_{i}', (.025, .018, .33), (x, -.07, .39), MAT_VEGETATION)
        _part(scene, 'BotanicalSetHeader', (.70 * scale, .06, .10), (0, 0, .74), MAT_STONE)
        _label(scene, 'BotanicalCatalogNumber', .30 * scale, -.10, .16)
    return scene


def _finish(variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = 1.28 * scale, .84 * scale
    _part(scene, 'ThresholdStoneBed', (width, depth, .13), (0, 0, .065), MAT_STONE)
    _part(scene, 'CarpetTransitionStrip', (width * .90, depth * .34, .055),
          (0, -.25 * depth, .16), material)
    _part(scene, 'MetalEdgeRetainer', (width * .94, .07, .065), (0, -.45 * depth, .16), MAT_STAINLESS)
    for i in range(4 + variant):
        x = (i - (3 + variant) / 2) * width * .80 / (3 + variant)
        _part(scene, f'ThresholdInlaySegment_{i}', (width * .80 / (4 + variant), .12, .035),
              (x, .12 * depth, .18), material)
        _part(scene, f'FloorAnchorFastener_{i}', (.04, .04, .03), (x, -.42 * depth, .20), MAT_BRASS)
    _label(scene, 'ThresholdFinishCode', .34 * width, -.40 * depth, .24)
    return scene


def _restaurant(variant, material):
    scene = trimesh.Scene()
    scale = SCALE[variant]
    width, depth = .82 * scale, .60 * scale
    _part(scene, 'CondimentCaddyBase', (width, depth, .09), (0, 0, .045), MAT_WOOD)
    _part(scene, 'CaddyRearRail', (width * .90, .06, .22), (0, .24 * depth, .22), material)
    for i in range(3 + variant):
        x = (i - (2 + variant) / 2) * width * .72 / (2 + variant)
        _part(scene, f'JamJar_{i}', (.12 * scale, .12 * scale, .22 + .02 * (i % 2)),
              (x, -.04, .20 + .01 * variant), MAT_GLASS)
        _part(scene, f'JarLid_{i}', (.13 * scale, .13 * scale, .045),
              (x, -.04, .34 + .02 * (i % 2)), MAT_STAINLESS)
        _part(scene, f'JarLabel_{i}', (.08, .02, .075), (x, -.105, .20), MAT_LABEL)
    _part(scene, 'ButterDish', (.22 * scale, .20 * scale, .085), (.30 * width, .12, .12), MAT_CERAMIC)
    _part(scene, 'CaddyServiceCard', (.23, .035, .11), (-.30 * width, -.18 * depth, .16), MAT_LINEN)
    _label(scene, 'BreakfastAllergenLegend', .30 * width, -.34 * depth, .20)
    return scene


def _amenity_singleton(number, material):
    scene = trimesh.Scene()
    if number == 4096:  # spa towel warming basket
        _part(scene, 'TowelWarmerBase', (.58, .48, .10), (0, 0, .05), MAT_STONE)
        _part(scene, 'WarmingBasketBody', (.52, .42, .50), (0, 0, .36), material)
        for i in range(4):
            _part(scene, f'BasketVent_{i}', (.34, .025, .035), (0, -.22, .20 + i * .10), MAT_STEEL)
        _part(scene, 'BasketLid', (.56, .45, .08), (0, 0, .65), MAT_WOOD)
        _part(scene, 'TowelWarmerDial', (.10, .06, .14), (.28, -.20, .40), MAT_ELECTRONICS)
        _label(scene, 'SpaTemperatureCareMark', .30, -.24, .21)
    elif number == 4097:  # poolside sunscreen dispenser
        _part(scene, 'DispenserFoot', (.48, .44, .10), (0, 0, .05), MAT_STONE)
        _part(scene, 'SunscreenDispenserColumn', (.40, .36, .90), (0, 0, .55), material)
        _part(scene, 'DispenserCanopy', (.54, .48, .12), (0, 0, 1.06), MAT_STEEL)
        for i, x in enumerate((-.12, .12)):
            _part(scene, f'SunscreenBottleSlot_{i}', (.12, .13, .38), (x, -.20, .42), MAT_CERAMIC)
            _part(scene, f'PumpLever_{i}', (.055, .10, .08), (x, -.23, .64), MAT_BRASS)
        _label(scene, 'SunscreenRefillInstructions', .30, -.24, .20)
    else:  # portable event coat stand
        _part(scene, 'CoatStandFootPlate', (.56, .48, .10), (0, 0, .05), MAT_STONE)
        _post(scene, 'CoatStandColumn', 0, 0, 1.72, .035, material)
        _part(scene, 'CoatStandCrown', (.26, .24, .11), (0, 0, 1.72), MAT_WOOD)
        for i in range(6):
            angle = i * 3.14159 / 3
            x, y = .27 * math.cos(angle), .27 * math.sin(angle)
            _part(scene, f'CoatStandHook_{i}', (.12, .06, .06), (x, y, 1.59), MAT_BRASS)
        _label(scene, 'CoatStandPropertyTag', .28, -.22, .24)
    return scene


def _exterior_singleton(number, material):
    scene = trimesh.Scene()
    if number == 4099:  # courtyard bollard light
        _part(scene, 'BollardFooting', (.44, .44, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'BollardPost', (.20, .20, .78), (0, 0, .51), material)
        _part(scene, 'BollardLightLens', (.17, .17, .20), (0, 0, .99), MAT_GLASS)
        _part(scene, 'BollardCap', (.27, .27, .08), (0, 0, 1.13), MAT_STEEL)
        for i, (x, y) in enumerate(((.12, 0), (0, .12), (-.12, 0), (0, -.12))):
            _part(scene, f'BollardShieldRib_{i}', (.035, .04, .42), (x, y, .50), MAT_BRASS)
        _label(scene, 'BollardVoltageMark', .26, -.15, .22)
    else:  # low garden path wayfinding marker
        _part(scene, 'PathMarkerStoneBase', (.58, .46, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'WayfindingMarkerBody', (.50, .30, .50), (0, 0, .37), material)
        _part(scene, 'WayfindingArrowPanel', (.42, .04, .26), (0, -.17, .45), MAT_LABEL)
        for i in range(3):
            z = .26 + i * .14
            _part(scene, f'PathMarkerReflector_{i}', (.36, .025, .035), (0, -.19, z), MAT_BRASS)
        _part(scene, 'MarkerTopCap', (.56, .38, .06), (0, 0, .65), MAT_STONE)
        _label(scene, 'GardenPathZoneTag', .27, -.20, .20)
    return scene


def build_asset(name, subcategory, material, asset_id, profile):
    try:
        number = int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'hotel final catalog asset ID outside A3901–A4200: {asset_id}') from exc
    if not 3901 <= number <= 4200:
        raise ValueError(f'hotel final catalog asset ID outside A3901–A4200: {asset_id}')

    if number <= 3980:
        kind, variant = divmod(number - 3901, 5)
        return _architecture(kind, variant, material)
    if number <= 4035:
        kind, variant = divmod(number - 3981, 5)
        return _guestroom(kind, variant, material)
    if number <= 4085:
        kind, variant = divmod(number - 4036, 5)
        return _decor(kind, variant, material)
    if number <= 4090:
        return _finish(number - 4086, material)
    if number <= 4095:
        return _restaurant(number - 4091, material)
    if number <= 4098:
        return _amenity_singleton(number, material)
    if number <= 4100:
        return _exterior_singleton(number, material)
    kind, variant = divmod(number - 4101, 5)
    return _guestroom(11 + kind, variant, material, capacity_tiers=True)
