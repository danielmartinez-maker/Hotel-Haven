"""Purpose-built design, guestroom, public, amenity, and decor assets A3251–A3500."""

import math

import trimesh

from hotel_continuation_factory import (
    MAT_BRASS, MAT_CERAMIC, MAT_ELECTRONICS, MAT_GLASS, MAT_LABEL,
    MAT_LINEN, MAT_STAINLESS, MAT_STEEL, MAT_STONE, MAT_UPHOLSTERY,
    MAT_WOOD, _casters, _floor_feet, _label, _part, _post,
)
from v2_asset_common import add, box, cyl, sphere


def _architecture(kind, v, material):
    scene=trimesh.Scene(); s=.90+.05*v; w=1.34*s; d=.84*s; h=2.52+.09*v
    if kind == 0:  # roof mechanical screen bay
        _part(scene,'RoofScreenFooting',(w,.34,.14),(0,0,.07),MAT_STONE)
        _part(scene,'RoofScreenBackPanel',(w,.12,h*.78),(0,.15,h*.39),material)
        for side,x in enumerate((-.47*w,.47*w)):
            _part(scene,f'ScreenEndFrame_{side}',(.10,.18,h),(x,0,h/2),MAT_STEEL)
        _part(scene,'ScreenHeadRail',(w,.16,.12),(0,0,h-.06),MAT_BRASS)
        for i in range(5+v):
            x=(i-(4+v)/2)*w*.82/(4+v)
            _part(scene,f'MechanicalLouver_{i}',(.075,.24,.22),(x,-.08,1.22+.20*(i%3)),material)
        _part(scene,'ScreenAccessHatch',(.25,.045,.34),(.30*w,-.10,1.90),MAT_STEEL)
        _label(scene,'RoofScreenWindRating',-.33*w,-.12,.25)
    elif kind == 1:  # atrium ceiling cassette
        z=2.82+.07*v; w=1.50*s; d=.94*s
        _part(scene,'AtriumCassetteCore',(w,d,.14),(0,0,z),MAT_LINEN)
        for side,y in enumerate((-.48*d,.48*d)):
            _part(scene,f'CassettePerimeter_{side}',(w*1.04,.10,.13),(0,y,z+.02),material)
        for i in range(5+v):
            x=(i-(4+v)/2)*w*.84/(4+v)
            _part(scene,f'CeilingBaffle_{i}',(.055,d*.82,.09),(x,0,z-.13),MAT_WOOD)
            _part(scene,f'CassetteHanger_{i}',(.025,.025,.42),(x,.25*d,z-.37),MAT_STEEL)
        _part(scene,'AtriumLinearLuminaire',(w*.54,.065,.045),(0,-.12,z+.10),MAT_ELECTRONICS)
        _part(scene,'CassetteServicePanel',(.24,.20,.035),(.36*w,.24*d,z+.08),MAT_STAINLESS)
        _label(scene,'CeilingCircuitMarker',.40*w,-.52*d,z-.16)
    elif kind == 2:  # stone portal reveal
        w=1.22*s; h=2.55+.08*v
        for side,x in enumerate((-.43*w,.43*w)):
            _part(scene,f'PortalStoneJamb_{side}',(.19,.22,h),(x,0,h/2),material)
            _part(scene,f'PortalShadowReveal_{side}',(.035,.25,h*.92),(x-.08, -.03,h/2),MAT_STEEL)
        _part(scene,'PortalStoneHead',(w,.22,.22),(0,0,h-.11),material)
        _part(scene,'PortalInnerBand',(w*.72,.08,.065),(0,-.13,h-.24),MAT_BRASS)
        _part(scene,'PortalThreshold',(w,.44,.13),(0,-.06,.065),MAT_STONE)
        _part(scene,'PortalFloorInlay',(w*.66,.07,.025),(0,-.29,.14),MAT_CERAMIC)
        _label(scene,'PortalRoomDirectory',.31*w,-.14,1.72)
        _part(scene,'PortalConcealedCloser',(.22,.08,.10),(0,.12,h-.04),MAT_STEEL)
    elif kind == 3:  # seismic beam collector node
        column_h=1.82+.06*v; beam_w=1.45*s
        _part(scene,'CollectorColumnShaft',(.28*s,.30*s,column_h),(0,0,column_h/2),material)
        _part(scene,'CollectorBasePlate',(.62*s,.62*s,.16),(0,0,.08),MAT_STEEL)
        _part(scene,'CollectorCapPlate',(.44*s,.46*s,.13),(0,0,column_h+.065),MAT_STAINLESS)
        _part(scene,'TransferBeam',(beam_w,.28,.24),(0,0,column_h+.22),MAT_STEEL)
        for side,x in enumerate((-.28*beam_w,.28*beam_w)):
            _part(scene,f'CollectorGusset_{side}',(.30,.12,.42),(x,-.18,column_h+.16),MAT_BRASS)
            for i in range(3):
                add(scene,cyl(.028,.06,(x+(i-1)*.08,-.26,column_h+.12),MAT_STAINLESS,10),f'CollectorBolt_{side}_{i}')
        _part(scene,'AnchorRodCover',(.18,.18,.08),(0,0,.20),MAT_STONE)
        _label(scene,'SeismicInspectionPlate',.30*beam_w,-.17,column_h-.22)
    elif kind == 4:  # smoke control louver wall
        h=2.35+.08*v; w=1.42*s
        _part(scene,'SmokeLouverBacking',(w,.12,h),(0,0,h/2),MAT_STONE)
        _part(scene,'SmokeLouverSill',(w*1.05,.18,.14),(0,0,.07),MAT_STEEL)
        _part(scene,'SmokeLouverHeader',(w*1.05,.18,.15),(0,0,h-.075),material)
        for side,x in enumerate((-.46*w,.46*w)):
            _part(scene,f'LouverSideChannel_{side}',(.10,.18,h),(x,0,h/2),MAT_STAINLESS)
        for i in range(6+v):
            z=.30+i*.25
            _part(scene,f'SmokeLouverBlade_{i}',(w*.82,.23,.075),(0,-.08,z),material)
        _part(scene,'LouverActuatorBox',(.26,.12,.30),(.34*w,-.16,.74),MAT_ELECTRONICS)
        _part(scene,'LouverLinkageBar',(w*.50,.05,.045),(0,-.20,1.10),MAT_STEEL)
        _label(scene,'SmokeControlZoneTag',-.34*w,-.16,.28)
    elif kind == 5:  # service corridor ceiling bulkhead return
        w=1.50*s; d=.72*s; z=2.35+.08*v
        _part(scene,'BulkheadCore',(w,d,.34),(0,0,z),material)
        _part(scene,'BulkheadFrontFascia',(w,.12,.44),(0,-.31*d,z),MAT_WOOD)
        _part(scene,'BulkheadShadowSlot',(w*.82,.035,.075),(0,-.39*d,z-.14),'MAT_BLACKENED_STEEL')
        for i in range(4+v):
            x=(i-(3+v)/2)*w*.84/(3+v)
            _part(scene,f'BulkheadSuspension_{i}',(.05,.05,.62),(x,.18*d,2.72),MAT_STEEL)
        _part(scene,'AccessPanelDoor',(.38,.25,.045),(.32*w,.20*d,z-.20),MAT_STAINLESS)
        _part(scene,'ReturnAirGrille',(w*.42,.04,.16),(-.28*w,-.39*d,z+.03),MAT_ELECTRONICS)
        _label(scene,'BulkheadServiceID',.38*w,-.40*d,z-.22)
    elif kind == 6:  # glazed vestibule windbreak
        w=1.26*s; h=2.34+.08*v
        _part(scene,'VestibuleSill',(w,.38,.14),(0,0,.07),MAT_STONE)
        _part(scene,'VestibuleHead',(w,.16,.14),(0,0,h+.02),MAT_STEEL)
        for i,x in enumerate((-.45*w,-.15*w,.15*w,.45*w)):
            _part(scene,f'WindbreakMullion_{i}',(.065,.12,h),(x,0,h/2),MAT_BRASS)
        for i,x in enumerate((-.30*w,0,.30*w)):
            _part(scene,f'WindbreakGlass_{i}',(w*.28,.035,h*.86),(x,-.04,h*.53),MAT_GLASS)
        _part(scene,'VestibuleReturnGlass',(.035,.56,h*.82),(.46*w,.22,h*.52),MAT_GLASS)
        _part(scene,'AirCurtainHousing',(w*.72,.26,.16),(0,.12,h+.14),MAT_ELECTRONICS)
        _label(scene,'VestibuleSafetyDecal',-.31*w,-.08,1.02)
    elif kind == 7:  # courtyard trench drain channel
        w=1.42*s; d=.44*s
        _part(scene,'TrenchDrainConcreteBed',(w,d,.18),(0,0,.09),MAT_STONE)
        _part(scene,'DrainChannelBody',(w*.92,d*.62,.12),(0,0,.18),MAT_STEEL)
        _part(scene,'DrainGratePanel',(w*.88,d*.42,.045),(0,0,.26),MAT_STAINLESS)
        for i in range(7+v):
            x=(i-(6+v)/2)*w*.82/(6+v)
            _part(scene,f'GrateSlot_{i}',(.035,d*.34,.016),(x,0,.29),'MAT_BLACKENED_STEEL')
        _part(scene,'CatchBasinBox',(.36,.34,.46),(.32*w,.02,.23),MAT_STONE)
        _part(scene,'BasinServiceCover',(.24,.22,.04),(.32*w,-.19,.45),MAT_STAINLESS)
        _part(scene,'OutfallPipe',(.12,.12,.48),(.32*w,.12,.50),MAT_STEEL)
        _label(scene,'DrainFlowArrow',-.34*w,-.20,.35)
    elif kind == 8:  # mezzanine balustrade return bay
        w=1.42*s; d=.62*s; rail_h=1.02+.04*v
        _part(scene,'BalustradeEdgeBeam',(w,d*.30,.17),(0,0,.085),MAT_STONE)
        for i in range(6+v):
            x=(i-(5+v)/2)*w*.84/(5+v)
            _post(scene,f'MezzanineBaluster_{i}',x,-.12,rail_h,.026,material)
        _part(scene,'TopHandrail',(w*.92,.095,.08),(0,-.12,rail_h+.04),MAT_WOOD)
        _part(scene,'Midrail',(w*.90,.055,.05),(0,-.12,.58),MAT_STEEL)
        _part(scene,'ReturnHandrail',(.09,d*.66,.09),(.46*w,.13,.96),MAT_WOOD)
        for i,x in enumerate((-.38*w,.38*w)):
            _part(scene,f'AnchorBase_{i}',(.16,.21,.08),(x,-.12,.20),MAT_STAINLESS)
        _label(scene,'GuardLoadPlaque',.33*w,-.22,.26)
    else:  # rated stair landing guard module
        w=1.32*s; d=.96*s; rail_h=1.08+.03*v
        _part(scene,'StairLandingSlab',(w,d,.16),(0,0,.08),MAT_STONE)
        _part(scene,'LandingNosing',(w,.06,.07),(0,-.46*d,.195),MAT_BRASS)
        for i in range(6+v):
            x=(i-(5+v)/2)*w*.82/(5+v)
            _post(scene,f'StairGuardBaluster_{i}',x,-.36*d,rail_h,.026,material)
        _part(scene,'StairGuardHandrail',(w*.90,.10,.09),(0,-.36*d,rail_h+.045),MAT_WOOD)
        _part(scene,'GuardReturnPost',(.10,.10,.62),(.44*w,-.05,.47),MAT_STEEL)
        _part(scene,'GuardToeBoard',(w*.87,.08,.13),(0,-.37*d,.30),MAT_STEEL)
        _label(scene,'StairGuardInspectionTag',.34*w,-.50*d,.25)
    return scene


def _guestroom(kind, v, material):
    scene=trimesh.Scene(); s=.88+.06*v; w=1.12*s; d=.60*s; top=.82+.04*v
    if kind == 0:  # entry bench with shoe drawers
        _part(scene,'EntryBenchSeat',(w,d*.70,.15),(0,0,.51),MAT_UPHOLSTERY)
        _part(scene,'EntryBenchApron',(w*.90,d*.64,.28),(0,.04,.31),material)
        _floor_feet(scene,w*.92,d*.66,.28,'EntryBenchLeg',MAT_WOOD)
        for i in range(3):
            x=(i-1)*w*.28
            _part(scene,f'ShoeDrawerFace_{i}',(w*.23,.035,.17),(x,-.34*d,.31),MAT_WOOD)
            add(scene,cyl(.025,.04,(x,-.37*d,.31),MAT_BRASS,10),f'ShoeDrawerPull_{i}')
        _part(scene,'BenchBackHookRail',(w*.82,.07,.08),(0,.28*d,1.12),MAT_STEEL)
        for i in range(4): _part(scene,f'BenchCoatHook_{i}',(.12,.08,.05),((i-1.5)*.22,.24*d,1.19),MAT_BRASS)
        _label(scene,'EntryBenchCareTag',.37*w,-.39*d,.23)
    elif kind == 1:  # window reading chaise
        length=1.56*s; seat=.42+.025*v
        _part(scene,'ChaiseBasePlinth',(w*.90,length*.90,.13),(0,0,.065),MAT_STONE)
        _part(scene,'ChaiseSeatShell',(w*.80,length*.88,.20),(0,0,seat),material)
        _part(scene,'ChaiseCushion',(w*.76,length*.82,.13),(0,-.02,seat+.16),MAT_UPHOLSTERY)
        _part(scene,'ChaiseRaisedHeadrest',(w*.78,.20,.36),(0,length*.34,seat+.36),MAT_LINEN)
        _part(scene,'ChaiseArmLeft',(.12,length*.66,.35),(-.43*w,0,seat+.10),MAT_WOOD)
        _part(scene,'ChaiseArmRight',(.12,length*.66,.35),(.43*w,0,seat+.10),MAT_WOOD)
        for i in range(3):
            x=(i-1)*.18*w
            _part(scene,f'ChaiseTuft_{i}',(.08,.08,.025),(x,-.22,seat+.23),MAT_BRASS)
        _label(scene,'ChaiseUpholsteryCode',.37*w,-.45*length,.23)
    elif kind == 2:  # writing desk with folding privacy wing
        desk_h=.78+.035*v
        _part(scene,'WorkdeskTop',(w,d,.10),(0,0,desk_h),material)
        _floor_feet(scene,w*.92,d*.88,desk_h-.08,'WorkdeskLeg',MAT_STEEL)
        _part(scene,'DeskDrawerBox',(w*.66,.20,.23),(0,.18*d,desk_h-.18),MAT_WOOD)
        _part(scene,'PrivacyWing',(w*.65,.07,.58),(0,.42*d,desk_h+.32),MAT_LINEN)
        for side,x in enumerate((-.34*w,.34*w)):
            _part(scene,f'PrivacyWingHinge_{side}',(.05,.10,.18),(x,.38*d,desk_h+.13),MAT_BRASS)
        _part(scene,'DeskLampBase',(.18,.18,.05),(-.34*w,-.25*d,desk_h+.075),MAT_STEEL)
        _part(scene,'DeskLampStem',(.035,.035,.34),(-.34*w,-.25*d,desk_h+.26),MAT_BRASS)
        _part(scene,'DeskLampShade',(.23,.16,.10),(-.34*w,-.25*d,desk_h+.46),MAT_ELECTRONICS)
        _label(scene,'GuestOfficePowerGuide',.39*w,-.52*d,.25)
    elif kind == 3:  # bedside charging bridge between nightstands
        _part(scene,'ChargingBridgeBase',(w,.50,.14),(0,0,.07),MAT_STONE)
        for side,x in enumerate((-.39*w,.39*w)):
            _part(scene,f'ChargingBridgePier_{side}',(.28,.42,.80+.04*v),(x,0,.54+.02*v),material)
        _part(scene,'HeadboardBridgeShelf',(w*.94,.44,.12),(0,0,1.00+.04*v),MAT_WOOD)
        _part(scene,'BridgeUpholsteredPanel',(w*.80,.10,.58),(0,.20,1.34+.04*v),MAT_UPHOLSTERY)
        _part(scene,'BedsideReadingRail',(w*.82,.08,.08),(0,-.20,1.20+.04*v),MAT_BRASS)
        for side,x in enumerate((-.28*w,.28*w)):
            _part(scene,f'USBChargePanel_{side}',(.14,.035,.12),(x,-.24,1.08+.04*v),MAT_ELECTRONICS)
            _part(scene,f'ReadingLight_{side}',(.10,.08,.16),(x,.12,1.62+.04*v),MAT_ELECTRONICS)
        _label(scene,'NightstandPowerResetTag',.38*w,-.23,.28)
    elif kind == 4:  # wardrobe and pull-out laundry hamper tower
        h=1.92+.06*v
        _part(scene,'WardrobeTowerPlinth',(w*.72,d*.72,.14),(0,0,.07),MAT_STONE)
        _part(scene,'WardrobeTowerCase',(w*.72,d*.72,h),(0,0,.14+h/2),material)
        _part(scene,'WardrobeDoor',(w*.55,.045,h*.82),(-.07*w,-.37*d,.14+h*.54),MAT_WOOD)
        _part(scene,'WardrobeDoorHandle',(.04,.05,.34),(.20*w,-.41*d,1.14),MAT_BRASS)
        _part(scene,'LaundryHamperFront',(w*.60,.07,.42),(.06*w,-.39*d,.38),MAT_LINEN)
        _part(scene,'HamperPull',(w*.24,.035,.07),(.06*w,-.43*d,.58),MAT_STEEL)
        _part(scene,'WardrobeTopCap',(w*.78,d*.78,.10),(0,0,h+.19),MAT_WOOD)
        _label(scene,'WardrobeSafeWeightLabel',-.32*w,-.41*d,.24)
    elif kind == 5:  # dressing mirror vanity console
        h=.80+.04*v
        _part(scene,'DressingVanityTop',(w,d*.80,.10),(0,0,h),MAT_STONE)
        _floor_feet(scene,w*.90,d*.72,h-.08,'DressingVanityLeg',material)
        _part(scene,'VanityDrawer',(w*.56,.08,.22),(0,.06,h-.17),MAT_WOOD)
        _part(scene,'VanityMirrorFrame',(w*.70,.07,.72),(0,.31*d,h+.42),MAT_BRASS)
        _part(scene,'VanityMirrorGlass',(w*.60,.035,.62),(0,.27*d,h+.42),MAT_GLASS)
        _part(scene,'MakeupLightBar',(w*.62,.06,.07),(0,.24*d,h+.80),MAT_ELECTRONICS)
        for i,x in enumerate((-.32*w,.32*w)):
            _part(scene,f'VanitySideSconce_{i}',(.10,.08,.28),(x,.22*d,h+.42),MAT_ELECTRONICS)
        _part(scene,'VanityStoolSeat',(.40,.38,.11),(0,-.48*d,.46),MAT_UPHOLSTERY)
        _floor_feet(scene,.36,.34,.40,'VanityStoolLeg',MAT_WOOD)
        _label(scene,'VanityOutletGuide',.38*w,-.42*d,.28)
    elif kind == 6:  # in-room dining stowaway credenza
        h=.92+.04*v
        _part(scene,'DiningCredenzaPlinth',(w,d,.13),(0,0,.065),MAT_STONE)
        _part(scene,'DiningCredenzaCase',(w,d,h-.13),(0,0,(h-.13)/2+.13),material)
        for i in range(2):
            x=(i-.5)*w*.43
            _part(scene,f'CredenzaDoor_{i}',(w*.40,.045,h*.65),(x,-.52*d,h*.52),MAT_WOOD)
            _part(scene,f'CredenzaPull_{i}',(.035,.035,.18),(x+.12,-.55*d,h*.52),MAT_BRASS)
        _part(scene,'FoldoutDiningLeaf',(w*.82,d*.82,.07),(0,.02,h+.10),MAT_WOOD)
        _part(scene,'FoldoutLeafHingeBar',(w*.78,.06,.06),(0,-.14,h+.03),MAT_STEEL)
        _part(scene,'CutleryDrawer',(w*.80,.055,.15),(0,-.53*d,.28),MAT_STAINLESS)
        _label(scene,'DiningServiceSetupCard',.39*w,-.55*d,.18)
    elif kind == 7:  # robe and garment valet stand
        _part(scene,'ValetStandBase',(w*.62,d*.70,.14),(0,0,.07),MAT_STONE)
        _post(scene,'ValetStandColumn',0,0,1.50+.06*v,.05,material)
        _part(scene,'ValetShoulderBar',(w*.72,.10,.10),(0,0,1.46+.06*v),MAT_WOOD)
        for side,x in enumerate((-.30*w,.30*w)):
            _part(scene,f'ValetGarmentHook_{side}',(.18,.12,.08),(x,0,1.34+.06*v),MAT_BRASS)
        _part(scene,'ValetTrouserBar',(w*.52,.07,.08),(0,0,.94),MAT_STEEL)
        _part(scene,'ShoeTray',(w*.56,d*.50,.06),(0,0,.26),MAT_WOOD)
        _part(scene,'ValetAccessoryDish',(.24,.18,.05),(0,-.15,1.62+.06*v),MAT_CERAMIC)
        _label(scene,'ValetGarmentCareTag',.28*w,-.35*d,.34)
    elif kind == 8:  # overbed writing table console
        h=.76+.04*v
        _part(scene,'OverbedDeskTop',(w,d,.11),(0,0,h),material)
        _floor_feet(scene,w*.90,d*.86,h-.09,'OverbedDeskLeg',MAT_STEEL)
        _part(scene,'DeskPrivacyScreen',(w*.72,.06,.42),(0,.40*d,h+.24),MAT_WOOD)
        _part(scene,'ScreenHingeLeft',(.06,.11,.16),(-.34*w,.37*d,h+.11),MAT_BRASS)
        _part(scene,'ScreenHingeRight',(.06,.11,.16),(.34*w,.37*d,h+.11),MAT_BRASS)
        _part(scene,'BookLightBase',(.16,.15,.045),(-.36*w,-.28*d,h+.08),MAT_STEEL)
        _part(scene,'BookLightArm',(.045,.045,.30),(-.36*w,-.28*d,h+.23),MAT_BRASS)
        _part(scene,'BookLightHead',(.23,.12,.07),(-.36*w,-.28*d,h+.40),MAT_ELECTRONICS)
        _part(scene,'CableTray',(w*.64,.12,.07),(0,.22*d,h-.15),MAT_STEEL)
        _label(scene,'OverbedTableLoadCard',.39*w,-.52*d,.25)
    else:  # accessible bedside transfer assist rail
        w=.98*s; d=.88*s; rail_h=.94+.04*v
        _part(scene,'TransferRailBasePlate',(w,d,.16),(0,0,.08),MAT_STONE)
        for i,x in enumerate((-.36*w,.36*w)):
            _part(scene,f'TransferRailUpright_{i}',(.09,.09,rail_h),(x,0,rail_h/2+.12),MAT_STEEL)
        _part(scene,'UpperAssistGrip',(w*.88,.14,.12),(0,0,rail_h+.12),material)
        _part(scene,'LowerAssistGrip',(w*.76,.12,.10),(0,0,.60),MAT_WOOD)
        _part(scene,'BedFrameClamp',(w*.28,.22,.22),(0,.33*d,.28),MAT_STAINLESS)
        for i,x in enumerate((-.32*w,.32*w)):
            _part(scene,f'ClampBolt_{i}',(.055,.055,.14),(x,.40*d,.28),MAT_BRASS)
        _part(scene,'TransferRailPadding',(w*.62,.16,.09),(0,-.02,.60),MAT_UPHOLSTERY)
        _label(scene,'SafeTransferLoadRating',.34*w,-.48*d,.26)
    return scene


def _public(kind, v, material):
    scene=trimesh.Scene(); s=.88+.06*v; w=1.12*s; d=.62*s; h=.88+.05*v
    if kind == 0:  # coat check counter with claim tokens
        _part(scene,'CoatCheckPlinth',(w,d,.14),(0,0,.07),MAT_STONE)
        _part(scene,'CoatCheckCabinet',(w,d*.82,h-.14),(0,0,(h-.14)/2+.14),material)
        _part(scene,'CoatCheckCountertop',(w*1.06,d,.10),(0,0,h),MAT_WOOD)
        for i in range(4):
            x=(i-1.5)*w*.22
            _part(scene,f'ClaimTokenHook_{i}',(.08,.08,.07),(x,-.20*d,h+.10),MAT_BRASS)
            _part(scene,f'ClaimTag_{i}',(.12,.025,.10),(x,-.53*d,h-.15),MAT_LABEL)
        _part(scene,'CoatCheckBagShelf',(w*.84,d*.66,.065),(0,.05,.32),MAT_STEEL)
        _part(scene,'CoatCheckTicketDrawer',(w*.46,.06,.14),(0,-.51*d,.30),MAT_STAINLESS)
        _label(scene,'CoatCheckHoursPlaque',.38*w,-.54*d,.22)
    elif kind == 1:  # piano bench with music cabinet
        _part(scene,'MusicCabinetBase',(w*.74,d*.68,.14),(0,0,.07),MAT_STONE)
        _part(scene,'MusicBookCabinet',(w*.72,d*.66,.72),(0,0,.57),material)
        _part(scene,'MusicCabinetTop',(w*.82,d*.76,.10),(0,0,.98),MAT_WOOD)
        _part(scene,'MusicScoreShelf',(w*.58,.12,.08),(0,-.14,1.08),MAT_STEEL)
        for i in range(4):
            _part(scene,f'ScoreFolder_{i}',(.12,.08,.27),((i-1.5)*.14,-.12,1.25),MAT_LINEN)
        _part(scene,'PianoBenchSeat',(w*.86,d*.54,.13),(0,-.34,.49),MAT_UPHOLSTERY)
        _floor_feet(scene,w*.78,d*.50,.42,'PianoBenchLeg',MAT_WOOD)
        _part(scene,'BenchHeightAdjuster',(w*.50,.06,.10),(0,-.34,.40),MAT_STEEL)
        _label(scene,'MusicCabinetInventoryTag',.30*w,-.39*d,.27)
    elif kind == 2:  # public device charging island
        _part(scene,'ChargingIslandFoot',(w*.72,d*.72,.16),(0,0,.08),MAT_STONE)
        _part(scene,'ChargingIslandPedestal',(w*.66,d*.66,.78+.04*v),(0,0,.55),material)
        _part(scene,'ChargingIslandWorktop',(w,d,.13),(0,0,1.02+.04*v),MAT_WOOD)
        for i in range(5+v):
            x=(i-(4+v)/2)*w*.76/(4+v)
            _part(scene,f'USBChargeDock_{i}',(.16,.18,.12),(x,-.18*d,1.15+.04*v),MAT_ELECTRONICS)
            _part(scene,f'CableKeeper_{i}',(.055,.04,.035),(x,.18*d,1.10+.04*v),MAT_BRASS)
        _part(scene,'IslandAccessPanel',(w*.48,.035,.32),(0,-.34*d,.60),MAT_STEEL)
        _part(scene,'CableServiceHatch',(.24,.20,.045),(0,.02,.96),MAT_STAINLESS)
        _label(scene,'ChargingUsageGuide',.39*w,-.36*d,.47)
    elif kind == 3:  # valet arrival stand and key dock
        _part(scene,'ValetStandFoot',(w*.70,d*.64,.13),(0,0,.065),MAT_STONE)
        _part(scene,'ValetStandPedestal',(w*.62,d*.54,.82),(0,0,.53),material)
        _part(scene,'ValetStandCounter',(w,d,.12),(0,0,.96),MAT_WOOD)
        _part(scene,'KeyTagRail',(w*.68,.055,.08),(0,-.24*d,1.08),MAT_BRASS)
        for i in range(4):
            x=(i-1.5)*w*.18
            _part(scene,f'ValetKeyHook_{i}',(.06,.07,.10),(x,-.28*d,1.00),MAT_STEEL)
            add(scene,cyl(.035,.035,(x,-.30*d,.94),MAT_BRASS,10),f'ValetKeyToken_{i}')
        _part(scene,'ArrivalRadioCradle',(.18,.12,.20),(.33*w,.16*d,1.12),MAT_ELECTRONICS)
        _part(scene,'StandServiceDrawer',(w*.62,.06,.15),(0,-.29*d,.33),MAT_STAINLESS)
        _label(scene,'ValetTicketSequence',-.34*w,-.29*d,.53)
    elif kind == 4:  # lobby seating alcove bench
        length=1.38*s; seat=.47+.02*v
        _part(scene,'AlcoveBenchBase',(w,length,.12),(0,0,.06),MAT_STONE)
        _part(scene,'AlcoveBenchSeat',(w*.94,length*.90,.18),(0,0,seat),material)
        _part(scene,'AlcoveSeatCushion',(w*.88,length*.86,.10),(0,0,seat+.14),MAT_UPHOLSTERY)
        _part(scene,'AlcoveBackPanel',(w*.94,.13,.78),(0,length*.40,.92),MAT_WOOD)
        for i in range(4):
            x=(i-1.5)*w*.20
            _part(scene,f'BackPanelUpholstery_{i}',(w*.18,.07,.62),(x,length*.34,.94),MAT_LINEN)
        _part(scene,'ArmLeft',(.14,length*.82,.52),(-.45*w,0,.66),MAT_WOOD)
        _part(scene,'ArmRight',(.14,length*.82,.52),(.45*w,0,.66),MAT_WOOD)
        _label(scene,'AlcoveReservationPlate',.42*w,-.47*length,.24)
    elif kind == 5:  # lobby library book return shelf
        h=1.82+.05*v
        _part(scene,'LibraryShelfBase',(w,d,.14),(0,0,.07),MAT_STONE)
        _part(scene,'LibraryShelfBack',(w*.92,.08,h),(0,.24*d,.14+h/2),material)
        for level,z in enumerate((.44,.82,1.20,1.58)):
            _part(scene,f'LibraryReturnShelf_{level}',(w*.88,d*.78,.065),(0,0,z),MAT_WOOD)
            _part(scene,f'ShelfLabelRail_{level}',(w*.86,.035,.08),(0,-.39*d,z+.07),MAT_LABEL)
        _part(scene,'BookReturnSlot',(w*.66,.05,.16),(0,-.42*d,1.74),MAT_STEEL)
        for i in range(3):
            _part(scene,f'ReturnBook_{i}',(.17,.13,.20),((i-1)*.19,.02,.31),MAT_LINEN)
        _label(scene,'LibraryHoursPlaque',.34*w,-.43*d,.22)
    elif kind == 6:  # public restroom privacy vanity divider
        w=1.20*s; d=.70*s; top=.84+.04*v
        _part(scene,'VanityDividerBase',(w,d*.38,.14),(0,0,.07),MAT_STONE)
        _part(scene,'VanityCabinet',(w*.92,d*.68,top-.14),(0,0,(top-.14)/2+.14),material)
        _part(scene,'VanityCounter',(w,d*.78,.10),(0,0,top),MAT_CERAMIC)
        _part(scene,'Basin',(w*.34,d*.44,.11),(-.23*w,-.03,top+.08),MAT_STAINLESS)
        _part(scene,'PrivacyDividerPanel',(.08,d*.92,.84),(.26*w,.10,top+.40),MAT_GLASS)
        _part(scene,'MirrorPanel',(w*.58,.04,.62),(-.14*w,.32*d,top+.38),MAT_GLASS)
        _part(scene,'FaucetSpout',(.045,.20,.08),(-.23*w,-.15,top+.18),MAT_BRASS)
        _part(scene,'SoapDispenser',(.14,.12,.26),(.38*w,.06,top+.15),MAT_STEEL)
        _label(scene,'PublicVanityCleaningCode',.38*w,-.39*d,.24)
    elif kind == 7:  # banquet registration desk
        top=.92+.035*v
        _part(scene,'RegistrationDeskBase',(w,d,.12),(0,0,.06),MAT_STONE)
        _part(scene,'RegistrationDeskCase',(w,d*.70,top-.12),(0,0,(top-.12)/2+.12),material)
        _part(scene,'DeskCountertop',(w*1.08,d,.11),(0,0,top),MAT_WOOD)
        _floor_feet(scene,w*.86,d*.48,top-.08,'RegistrationDeskFoot',MAT_STEEL)
        for i in range(3):
            x=(i-1)*w*.28
            _part(scene,f'BadgeTray_{i}',(w*.22,d*.36,.06),(x,-.04,top+.085),MAT_STAINLESS)
            _part(scene,f'NameBadgeStack_{i}',(w*.16,.16,.07),(x,-.04,top+.15),MAT_LINEN)
        _part(scene,'CheckInTablet',(.24,.05,.18),(.36*w,.20*d,top+.19),MAT_ELECTRONICS)
        _label(scene,'RegistrationQueueMarker',-.36*w,-.52*d,.28)
    elif kind == 8:  # guest services map display tower
        _part(scene,'MapTowerBase',(.66*s,.62*s,.15),(0,0,.075),MAT_STONE)
        _part(scene,'MapTowerColumn',(.40*s,.38*s,1.42+.04*v),(0,0,.86),material)
        _part(scene,'MapDisplayFrame',(.78*s,.08,.62),(0,-.22*s,1.84+.04*v),MAT_WOOD)
        _part(scene,'HotelMapPanel',(.66*s,.035,.48),(0,-.27*s,1.84+.04*v),MAT_LABEL)
        _part(scene,'BrochurePocketLeft',(.28*s,.10,.22),(-.22*s,-.30*s,1.30),MAT_GLASS)
        _part(scene,'BrochurePocketRight',(.28*s,.10,.22),(.22*s,-.30*s,1.30),MAT_GLASS)
        _part(scene,'MapTowerHeader',(.70*s,.12,.15),(0,-.19*s,2.22+.04*v),MAT_BRASS)
        _label(scene,'MapTowerAccessibleRoute',0,-.33*s,.45)
    else:  # luggage claim ticket kiosk
        _part(scene,'ClaimKioskFoot',(.68*s,.58*s,.14),(0,0,.07),MAT_STONE)
        _part(scene,'ClaimKioskPedestal',(.50*s,.44*s,1.02+.04*v),(0,0,.65),material)
        _part(scene,'ClaimScreenFrame',(.72*s,.09,.48),(0,-.23*s,1.40+.04*v),MAT_STEEL)
        _part(scene,'ClaimTouchDisplay',(.60*s,.04,.36),(0,-.28*s,1.40+.04*v),MAT_ELECTRONICS)
        _part(scene,'TicketDispenser',(.28*s,.15,.22),(.22*s,-.28*s,1.00),MAT_STAINLESS)
        _part(scene,'TicketFeedSlot',(.18*s,.04,.04),(.22*s,-.37*s,1.06),'MAT_BLACKENED_STEEL')
        _part(scene,'LuggageTagScanner',(.18,.08,.15),(-.22*s,-.28*s,.95),MAT_ELECTRONICS)
        _label(scene,'ClaimInstructions',-.28*s,-.31*s,.53)
    return scene


def _amenity(kind, v, material):
    scene=trimesh.Scene(); s=.88+.06*v; w=1.06*s; d=.60*s; top=.90+.05*v
    if kind == 0:  # spa heated towel cabinet
        _part(scene,'TowelCabinetFoot',(w*.72,d*.68,.14),(0,0,.07),MAT_STONE)
        _part(scene,'HeatedTowelCase',(w*.70,d*.66,1.10+.04*v),(0,0,.69),material)
        for i in range(3):
            z=.39+i*.31
            _part(scene,f'TowelCabinetDoor_{i}',(w*.58,.045,.25),(0,-.34*d,z),MAT_STAINLESS)
            _part(scene,f'TowelDoorHandle_{i}',(.16,.04,.035),(0,-.38*d,z),MAT_BRASS)
        _part(scene,'TowelCabinetTop',(w*.82,d*.76,.08),(0,0,1.28+.04*v),MAT_WOOD)
        _part(scene,'TowelTemperatureDisplay',(.20,.04,.13),(.27*w,-.37*d,1.02),MAT_ELECTRONICS)
        _label(scene,'TowelRotationGuide',-.30*w,-.38*d,.25)
    elif kind == 1:  # sauna tier bench assembly
        length=1.32*s
        _part(scene,'SaunaBenchBase',(w,length,.12),(0,0,.06),MAT_STONE)
        for i,(z,width) in enumerate(((.50,.82),( .88,.95),(1.26,1.0))):
            _part(scene,f'SaunaBenchTier_{i}',(w*width,length*.90,.10),(0,0,z),material)
            for slat in range(4+v):
                x=(slat-(3+v)/2)*w*width*.84/(3+v)
                _part(scene,f'TierSlat_{i}_{slat}',(.035,length*.84,.035),(x,0,z+.07),MAT_WOOD)
        for side,x in enumerate((-.42*w,.42*w)):
            _part(scene,f'BenchSupport_{side}',(.10,length*.78,.78),(x,.04,.42),MAT_STEEL)
        _label(scene,'SaunaBenchCleaningTag',.38*w,-.48*length,.24)
    elif kind == 2:  # relaxation chaise and footrest
        length=1.38*s; seat=.42+.02*v
        _part(scene,'RelaxationChaiseBase',(w*.88,length*.90,.13),(0,0,.065),MAT_STONE)
        _part(scene,'ChaiseDeck',(w*.80,length*.84,.18),(0,0,seat),material)
        _part(scene,'ChaiseCushion',(w*.75,length*.78,.12),(0,0,seat+.15),MAT_UPHOLSTERY)
        _part(scene,'ElevatedHeadBolster',(w*.68,.18,.24),(0,length*.33,seat+.32),MAT_LINEN)
        for side,x in enumerate((-.39*w,.39*w)):
            _part(scene,f'ChaiseSideRail_{side}',(.08,length*.78,.22),(x,0,seat-.06),MAT_WOOD)
        _part(scene,'SeparateFootrestBase',(w*.64,.32,.13),(0,-.62*length,.065),MAT_STONE)
        _part(scene,'FootrestCushion',(w*.60,.28,.20),(0,-.62*length,.23),MAT_UPHOLSTERY)
        _label(scene,'RelaxationUpholsteryCode',.37*w,-.45*length,.22)
    elif kind == 3:  # yoga bolster and mat storage rack
        _part(scene,'YogaRackFoot',(w*.80,d*.70,.13),(0,0,.065),MAT_STONE)
        for side,x in enumerate((-.40*w,.40*w)):
            _part(scene,f'YogaRackUpright_{side}',(.07,.08,1.62),(x,0,.88),material)
        for tier,z in enumerate((.42,.82,1.22)):
            _part(scene,f'BolsterShelf_{tier}',(w*.86,d*.70,.07),(0,0,z),MAT_WOOD)
            for i in range(3+v):
                x=(i-(2+v)/2)*w*.68/(2+v)
                add(scene,cyl(.11,.32,(x,.01,z+.18),MAT_LINEN,16),f'YogaBolster_{tier}_{i}')
        _part(scene,'YogaMatUpperRail',(w*.82,.08,.08),(0,.22,1.70),MAT_STEEL)
        _label(scene,'YogaRackCapacityCard',.34*w,-.37*d,.25)
    elif kind == 4:  # pool towel exchange hamper
        _casters(scene,w,d,'PoolTowelCaster')
        _part(scene,'TowelExchangeFrame',(w,d,.10),(0,0,.18),MAT_STEEL)
        _part(scene,'CleanTowelBin',(w*.46,d*.74,.62),(-.25*w,0,.55),MAT_LINEN)
        _part(scene,'UsedTowelBin',(w*.46,d*.74,.62),(.25*w,0,.55),MAT_CERAMIC)
        _part(scene,'CleanBinRim',(w*.48,d*.78,.07),(-.25*w,0,.90),MAT_WOOD)
        _part(scene,'UsedBinRim',(w*.48,d*.78,.07),(.25*w,0,.90),MAT_STAINLESS)
        _part(scene,'ExchangeCounter',(w*.92,d*.88,.09),(0,0,1.02),material)
        _label(scene,'CleanTowelSign',-.25*w,-.40*d,1.09)
        _label(scene,'UsedTowelSign',.25*w,-.40*d,1.09)
    elif kind == 5:  # children activity table and chair set
        table_h=.58+.025*v
        _part(scene,'KidsActivityTop',(w,d,.10),(0,0,table_h),material)
        _floor_feet(scene,w*.90,d*.84,table_h-.08,'KidsTableLeg',MAT_WOOD)
        _part(scene,'ActivitySurfaceInlay',(w*.76,d*.70,.025),(0,0,table_h+.065),MAT_CERAMIC)
        for side,x in enumerate((-.42*w,.42*w)):
            _part(scene,f'KidsChairSeat_{side}',(.34,.34,.11),(x,-.44*d,.34),MAT_UPHOLSTERY)
            _floor_feet(scene,.30,.30,.30,f'KidsChairLeg_{side}',MAT_STEEL)
            _part(scene,f'KidsChairBack_{side}',(.32,.08,.38),(x,-.25*d,.56),MAT_WOOD)
        _part(scene,'CrayonTray',(w*.58,.16,.065),(0,.32*d,table_h+.04),MAT_WOOD)
        for i in range(4):
            _part(scene,f'Crayon_{i}',(.12,.035,.035),((i-1.5)*.15,.32*d,table_h+.09),MAT_BRASS)
        _label(scene,'KidsAreaCleaningLabel',.38*w,-.50*d,.22)
    elif kind == 6:  # outdoor recreation gear rack
        _part(scene,'GearRackFoot',(w*.82,d*.72,.14),(0,0,.07),MAT_STONE)
        for side,x in enumerate((-.40*w,.40*w)):
            _part(scene,f'GearRackUpright_{side}',(.08,.10,1.58),(x,0,.86),material)
        for i,z in enumerate((.44,.88,1.32)):
            _part(scene,f'GearRackShelf_{i}',(w*.86,d*.74,.07),(0,0,z),MAT_STEEL)
        for i in range(4+v):
            x=(i-(3+v)/2)*w*.72/(3+v)
            _part(scene,f'HelmetHook_{i}',(.20,.10,.06),(x,-.04,1.54),MAT_BRASS)
            _part(scene,f'GearBin_{i}',(.15,.18,.24),(x,.03,.58),MAT_LINEN)
        _part(scene,'GearRackHeader',(w*.80,.08,.16),(0,-.04,1.76),MAT_WOOD)
        _label(scene,'RecreationReturnInstruction',.35*w,-.34*d,.26)
    elif kind == 7:  # spa tea service island
        _part(scene,'SpaTeaIslandBase',(w,d,.14),(0,0,.07),MAT_STONE)
        _part(scene,'SpaTeaIslandCase',(w*.84,d*.76,.64),(0,0,.48),material)
        _part(scene,'TeaIslandCounter',(w*1.08,d,.12),(0,0,.88),MAT_WOOD)
        _floor_feet(scene,w*.84,d*.76,.16,'TeaIslandLeg',MAT_STEEL)
        for i in range(3):
            x=(i-1)*w*.26
            add(scene,cyl(.10,.23,(x,0,1.08),MAT_CERAMIC,16),f'HerbalTeaCanister_{i}')
            _part(scene,f'CanisterLid_{i}',(.13,.13,.04),(x,0,1.22),MAT_BRASS)
        _part(scene,'HotWaterUrn',(.22,.22,.43),(.36*w,.08,1.10),MAT_STAINLESS)
        _part(scene,'TeaCupShelf',(w*.82,.16,.07),(0,.28*d,.36),MAT_WOOD)
        _label(scene,'TeaAllergenCard',.37*w,-.51*d,.24)
    elif kind == 8:  # hydrotherapy shower column
        _part(scene,'ShowerColumnFoot',(w*.42,d*.46,.16),(0,0,.08),MAT_STONE)
        _part(scene,'HydrotherapyColumn',(w*.36,d*.40,2.10+.05*v),(0,0,1.21),material)
        _part(scene,'OverheadRainHead',(w*.94,d*.70,.12),(0,.18,2.34+.05*v),MAT_STAINLESS)
        _part(scene,'RainHeadArm',(.07,.07,.38),(0,.12,2.12+.05*v),MAT_BRASS)
        for i in range(4+v):
            z=.75+i*.25
            _part(scene,f'BodyJet_{i}',(.13,.06,.10),(0,-.22,z),MAT_STEEL)
        _part(scene,'ThermostaticMixer',(.22,.08,.28),(.24*w,-.22,.94),MAT_ELECTRONICS)
        _part(scene,'HandShowerDock',(.13,.08,.15),(-.25*w,-.20,1.70),MAT_STAINLESS)
        _part(scene,'FlexibleHose',(.035,.04,.82),(-.25*w,-.22,1.22),MAT_STEEL)
        _label(scene,'HydrotherapyFlowGuide',.28*w,-.24,.42)
    else:  # event lounge divider planter
        length=1.36*s; h=1.22+.04*v
        _part(scene,'DividerPlanterBase',(w,length,.18),(0,0,.09),MAT_STONE)
        _part(scene,'PlanterTrough',(w*.92,length*.88,.36),(0,0,.36),material)
        _part(scene,'PlanterSoil',(w*.78,length*.74,.08),(0,0,.58),MAT_WOOD)
        for i in range(5+v):
            x=(i-(4+v)/2)*w*.70/(4+v)
            y=((i%2)-.5)*length*.38
            add(scene,sphere(.14,(x,y,.84),MAT_LINEN,subdivisions=1),f'DividerFoliage_{i}')
        _part(scene,'DividerTopRail',(w*.92,.07,.07),(0,0,h),MAT_BRASS)
        for side,x in enumerate((-.43*w,.43*w)):
            _part(scene,f'DividerEndPost_{side}',(.08,.08,h),(x,0,h/2),MAT_STEEL)
        _label(scene,'PlanterWateringCode',.36*w,-.48*length,.25)
    return scene


def _decor(kind, v, material):
    scene=trimesh.Scene(); s=.90+.055*v; w=1.08*s; d=.52*s
    if kind == 0:  # heritage corridor art triptych
        h=1.32+.06*v
        _part(scene,'TriptychMountingBacker',(w,.10,h),(0,0,h/2),MAT_STEEL)
        for i,x in enumerate((-.32*w,0,.32*w)):
            _part(scene,f'ArtworkFrame_{i}',(w*.28,.10,h*.86),(x,-.08,h/2),material)
            _part(scene,f'ArtworkMat_{i}',(w*.22,.05,h*.74),(x,-.14,h/2),MAT_WOOD)
            _part(scene,f'ArtworkCanvas_{i}',(w*.18,.035,h*.68),(x,-.18,h/2),MAT_LINEN)
            _part(scene,f'ArtworkAccent_{i}',(w*.09,.025,h*.18),(x,-.205,h*.52),MAT_BRASS)
        _label(scene,'ArtCollectionCredit',.38*w,-.22,.19)
    elif kind == 1:  # lobby sculptural vessel on plinth
        _part(scene,'SculpturePlinthFoot',(.70*s,.70*s,.14),(0,0,.07),MAT_STONE)
        _part(scene,'SculpturePlinthColumn',(.48*s,.48*s,.78),(0,0,.53),material)
        _part(scene,'SculpturePlinthCap',(.68*s,.68*s,.10),(0,0,.97),MAT_BRASS)
        add(scene,sphere(.35*s,(0,0,1.38),MAT_CERAMIC,subdivisions=2), 'AbstractCeramicVessel')
        add(scene,cyl(.15*s,.06,(0,0,1.74),MAT_BRASS,20),'VesselLip')
        _part(scene,'PlinthAssetPlate',(.22,.025,.08),(.29*s,-.25*s,.64),MAT_LABEL)
        _part(scene,'PlinthFloorAnchor',(w*.48,d*.48,.06),(0,0,.17),MAT_STEEL)
        _label(scene,'SculptureCareMark',-.28*s,-.25*s,.25)
    elif kind == 2:  # grandfather lobby clock
        _part(scene,'ClockBasePlinth',(w*.66,d*.66,.14),(0,0,.07),MAT_STONE)
        _part(scene,'ClockCase',(w*.56,d*.50,1.72+.05*v),(0,0,.14+(1.72+.05*v)/2),material)
        _part(scene,'ClockHood',(w*.70,d*.62,.22),(0,0,2.00+.05*v),MAT_WOOD)
        _part(scene,'ClockFace',(w*.28,.035,.28),(0,-.27*d,1.72+.05*v),MAT_CERAMIC)
        add(scene,cyl(.018,.08,(0,-.30*d,1.75+.05*v),MAT_BRASS,12),'ClockHandMinute')
        add(scene,cyl(.014,.06,(0,-.31*d,1.74+.05*v),MAT_STEEL,12),'ClockHandHour')
        _part(scene,'ClockPendulumWindow',(w*.20,.025,.55),(0,-.28*d,.74),MAT_GLASS)
        _part(scene,'ClockPendulumBob',(.14,.06,.16),(0,-.32*d,.67),MAT_BRASS)
        _label(scene,'ClockWindingInstruction',.30*w,-.31*d,.42)
    elif kind == 3:  # corridor lantern sconce pair
        for i,x in enumerate((-.34*w,.34*w)):
            _part(scene,f'SconceBackplate_{i}',(.18,.06,.30),(x,0,.62),MAT_STEEL)
            _part(scene,f'SconceArm_{i}',(.07,.34,.07),(x,-.18,.70),MAT_BRASS)
            _part(scene,f'LanternGlass_{i}',(.25,.22,.42),(x,-.34,.94),MAT_GLASS)
            _part(scene,f'LanternTop_{i}',(.30,.28,.08),(x,-.34,1.19),material)
            _part(scene,f'LanternBase_{i}',(.29,.27,.08),(x,-.34,.69),MAT_WOOD)
            _part(scene,f'LanternLamp_{i}',(.10,.10,.18),(x,-.34,.94),MAT_ELECTRONICS)
        _part(scene,'SconceMountingRail',(w*.92,.08,.12),(0,.02,.62),MAT_WOOD)
        _label(scene,'LanternWattageLabel',0,-.40,.30)
    elif kind == 4:  # room wayfinding plaque set
        h=.28+.025*v
        for i,(x,z) in enumerate(((-.30*w,.55),(0,.92),(.30*w,1.29))):
            _part(scene,f'WayfindingPlaqueBack_{i}',(w*.42,.07,h),(x,0,z),MAT_WOOD)
            _part(scene,f'WayfindingPlaqueFace_{i}',(w*.36,.035,h*.72),(x,-.055,z),material)
            _part(scene,f'WayfindingArrow_{i}',(.09,.025,.07),(x-.10*w,-.08,z),MAT_BRASS)
            _label(scene,f'WayfindingBraille_{i}',x+.10*w,-.08,z-.03,.12)
        _part(scene,'PlaqueSetMountingBoard',(w,.08,1.28),(0,.08,.92),MAT_STEEL)
        _label(scene,'PlaqueCleaningCode',.41*w,-.02,.24)
    elif kind == 5:  # event table floral centerpiece
        _part(scene,'CenterpieceTrayBase',(w*.82,d*.82,.10),(0,0,.05),MAT_STONE)
        _part(scene,'CenterpieceVase',(.34*s,.34*s,.52),(0,0,.36),MAT_CERAMIC)
        add(scene,cyl(.15*s,.05,(0,0,.64),MAT_BRASS,18),'VaseNeck')
        for i in range(5+v):
            angle=2*math.pi*i/(5+v)
            x=.26*s*math.cos(angle); y=.20*s*math.sin(angle)
            _part(scene,f'FloralStem_{i}',(.035,.035,.42),(x,y,.82),MAT_STEEL)
            add(scene,sphere(.11,(x,y,1.06),MAT_LINEN,subdivisions=1),f'FlowerHead_{i}')
        _part(scene,'VaseWaterLine',(.20*s,.20*s,.018),(0,0,.31),MAT_GLASS)
        _label(scene,'EventFloralCareCard',.30*w,-.42*d,.22)
    elif kind == 6:  # guestroom welcome tray arrangement
        _part(scene,'WelcomeTrayBase',(w,d,.06),(0,0,.03),material)
        _part(scene,'TrayRaisedRim',(w*.94,.045,.08),(0,-.23*d,.10),MAT_WOOD)
        _part(scene,'WelcomeWaterCarafe',(.16,.16,.31),(-.27*w,0,.215),MAT_GLASS)
        _part(scene,'CarafeStopper',(.09,.09,.06),(-.27*w,0,.40),MAT_BRASS)
        for i in range(2):
            x=(i-.5)*.23*w
            add(scene,cyl(.075,.045,(x,.09,.09),MAT_CERAMIC,14),f'WelcomeCup_{i}')
        _part(scene,'WelcomeNoteCard',(.28,.035,.14),(.32*w,-.08,.14),MAT_LABEL)
        _part(scene,'LocalTreatBox',(.26,.20,.15),(0,.12,.135),MAT_LINEN)
        _label(scene,'DietaryContentsTag',.36*w,-.25*d,.18,.16)
    elif kind == 7:  # ceramic planter column pair
        for i,x in enumerate((-.32*w,.32*w)):
            _part(scene,f'PlanterColumnFoot_{i}',(.38*s,.38*s,.12),(x,0,.06),MAT_STONE)
            _part(scene,f'PlanterColumnBody_{i}',(.32*s,.32*s,.78),(x,0,.51),material)
            _part(scene,f'PlanterColumnRim_{i}',(.42*s,.42*s,.09),(x,0,.92),MAT_BRASS)
            _part(scene,f'PlanterSoil_{i}',(.30*s,.30*s,.07),(x,0,.99),MAT_STONE)
            for j in range(3+v//2):
                angle=2*math.pi*j/(3+v//2)
                add(scene,sphere(.15,(x+.12*math.cos(angle),.12*math.sin(angle),1.22),MAT_LINEN,subdivisions=1),f'PlanterLeaf_{i}_{j}')
        _part(scene,'PlanterPairConnectingTray',(w*.72,.22,.06),(0,0,.16),MAT_WOOD)
        _label(scene,'PlanterWateringLabel',.42*w,-.22,.24)
    elif kind == 8:  # retail display bust pedestal
        _part(scene,'RetailBustBase',(w*.62,d*.62,.13),(0,0,.065),MAT_STONE)
        _part(scene,'BustPedestal',(w*.48,d*.48,.72),(0,0,.49),material)
        _part(scene,'BustPedestalTop',(w*.60,d*.60,.08),(0,0,.89),MAT_BRASS)
        add(scene,sphere(.21,(0,0,1.20),MAT_CERAMIC,subdivisions=2),'DisplayBustHead')
        _part(scene,'BustShoulderForm',(.50,.28,.32),(0,0,1.02),MAT_LINEN)
        _part(scene,'BustNeckForm',(.16,.15,.20),(0,0,1.20),MAT_STONE)
        _part(scene,'RetailPriceCard',(.26,.035,.12),(.27*w,-.28*d,.78),MAT_LABEL)
        _part(scene,'PedestalFloorPad',(w*.54,d*.54,.035),(0,0,.147),MAT_STEEL)
        _label(scene,'DisplayCollectionCode',-.28*w,-.28*d,.22)
    else:  # brass bookend and shelf accent set
        _part(scene,'AccentShelfBase',(w,d*.55,.075),(0,0,.0375),MAT_WOOD)
        for side,x in enumerate((-.36*w,.36*w)):
            _part(scene,f'BookendFoot_{side}',(.24,.38,.06),(x,0,.105),MAT_BRASS)
            _part(scene,f'BookendUpright_{side}',(.07,.38,.34),(x,0,.30),material)
            _part(scene,f'BookendFinial_{side}',(.13,.13,.13),(x,0,.52),MAT_STONE)
        for i in range(3+v):
            x=(i-(2+v)/2)*w*.14/(2+v)
            _part(scene,f'DisplayBook_{i}',(.14,.30,.36),(x,0,.29),MAT_LINEN)
        _part(scene,'ShelfBackReveal',(w*.92,.05,.30),(0,.25*d,.23),MAT_STEEL)
        _label(scene,'ShelfAccentInventoryTag',.38*w,-.31*d,.22)
    return scene


def build_asset(name, subcategory, material, asset_id, profile):
    try:
        number=int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'design completion asset ID outside A3251–A3500: {asset_id}') from exc
    if not 3251 <= number <= 3500:
        raise ValueError(f'design completion asset ID outside A3251–A3500: {asset_id}')
    batch,within_batch=divmod(number-3251,50)
    kind,variant=divmod(within_batch,5)
    builder=(_architecture,_guestroom,_public,_amenity,_decor)[batch]
    return builder(kind,variant,material)
