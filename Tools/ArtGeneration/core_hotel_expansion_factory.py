"""Purpose-built architecture, guestroom, public, housekeeping and dining assets A2751–A3000."""

import math

import trimesh

from v2_asset_common import add, box, cyl, sphere


MAT_STEEL = 'MAT_BLACKENED_STEEL'
MAT_STAINLESS = 'MAT_STAINLESS'
MAT_BRASS = 'MAT_BRASS_POLISHED'
MAT_STONE = 'MAT_STONE_LIGHT'
MAT_WOOD = 'MAT_WOOD_WARM'
MAT_GLASS = 'MAT_GLASS_CLEAR'
MAT_LABEL = 'MAT_SIGNAGE'
MAT_CERAMIC = 'MAT_CERAMIC_FIXTURE'
MAT_LINEN = 'MAT_LINEN'
MAT_ELECTRONICS = 'MAT_ELECTRONICS'


def _rot(mesh, angle, axis, point=None):
    mesh.apply_transform(trimesh.transformations.rotation_matrix(angle, axis, point=point))
    return mesh


def _legs(scene, width, depth, height, material=MAT_STEEL, prefix='Leg'):
    for index, (x, y) in enumerate(((-1,-1),(-1,1),(1,-1),(1,1))):
        add(scene,box((.05,.05,height),(x*width*.42,y*depth*.37,height/2),material),f'{prefix}_{index}')


def _wheels(scene, width, depth, radius=.07, prefix='Caster'):
    for index,(x,y) in enumerate(((-1,-1),(-1,1),(1,-1),(1,1))):
        center=(x*width*.40,y*depth*.38,radius)
        wheel=cyl(radius,.045,center,MAT_STEEL,12)
        _rot(wheel,math.pi/2,[1,0,0],center)
        add(scene,wheel,f'{prefix}_{index}')


def _post(scene,name,x,y,height,radius=.04,material=MAT_STEEL):
    add(scene,cyl(radius,height,(x,y,height/2),material,12),name)


# Batch 56: life-safety, structure, and hotel service-core construction details.
def _corridor_door(v,mat):
    scene=trimesh.Scene(); w=.96+.07*v; h=2.12+.08*v
    add(scene,box((w,.14,.08),(0,0,.04),MAT_STONE),'DoorThreshold')
    for side,x in enumerate((-.47*w,.47*w)):
        add(scene,box((.09,.14,h),(x,0,h/2+.08),mat),f'FireDoorJamb_{side}')
    add(scene,box((w,.14,.12),(0,0,h+.08),mat),'FireDoorHeader')
    add(scene,box((w*.82,.06,h*.88),(0,-.085,h*.52),MAT_WOOD),'RatedDoorSlab')
    add(scene,box((.10,.06,.28),(w*.31,-.13,1.05),MAT_STEEL),'PanicHardware')
    add(scene,cyl(.03,.04,(w*.34,-.15,1.05),MAT_BRASS,8),'DoorLever')
    add(scene,box((.19,.025,.09),(-w*.34,-.13,h-.24),MAT_LABEL),'FireRatingPlate')
    add(scene,box((.18,.07,.05),(0,.12,h+.08),MAT_STEEL),'CloserMount')
    return scene


def _elevator_jamb(v,mat):
    scene=trimesh.Scene(); w=1.32+.10*v; h=2.32+.08*v
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,box((.14,.18,h),(x,0,h/2),mat),f'ElevatorJamb_{side}')
        add(scene,box((.035,.22,h*.78),(x,-.10,h*.48),MAT_BRASS),f'JambReveal_{side}')
    add(scene,box((w,.18,.16),(0,0,h-.08),mat),'ElevatorHead')
    add(scene,box((w*.62,.05,.08),(0,-.11,h-.08),MAT_STEEL),'DoorTrackCover')
    add(scene,box((.13,.04,.32),(.54*w,-.15,1.22),MAT_ELECTRONICS),'CallButtonPanel')
    for i,z in enumerate((1.30,1.12)):
        add(scene,cyl(.026,.04,(.54*w,-.18,z),MAT_BRASS,10),f'CallButton_{i}')
    add(scene,box((.20,.025,.06),(-.30*w,-.14,.18),MAT_LABEL),'LiftServicePlate')
    return scene


def _partition_track(v,mat):
    scene=trimesh.Scene(); w=1.50+.12*v; z=3.10
    add(scene,box((w,.16,.12),(0,0,z),mat),'OperablePartitionTrack')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.82/(2+v)
        add(scene,cyl(.055,.09,(x,0,z-.12),MAT_STEEL,12),f'TrackRoller_{i}')
        add(scene,box((.055,.10,.34),(x,0,z-.34),MAT_BRASS),f'PanelHanger_{i}')
    for side,x in enumerate((-.47*w,.47*w)):
        add(scene,box((.07,.18,.22),(x,0,z-.08),MAT_STEEL),f'TrackEndStop_{side}')
    add(scene,box((.19,.025,.05),(w*.36,-.10,z-.23),MAT_LABEL),'TrackLoadPlate')
    return scene


def _balcony_bay(v,mat):
    scene=trimesh.Scene(); w=1.32+.10*v; d=.92+.08*v; h=1.12+.08*v
    add(scene,box((w,d,.18),(0,0,.09),MAT_STONE),'BalconySlab')
    add(scene,box((w*.92,.035,.10),(0,-d*.46,.22),MAT_BRASS),'BalconyThreshold')
    for side,x in enumerate((-.46*w,.46*w)):
        add(scene,box((.12,d,.20),(x,0,.10),MAT_STEEL),f'SlabEdgeBeam_{side}')
    for i in range(6+v):
        x=(i-(5+v)/2)*w*.82/(5+v)
        _post(scene,f'BalustradePost_{i}',x,-d*.39,h,.028,mat)
    for z in (.42,.82,h):
        add(scene,box((w*.84,.05,.05),(0,-d*.39,z),MAT_STEEL),'Guardrail')
    add(scene,box((.20,.025,.06),(w*.35,-d*.50,.18),MAT_LABEL),'BalconyLoadTag')
    return scene


def _shear_wall_node(v,mat):
    scene=trimesh.Scene(); w=1.08+.08*v; h=1.22+.10*v
    add(scene,box((w,.24,h),(0,0,1.55),mat),'ShearWallPanel')
    for side,x in enumerate((-.43*w,.43*w)):
        add(scene,box((.10,.30,h*.92),(x,0,1.55),MAT_STEEL),f'BoundaryColumn_{side}')
        for i in range(4+v):
            z=1.55-h*.40+i*h*.80/(3+v)
            add(scene,cyl(.025,.34,(x,0,z),MAT_BRASS,8),f'HoldDownAnchor_{side}_{i}')
    add(scene,box((w*.88,.06,.09),(0,0,1.55+h*.45),MAT_STEEL),'WallCapPlate')
    add(scene,box((.18,.025,.05),(w*.34,-.15,1.55-h*.42),MAT_LABEL),'SeismicDetailCode')
    return scene


def _stair_guard(v,mat):
    scene=trimesh.Scene(); w=1.22+.10*v; d=.72+.06*v; h=1.04+.08*v
    add(scene,box((w,d,.14),(0,0,.07),MAT_STONE),'StairLanding')
    for i in range(5+v):
        x=(i-(4+v)/2)*w*.84/(4+v)
        _post(scene,f'LandingBaluster_{i}',x,-d*.38,h,.025,mat)
    add(scene,box((w*.90,.08,.08),(0,-d*.38,h),MAT_WOOD),'StairHandrail')
    add(scene,box((w*.88,.045,.045),(0,-d*.38,.60),MAT_STEEL),'Midrail')
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,box((.12,d*.90,.08),(x,0,.17),MAT_STEEL),f'GuardBasePlate_{side}')
    add(scene,box((.18,.025,.06),(w*.34,-d*.51,.20),MAT_LABEL),'HandrailHeightTag')
    return scene


def _riser_raceway(v,mat):
    scene=trimesh.Scene(); w=.62+.05*v; d=.28+.025*v; h=2.12+.12*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STEEL),'RiserPlinth')
    add(scene,box((w,d,h),(0,0,h/2+.06),mat),'RiserEnclosure')
    add(scene,box((w*.78,.035,h*.76),(0,-d*.53,h*.55),MAT_WOOD),'AccessDoor')
    for i in range(3+v):
        z=.44+i*.20
        add(scene,box((w*.48,.045,.06),(0,-d*.57,z),MAT_STEEL),f'CableEntry_{i}')
    for side,x in enumerate((-.34*w,.34*w)):
        add(scene,cyl(.025,.04,(x,-d*.58,.56),MAT_BRASS,8),f'Latch_{side}')
    add(scene,box((.18,.025,.08),(0,-d*.60,h-.20),MAT_LABEL),'RiserFloorID')
    return scene


def _floor_hatch(v,mat):
    scene=trimesh.Scene(); w=.82+.08*v; d=.68+.06*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'HatchFrame')
    add(scene,box((w*.90,d*.88,.08),(0,0,.16),mat),'SumpAccessPanel')
    add(scene,box((w*.82,.035,.025),(0,-d*.30,.215),MAT_STEEL),'HatchHingeBar')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.64/(2+v)
        add(scene,box((.026,d*.62,.02),(x,0,.215),MAT_BRASS),f'GripRib_{i}')
    add(scene,cyl(.035,.04,(w*.32,-d*.36,.22),MAT_STEEL,8),'LiftRing')
    add(scene,box((.18,.025,.05),(0,d*.54,.20),MAT_LABEL),'HatchAssetTag')
    return scene


def _roof_screen(v,mat):
    scene=trimesh.Scene(); w=1.20+.10*v; h=1.58+.12*v
    add(scene,box((w,.12,.14),(0,0,.07),MAT_STONE),'ScreenSill')
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,box((.10,.16,h),(x,0,h/2+.14),MAT_STEEL),f'ScreenFrame_{side}')
    add(scene,box((w,.16,.10),(0,0,h+.14),MAT_STEEL),'ScreenHeader')
    for i in range(5+v):
        x=(i-(4+v)/2)*w*.84/(4+v)
        add(scene,box((.06,.05,h*.84),(x,0,h*.54),mat),f'PlantLouver_{i}')
    add(scene,box((.20,.025,.06),(w*.34,-.09,.20),MAT_LABEL),'RoofScreenTag')
    return scene


def _expansion_cover(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; d=.40+.04*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'ExpansionJointSubstrate')
    add(scene,box((w*.82,d*.42,.065),(0,0,.1525),mat),'MovementCoverPlate')
    for side,y in enumerate((-.40*d,.40*d)):
        add(scene,box((w*.96,.045,.035),(0,y,.14),MAT_STEEL),f'JointEdgeAngle_{side}')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.72/(3+v)
        add(scene,cyl(.022,.025,(x,0,.195),MAT_BRASS,8),f'CoverFastener_{i}')
    add(scene,box((.18,.025,.05),(w*.34,-d*.54,.18),MAT_LABEL),'MovementRating')
    return scene


def _architecture(family,v,mat):
    return (_corridor_door,_elevator_jamb,_partition_track,_balcony_bay,_shear_wall_node,
            _stair_guard,_riser_raceway,_floor_hatch,_roof_screen,_expansion_cover)[family](v,mat)


# Batch 57: guestroom casegoods, sleeping, storage, and in-room service fixtures.
def _sofa_bed(v,mat):
    scene=trimesh.Scene(); w=1.42+.10*v; d=.72+.06*v; h=.48+.04*v
    add(scene,box((w,d,.12),(0,0,h),mat),'SofaBedSeatFrame')
    add(scene,box((w*.84,.14,.52),(0,d*.42,h+.24),mat),'SofaBack')
    for side,x in enumerate((-.46*w,.46*w)):
        add(scene,box((.13,d*.86,.48),(x,0,h-.12),MAT_WOOD),f'SofaArm_{side}')
    add(scene,box((w*.92,d*.90,.09),(0,0,.045),MAT_STEEL),'PulloutBedPlatform')
    for side,y in enumerate((-.33*d,.33*d)):
        add(scene,box((w*.78,.055,.06),(0,y,.18),MAT_STEEL),f'BedSupportRail_{side}')
    add(scene,box((w*.78,d*.76,.13),(0,0,h+.10),MAT_LINEN),'SeatCushion')
    return scene


def _window_seat(v,mat):
    scene=trimesh.Scene(); w=1.20+.10*v; d=.58+.05*v; h=.54+.04*v
    add(scene,box((w,d,h),(0,0,h/2),mat),'WindowSeatCase')
    add(scene,box((w*.96,d*.92,.12),(0,0,h+.06),MAT_LINEN),'WindowSeatCushion')
    add(scene,box((w*.96,.09,.52),(0,d*.43,h+.32),MAT_WOOD),'WindowSeatBack')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*w*.56/(1+v//2)
        add(scene,box((w*.22,.035,.20),(x,-d*.52,h*.42),MAT_WOOD),f'DrawerFace_{i}')
        add(scene,cyl(.022,.035,(x,-d*.55,h*.42),MAT_BRASS,8),f'DrawerPull_{i}')
    add(scene,box((.20,.025,.06),(w*.34,-d*.54,.18),MAT_LABEL),'CasegoodsLotTag')
    return scene


def _underbed_drawer(v,mat):
    scene=trimesh.Scene(); w=.90+.08*v; d=.58+.05*v; h=.22+.025*v
    add(scene,box((w,d,.08),(0,0,h/2),mat),'LuggageDrawerCase')
    add(scene,box((w*.94,.035,h*.72),(0,-d*.52,h*.52),MAT_WOOD),'DrawerFront')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.70/(2+v)
        add(scene,box((.10,d*.72,.04),(x,0,h*.78),MAT_STONE),f'FoldedLuggageDivider_{i}')
    add(scene,box((.22,.035,.04),(0,-d*.55,h*.55),MAT_BRASS),'RecessedPull')
    for side,x in enumerate((-.43*w,.43*w)):
        add(scene,cyl(.035,.045,(x,0,.035),MAT_STEEL,10),f'LowProfileRoller_{side}')
    add(scene,box((.18,.025,.05),(w*.32,-d*.55,.08),MAT_LABEL),'DrawerID')
    return scene


def _robe_valet(v,mat):
    scene=trimesh.Scene(); w=.48+.04*v; h=1.22+.10*v
    add(scene,box((w,.34,.08),(0,0,.04),MAT_STONE),'ValetBase')
    _post(scene,'ValetStem',0,0,h,.035,mat)
    add(scene,box((w*.86,.06,.08),(0,0,h-.06),MAT_WOOD),'JacketHangerBar')
    add(scene,box((w*.72,.08,.05),(0,0,.72),MAT_BRASS),'TrouserBar')
    for i in range(2+v):
        x=(i-(1+v)/2)*w*.45/(1+v)
        add(scene,cyl(.025,.04,(x,-.06,h-.04),MAT_STEEL,8),f'GarmentHook_{i}')
    add(scene,box((.18,.025,.06),(w*.25,-.19,.15),MAT_LABEL),'ValetMakerPlate')
    return scene


def _minibar_cabinet(v,mat):
    scene=trimesh.Scene(); w=.62+.05*v; d=.48+.04*v; h=.92+.08*v
    add(scene,box((w,d,.08),(0,0,.04),MAT_WOOD),'MinibarPlinth')
    add(scene,box((w,d,h),(0,0,h/2+.08),mat),'MinibarCabinet')
    add(scene,box((w*.76,.035,h*.76),(0,-d*.53,h*.54),MAT_WOOD),'MinibarDoor')
    add(scene,box((w*.64,d*.60,.06),(0,0,h*.68),MAT_STEEL),'BottleShelf')
    add(scene,box((w*.60,d*.58,.06),(0,0,h*.36),MAT_STEEL),'SnackShelf')
    add(scene,cyl(.025,.04,(w*.30,-d*.57,h*.48),MAT_BRASS,8),'DoorLock')
    for i in range(4+v):
        z=.22+i*.07
        add(scene,box((.12,.025,.018),(0,-d*.56,z),MAT_STEEL),f'CompressorVent_{i}')
    add(scene,box((.16,.025,.06),(w*.30,-d*.56,.20),MAT_LABEL),'MinibarEnergyTag')
    return scene


def _coffee_niche(v,mat):
    scene=trimesh.Scene(); w=.76+.06*v; d=.46+.04*v; h=1.32+.08*v
    add(scene,box((w,d,.08),(0,0,.04),MAT_WOOD),'CoffeeNicheBase')
    add(scene,box((w,d,h),(0,0,h/2+.08),mat),'CoffeeCabinet')
    add(scene,box((w*.82,.03,h*.58),(0,-d*.53,h*.58),MAT_WOOD),'NicheBackPanel')
    add(scene,box((w*.80,d*.68,.055),(0,0,h*.68),MAT_STONE),'CupShelf')
    add(scene,box((w*.64,d*.62,.055),(0,0,h*.32),MAT_STONE),'LowerServiceShelf')
    add(scene,box((.26,.22,.27),(0,-d*.13,h*.82),MAT_ELECTRONICS),'CoffeeMachine')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*.12
        add(scene,cyl(.045,.07,(x,d*.22,h*.75),MAT_CERAMIC,10),f'CoffeeCup_{i}')
    add(scene,box((.18,.025,.06),(w*.32,-d*.55,.18),MAT_LABEL),'NicheServiceCard')
    return scene


def _safe_shelf(v,mat):
    scene=trimesh.Scene(); w=.66+.05*v; d=.46+.04*v; h=.96+.06*v
    add(scene,box((w,d,.07),(0,0,.035),MAT_WOOD),'WardrobeShelfBase')
    add(scene,box((w,d,h),(0,0,h/2+.07),mat),'SafeShelfCase')
    add(scene,box((w*.70,.035,h*.72),(0,-d*.53,h*.52),MAT_STEEL),'SafeDoor')
    add(scene,cyl(.032,.04,(w*.25,-d*.57,h*.52),MAT_BRASS,8),'SafeHandle')
    add(scene,box((.14,.035,.12),(-w*.22,-d*.57,h*.52),MAT_ELECTRONICS),'DigitalKeypad')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.65/(2+v)
        add(scene,box((.04,d*.70,.03),(x,0,h*.23),MAT_WOOD),f'JewelryTrayDivider_{i}')
    add(scene,box((.18,.025,.05),(0,-d*.57,.18),MAT_LABEL),'SafeSetupCard')
    return scene


def _desk_hutch(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; d=.56+.05*v; h=.78+.05*v
    add(scene,box((w,d,.10),(0,0,h),mat),'WritingDeskTop')
    _legs(scene,w,d,h-.05,MAT_WOOD,'DeskLeg')
    add(scene,box((w*.80,.11,.46),(0,d*.36,h+.28),MAT_WOOD),'DeskHutchBack')
    for i in range(2+v//2):
        z=h+.15+i*.15
        add(scene,box((w*.72,.18,.045),(0,d*.25,z),MAT_STONE),f'WritingShelf_{i}')
    add(scene,box((.20,.16,.04),(w*.29,-d*.12,h+.08),MAT_ELECTRONICS),'DeskTaskLampBase')
    _post(scene,'DeskTaskLampStem',w*.29,-d*.12,.28,.015,MAT_BRASS)
    add(scene,box((.16,.06,.07),(w*.29,-d*.12,.31),MAT_GLASS),'DeskLampShade')
    return scene


def _rollaway_cot(v,mat):
    scene=trimesh.Scene(); w=1.12+.10*v; d=.62+.05*v; h=.42+.04*v
    add(scene,box((w,d,.08),(0,0,h),MAT_STEEL),'RollawayCotFrame')
    add(scene,box((w*.94,d*.92,.12),(0,0,h+.10),mat),'CotMattress')
    for side,y in enumerate((-.42*d,.42*d)):
        add(scene,box((w*.86,.035,.06),(0,y,h-.03),MAT_STEEL),f'FoldLegRail_{side}')
    _legs(scene,w*.88,d*.82,h-.05,MAT_STEEL,'CotLeg')
    _wheels(scene,w*.92,d*.90,.06,'CotCaster')
    add(scene,box((.18,.025,.06),(w*.34,-d*.52,h+.08),MAT_LABEL),'CotCapacityLabel')
    return scene


def _headboard(v,mat):
    scene=trimesh.Scene(); w=1.78+.12*v; h=1.12+.08*v
    add(scene,box((w,.14,h),(0,0,1.48),mat),'UpholsteredHeadboard')
    add(scene,box((w*.84,.05,h*.76),(0,-.10,1.48),MAT_LINEN),'HeadboardCenterPanel')
    for side,x in enumerate((-.45*w,.45*w)):
        add(scene,box((.08,.18,h*.92),(x,0,1.48),MAT_WOOD),f'HeadboardWing_{side}')
        add(scene,box((.26,.32,.12),(x,-.20,.52),MAT_WOOD),f'BedsideLedge_{side}')
        _post(scene,f'ReadingLightStem_{side}',x,-.16,1.72,.014,MAT_BRASS)
        add(scene,sphere(.055,(x,-.16,1.74),MAT_GLASS,0),f'ReadingLight_{side}')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.66/(3+v)
        add(scene,cyl(.018,.025,(x,-.14,1.48),MAT_BRASS,8),f'PanelButton_{i}')
    add(scene,box((.20,.025,.06),(w*.32,-.19,1.00),MAT_LABEL),'HeadboardModelTag')
    return scene


def _guestroom(family,v,mat):
    return (_sofa_bed,_window_seat,_underbed_drawer,_robe_valet,_minibar_cabinet,_coffee_niche,
            _safe_shelf,_desk_hutch,_rollaway_cot,_headboard)[family](v,mat)


# Batch 58: lobby service, guest information, and public circulation fixtures.
def _concierge_station(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; d=.58+.05*v; h=1.06+.06*v
    add(scene,box((w,d,.14),(0,0,h),mat),'ConciergeCounter')
    _legs(scene,w,d,h-.07,MAT_WOOD,'CounterSupport')
    add(scene,box((w*.70,.045,.30),(0,d*.28,h+.21),MAT_STEEL),'PrivacyScreen')
    add(scene,box((.34,.22,.12),(0,-d*.10,h+.13),MAT_ELECTRONICS),'WorkTerminal')
    add(scene,box((.28,.025,.08),(0,-d*.53,h-.10),MAT_LABEL),'ConciergeNamePlate')
    for side,x in enumerate((-.32*w,.32*w)):
        add(scene,box((.18,.26,.035),(x,d*.12,h+.09),MAT_WOOD),f'GuestFormTray_{side}')
    return scene


def _queue_stanchions(v,mat):
    scene=trimesh.Scene(); span=1.12+.10*v
    for side,x in enumerate((-.46*span,.46*span)):
        add(scene,cyl(.16,.09,(x,0,.045),MAT_STONE,14),f'StanchionBase_{side}')
        _post(scene,f'QueuePost_{side}',x,0,.94+.06*v,.035,mat)
        add(scene,cyl(.09,.06,(x,0,.99+.06*v),MAT_BRASS,12),f'RopeFinial_{side}')
    add(scene,box((span*.70,.04,.045),(0,0,.72+.05*v),MAT_LINEN),'QueueRope')
    add(scene,box((.20,.025,.06),(0,-.12,.22),MAT_LABEL),'QueueCapacityTag')
    return scene


def _luggage_claim_bench(v,mat):
    scene=trimesh.Scene(); w=1.34+.10*v; d=.56+.05*v; h=.52+.04*v
    add(scene,box((w,d,.12),(0,0,h),mat),'BaggageClaimBenchSeat')
    add(scene,box((w*.92,.08,.38),(0,d*.40,h+.24),MAT_WOOD),'BenchBack')
    _legs(scene,w,d,h-.05,MAT_STEEL,'BenchLeg')
    add(scene,box((w*.74,d*.70,.045),(0,0,.13),MAT_STEEL),'BagShelf')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.66/(2+v)
        add(scene,box((.04,d*.62,.025),(x,0,.18),MAT_BRASS),f'BagShelfDivider_{i}')
    add(scene,box((.20,.025,.06),(w*.35,-d*.53,h-.08),MAT_LABEL),'BenchFinishTag')
    return scene


def _coat_check_rack(v,mat):
    scene=trimesh.Scene(); w=.62+.05*v; h=1.62+.10*v
    add(scene,box((w,.40,.09),(0,0,.045),MAT_STONE),'TicketRackFoot')
    for side,x in enumerate((-.40*w,.40*w)):
        _post(scene,f'RackUpright_{side}',x,0,h,.035,mat)
    add(scene,box((w,.07,.07),(0,0,h),MAT_WOOD),'CoatHangerBar')
    for i in range(5+v):
        x=(i-(4+v)/2)*w*.74/(4+v)
        add(scene,cyl(.025,.05,(x,-.10,h-.05),MAT_BRASS,8),f'CoatHook_{i}')
        add(scene,box((.08,.03,.10),(x,-.13,.34),MAT_LABEL),f'TicketSlot_{i}')
    add(scene,box((.20,.025,.06),(0,-.22,.17),MAT_LABEL),'CoatCheckServicePlate')
    return scene


def _charging_tower(v,mat):
    scene=trimesh.Scene(); w=.42+.04*v; h=1.28+.12*v
    add(scene,cyl(.22,.10,(0,0,.05),MAT_STONE,16),'ChargeTowerBase')
    add(scene,box((w,.18,h),(0,0,h/2+.10),mat),'GuestChargingColumn')
    for side,x in enumerate((-.31*w,.31*w)):
        add(scene,box((.12,.035,.22),(x,-.11,h*.68),MAT_ELECTRONICS),f'USBPortPanel_{side}')
        for row,z in enumerate((h*.64,h*.72,h*.80)):
            add(scene,cyl(.018,.025,(x,-.14,z),MAT_BRASS,8),f'ChargePort_{side}_{row}')
    add(scene,box((w*.72,.025,.11),(0,-.12,h*.40),MAT_GLASS),'StatusDisplay')
    add(scene,box((.18,.025,.06),(0,-.12,.20),MAT_LABEL),'TowerRateLabel')
    return scene


def _message_cubby(v,mat):
    scene=trimesh.Scene(); w=.88+.08*v; d=.34+.035*v; h=1.52+.10*v
    add(scene,box((w,d,.10),(0,0,.05),MAT_WOOD),'MessageCabinetPlinth')
    add(scene,box((w,d,h),(0,0,h/2+.10),mat),'GuestMessageCabinet')
    rows,cols=3+v//2,3
    for row in range(rows):
        for col in range(cols):
            x=(col-1)*w*.66/cols; z=.32+row*.25
            add(scene,box((w*.19,.045,.19),(x,-d*.54,z),MAT_WOOD),f'MessageCubbie_{row}_{col}')
            add(scene,box((.11,.025,.04),(x,-d*.57,z+.07),MAT_LABEL),f'CubbieNumber_{row}_{col}')
    add(scene,box((.22,.025,.06),(0,-d*.58,h+.02),MAT_LABEL),'MessageDeskDirectory')
    return scene


def _phone_booth(v,mat):
    scene=trimesh.Scene(); w=.92+.06*v; d=.84+.06*v; h=2.24+.10*v
    add(scene,box((w,d,.10),(0,0,.05),MAT_STONE),'PhoneBoothFloor')
    for side,x in enumerate((-.45*w,.45*w)):
        add(scene,box((.09,d,h),(x,0,h/2+.10),mat),f'AcousticBoothWall_{side}')
    add(scene,box((w,.10,.09),(0,0,h+.10),mat),'PhoneBoothCeiling')
    add(scene,box((.70,.045,h*.84),(0,d*.46,h*.52),MAT_GLASS),'BoothRearGlass')
    add(scene,box((.16,.035,.56),(.28*w,-d*.48,1.22),MAT_ELECTRONICS),'WallPhone')
    add(scene,box((.32,.25,.06),(-.12*w,-d*.18,1.04),MAT_WOOD),'PhoneShelf')
    add(scene,box((.22,.025,.06),(0,-d*.51,.20),MAT_LABEL),'BoothAcousticRating')
    return scene


def _bell_cart_dock(v,mat):
    scene=trimesh.Scene(); w=1.22+.10*v; d=.82+.06*v; h=2.02+.12*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'CartDockBase')
    for side,x in enumerate((-.43*w,.43*w)):
        add(scene,box((.10,d*.90,h),(x,0,h/2+.12),mat),f'CartGuideRail_{side}')
    add(scene,box((w*.92,.12,.12),(0,d*.38,h+.12),MAT_WOOD),'DockHeader')
    add(scene,box((w*.76,.05,.08),(0,-d*.48,.32),MAT_STEEL),'WheelStop')
    for i in range(3+v):
        z=.55+i*.22
        add(scene,box((w*.66,.045,.035),(0,d*.18,z),MAT_BRASS),f'CartShelfGuide_{i}')
    add(scene,box((.20,.025,.06),(0,-d*.52,.20),MAT_LABEL),'DockAssetCode')
    return scene


def _info_kiosk(v,mat):
    scene=trimesh.Scene(); w=.62+.05*v; h=1.48+.12*v
    add(scene,box((w*.80,.58,.10),(0,0,.05),MAT_STONE),'KioskFoot')
    add(scene,box((w,.25,h),(0,0,h/2+.10),mat),'InformationKioskBody')
    add(scene,box((w*.82,.035,.44),(0,-.15,h*.72),MAT_GLASS),'InteractiveMapScreen')
    add(scene,box((w*.70,.035,.15),(0,-.15,h*.37),MAT_ELECTRONICS),'KioskKeypad')
    add(scene,cyl(.035,.04,(w*.30,-.18,h*.37),MAT_BRASS,8),'KioskAssistButton')
    add(scene,box((.18,.025,.06),(0,-.16,.20),MAT_LABEL),'InformationRevision')
    return scene


def _lobby_banquette(v,mat):
    scene=trimesh.Scene(); w=1.50+.12*v; d=.68+.06*v; h=.50+.04*v
    add(scene,box((w,d,.20),(0,0,h),mat),'BanquetteBase')
    add(scene,box((w*.96,d*.92,.14),(0,0,h+.15),MAT_LINEN),'BanquetteSeatCushion')
    add(scene,box((w*.94,.14,.58),(0,d*.40,h+.48),mat),'BanquetteBack')
    for side,x in enumerate((-.43*w,.43*w)):
        add(scene,box((.12,d*.84,.35),(x,0,.18),MAT_WOOD),f'BanquetteEnd_{side}')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.70/(2+v)
        add(scene,box((.035,d*.62,.05),(x,0,.15),MAT_STEEL),f'BanquetteSupport_{i}')
    add(scene,box((.18,.025,.06),(w*.37,-d*.53,h-.09),MAT_LABEL),'UpholsteryLotTag')
    return scene


def _public(family,v,mat):
    return (_concierge_station,_queue_stanchions,_luggage_claim_bench,_coat_check_rack,_charging_tower,
            _message_cubby,_phone_booth,_bell_cart_dock,_info_kiosk,_lobby_banquette)[family](v,mat)


# Batch 59: housekeeping workflow, linen movement, and floor-care equipment.
def _laundry_sorter(v,mat):
    scene=trimesh.Scene(); w=.84+.08*v; d=.58+.05*v; h=.88+.06*v
    add(scene,box((w,d,.10),(0,0,.22),MAT_STEEL),'SorterCartBase')
    _wheels(scene,w,d,.065,'SorterCaster')
    for i in range(3+v//2):
        x=(i-(2+v//2)/2)*w*.68/(2+v//2)
        add(scene,box((w*.25,d*.72,h),(x,0,h/2+.23),mat),f'LinenSortBin_{i}')
        add(scene,box((w*.22,.025,.09),(x,-d*.38,h+.23),MAT_LABEL),f'BinTextileCode_{i}')
    add(scene,box((.18,.025,.06),(0,-d*.52,.24),MAT_LABEL),'SorterRouteTag')
    return scene


def _linen_cage(v,mat):
    scene=trimesh.Scene(); w=.92+.08*v; d=.68+.06*v; h=1.38+.10*v
    add(scene,box((w*.92,d*.90,.10),(0,0,.22),MAT_STEEL),'CageCartBase')
    _wheels(scene,w,d,.07,'CageCaster')
    for side,x in enumerate((-.44*w,.44*w)):
        _post(scene,f'CageUpright_{side}',x,0,h,.035,mat)
    for z in (.48,.88,1.28):
        add(scene,box((w*.84,d*.78,.045),(0,0,z),MAT_STEEL),'LinenCageShelf')
    for side,y in enumerate((-.38*d,.38*d)):
        add(scene,box((w*.84,.035,h*.78),(0,y,h*.52),MAT_GLASS),f'CageMeshPanel_{side}')
    add(scene,box((.22,.025,.07),(0,-d*.53,.30),MAT_LABEL),'LinenCageRouteCard')
    return scene


def _room_service_cart(v,mat):
    scene=trimesh.Scene(); w=.88+.08*v; d=.56+.05*v; h=.92+.06*v
    add(scene,box((w,d,.11),(0,0,h),mat),'RoomServiceCartTop')
    _legs(scene,w,d,h-.05,MAT_STEEL,'ServiceCartPost')
    _wheels(scene,w,d,.07,'ServiceCartCaster')
    add(scene,box((w*.76,d*.72,.045),(0,0,.46),MAT_WOOD),'LowerServiceShelf')
    for i in range(2+v):
        x=(i-(1+v)/2)*w*.68/(1+v)
        add(scene,cyl(.075,.035,(x,0,h+.08),MAT_CERAMIC,12),f'RoomServiceCloche_{i}')
    add(scene,box((.20,.025,.06),(0,-d*.53,h-.08),MAT_LABEL),'ServiceCartRouteCard')
    return scene


def _lost_found(v,mat):
    scene=trimesh.Scene(); w=.78+.06*v; d=.46+.04*v; h=1.24+.10*v
    add(scene,box((w,d,.08),(0,0,.04),MAT_WOOD),'LostFoundPlinth')
    add(scene,box((w,d,h),(0,0,h/2+.08),mat),'LostFoundCabinet')
    for i in range(2+v//2):
        z=.34+i*.22
        add(scene,box((w*.78,d*.60,.05),(0,0,z),MAT_STEEL),f'PropertyShelf_{i}')
    add(scene,box((w*.78,.04,h*.68),(0,-d*.53,h*.55),MAT_WOOD),'CabinetDoor')
    for side,x in enumerate((-.30*w,.30*w)):
        add(scene,cyl(.024,.035,(x,-d*.57,h*.53),MAT_BRASS,8),f'DoorHinge_{side}')
    add(scene,cyl(.028,.04,(w*.30,-d*.58,h*.53),MAT_STEEL,8),'CabinetLock')
    add(scene,box((.22,.025,.07),(0,-d*.58,.20),MAT_LABEL),'PropertyLogID')
    return scene


def _vacuum_dock(v,mat):
    scene=trimesh.Scene(); w=.70+.06*v; d=.40+.04*v; h=1.12+.10*v
    add(scene,box((w,d,.08),(0,0,.04),MAT_STONE),'VacuumDockBase')
    add(scene,box((w*.86,.10,h),(0,0,h/2+.08),mat),'VacuumChargingColumn')
    add(scene,box((w*.70,.045,.22),(0,-d*.54,h*.70),MAT_ELECTRONICS),'ChargerPanel')
    add(scene,box((w*.62,.10,.16),(0,-d*.42,.22),MAT_STEEL),'NozzleCradle')
    _post(scene,'VacuumHoseGuide',w*.30,0,.76,.035,MAT_STEEL)
    add(scene,cyl(.18,.08,(0,0,.42),MAT_STEEL,16),'VacuumCanister')
    add(scene,box((.18,.025,.06),(0,-d*.56,.18),MAT_LABEL),'DockCircuitLabel')
    return scene


def _chemical_station(v,mat):
    scene=trimesh.Scene(); w=.76+.06*v; d=.46+.04*v; h=.98+.08*v
    add(scene,box((w,d,.10),(0,0,h),mat),'DilutionCounter')
    _legs(scene,w,d,h-.05,MAT_STEEL,'DilutionStationLeg')
    add(scene,box((w*.76,.07,.42),(0,d*.34,h+.23),MAT_STEEL),'BacksplashPanel')
    for i in range(3+v//2):
        x=(i-(2+v//2)/2)*w*.58/(2+v//2)
        add(scene,cyl(.055,.18,(x,-d*.15,h+.18),MAT_CERAMIC,12),f'ChemicalBottle_{i}')
        add(scene,cyl(.023,.05,(x,-d*.15,h+.30),MAT_BRASS,8),f'BottleCap_{i}')
    add(scene,box((.24,.025,.09),(0,-d*.53,h-.10),MAT_LABEL),'DilutionRatioCard')
    return scene


def _mop_rack(v,mat):
    scene=trimesh.Scene(); w=.86+.08*v; h=1.32+.10*v
    add(scene,box((w,.10,.08),(0,0,1.45),mat),'WallRackBackplate')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.76/(3+v)
        add(scene,cyl(.05,.05,(x,-.09,1.62),MAT_BRASS,10),f'ToolHook_{i}')
        add(scene,box((.045,.05,h*.56),(x,-.12,1.45-h*.28),MAT_WOOD),f'MopHandle_{i}')
        add(scene,box((.16,.10,.10),(x,-.14,.86),MAT_LINEN),f'MopHead_{i}')
    add(scene,box((.18,.025,.06),(w*.31,-.16,1.25),MAT_LABEL),'JanitorialRackID')
    return scene


def _linen_trolley(v,mat):
    scene=trimesh.Scene(); w=.90+.08*v; d=.62+.05*v; h=.98+.08*v
    add(scene,box((w,d,.12),(0,0,h),mat),'CleanLinenTrolleyTop')
    _legs(scene,w,d,h-.06,MAT_STEEL,'TrolleyUpright')
    _wheels(scene,w,d,.07,'LinenTrolleyCaster')
    for i in range(2+v//2):
        z=.30+i*.22
        add(scene,box((w*.78,d*.74,.045),(0,0,z),MAT_WOOD),f'LinenShelf_{i}')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.68/(2+v)
        add(scene,box((.045,.045,.18),(x,d*.34,h+.10),MAT_STEEL),f'FreshTowelStack_{i}')
    add(scene,box((.20,.025,.06),(0,-d*.53,h-.08),MAT_LABEL),'TrolleyRoomRangeTag')
    return scene


def _turnover_chest(v,mat):
    scene=trimesh.Scene(); w=.82+.07*v; d=.58+.05*v; h=.62+.05*v
    add(scene,box((w,d,h),(0,0,h/2),mat),'TurnoverSupplyChest')
    add(scene,box((w*.94,d*.92,.08),(0,0,h+.04),MAT_WOOD),'ChestLid')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.70/(2+v)
        add(scene,box((.16,d*.72,.08),(x,0,h*.46),MAT_STONE),f'SupplyCompartment_{i}')
        add(scene,box((.11,.025,.05),(x,-d*.52,h*.30),MAT_LABEL),f'CompartmentLabel_{i}')
    for side,x in enumerate((-.42*w,.42*w)):
        add(scene,box((.08,.12,.045),(x,0,h*.55),MAT_BRASS),f'ChestHandle_{side}')
    add(scene,box((.18,.025,.05),(0,-d*.53,.10),MAT_LABEL),'TurnoverKitID')
    return scene


def _floor_scrubber(v,mat):
    scene=trimesh.Scene(); w=.74+.06*v; d=.96+.08*v; h=.70+.06*v
    add(scene,box((w,d*.72,.12),(0,0,h),mat),'ScrubberBody')
    add(scene,cyl(.29+.02*v,.12,(0,d*.28,.10),MAT_STEEL,20),'ScrubbingBrush')
    add(scene,cyl(.24,.07,(0,d*.28,.19),MAT_CERAMIC,20),'BrushHub')
    _wheels(scene,w*.82,d*.80,.08,'ScrubberCaster')
    add(scene,box((w*.66,.18,.13),(0,-d*.30,h+.08),MAT_ELECTRONICS),'OperatorControlPanel')
    _post(scene,'HandleStem',0,-d*.38,.62,.025,MAT_STEEL)
    add(scene,box((.36,.06,.06),(0,-d*.38,.64),MAT_WOOD),'PushHandle')
    add(scene,box((.20,.025,.06),(0,-d*.53,.24),MAT_LABEL),'ScrubberModelPlate')
    return scene


def _housekeeping(family,v,mat):
    return (_laundry_sorter,_linen_cage,_room_service_cart,_lost_found,_vacuum_dock,
            _chemical_station,_mop_rack,_linen_trolley,_turnover_chest,_floor_scrubber)[family](v,mat)


# Batch 60: chef's table, buffet, and tableside dining service equipment.
def _pasta_counter(v,mat):
    scene=trimesh.Scene(); w=1.16+.10*v; d=.70+.06*v; h=.88+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'PastaServiceCounter')
    _legs(scene,w,d,h-.06,MAT_STEEL,'PastaStationLeg')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*w*.44/(1+v//2)
        add(scene,cyl(.14,.035,(x,-.03,h+.08),MAT_STEEL,16),f'PastaBoilerWell_{i}')
        add(scene,cyl(.10,.035,(x,-.03,h+.12),MAT_CERAMIC,16),f'BoilerInset_{i}')
    add(scene,box((w*.58,.045,.16),(0,d*.30,h+.16),MAT_STEEL),'IngredientRail')
    add(scene,box((.22,.025,.07),(0,-d*.53,h-.09),MAT_LABEL),'PastaStationMenu')
    return scene


def _charcuterie_case(v,mat):
    scene=trimesh.Scene(); w=1.05+.09*v; d=.62+.05*v; h=.78+.06*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'CharcuterieCaseBase')
    add(scene,box((w*.82,d*.74,h-.12),(0,0,h/2),mat),'DisplayPedestal')
    add(scene,box((w*.88,d*.78,.36),(0,0,h+.12),MAT_GLASS),'ChilledGlassCase')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.68/(2+v)
        add(scene,box((.12,.16,.045),(x,-d*.10,h+.14),MAT_WOOD),f'TastingBoard_{i}')
    for side,x in enumerate((-.42*w,.42*w)):
        add(scene,box((.035,d*.64,.06),(x,0,h+.10),MAT_STEEL),f'CaseShelfEdge_{side}')
    add(scene,box((.20,.025,.06),(0,-d*.53,h*.48),MAT_LABEL),'CharcuterieOriginCard')
    return scene


def _buffet_riser(v,mat):
    scene=trimesh.Scene(); w=1.08+.10*v; d=.58+.05*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STEEL),'RiserBase')
    for i in range(2+v//2):
        z=.24+i*.20
        width=w*(.82-.08*i)
        add(scene,box((width,d*.70,.07),(0,0,z),mat),f'BuffetDisplayTier_{i}')
        add(scene,box((width*.92,.035,.05),(0,-d*.36,z+.055),MAT_BRASS),f'TierFrontTrim_{i}')
    for side,x in enumerate((-.42*w,.42*w)):
        add(scene,box((.06,d*.62,.38),(x,0,.24),MAT_STEEL),f'RiserUpright_{side}')
    add(scene,box((.19,.025,.06),(0,-d*.53,.16),MAT_LABEL),'RiserLoadLabel')
    return scene


def _ice_cream_case(v,mat):
    scene=trimesh.Scene(); w=1.10+.09*v; d=.64+.05*v; h=.84+.06*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'IceCreamCaseBase')
    add(scene,box((w*.82,d*.70,h-.12),(0,0,h/2),mat),'ColdPanPedestal')
    add(scene,box((w*.88,d*.75,.35),(0,0,h+.12),MAT_GLASS),'ColdPanGlassGuard')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.74/(3+v)
        add(scene,box((w*.14,d*.28,.10),(x,0,h+.14),MAT_STEEL),f'GelatoPan_{i}')
        add(scene,box((w*.14,d*.28,.03),(x,0,h+.205),MAT_CERAMIC),f'GelatoLid_{i}')
    add(scene,box((w*.88,.025,.05),(0,-d*.51,h-.10),MAT_LABEL),'FlavorStrip')
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,box((.035,d*.70,.06),(x,0,h+.12),MAT_BRASS),f'CaseShelfDivider_{side}')
    return scene


def _coffee_urn_console(v,mat):
    scene=trimesh.Scene(); w=1.12+.10*v; d=.54+.05*v; h=.82+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'CoffeeUrnConsole')
    _legs(scene,w,d,h-.06,MAT_WOOD,'ConsoleLeg')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*w*.46/(1+v//2)
        add(scene,cyl(.14,.28,(x,d*.05,h+.20),MAT_STAINLESS,16),f'HotBeverageUrn_{i}')
        add(scene,cyl(.07,.05,(x,d*.05,h+.37),MAT_STEEL,12),f'UrnLid_{i}')
        add(scene,cyl(.022,.10,(x,-d*.20,h+.04),MAT_BRASS,8),f'UrnSpigot_{i}')
    add(scene,box((w*.78,.035,.22),(0,d*.35,h+.18),MAT_STEEL),'CupShelf')
    add(scene,box((.22,.025,.07),(0,-d*.53,h-.09),MAT_LABEL),'CoffeeServiceLabel')
    return scene


def _dumpling_steamer(v,mat):
    scene=trimesh.Scene(); w=.82+.07*v; d=.68+.06*v; h=.86+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'SteamerCabinet')
    _legs(scene,w,d,h-.06,MAT_STEEL,'SteamerLeg')
    add(scene,box((w*.74,d*.70,.08),(0,0,h+.10),MAT_STEEL),'SteamWell')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*w*.42/(1+v//2)
        add(scene,cyl(.15,.08,(x,0,h+.18),MAT_CERAMIC,16),f'BambooBasket_{i}')
        add(scene,cyl(.16,.045,(x,0,h+.24),MAT_WOOD,16),f'BasketLid_{i}')
    add(scene,box((w*.50,.06,.14),(0,d*.31,h+.17),MAT_STEEL),'SteamerSplashRail')
    add(scene,box((.20,.025,.07),(0,-d*.53,h-.09),MAT_LABEL),'SteamerFoodSafetyTag')
    return scene


def _wine_cooler_rack(v,mat):
    scene=trimesh.Scene(); w=.88+.08*v; d=.54+.05*v; h=1.06+.08*v
    add(scene,box((w,d,h),(0,0,h/2),mat),'WineCoolerCabinet')
    add(scene,box((w*.80,.035,h*.76),(0,-d*.53,h*.53),MAT_GLASS),'CoolerDoor')
    for i in range(3+v):
        z=.30+i*.22
        add(scene,box((w*.68,d*.60,.035),(0,0,z),MAT_STEEL),f'BottleRackRail_{i}')
    for row in range(2+v//2):
        y=(row-(1+v//2)/2)*d*.24/(1+v//2)
        add(scene,cyl(.06,.18,(0,y,.34),MAT_BRASS,12),f'DisplayBottle_{row}')
    add(scene,box((.20,.025,.06),(w*.30,-d*.56,.18),MAT_LABEL),'WineTemperatureCard')
    return scene


def _flambe_cart(v,mat):
    scene=trimesh.Scene(); w=.86+.08*v; d=.58+.05*v; h=.90+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'FlambeCartTop')
    _legs(scene,w,d,h-.06,MAT_STEEL,'FlambeCartLeg')
    _wheels(scene,w,d,.07,'FlambeCaster')
    add(scene,box((w*.82,.08,.50),(0,d*.34,h+.29),MAT_STEEL),'SafetyHeatShield')
    for side,x in enumerate((-.29*w,.29*w)):
        add(scene,cyl(.14,.04,(x,0,h+.08),MAT_STEEL,16),f'InductionZone_{side}')
        add(scene,cyl(.09,.045,(x,0,h+.12),MAT_CERAMIC,12),f'PanSupport_{side}')
    add(scene,box((.18,.025,.06),(0,-d*.53,h-.10),MAT_LABEL),'OpenFlameOperatingCard')
    return scene


def _salad_trolley(v,mat):
    scene=trimesh.Scene(); w=.80+.08*v; d=.58+.05*v; h=.86+.06*v
    add(scene,box((w,d,.10),(0,0,h),mat),'TablesideSaladTop')
    _legs(scene,w,d,h-.05,MAT_WOOD,'SaladTrolleyLeg')
    _wheels(scene,w,d,.065,'SaladTrolleyCaster')
    add(scene,box((w*.74,d*.64,.045),(0,0,.42),MAT_STEEL),'CondimentShelf')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.68/(2+v)
        add(scene,cyl(.06,.14,(x,0,h+.09),MAT_GLASS,12),f'DressingCruet_{i}')
        add(scene,cyl(.025,.045,(x,0,h+.18),MAT_BRASS,8),f'CruetStopper_{i}')
    add(scene,box((.18,.025,.06),(0,-d*.53,h-.08),MAT_LABEL),'TablesideMenuCard')
    return scene


def _cheese_showcase(v,mat):
    scene=trimesh.Scene(); w=1.06+.10*v; d=.60+.05*v; h=.86+.06*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'CheeseShowcaseBase')
    add(scene,box((w*.84,d*.72,h+.06),(0,0,(h+.06)/2),mat),'CheeseShowcasePedestal')
    add(scene,box((w*.86,d*.74,.40),(0,0,h+.26),MAT_GLASS),'AgingGlassShowcase')
    for i in range(2+v//2):
        z=h+.12+i*.16
        add(scene,box((w*.72,d*.58,.04),(0,0,z),MAT_WOOD),f'CheeseDisplayShelf_{i}')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.68/(3+v)
        add(scene,cyl(.07,.045,(x,0,h+.12),MAT_CERAMIC,12),f'CheeseWheel_{i}')
    add(scene,box((.24,.025,.07),(0,-d*.53,h-.09),MAT_LABEL),'CheeseOriginAndAgeCard')
    return scene


def _restaurant(family,v,mat):
    return (_pasta_counter,_charcuterie_case,_buffet_riser,_ice_cream_case,_coffee_urn_console,
            _dumpling_steamer,_wine_cooler_rack,_flambe_cart,_salad_trolley,_cheese_showcase)[family](v,mat)


def build_asset(name,subcategory,mat,asset_id,profile):
    try:
        number=int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'core hotel expansion asset ID outside A2751–A3000: {asset_id}') from exc
    if not 2751 <= number <= 3000:
        raise ValueError(f'core hotel expansion asset ID outside A2751–A3000: {asset_id}')
    batch_index=(number-2751)//50
    family,variant=divmod((number-2751)%50,5)
    return (_architecture,_guestroom,_public,_housekeeping,_restaurant)[batch_index](family,variant,mat)
