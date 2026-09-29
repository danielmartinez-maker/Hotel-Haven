"""Purpose-built continuation assets HH_A3001–HH_A3250."""

import math

import trimesh

from v2_asset_common import add, box, cyl, sphere


MAT_STEEL = 'MAT_BLACKENED_STEEL'
MAT_BLACKENED_STEEL = MAT_STEEL
MAT_STAINLESS = 'MAT_STAINLESS'
MAT_BRASS = 'MAT_BRASS_POLISHED'
MAT_STONE = 'MAT_STONE_LIGHT'
MAT_WOOD = 'MAT_WOOD_WARM'
MAT_GLASS = 'MAT_GLASS_CLEAR'
MAT_LABEL = 'MAT_SIGNAGE'
MAT_CERAMIC = 'MAT_CERAMIC_FIXTURE'
MAT_LINEN = 'MAT_LINEN'
MAT_ELECTRONICS = 'MAT_ELECTRONICS'
MAT_UPHOLSTERY = 'MAT_UPHOLSTERY'


def _part(scene, name, size, center, material):
    add(scene, box(size, center, material), name)


def _floor_feet(scene, width, depth, height, prefix, material=MAT_STEEL, center=(0.0, 0.0)):
    for index, (sx, sy) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        _part(scene, f'{prefix}_{index}', (.055, .055, height),
              (center[0] + sx * width * .40, center[1] + sy * depth * .37, height / 2), material)


def _casters(scene, width, depth, prefix, *, animated=False):
    for index, (sx, sy) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        center = (sx * width * .40, sy * depth * .37, .065)
        wheel = cyl(.065, .05, center, MAT_STEEL, 12)
        wheel.apply_transform(trimesh.transformations.rotation_matrix(
            math.pi / 2, [1, 0, 0], point=center))
        node_name = f'MOV_Wheel_{prefix}_{index}' if animated else f'{prefix}_{index}'
        add(scene, wheel, node_name)


def _post(scene, name, x, y, height, radius, material):
    add(scene, cyl(radius, height, (x, y, height / 2), material, 12), name)


def _label(scene, name, x, y, z, width=.20):
    _part(scene, name, (width, .025, .07), (x, y, z), MAT_LABEL)


def _architecture(kind, v, material):
    scene = trimesh.Scene()
    scale = .90 + .055 * v
    w, d = 1.30 * scale, .95 * scale
    h = 2.45 + .11 * v
    if kind == 0:  # entrance canopy bay
        _part(scene, 'CanopyFootingLeft', (.18, .22, .12), (-w * .42, 0, .06), MAT_STONE)
        _part(scene, 'CanopyFootingRight', (.18, .22, .12), (w * .42, 0, .06), MAT_STONE)
        for side, x in enumerate((-.42 * w, .42 * w)):
            _part(scene, f'CanopyColumn_{side}', (.09, .10, h), (x, 0, h / 2), material)
        _part(scene, 'CanopyFrontBeam', (w, .13, .14), (0, -.36 * d, h + .08), MAT_STEEL)
        _part(scene, 'CanopyRearBeam', (w, .13, .14), (0, .36 * d, h + .08), MAT_STEEL)
        for i in range(4 + v):
            x = (i - (3 + v) / 2) * w * .84 / (3 + v)
            _part(scene, f'CanopyRafter_{i}', (.06, d * .80, .08), (x, 0, h + .18), MAT_WOOD)
        _part(scene, 'CanopyGlassPanel', (w * .88, d * .70, .045), (0, 0, h + .25), MAT_GLASS)
        _part(scene, 'CanopyDrainGutter', (w * .92, .08, .10), (0, -.44 * d, h + .12), MAT_STAINLESS)
    elif kind == 1:  # suspended acoustic ceiling raft
        w, d = 1.45 * scale, .96 * scale
        z = 2.92 + .07 * v
        _part(scene, 'AcousticRaftCore', (w, d, .13), (0, 0, z), MAT_LINEN)
        _part(scene, 'RaftShadowReveal', (w * .90, d * .90, .07), (0, 0, z - .10), MAT_STEEL)
        for i in range(4 + v):
            x = (i - (3 + v) / 2) * w * .78 / (3 + v)
            _part(scene, f'RaftPerforationRail_{i}', (.035, d * .76, .045), (x, 0, z + .09), MAT_WOOD)
        for i, x in enumerate((-.38 * w, .38 * w)):
            _part(scene, f'SuspensionRod_{i}', (.025, .025, .44), (x, 0, z - .31), MAT_BRASS)
        _part(scene, 'RaftLinearLight', (w * .52, .045, .035), (0, 0, z - .02), MAT_ELECTRONICS)
        _label(scene, 'ServiceAccessMark', .36 * w, -.50 * d, z - .12)
    elif kind == 2:  # exterior rainscreen fin panel
        panel_h = 2.42 + .08 * v
        _part(scene, 'RainscreenBacking', (w, .10, panel_h), (0, 0, panel_h / 2), MAT_STONE)
        _part(scene, 'RainscreenBaseTrack', (w * 1.04, .15, .14), (0, 0, .07), MAT_STEEL)
        _part(scene, 'RainscreenHeadTrack', (w * 1.04, .15, .12), (0, 0, panel_h + .06), MAT_STEEL)
        for i in range(5 + v):
            x = (i - (4 + v) / 2) * w * .82 / (4 + v)
            _part(scene, f'VerticalCeramicFin_{i}', (.09, .20, panel_h * .94), (x, -.10, panel_h / 2), material)
            _part(scene, f'FinClip_{i}', (.12, .15, .06), (x, -.11, panel_h * .68), MAT_BRASS)
        _label(scene, 'EnvelopeDrainageTag', -.36 * w, -.17, .20)
    elif kind == 3:  # smoke curtain and headbox assembly
        head = 2.75 + .08 * v
        _part(scene, 'SmokeCurtainHeadbox', (w, .20, .18), (0, 0, head), MAT_STEEL)
        for side, x in enumerate((-.46 * w, .46 * w)):
            _part(scene, f'CurtainGuideRail_{side}', (.065, .12, head), (x, 0, head / 2), material)
        _part(scene, 'FireCurtainFabric', (w * .82, .025, head * .72), (0, -.03, head * .60), MAT_LINEN)
        _part(scene, 'CurtainBottomBar', (w * .84, .07, .08), (0, -.035, head * .24), MAT_STAINLESS)
        for i, x in enumerate((-.35 * w, .35 * w)):
            _part(scene, f'CurtainLimitSwitch_{i}', (.10, .08, .12), (x, -.08, head - .15), MAT_ELECTRONICS)
        _label(scene, 'CurtainInspectionLabel', .35 * w, -.11, head - .31)
    elif kind == 4:  # protected column and capital detail
        h = 2.50 + .10 * v
        _part(scene, 'ColumnShaft', (.36 * scale, .36 * scale, h), (0, 0, h / 2), material)
        _part(scene, 'ColumnBaseShoe', (.58 * scale, .58 * scale, .18), (0, 0, .09), MAT_STONE)
        _part(scene, 'ColumnNeckBand', (.43 * scale, .43 * scale, .11), (0, 0, h - .20), MAT_BRASS)
        _part(scene, 'CapitalAbacus', (.78 * scale, .78 * scale, .20), (0, 0, h - .045), MAT_STONE)
        for side, (x, y) in enumerate(((0, -.25), (.25, 0), (0, .25), (-.25, 0))):
            _part(scene, f'CapitalCorbel_{side}', (.15, .15, .17), (x * scale, y * scale, h - .19), MAT_WOOD)
        _label(scene, 'ColumnFireRatingTag', -.15 * scale, -.20 * scale, .30)
    elif kind == 5:  # courtyard pergola bay
        h = 2.46 + .09 * v
        for i, (x, y) in enumerate(((-.42*w,-.34*d),(-.42*w,.34*d),(.42*w,-.34*d),(.42*w,.34*d))):
            _part(scene, f'PergolaPostFoot_{i}', (.15, .15, .12), (x, y, .06), MAT_STONE)
            _part(scene, f'PergolaPost_{i}', (.09, .09, h), (x, y, h / 2), material)
        for side, y in enumerate((-.34*d, .34*d)):
            _part(scene, f'PergolaLongBeam_{side}', (w * .94, .12, .14), (0, y, h + .08), MAT_WOOD)
        for i in range(4 + v):
            x = (i - (3 + v) / 2) * w * .84 / (3 + v)
            _part(scene, f'PergolaSlat_{i}', (.06, d * .82, .07), (x, 0, h + .19), MAT_STEEL)
        _part(scene, 'PergolaGutter', (w * .96, .08, .08), (0, -.40*d, h + .12), MAT_STAINLESS)
    elif kind == 6:  # loading dock portal and bumpers
        h = 2.70 + .08 * v
        _part(scene, 'DockThreshold', (w, .40, .16), (0, -.20, .08), MAT_STONE)
        for side, x in enumerate((-.45*w, .45*w)):
            _part(scene, f'DockPortalJamb_{side}', (.16, .20, h), (x, 0, h/2), material)
            _part(scene, f'DockBumper_{side}', (.21, .12, .42), (x, -.15, .31), MAT_STEEL)
        _part(scene, 'DockPortalHeader', (w, .20, .20), (0, 0, h - .10), material)
        _part(scene, 'DockCanopyApron', (w * 1.12, .56, .10), (0, -.22, h + .08), MAT_STAINLESS)
        _part(scene, 'DockClearanceBar', (w * .72, .04, .08), (0, -.43, 2.18), MAT_BRASS)
        _part(scene, 'DockSignalLight', (.10, .12, .26), (.39*w, -.15, 2.30), MAT_ELECTRONICS)
        _label(scene, 'DockClearancePlate', -.32*w, -.12, 1.88)
    elif kind == 7:  # roof drain and overflow scupper module
        h = 2.52 + .08 * v
        _part(scene, 'ParapetSection', (w, .24, h), (0, 0, h/2), material)
        _part(scene, 'CopingStone', (w * 1.08, .35, .12), (0, 0, h + .06), MAT_STONE)
        _part(scene, 'PrimaryScupperMouth', (.34, .28, .18), (.24*w, -.17, h - .25), MAT_STAINLESS)
        _part(scene, 'OverflowScupperMouth', (.24, .26, .14), (-.26*w, -.17, h - .42), MAT_BRASS)
        _part(scene, 'Downspout', (.10, .12, h - .28), (.24*w, -.24, (h-.28)/2), MAT_STEEL)
        _part(scene, 'CleanoutDoor', (.13, .04, .19), (.24*w, -.31, .38), MAT_STAINLESS)
        _label(scene, 'RoofDrainLabel', -.30*w, -.16, h - .65)
    elif kind == 8:  # acoustic wall with replaceable timber ribs
        h = 2.52 + .08 * v
        _part(scene, 'AcousticWallCore', (w, .12, h), (0, 0, h/2), MAT_LINEN)
        _part(scene, 'WallPanelToeRail', (w * 1.02, .16, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'WallPanelTopRail', (w * 1.02, .16, .12), (0, 0, h - .06), MAT_STEEL)
        for i in range(5 + v):
            x = (i - (4 + v)/2) * w * .82/(4 + v)
            _part(scene, f'TimberAcousticRib_{i}', (.07, .12, h*.90), (x, -.08, h/2), material)
            _part(scene, f'RibMount_{i}', (.035, .035, .08), (x, -.14, h*.72), MAT_BRASS)
        _label(scene, 'AcousticPanelBatchTag', .36*w, -.15, .22)
    else:  # framed corner glazing return
        h = 2.42 + .09 * v
        _part(scene, 'GlazingSill', (w, .24, .16), (0, 0, .08), MAT_STONE)
        _part(scene, 'GlazingHead', (w, .16, .14), (0, 0, h + .02), MAT_STEEL)
        for side, x in enumerate((-.45*w, .45*w)):
            _part(scene, f'CornerMullion_{side}', (.075, .14, h), (x, 0, h/2), material)
        _part(scene, 'ReturnMullion', (.08, .60, h), (.44*w, .20, h/2), MAT_BRASS)
        _part(scene, 'VisionGlass_A', (w*.78, .035, h*.84), (-.05*w, -.06, h*.52), MAT_GLASS)
        _part(scene, 'VisionGlass_B', (.035, .43, h*.84), (.42*w, .20, h*.52), MAT_GLASS)
        _part(scene, 'GlazingGasket', (w*.80, .045, .035), (0, -.09, h*.14), MAT_STEEL)
        _label(scene, 'GlazingSafetyMark', -.32*w, -.10, 1.02)
    return scene


def _guestroom(kind, v, material):
    scene = trimesh.Scene()
    s = .88 + .06 * v
    w, d, h = 1.12*s, .58*s, .82 + .07*v
    if kind == 0:  # room entry wardrobe and valet bench
        _part(scene, 'WardrobePlinth', (w*.70, d*.62, .12), (0, 0, .06), MAT_STONE)
        _part(scene, 'EntryWardrobeCase', (w*.72, d*.62, h*1.40), (0, 0, .12+h*1.40/2), material)
        _part(scene, 'WardrobeDoorInset', (w*.52, .035, h*1.12), (-.08*w, -.33*d, .12+h*.65), MAT_WOOD)
        _part(scene, 'ValetDrawer', (w*.56, .04, .16), (.05*w, -.34*d, .30), MAT_STEEL)
        _part(scene, 'ValetBenchSeat', (w*.82, d*.60, .16), (0, .18*d, .27), MAT_UPHOLSTERY)
        _floor_feet(scene, w, d, .20, 'ValetBenchLeg', MAT_WOOD)
        _part(scene, 'WardrobePull', (.035, .04, .24), (.25*w, -.37*d, .90), MAT_BRASS)
        _label(scene, 'RoomEntryCareTag', .31*w, -.34*d, .38)
    elif kind == 1:  # window banquette with side tables
        seat = .44 + .025*v
        _part(scene, 'BanquetteSeatBase', (w, d*.76, seat), (0, 0, seat/2), material)
        _part(scene, 'BanquetteCushion', (w*.92, d*.72, .13), (0, -.02, seat+.065), MAT_UPHOLSTERY)
        _part(scene, 'BanquetteBack', (w*.92, .12, .62), (0, d*.31, seat+.31), MAT_LINEN)
        for side, x in enumerate((-.44*w, .44*w)):
            _part(scene, f'BanquetteArm_{side}', (.13, d*.82, .50), (x, 0, .25), material)
            _part(scene, f'BanquetteBolster_{side}', (.12, .13, .48), (x, d*.20, seat+.25), MAT_UPHOLSTERY)
        _part(scene, 'WindowSeatToeKick', (w*.84, d*.62, .10), (0, 0, .05), MAT_STONE)
        _part(scene, 'ReadingOutletPlate', (.13, .025, .10), (.36*w, -.39*d, .55), MAT_ELECTRONICS)
        _label(scene, 'CushionCareLabel', -.37*w, -.39*d, .18)
    elif kind == 2:  # media credenza with floating screen frame
        top = .72 + .05*v
        _part(scene, 'MediaCredenzaCase', (w, d, top-.14), (0, 0, (top-.14)/2+.08), material)
        _part(scene, 'MediaCredenzaTop', (w*1.08, d*1.08, .10), (0, 0, top), MAT_WOOD)
        _part(scene, 'MediaPlinth', (w*.88, d*.84, .12), (0, .02, .06), MAT_STONE)
        for i in range(3):
            x = (i-1)*w*.28
            _part(scene, f'MediaCabinetDoor_{i}', (w*.24, .035, top*.55), (x, -.51*d, top*.43), MAT_WOOD)
            _part(scene, f'MediaDoorPull_{i}', (.035, .035, .16), (x+.07, -.54*d, top*.44), MAT_BRASS)
        _part(scene, 'ScreenUpright', (w*.62, .065, .72), (0, .28*d, top+.38), MAT_STEEL)
        _part(scene, 'ScreenPanel', (w*.54, .055, .48), (0, .23*d, top+.42), MAT_ELECTRONICS)
        _part(scene, 'Soundbar', (w*.40, .08, .08), (0, -.16*d, top+.10), MAT_STEEL)
        _label(scene, 'MediaInputGuide', .39*w, -.54*d, .24)
    elif kind == 3:  # two-sided dressing vanity
        top = .78 + .04*v
        _part(scene, 'VanityWorktop', (w, d*.86, .10), (0, 0, top), MAT_STONE)
        _part(scene, 'VanityApron', (w*.88, .09, .32), (0, .10*d, top-.20), material)
        _floor_feet(scene, w, d, top-.08, 'VanityPedestal', material)
        _part(scene, 'VanityMirrorFrame', (w*.62, .08, .72), (0, .32*d, top+.40), MAT_WOOD)
        _part(scene, 'VanityMirrorGlass', (w*.52, .035, .60), (0, .27*d, top+.42), MAT_GLASS)
        for side, x in enumerate((-.38*w, .38*w)):
            _part(scene, f'VanitySconce_{side}', (.10, .08, .28), (x, .24*d, top+.38), MAT_ELECTRONICS)
        _part(scene, 'HairToolDrawer', (w*.40, .07, .14), (0, -.43*d, top-.15), MAT_WOOD)
        _label(scene, 'VanityLoadLabel', -.38*w, -.46*d, .25)
    elif kind == 4:  # luggage niche and valet hook tower
        _part(scene, 'LuggageNicheBase', (w, d*.78, .14), (0, 0, .07), MAT_STONE)
        _part(scene, 'NicheBackPanel', (w*.86, .10, h*1.50), (0, .30*d, .14+h*1.50/2), material)
        for side, x in enumerate((-.43*w, .43*w)):
            _part(scene, f'NicheUpright_{side}', (.12, d*.82, h*1.55), (x, 0, .14+h*1.55/2), MAT_WOOD)
        for shelf, z in enumerate((.52, .98, 1.42)):
            _part(scene, f'LuggageNicheShelf_{shelf}', (w*.78, d*.76, .065), (0, 0, z), material)
        for i in range(3):
            x = (i-1)*w*.25
            _part(scene, f'ValetHook_{i}', (.10, .11, .05), (x, -.15, 1.76), MAT_BRASS)
        _label(scene, 'NicheWeightCard', .32*w, -.40*d, .24)
    elif kind == 5:  # wrapped lounge armchair
        seat = .48 + .02*v
        _floor_feet(scene, w*.82, d*.82, .42, 'LoungeChairFoot', MAT_WOOD)
        _part(scene, 'LoungeChairSeatShell', (w*.82, d*.82, .18), (0, 0, seat), material)
        _part(scene, 'LoungeChairSeatCushion', (w*.76, d*.72, .12), (0, -.02, seat+.14), MAT_UPHOLSTERY)
        _part(scene, 'LoungeChairBackShell', (w*.78, .16, .72), (0, d*.30, seat+.43), material)
        _part(scene, 'LoungeChairBackCushion', (w*.68, .08, .58), (0, d*.20, seat+.45), MAT_LINEN)
        for side, x in enumerate((-.44*w, .44*w)):
            _part(scene, f'LoungeChairArm_{side}', (.13, d*.80, .45), (x, 0, seat+.05), MAT_WOOD)
            _part(scene, f'ArmPad_{side}', (.15, d*.68, .07), (x, 0, seat+.29), MAT_UPHOLSTERY)
        _part(scene, 'LoungeChairLowerRail', (w*.68, .07, .09), (0, .22*d, .23), MAT_STEEL)
    elif kind == 6:  # room service writing bureau
        top = .77 + .04*v
        _part(scene, 'WritingBureauTop', (w, d, .11), (0, 0, top), MAT_WOOD)
        _floor_feet(scene, w*.90, d*.90, top-.08, 'WritingBureauLeg', material)
        _part(scene, 'BureauDrawerBox', (w*.84, .22, .24), (0, .18*d, top-.20), material)
        for i in range(3):
            x = (i-1)*w*.28
            _part(scene, f'BureauDrawerFace_{i}', (w*.24, .035, .11), (x, -.40*d, top-.17), MAT_WOOD)
            add(scene, cyl(.025, .04, (x, -.44*d, top-.17), MAT_BRASS, 10), f'BureauDrawerKnob_{i}')
        _part(scene, 'BureauBookShelf', (w*.78, .20, .065), (0, .24*d, top+.27), MAT_WOOD)
        _part(scene, 'BureauCableGrommet', (.07, .07, .03), (.37*w, -.38*d, top+.07), MAT_STEEL)
        _label(scene, 'BureauOutletLabel', -.37*w, -.40*d, .28)
    elif kind == 7:  # luggage bench with garment drop rail
        seat = .48 + .025*v
        _part(scene, 'LuggageBenchCushion', (w, d*.66, .15), (0, 0, seat), MAT_UPHOLSTERY)
        _part(scene, 'BenchApron', (w*.86, d*.58, .20), (0, .04, seat-.16), material)
        _floor_feet(scene, w*.94, d*.68, seat-.08, 'LuggageBenchLeg', MAT_STEEL)
        for side, x in enumerate((-.40*w, .40*w)):
            _part(scene, f'GarmentRailPost_{side}', (.055, .055, 1.18), (x, .28*d, .59), MAT_WOOD)
        _part(scene, 'GarmentRail', (w*.84, .06, .06), (0, .28*d, 1.19), MAT_BRASS)
        _part(scene, 'RailEndKnobLeft', (.10, .10, .10), (-.44*w, .28*d, 1.19), MAT_WOOD)
        _part(scene, 'RailEndKnobRight', (.10, .10, .10), (.44*w, .28*d, 1.19), MAT_WOOD)
        _label(scene, 'BenchCareInstruction', .39*w, -.36*d, .24)
    elif kind == 8:  # beverage and minibar credenza
        top = .92 + .04*v
        _part(scene, 'MinibarCredenzaCase', (w, d, top-.12), (0, 0, (top-.12)/2+.08), material)
        _part(scene, 'MinibarPlinth', (w*.90, d*.88, .16), (0, 0, .08), MAT_STONE)
        _part(scene, 'GlassDoorFrame', (w*.42, .045, .48), (-.24*w, -.51*d, .52), MAT_STEEL)
        _part(scene, 'MinibarGlassDoor', (w*.36, .025, .40), (-.24*w, -.54*d, .52), MAT_GLASS)
        _part(scene, 'TeaTrayShelf', (w*.48, d*.90, .075), (.23*w, 0, top+.06), MAT_WOOD)
        for i in range(3):
            x = (.08 + i*.13)*w
            add(scene, cyl(.045, .20, (x, -.05, top+.20), MAT_CERAMIC, 12), f'TeaCanister_{i}')
        _part(scene, 'CupRail', (w*.48, .04, .08), (.23*w, -.36*d, top+.10), MAT_BRASS)
        _label(scene, 'MinibarPriceGuide', .35*w, -.55*d, .24)
    else:  # folding privacy screen with weighted feet
        panel_h = 1.62 + .05*v
        for i, x in enumerate((-.42*w, -.14*w, .14*w, .42*w)):
            angle = (-1 if i % 2 == 0 else 1) * .12
            panel = box((w*.30, .055, panel_h), (x, 0, .12+panel_h/2), material)
            panel.apply_transform(trimesh.transformations.rotation_matrix(angle, [0, 0, 1], point=(x, 0, .12+panel_h/2)))
            add(scene, panel, f'PrivacyScreenPanel_{i}')
            _part(scene, f'ScreenFoot_{i}', (.32, .36, .12), (x, 0, .06), MAT_STONE)
        for i, x in enumerate((-.28*w, 0, .28*w)):
            _part(scene, f'PrivacyScreenHinge_{i}', (.045, .08, .15), (x, -.055, .64), MAT_BRASS)
        _part(scene, 'ScreenFabricInset', (w*.18, .025, panel_h*.62), (.14*w, -.034, .12+panel_h*.52), MAT_LINEN)
        _label(scene, 'ScreenStorageTag', .46*w, -.045, .24)
    return scene


def _public(kind, v, material):
    scene = trimesh.Scene()
    s = .88 + .06*v
    w, d, top = 1.10*s, .62*s, .94 + .06*v
    if kind == 0:  # concierge lectern with bag shelf
        _part(scene, 'ConciergeLecternPlinth', (w*.68, d*.68, .13), (0, 0, .065), MAT_STONE)
        _part(scene, 'LecternPedestal', (w*.62, d*.54, top-.12), (0, 0, (top-.12)/2+.12), material)
        _part(scene, 'ConciergeCountertop', (w, d, .12), (0, 0, top), MAT_WOOD)
        _part(scene, 'GuestWritingPad', (w*.58, d*.54, .025), (0, -.04, top+.075), MAT_LINEN)
        _part(scene, 'BagShelf', (w*.72, d*.48, .06), (0, .03, .28), MAT_STEEL)
        _part(scene, 'ServiceBellBase', (.14, .14, .035), (-.32*w, -.30*d, top+.09), MAT_BRASS)
        add(scene, sphere(.035, (-.32*w, -.30*d, top+.13), MAT_BRASS, subdivisions=1), 'ServiceBell')
        _part(scene, 'GuestServiceTablet', (.20, .035, .14), (.28*w, .22*d, top+.16), MAT_ELECTRONICS)
        _label(scene, 'ConciergeRolePlaque', .30*w, -.52*d, .30)
    elif kind == 1:  # lobby coat and umbrella tree
        _part(scene, 'CoatTreeBase', (.62*s, .62*s, .14), (0, 0, .07), MAT_STONE)
        _post(scene, 'CoatTreeColumn', 0, 0, 1.72+.08*v, .045, material)
        for level, z in enumerate((1.08, 1.35, 1.62)):
            for i in range(6 + v):
                angle = 2*math.pi*i/(6+v) + level*.26
                x, y = .22*s*math.cos(angle), .22*s*math.sin(angle)
                _part(scene, f'CoatHook_{level}_{i}', (.22, .045, .045), (x, y, z), MAT_BRASS)
        _part(scene, 'UmbrellaSleeveRing', (.42*s, .42*s, .08), (0, 0, .23), MAT_STEEL)
        _part(scene, 'DripTray', (.46*s, .46*s, .04), (0, 0, .16), MAT_CERAMIC)
        _label(scene, 'CoatTreeCarePlate', 0, -.29*s, .34)
    elif kind == 2:  # bell service cart
        _part(scene, 'BellCartLowerDeck', (w, d, .10), (0, 0, .34), MAT_BRASS)
        _part(scene, 'BellCartUpperDeck', (w*.92, d*.92, .09), (0, 0, .88), MAT_WOOD)
        _casters(scene, w, d, 'BellCartCaster')
        for i, (x, y) in enumerate(((-.43*w,-.40*d),(-.43*w,.40*d),(.43*w,-.40*d),(.43*w,.40*d))):
            _part(scene, f'BellCartUpright_{i}', (.045,.045,.98), (x,y,.66), MAT_BRASS)
        _part(scene, 'BellCartCanopyRim', (w*1.08,d*1.08,.08), (0,0,1.22), MAT_BRASS)
        _part(scene, 'BellCartTopCanopy', (w,d,.06), (0,0,1.28), MAT_LINEN)
        _part(scene, 'BellCartHandle', (.12,.10,.30), (0,.40*d,1.02), MAT_STEEL)
        _label(scene, 'BellCartLoadPlate', .34*w, -.52*d, .43)
    elif kind == 3:  # queue stanchion kit on shared rail base
        for i, x in enumerate((-.44*w, -.15*w, .15*w, .44*w)):
            _part(scene, f'QueuePostFoot_{i}', (.18,.18,.09), (x,0,.045), MAT_STONE)
            _post(scene, f'QueuePost_{i}', x, 0, 1.02+.04*v, .026, MAT_BRASS)
            _part(scene, f'QueuePostCap_{i}', (.10,.10,.07), (x,0,1.04+.04*v), MAT_STEEL)
        _part(scene, 'QueueGuideRail_A', (w*.30,.035,.045), (-.295*w,0,.88), MAT_LINEN)
        _part(scene, 'QueueGuideRail_B', (w*.30,.035,.045), (.295*w,0,.88), MAT_LINEN)
        _part(scene, 'QueueDirectionPanel', (.30,.04,.22), (0,-.08,.42), material)
        _label(scene, 'QueueCapacityCard', 0, -.11, .27)
    elif kind == 4:  # public washroom vanity console
        top = .86+.04*v
        _part(scene, 'PublicVanityCabinet', (w,d*.72,top-.16), (0,0,(top-.16)/2+.08), material)
        _part(scene, 'PublicVanityPlinth', (w*.90,d*.68,.16),(0,0,.08),MAT_STONE)
        _part(scene, 'PublicVanityCounter', (w*1.06,d*.80,.10),(0,0,top),MAT_STONE)
        add(scene, cyl(.25*s,.08,(0,-.02,top+.08),MAT_CERAMIC,20), 'PublicBasin')
        add(scene, cyl(.04,.25,(0,.20,top+.19),MAT_STAINLESS,12), 'PublicBasinFaucet')
        for side,x in enumerate((-.33*w,.33*w)):
            _part(scene,f'VanityMirrorPanel_{side}',(.34,.045,.62),(x,.34*d,top+.40),MAT_GLASS)
            _part(scene,f'MirrorSconce_{side}',(.08,.07,.30),(x,.28*d,top+.42),MAT_ELECTRONICS)
        _part(scene,'TowelShelf',(w*.80,.14,.06),(0,.30*d,.34),MAT_WOOD)
        _label(scene,'PublicFixtureCleaningCode',.39*w,-.40*d,.25)
    elif kind == 5:  # bottle refill and chilled water station
        _part(scene, 'WaterStationFoot', (.70*s,.66*s,.14),(0,0,.07),MAT_STONE)
        _part(scene, 'WaterStationPedestal', (.62*s,.56*s,.85+.04*v),(0,0,.55),material)
        _part(scene, 'WaterFillTray', (.80*s,.48*s,.09),(0,-.02,.99+.04*v),MAT_STAINLESS)
        _part(scene, 'BottleFillNiche', (.34*s,.20,.38),(0,-.18,.98+.04*v),MAT_STEEL)
        _part(scene, 'BottleSensor', (.10,.08,.10),(0,-.30,1.02+.04*v),MAT_ELECTRONICS)
        _part(scene, 'StatusScreen', (.22,.035,.15),(.26*s,-.29,1.18+.04*v),MAT_ELECTRONICS)
        _part(scene, 'WaterFilterAccess', (.34*s,.035,.34),(0,-.29,.53),MAT_STEEL)
        _label(scene, 'WaterFilterDateTag',-.25*s,-.32,.40)
    elif kind == 6:  # guest parcel and key locker wall
        rows, cols = 3, 3+v//2
        locker_w, locker_h = .30*s, .46*s
        overall_w = cols*locker_w
        _part(scene, 'LockerToeKick',(overall_w+.10,.58,.14),(0,0,.07),MAT_STONE)
        for r in range(rows):
            for c in range(cols):
                x=(c-(cols-1)/2)*(locker_w+.018)
                z=.14+locker_h/2+r*(locker_h+.025)
                _part(scene,f'ParcelLockerDoor_{r}_{c}',(locker_w,.045,locker_h),(x,-.30,z),material)
                _part(scene,f'LockerKeypad_{r}_{c}',(.08,.035,.11),(x+.09,-.34,z-.06),MAT_ELECTRONICS)
                _part(scene,f'LockerPull_{r}_{c}',(.04,.03,.08),(x-.10,-.34,z-.02),MAT_BRASS)
        _part(scene,'LockerTopCap',(overall_w+.10,.62,.10),(0,0,.14+rows*(locker_h+.025)),MAT_WOOD)
        _label(scene,'ParcelPickupInstruction',0,-.34,.30)
    elif kind == 7:  # quiet lobby writing table with bench stools
        top=.76+.035*v
        _part(scene,'WritingTabletop',(w,d,.10),(0,0,top),material)
        _floor_feet(scene,w*.90,d*.86,top-.08,'WritingTableLeg',MAT_STEEL)
        _part(scene,'TablePrivacyRail',(w*.75,.06,.22),(0,.18*d,top-.18),MAT_WOOD)
        for side,x in enumerate((-.40*w,.40*w)):
            _part(scene,f'PowerGrommet_{side}',(.08,.08,.035),(x,-.30*d,top+.07),MAT_ELECTRONICS)
        for side,y in enumerate((-.62*d,.62*d)):
            _part(scene,f'WritingBenchSeat_{side}',(w*.80,.22,.12),(0,y,.45),MAT_UPHOLSTERY)
            _floor_feet(scene,w*.72,.20,.39,f'WritingBenchFoot_{side}',MAT_WOOD)
        _label(scene,'QuietZoneNotice',.38*w,-.52*d,.23)
    elif kind == 8:  # self-service check-in kiosk
        _part(scene,'KioskBasePlate',(.66*s,.58*s,.13),(0,0,.065),MAT_STONE)
        _part(scene,'KioskPedestal',(.44*s,.40*s,1.10+.04*v),(0,0,.68),material)
        _part(scene,'KioskDisplayFrame',(.70*s,.10,.56),(0,-.23*s,1.34+.04*v),MAT_STEEL)
        _part(scene,'KioskTouchscreen',(.58*s,.045,.42),(0,-.285*s,1.36+.04*v),MAT_ELECTRONICS)
        _part(scene,'KioskCardReader',(.15,.08,.09),(.21*s,-.30*s,1.00),MAT_STAINLESS)
        _part(scene,'KioskReceiptSlot',(.18,.04,.045),(-.16*s,-.31*s,.89),MAT_BLACKENED_STEEL)
        _part(scene,'KioskPrivacyWing',(.08,.50,.36),(.35*s,0,1.18),MAT_WOOD)
        _label(scene,'KioskAccessibilityMark',-.24*s,-.30*s,.50)
    else:  # umbrella drying and return rack
        _part(scene,'UmbrellaRackBase',(.68*s,.52*s,.16),(0,0,.08),MAT_STONE)
        _part(scene,'UmbrellaRackTray',(.56*s,.42*s,.10),(0,0,.19),MAT_STAINLESS)
        for i in range(5+v):
            x=(i-(4+v)/2)*.14*s
            _post(scene,f'UmbrellaSleeve_{i}',x,0,.86+.04*v,.028,MAT_STEEL)
            _part(scene,f'SleeveCollar_{i}',(.09,.09,.08),(x,0,.82+.04*v),MAT_BRASS)
        _part(scene,'DripTraySpout',(.06,.16,.06),(.27*s,.20,.18),MAT_STAINLESS)
        _part(scene,'UmbrellaReturnBin',(.54*s,.28,.24),(0,.27,.33),material)
        _part(scene,'ReturnBinOpening',(.42*s,.05,.045),(0,.12,.46),MAT_BLACKENED_STEEL)
        _label(scene,'UmbrellaReturnGuide',0,-.27,.33)
    return scene


def _restaurant(kind, v, material):
    scene=trimesh.Scene()
    s=.88+.06*v
    w,d=1.05*s,.62*s
    top=.88+.05*v
    if kind == 0:  # bakery pastry display counter
        _part(scene,'PastryCounterBase',(w,d,.12),(0,0,.06),MAT_STONE)
        _part(scene,'PastryDisplayPedestal',(w*.88,d*.78,top-.12),(0,0,(top-.12)/2+.12),material)
        _part(scene,'GlassDisplayFront',(w*.90,.035,.46),(0,-.24*d,top+.24),MAT_GLASS)
        _part(scene,'GlassDisplayBack',(w*.90,.035,.46),(0,.24*d,top+.24),MAT_GLASS)
        _part(scene,'DisplayGlassTop',(w*.92,d*.54,.04),(0,0,top+.49),MAT_GLASS)
        for i in range(3+v//2):
            z=top+.10+i*.12
            _part(scene,f'PastryShelf_{i}',(w*.80,d*.60,.045),(0,0,z),MAT_STAINLESS)
            for j in range(2+v//2):
                x=(j-(1+v//2)/2)*w*.48/(1+v//2)
                add(scene,sphere(.055*s,(x,-.06,z+.08),MAT_CERAMIC,subdivisions=1),f'PastrySample_{i}_{j}')
        _label(scene,'PastryAllergenCard',.37*w,-.49*d,.30)
    elif kind == 1:  # carving and roast station
        _part(scene,'CarvingStationPlinth',(w,d,.14),(0,0,.07),MAT_STEEL)
        _part(scene,'CarvingCounter',(w,d,.10),(0,0,top),material)
        _floor_feet(scene,w*.92,d*.90,top-.08,'CarvingStationFoot',MAT_STEEL)
        _part(scene,'HeatLampBridge',(w*.72,.08,.07),(0,.24*d,top+.61),MAT_BRASS)
        for side,x in enumerate((-.30*w,.30*w)):
            _part(scene,f'HeatLampStem_{side}',(.045,.045,.54),(x,.24*d,top+.30),MAT_STEEL)
            add(scene,cyl(.12,.07,(x,.24*d,top+.58),MAT_ELECTRONICS,16),f'HeatLamp_{side}')
        _part(scene,'CarvingBoard',(w*.48,d*.45,.06),(0,-.12*d,top+.08),MAT_WOOD)
        _part(scene,'KnifeSanitizerDock',(.20,.16,.22),(.38*w,-.10*d,top+.17),MAT_STAINLESS)
        _label(scene,'CarvingTemperatureGuide',-.37*w,-.50*d,.24)
    elif kind == 2:  # cocktail garnish rail and ice well
        _part(scene,'GarnishRailBase',(w,d*.62,.12),(0,0,.06),MAT_STONE)
        _part(scene,'GarnishCounter',(w,d*.62,.09),(0,0,top),material)
        _floor_feet(scene,w*.90,d*.62,top-.07,'GarnishStationLeg',MAT_STEEL)
        for i in range(4+v):
            x=(i-(3+v)/2)*w*.74/(3+v)
            _part(scene,f'GarnishPan_{i}',(w*.16,d*.34,.13),(x,0,top+.12),MAT_STAINLESS)
            _part(scene,f'GarnishPanLid_{i}',(w*.16,d*.34,.025),(x,0,top+.20),MAT_STEEL)
        _part(scene,'IceWell',(w*.27,d*.28,.20),(.34*w,.02,top+.13),MAT_STAINLESS)
        _part(scene,'BottleSpeedRail',(w*.82,.09,.10),(0,.30*d,top+.25),MAT_BRASS)
        _label(scene,'GarnishRotationTag',-.36*w,-.35*d,.25)
    elif kind == 3:  # plate warmer tower
        _part(scene,'PlateWarmerBase',(.72*s,.68*s,.14),(0,0,.07),MAT_STONE)
        _part(scene,'PlateWarmerCabinet',(.62*s,.56*s,1.02+.05*v),(0,0,.65),material)
        for i in range(3+v//2):
            z=.42+i*.24
            _part(scene,f'WarmerDrawerFace_{i}',(.52*s,.045,.18),(0,-.29*s,z),MAT_STAINLESS)
            _part(scene,f'WarmerDrawerHandle_{i}',(.18,.055,.045),(0,-.33*s,z),MAT_BRASS)
        _part(scene,'WarmerTopInsulation',(.66*s,.60*s,.08),(0,0,1.22+.05*v),MAT_STEEL)
        _part(scene,'TemperatureController',(.20,.04,.14),(.18*s,-.31*s,1.04),MAT_ELECTRONICS)
        _part(scene,'PlateStackTop',(.32*s,.32*s,.16),(0,0,1.36+.05*v),MAT_CERAMIC)
        _label(scene,'WarmerServiceTag',-.22*s,-.31*s,.32)
    elif kind == 4:  # tea infusion trolley
        _casters(scene,w,d,'TeaTrolleyCaster')
        _part(scene,'TeaTrolleyLowerShelf',(w,d,.08),(0,0,.30),MAT_WOOD)
        _part(scene,'TeaTrolleyUpperShelf',(w*.96,d*.92,.09),(0,0,.79),material)
        _part(scene,'TeaTrolleyTray',(w*.82,d*.74,.035),(0,0,.86),MAT_STAINLESS)
        for i,(x,y) in enumerate(((-.42*w,-.36*d),(-.42*w,.36*d),(.42*w,-.36*d),(.42*w,.36*d))):
            _part(scene,f'TeaTrolleyPost_{i}',(.045,.045,.92),(x,y,.68),MAT_STEEL)
        for i,x in enumerate((-.25*w,0,.25*w)):
            add(scene,cyl(.10,.20,(x,0,.98),MAT_CERAMIC,16),f'TeaCanister_{i}')
            _part(scene,f'TeaCanisterLid_{i}',(.10,.10,.045),(x,0,1.11),MAT_BRASS)
        _part(scene,'TeaTrolleyHandle',(.10,.08,.24),(0,.42*d,.96),MAT_WOOD)
        _label(scene,'TeaAllergenAndSteepCard',.34*w,-.51*d,.43)
    elif kind == 5:  # chilled oyster and seafood presentation
        _part(scene,'SeafoodDisplayBase',(w,d,.13),(0,0,.065),MAT_STONE)
        _part(scene,'IceBedTray',(w*.88,d*.72,.18),(0,0,.22),MAT_STAINLESS)
        _part(scene,'DisplayIceBed',(w*.80,d*.64,.12),(0,-.01,.36),MAT_GLASS)
        for i in range(3+v//2):
            x=(i-(2+v//2)/2)*w*.50/(2+v//2)
            add(scene,cyl(.11*s,.055,(x,0,.45),MAT_CERAMIC,16),f'OysterPlatter_{i}')
            for j in range(4+v):
                angle=2*math.pi*j/(4+v)
                px=x+.065*math.cos(angle); py=.06*math.sin(angle)
                add(scene,sphere(.028*s,(px,py,.50),MAT_STONE,subdivisions=1),f'OysterShell_{i}_{j}')
        _part(scene,'DisplaySneezeGuard',(w*.92,.035,.40),(0,.23*d,.58),MAT_GLASS)
        _part(scene,'GuardSupportLeft',(.035,.10,.34),(-.43*w,.23*d,.53),MAT_STEEL)
        _part(scene,'GuardSupportRight',(.035,.10,.34),(.43*w,.23*d,.53),MAT_STEEL)
        _label(scene,'SeafoodOriginCard',.34*w,-.50*d,.22)
    elif kind == 6:  # bread and cutlery stand
        _part(scene,'BreadStandBase',(w*.72,d*.72,.12),(0,0,.06),MAT_STONE)
        _floor_feet(scene,w*.68,d*.64,.94,'BreadStandLeg',MAT_WOOD)
        for level,z in enumerate((.48,.82,1.16)):
            _part(scene,f'BreadBasketShelf_{level}',(w*.82,d*.62,.075),(0,0,z),material)
            _part(scene,f'BasketFrontRail_{level}',(w*.82,.04,.12),(0,-.30*d,z+.08),MAT_BRASS)
        _part(scene,'CutleryCupLeft',(.18,.18,.27),(-.28*w,0,1.36),MAT_STAINLESS)
        _part(scene,'CutleryCupRight',(.18,.18,.27),(.28*w,0,1.36),MAT_STAINLESS)
        for i in range(3):
            _part(scene,f'BreadKnifeSlot_{i}',(.10,.12,.08),((i-1)*.18, -.12, .31),MAT_STEEL)
        _label(scene,'BreadHandlingGuide',0,-.36*d,.24)
    elif kind == 7:  # beverage carbonation and syrup tower
        _part(scene,'BeverageTowerPlinth',(.66*s,.62*s,.14),(0,0,.07),MAT_STONE)
        _part(scene,'BeverageTowerCabinet',(.56*s,.52*s,.88+.04*v),(0,0,.58),material)
        _part(scene,'CarbonationCanister',( .23*s,.23*s,.64),(-.18*s,.03,.35),MAT_STAINLESS)
        _part(scene,'SyrupCanister',( .22*s,.22*s,.57),(.18*s,.03,.31),MAT_STEEL)
        _part(scene,'DispenseHead',( .48*s,.12,.22),(0,-.22*s,1.13),MAT_BLACKENED_STEEL)
        for i in range(3+v//2):
            x=(i-(2+v//2)/2)*.14*s
            _part(scene,f'BeverageValve_{i}',(.06,.10,.14),(x,-.30*s,1.10),MAT_BRASS)
            add(scene,cyl(.025,.08,(x,-.33*s,.89),MAT_STAINLESS,10),f'BeverageNozzle_{i}')
        _part(scene,'CupRestGrate',(.50*s,.28,.05),(0,-.15*s,.82),MAT_STAINLESS)
        _label(scene,'CarbonationPressureTag',.28*s,-.28*s,.42)
    elif kind == 8:  # return-tray counter with waste-sort openings
        _part(scene,'TrayReturnCounterBase',(w,d,.12),(0,0,.06),MAT_STONE)
        _part(scene,'TrayReturnCabinet',(w,d*.84,top-.12),(0,0,(top-.12)/2+.12),material)
        _part(scene,'ReturnCountertop',(w*1.06,d,.10),(0,0,top),MAT_STAINLESS)
        for i in range(3):
            x=(i-1)*w*.28
            _part(scene,f'WasteSortOpening_{i}',(w*.20,d*.32,.035),(x,-.08,top+.055),MAT_BLACKENED_STEEL)
            _part(scene,f'WasteCategoryTab_{i}',(w*.20,.035,.08),(x,-.50*d,top+.10),MAT_LABEL)
        _part(scene,'TraySlideRailLeft',(.07,d*.72,.12),(-.48*w,.03,top+.14),MAT_BRASS)
        _part(scene,'TraySlideRailRight',(.07,d*.72,.12),(.48*w,.03,top+.14),MAT_BRASS)
        _part(scene,'TrayStack',(.42*w,.32,.16),(0,.12,top+.20),MAT_STEEL)
        _label(scene,'ReturnSanitationNotice',.38*w,-.52*d,.28)
    else:  # sommelier decanting cart
        _casters(scene,w,d,'SommelierCartCaster')
        _part(scene,'DecantingCartLowerShelf',(w,d,.08),(0,0,.32),MAT_WOOD)
        _part(scene,'DecantingCartTop',(w*1.02,d,.11),(0,0,.88),material)
        for i,(x,y) in enumerate(((-.42*w,-.38*d),(-.42*w,.38*d),(.42*w,-.38*d),(.42*w,.38*d))):
            _part(scene,f'DecantingCartUpright_{i}',(.05,.05,.82),(x,y,.65),MAT_BRASS)
        _part(scene,'DecantingGlassRack',(w*.70,.12,.09),(0,.22*d,1.03),MAT_STEEL)
        for i in range(3+v//2):
            x=(i-(2+v//2)/2)*w*.46/(2+v//2)
            add(scene,cyl(.065,.24,(x,-.10*d,1.06),MAT_GLASS,14),f'Decanter_{i}')
        _part(scene,'BottleCradle',(w*.76,.20,.08),(0,-.22*d,1.00),MAT_WOOD)
        _label(scene,'SommelierCartServiceTag',.35*w,-.51*d,.48)
    return scene


def _housekeeping(kind, v, material):
    scene=trimesh.Scene()
    s=.88+.06*v
    w,d=.88*s,.56*s
    if kind == 0:  # tall linen issue trolley
        _casters(scene,w,d,'LinenIssueCaster')
        _part(scene,'LinenTrolleyLowerDeck',(w,d,.09),(0,0,.20),MAT_STEEL)
        _part(scene,'CleanLinenShelf',(w*.92,d*.88,.08),(0,0,.56),MAT_LINEN)
        _part(scene,'TopLinenShelf',(w*.92,d*.88,.08),(0,0,.98),MAT_WOOD)
        for i,(x,y) in enumerate(((-.43*w,-.40*d),(-.43*w,.40*d),(.43*w,-.40*d),(.43*w,.40*d))):
            _part(scene,f'LinenTrolleyUpright_{i}',(.05,.05,1.06),(x,y,.74),MAT_STEEL)
        _part(scene,'LinenRetainingRail',(w*.86,.045,.08),(0,-.40*d,1.09),MAT_BRASS)
        _part(scene,'LinenTrolleyHandle',(.08,.08,.24),(0,.42*d,1.12),MAT_WOOD)
        _label(scene,'CleanLinenRouteTag',.35*w,-.49*d,.37)
    elif kind == 1:  # in-room amenities refill cart
        _casters(scene,w,d,'RefillCartCaster')
        _part(scene,'RefillCartLowerShelf',(w,d,.09),(0,0,.24),MAT_STEEL)
        _part(scene,'RefillCartUpperShelf',(w*.92,d*.90,.08),(0,0,.79),MAT_WOOD)
        _part(scene,'AmenityBinLeft',(.34*w,d*.72,.32),(-.27*w,0,.49),material)
        _part(scene,'AmenityBinRight',(.34*w,d*.72,.32),(.27*w,0,.49),MAT_LINEN)
        for i in range(3):
            x=(i-1)*.22*w
            _part(scene,f'RefillBottle_{i}',(.09,.10,.25),(x,.02,.98),MAT_CERAMIC)
            _part(scene,f'BottlePump_{i}',(.035,.035,.08),(x,.02,1.14),MAT_STEEL)
        _part(scene,'RefillCartHandle',(.08,.08,.26),(0,.40*d,1.08),MAT_WOOD)
        _label(scene,'RefillParLevelCard',.35*w,-.50*d,.34)
    elif kind == 2:  # compact carpet extractor
        _part(scene,'ExtractorBaseSkid',(.72*s,.54*s,.12),(0,0,.06),MAT_STEEL)
        _part(scene,'ExtractorRecoveryTank',(.40*s,.46*s,.55),(-.12*s,.02,.40),MAT_ELECTRONICS)
        _part(scene,'ExtractorSolutionTank',(.25*s,.40*s,.43),(.22*s,.01,.34),MAT_CERAMIC)
        _casters(scene,.72*s,.54*s,'ExtractorWheel')
        _part(scene,'ExtractorHandleStem',(.06,.06,.74),(-.20*s,.18*s,.49),MAT_STEEL)
        _part(scene,'ExtractorHandleGrip',(.42,.06,.07),(-.20*s,.18*s,.88),MAT_WOOD)
        _part(scene,'ExtractorSuctionNozzle',(.66*s,.16,.15),(0,-.27*s,.17),MAT_BLACKENED_STEEL)
        _part(scene,'ExtractorHose',( .06,.34,.44),(.29*s,.08,.51),MAT_STEEL)
        _label(scene,'ExtractorDilutionLabel',.20*s,-.24*s,.47)
    elif kind == 3:  # mop wringer and bucket cart
        _casters(scene,w,d,'MopCartCaster')
        _part(scene,'MopCartLowerFrame',(w,d,.08),(0,0,.24),MAT_STEEL)
        _part(scene,'MopBucket',(w*.48,d*.66,.36),(-.18*w,0,.46),MAT_CERAMIC)
        _part(scene,'MopWringerFrame',(.26*w,d*.66,.42),(.25*w,0,.58),MAT_STEEL)
        _part(scene,'MopWringerLever',(.07,.10,.44),(.30*w,.02,.80),MAT_BRASS)
        _part(scene,'MopPressPlate',(.22*w,.18,.08),(.25*w,-.02,.52),MAT_BLACKENED_STEEL)
        _part(scene,'MopHandleSocket',(.12,.12,.18),(.42*w,.10,.35),MAT_STEEL)
        _part(scene,'MopHandle',(.045,.045,1.20),(.42*w,.10,.95),MAT_WOOD)
        _part(scene,'MopHead',(.34,.11,.18),(.42*w,.10,.33),MAT_LINEN)
        _label(scene,'WetFloorProcedure',.33*w,-.51*d,.29)
    elif kind == 4:  # segregated linen hamper stand
        _casters(scene,w,d,'HamperCaster')
        _part(scene,'HamperBaseDeck',(w,d,.08),(0,0,.18),MAT_STEEL)
        for i,x in enumerate((-.28*w,0,.28*w)):
            _part(scene,f'HamperBag_{i}',(w*.25,d*.70,.70),(x,0,.57),MAT_LINEN)
            _part(scene,f'HamperBagRim_{i}',(w*.28,d*.72,.08),(x,0,.95),MAT_STEEL)
            _part(scene,f'HamperBagLabel_{i}',(w*.18,.025,.10),(x,-.36*d,.67),MAT_LABEL)
        for side,x in enumerate((-.46*w,.46*w)):
            _part(scene,f'HamperFrameUpright_{side}',(.05,.05,.92),(x,0,.64),MAT_STEEL)
        _part(scene,'HamperPushBar',(w*.90,.06,.07),(0,.34*d,1.08),MAT_WOOD)
        _label(scene,'LinenSegregationNotice',0,-.39*d,.25)
    elif kind == 5:  # chemical dilution and eye-wash station
        _part(scene,'DilutionStationFoot',(.76*s,.60*s,.14),(0,0,.07),MAT_STONE)
        _part(scene,'DilutionStationBackboard',(.68*s,.12,1.20),(0,.22*s,.74),material)
        _part(scene,'ChemicalMixShelf',(.72*s,.38,.09),(0,0,.44),MAT_STEEL)
        for i in range(4+v//2):
            x=(i-(3+v//2)/2)*.14*s
            _part(scene,f'ChemicalBottle_{i}',(.10,.12,.30),(x,-.04,.64),MAT_CERAMIC)
            _part(scene,f'ChemicalBottleCap_{i}',(.07,.07,.07),(x,-.04,.83),MAT_BRASS)
        _part(scene,'DilutionControlBox',(.24,.12,.27),(.24*s,-.12,.98),MAT_ELECTRONICS)
        _part(scene,'EyeWashBowl',(.26,.20,.08),(-.25*s,-.05,1.07),MAT_STAINLESS)
        for side,x in enumerate((-.32*s,-.18*s)):
            add(scene,cyl(.022,.14,(x,-.05,1.19),MAT_STAINLESS,10),f'EyeWashNozzle_{side}')
        _label(scene,'ChemicalPPEInstruction',.28*s,-.13,.50)
    elif kind == 6:  # lost property sorting cabinet
        _part(scene,'LostPropertyPlinth',(w,d*.82,.14),(0,0,.07),MAT_STONE)
        _part(scene,'SortingCabinetCase',(w,d*.82,1.05+.05*v),(0,0,.67),material)
        for i in range(4):
            x=(i-1.5)*w*.22
            _part(scene,f'SortingCubby_{i}',(w*.19,.04,.38),(x,-.43*d,.67),MAT_WOOD)
            _part(scene,f'PropertyBag_{i}',(.13,.16,.15),(x,.05,.36),MAT_LINEN)
        _part(scene,'CaseworkTop',(w*1.06,d*.88,.09),(0,0,1.24+.05*v),MAT_WOOD)
        _part(scene,'SecureDrawer',(w*.86,.05,.18),(0,-.44*d,.27),MAT_STEEL)
        _part(scene,'DrawerKeyLock',(.08,.035,.08),(.32*w,-.48*d,.27),MAT_BRASS)
        _label(scene,'PropertyIntakeCard',-.34*w,-.47*d,.98)
    elif kind == 7:  # laundry folding and inspection table
        top=.84+.035*v
        _part(scene,'FoldingTableTop',(w,d,.12),(0,0,top),MAT_WOOD)
        _floor_feet(scene,w*.92,d*.88,top-.10,'FoldingTableLeg',MAT_STEEL)
        _part(scene,'InspectionLightRail',(w*.78,.06,.06),(0,.28*d,top+.42),MAT_STEEL)
        for side,x in enumerate((-.32*w,.32*w)):
            _part(scene,f'LightRailUpright_{side}',(.045,.045,.40),(x,.28*d,top+.20),MAT_BRASS)
            _part(scene,f'InspectionLight_{side}',(.22,.08,.055),(x,0,top+.36),MAT_ELECTRONICS)
        for i in range(3):
            x=(i-1)*w*.28
            _part(scene,f'FoldGuideLine_{i}',(.025,d*.72,.012),(x,0,top+.066),MAT_LABEL)
        _part(scene,'LintBrushDock',(.16,.10,.08),(.40*w,.20*d,top+.12),MAT_STEEL)
        _label(scene,'LaundryInspectionStandard',-.36*w,-.52*d,.27)
    else:  # window and glass cleaning pole/caddy stand
        _part(scene,'GlassCaddyBase',(.70*s,.50*s,.13),(0,0,.065),MAT_STONE)
        _part(scene,'CaddyReservoir',(.46*s,.36*s,.38),(0,0,.32),MAT_CERAMIC)
        _part(scene,'CaddyToolBoard',(.62*s,.08,1.08),(0,.18*s,.73),material)
        for i,x in enumerate((-.20*s,0,.20*s)):
            _part(scene,f'SqueegeeHook_{i}',(.16,.06,.06),(x,.10,.62),MAT_BRASS)
            _part(scene,f'SqueegeeHandle_{i}',(.035,.035,.66),(x,.10,1.00),MAT_STEEL)
            _part(scene,f'SqueegeeBlade_{i}',(.22,.035,.06),(x,.10,1.31),MAT_BLACKENED_STEEL)
        _part(scene,'ExtensionPoleClip',(.12,.08,.12),(.27*s,.12,.35),MAT_STEEL)
        _part(scene,'WindowMopPad',(.24,.09,.13),(-.27*s,.10,.33),MAT_LINEN)
        _label(scene,'GlassCareDilutionCard',0,-.28*s,.22)
    return scene


def _finish_system(v, material):
    scene=trimesh.Scene()
    w=1.10+.09*v
    d=.46+.04*v
    _part(scene,'TransitionSubstrate',(w,d,.12),(0,0,.06),MAT_STEEL)
    _part(scene,'StoneThresholdCap',(w*.98,d*.46,.08),(0,-.24*d,.16),material)
    _part(scene,'CarpetTileLanding',(w*.98,d*.47,.055),(0,.24*d,.148),MAT_LINEN)
    _part(scene,'BrassDividerStrip',(w*.98,.035,.055),(0,0,.158),MAT_BRASS)
    _part(scene,'AntiSlipInlay',(w*.82,.025,.015),(0,-.24*d,.208),MAT_BLACKENED_STEEL)
    for i,x in enumerate((-.40*w,0,.40*w)):
        add(scene,cyl(.025,.08,(x,0,.13),MAT_STEEL,10),f'AnchoringPin_{i}')
    _part(scene,'ExpansionGapBacking',(w*.86,.025,.045),(0,0,.11),MAT_CERAMIC)
    _label(scene,'ThresholdAccessibilityMark',.34*w,-.48*d,.22)
    return scene


def build_asset(name, subcategory, material, asset_id, profile):
    try:
        number=int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'hotel continuation asset ID outside A3001–A3250: {asset_id}') from exc
    if not 3001 <= number <= 3250:
        raise ValueError(f'hotel continuation asset ID outside A3001–A3250: {asset_id}')
    offset=number-3001
    batch,within_batch=divmod(offset,50)
    kind,variant=divmod(within_batch,5)
    if batch == 0:
        return _architecture(kind,variant,material)
    if batch == 1:
        return _guestroom(kind,variant,material)
    if batch == 2:
        return _public(kind,variant,material)
    if batch == 3:
        return _restaurant(kind,variant,material)
    if within_batch < 45:
        return _housekeeping(kind,variant,material)
    return _finish_system(variant,material)
