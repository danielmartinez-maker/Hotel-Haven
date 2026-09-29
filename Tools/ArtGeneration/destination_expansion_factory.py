"""Deterministic event, decor, landscape, finish, and dining assets A2501–A2750."""

import math

import trimesh

from v2_asset_common import add, box, cyl, sphere


MAT_STEEL = 'MAT_BLACKENED_STEEL'
MAT_BRASS = 'MAT_BRASS_POLISHED'
MAT_STONE = 'MAT_STONE_LIGHT'
MAT_WOOD = 'MAT_WOOD_WARM'
MAT_GLASS = 'MAT_GLASS_CLEAR'
MAT_LABEL = 'MAT_SIGNAGE'
MAT_CERAMIC = 'MAT_CERAMIC_FIXTURE'
MAT_ELECTRONICS = 'MAT_ELECTRONICS'


def _rot(mesh, angle, axis, point=None):
    mesh.apply_transform(trimesh.transformations.rotation_matrix(angle, axis, point=point))
    return mesh


def _legs(scene, width, depth, height, material=MAT_STEEL, prefix='Leg'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.05, .05, height), (x * width * .42, y * depth * .37, height / 2), material),
            f'{prefix}_{index}')


def _wheels(scene, width, depth, radius=.075, prefix='Caster'):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        center = (x * width * .40, y * depth * .38, radius)
        wheel = cyl(radius, .045, center, MAT_STEEL, 12)
        _rot(wheel, math.pi / 2, [1, 0, 0], center)
        add(scene, wheel, f'{prefix}_{index}')


def _post(scene, name, x, y, height, radius=.045, material=MAT_STEEL):
    add(scene, cyl(radius, height, (x, y, height / 2), material, 12), name)


# Batch 51: flexible event, conference, and banquet transformation equipment.
def _stage_riser(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.25 + .12 * v, .92 + .08 * v, .28 + .04 * v
    add(scene, box((w, d, .10), (0, 0, h), mat), 'StageDeck')
    add(scene, box((w * .96, .035, .06), (0, -d * .48, h + .08), MAT_BRASS), 'StageEdgeTrim')
    for index, (x, y) in enumerate(((-1,-1),(-1,1),(1,-1),(1,1))):
        _post(scene, f'HeightAdjustLeg_{index}', x*w*.40, y*d*.38, h-.05, .035, MAT_STEEL)
        add(scene, box((.13,.13,.035),(x*w*.40,y*d*.38,.0175),MAT_STONE),f'FootPad_{index}')
    add(scene, box((w*.48,.045,.065),(0,d*.38,h-.04),MAT_STEEL), 'RiserCrossBrace')
    add(scene, box((.20,.025,.06),(w*.35,-d*.50,h+.12),MAT_LABEL), 'DeckLoadLabel')
    return scene


def _dance_floor(v, mat):
    scene = trimesh.Scene(); w, d = 1.40 + .12*v, 1.05 + .10*v
    add(scene, box((w,d,.08),(0,0,.04),MAT_STEEL),'DanceFloorFrame')
    add(scene, box((w*.97,d*.97,.035),(0,0,.0975),mat),'DanceFloorSurface')
    for x in (-.25,0,.25):
        add(scene, box((.025,d*.90,.012),(x*w,0,.121),MAT_BRASS),'FloorInlay')
    for side, y in enumerate((-.48*d,.48*d)):
        add(scene, box((w*.90,.025,.03),(0,y,.12),MAT_STEEL),f'PerimeterRail_{side}')
    add(scene, box((.17,.045,.05),(w*.39,-d*.51,.12),MAT_LABEL),'FloorPanelID')
    return scene


def _event_lectern(v, mat):
    scene = trimesh.Scene(); w, d, h = .70 + .06*v, .52 + .04*v, 1.12 + .06*v
    add(scene, box((w*.48,d*.48,.08),(0,0,.04),MAT_STONE),'LecternFoot')
    add(scene, box((.11,.11,h*.76),(0,0,h*.43),MAT_STEEL),'LecternColumn')
    add(scene, box((w,d,.10),(0,0,h),mat),'ReadingTop')
    add(scene, box((w*.80,.045,.12),(0,-d*.40,h-.07),MAT_WOOD),'BookStop')
    _post(scene,'MicrophoneStem',w*.28,-d*.22,h+.13,.018,MAT_STEEL)
    add(scene, sphere(.045,(w*.30,-d*.22,h+.27),MAT_STEEL,0),'MicrophoneHead')
    add(scene, box((.24,.025,.09),(0,-d*.53,.32),MAT_LABEL),'EventLecternBadge')
    return scene


def _projector_lift(v, mat):
    scene=trimesh.Scene(); w,d,h=.54+.04*v,.48+.035*v,1.20+.10*v
    add(scene,box((w*.72,d*.72,.10),(0,0,.22),MAT_STEEL),'LiftBase')
    _wheels(scene,w*.80,d*.80,.065,'LiftCaster')
    _post(scene,'TelescopingMast',0,0,h-.12,.045,MAT_STEEL)
    add(scene,box((w,d*.86,.07),(0,0,h),MAT_STEEL),'ProjectorPlatform')
    add(scene,box((w*.82,d*.76,.20),(0,0,h+.13),mat),'AVProjectorBody')
    add(scene,cyl(.055,.035,(0,-d*.40,h+.13),MAT_GLASS,16),'ProjectionLens')
    add(scene,box((.16,.035,.09),(w*.40,0,.38),MAT_LABEL),'LiftSafetyLabel')
    return scene


def _stage_floorbox(v, mat):
    scene=trimesh.Scene(); w,d=.42+.04*v,.32+.035*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STEEL),'StageFloorboxHousing')
    add(scene,box((w*.86,d*.82,.035),(0,0,.1375),mat),'FloorboxCover')
    for index in range(4+v):
        x=(index-(3+v)/2)*w*.48/(3+v)
        add(scene,box((.04,.07,.035),(x,0,.17),MAT_BRASS),f'CablePort_{index}')
    add(scene,box((w*.64,.035,.055),(0,-d*.52,.16),MAT_LABEL),'CircuitLabel')
    for corner,(x,y) in enumerate(((-1,-1),(-1,1),(1,-1),(1,1))):
        add(scene,cyl(.018,.02,(x*w*.42,y*d*.40,.15),MAT_STEEL,8),f'CoverScrew_{corner}')
    return scene


def _linen_console(v, mat):
    scene=trimesh.Scene(); w,d,h=1.05+.10*v,.48+.05*v,.88+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'LinenServiceTop')
    _legs(scene,w,d,h-.06,MAT_STEEL,'ConsolePost')
    for i in range(2+v//2):
        z=.24+i*.17
        add(scene,box((w*.72,d*.68,.035),(0,0,z),MAT_WOOD),f'FoldedLinenShelf_{i}')
    _wheels(scene,w*.95,d*.95,.065,'ConsoleCaster')
    add(scene,box((.23,.03,.09),(0,-d*.53,h-.10),MAT_LABEL),'LinenCartTag')
    return scene


def _wedding_arch(v, mat):
    scene=trimesh.Scene(); w,h=1.25+.10*v,2.15+.12*v
    for side,x in enumerate((-.45*w,.45*w)):
        add(scene,box((.12,.12,.18),(x,0,.09),MAT_STONE),f'ArchFoot_{side}')
        _post(scene,f'ArchUpright_{side}',x,0,h*.84,.055,mat)
    add(scene,box((w*.95,.12,.13),(0,0,h*.84),mat),'ArchHeader')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.72/(2+v)
        add(scene,sphere(.09+.01*v,(x,-.08,h*.84+.13),MAT_CERAMIC,0),f'FloralAnchor_{i}')
    add(scene,box((w*.62,.045,.06),(0,-.08,h*.72),MAT_LABEL),'ArchSetupMarker')
    return scene


def _registration_stand(v, mat):
    scene=trimesh.Scene(); w,d,h=.92+.08*v,.55+.05*v,1.02+.07*v
    add(scene,box((w,d,.12),(0,0,h),mat),'RegistrationCounter')
    _legs(scene,w,d,h-.06,MAT_WOOD,'CounterLeg')
    add(scene,box((w*.66,.025,.28),(0,-d*.46,h+.23),MAT_GLASS),'GuestListPanel')
    for i in range(2+v):
        x=(i-(1+v)/2)*.15
        add(scene,box((.08,.16,.06),(x,d*.18,h+.09),MAT_WOOD),f'BadgeTray_{i}')
    add(scene,box((.24,.025,.08),(0,-d*.52,h-.08),MAT_LABEL),'RegistrationHeader')
    return scene


def _chair_dolly(v, mat):
    scene=trimesh.Scene(); w,d,h=.62+.05*v,.66+.04*v,.78+.05*v
    add(scene,box((w*.84,d*.82,.08),(0,0,.22),MAT_STEEL),'DollyBase')
    for side,x in enumerate((-.42*w,.42*w)):
        add(scene,box((.055,d*.78,h),(x,0,h*.50+.24),mat),f'ChairStackRail_{side}')
    for i in range(3+v):
        z=.42+i*.105
        add(scene,box((w*.68,.10,.045),(0,0,z),MAT_WOOD),f'StackedChairSeat_{i}')
    _wheels(scene,w,d,.075,'DollyCaster')
    add(scene,box((.18,.03,.07),(0,-d*.48,.26),MAT_LABEL),'DollyPropertyTag')
    return scene


def _partition_cart(v, mat):
    scene=trimesh.Scene(); w,h=.56+.05*v,1.60+.12*v
    add(scene,box((w*.74,.10,.12),(0,0,.20),MAT_STEEL),'PartitionCartBase')
    _wheels(scene,w,.30,.07,'PartitionCaster')
    for i in range(2+v//2):
        y=(i-(1+v//2)/2)*.11
        add(scene,box((w,.055,h),(0,y,h/2+.24),mat),f'PortableWallPanel_{i}')
        add(scene,box((w*.94,.018,.05),(0,y,h+.22),MAT_BRASS),f'PanelCap_{i}')
    add(scene,box((.18,.04,.08),(0,-.18,.26),MAT_LABEL),'CartCapacityLabel')
    return scene


def _events(family,v,mat):
    return (_stage_riser,_dance_floor,_event_lectern,_projector_lift,_stage_floorbox,
            _linen_console,_wedding_arch,_registration_stand,_chair_dolly,_partition_cart)[family](v,mat)


# Batch 52: sculptural hotel decor, furnishings, and architectural art features.
def _floor_sculpture(v,mat):
    scene=trimesh.Scene(); w=.50+.06*v; h=1.25+.18*v
    add(scene,box((w*1.35,w*1.20,.18),(0,0,.09),MAT_STONE),'SculpturePlinth')
    add(scene,cyl(.13+.015*v,h*.58,(0,0,.22+h*.29),MAT_BRASS,16),'SculptureColumn')
    for i in range(3+v):
        z=.42+i*.19
        x=(i%2-.5)*.20
        add(scene,sphere(.13+.006*v,(x,0,z),mat,0),f'AbstractForm_{i}')
    add(scene,box((w*.90,.025,.07),(0,-w*.62,.20),MAT_LABEL),'SculptureCreditPlate')
    return scene


def _botanical_divider(v,mat):
    scene=trimesh.Scene(); w=1.15+.12*v; h=1.55+.12*v
    for side,x in enumerate((-.45*w,.45*w)):
        add(scene,box((.16,.42,.08),(x,0,.04),MAT_WOOD),f'DividerFoot_{side}')
        _post(scene,f'DividerPost_{side}',x,0,h,.035,MAT_WOOD)
    add(scene,box((w,.045,h*.82),(0,0,h*.52),MAT_STEEL),'DividerFrame')
    for i in range(5+v):
        x=(i-(4+v)/2)*w*.84/(4+v)
        add(scene,box((.035,.025,h*.74),(x,-.04,h*.52),mat),f'BotanicalSlat_{i}')
        add(scene,sphere(.055,(x,-.07,.60+.11*(i%3)),MAT_CERAMIC,0),f'LeafCluster_{i}')
    add(scene,box((.20,.03,.07),(0,-.08,.24),MAT_LABEL),'DividerMakerMark')
    return scene


def _lobby_mirror(v,mat):
    scene=trimesh.Scene(); w=1.05+.10*v; h=1.55+.12*v
    add(scene,box((w,.10,h),(0,0,h/2),MAT_WOOD),'MirrorBacking')
    add(scene,box((w*.78,.035,h*.78),(0,-.065,h*.52),mat),'MirrorGlass')
    for side,x in enumerate((-.45*w,.45*w)):
        add(scene,box((.075,.06,h*.88),(x,-.09,h*.52),MAT_BRASS),f'MirrorSideFrame_{side}')
    for side,z in enumerate((.12,h*.92)):
        add(scene,box((w*.90,.06,.08),(0,-.09,z),MAT_BRASS),f'MirrorRail_{side}')
    add(scene,box((.16,.025,.08),(w*.35,-.12,.16),MAT_LABEL),'MirrorMakerPlaque')
    return scene


def _fireplace_surround(v,mat):
    scene=trimesh.Scene(); w=1.30+.12*v; h=1.35+.12*v
    add(scene,box((w,.24,.12),(0,0,.06),MAT_STONE),'Hearth')
    for side,x in enumerate((-.38*w,.38*w)):
        add(scene,box((.18,.18,h),(x,0,h/2+.12),mat),f'MantelPier_{side}')
    add(scene,box((w,.22,.16),(0,0,h+.12),mat),'MantelShelf')
    add(scene,box((w*.46,.08,h*.55),(0,-.05,h*.48),MAT_STEEL),'FireboxRecess')
    add(scene,box((w*.52,.03,.07),(0,-.10,.20),MAT_BRASS),'FireboxTrim')
    add(scene,box((.24,.025,.07),(0,-.14,h+.06),MAT_LABEL),'MantelMakerMark')
    return scene


def _pendant_cluster(v,mat):
    scene=trimesh.Scene(); w=.92+.10*v; ceiling=3.08
    add(scene,box((.16,.16,.06),(0,0,ceiling),MAT_STEEL),'CeilingCanopy')
    for i in range(3+v):
        x=(i-(2+v)/2)*w/(2+v)
        length=.68+.09*((i+v)%3)
        _post(scene,f'PendantCord_{i}',x,0,length,.012,MAT_STEEL)
        z=ceiling-length
        add(scene,cyl(.12+.01*v,.16,(x,0,z),mat,12),f'PendantShade_{i}')
        add(scene,cyl(.08,.025,(x,0,z-.095),MAT_CERAMIC,12),f'PendantDiffuser_{i}')
    add(scene,box((.20,.025,.06),(0,-.12,.28),MAT_LABEL),'LightingSeriesTag')
    return scene


def _artisan_relief(v,mat):
    scene=trimesh.Scene(); w=1.15+.10*v; h=.78+.08*v
    add(scene,box((w,.09,h),(0,0,1.55),MAT_WOOD),'ReliefPanelBacking')
    add(scene,box((w*.90,.04,h*.86),(0,-.065,1.55),mat),'ReliefField')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.70/(3+v)
        add(scene,box((.055,.055,h*.64),(x,-.105,1.55),MAT_BRASS if i%2 else MAT_STONE),f'ReliefRidge_{i}')
    for i,x in enumerate((-.42*w,.42*w)):
        add(scene,cyl(.025,.035,(x,-.12,1.55),MAT_STEEL,8),f'WallFixing_{i}')
    add(scene,box((.18,.025,.06),(w*.34,-.14,1.55-h*.40),MAT_LABEL),'ArtistSignature')
    return scene


def _sculptural_side_table(v,mat):
    scene=trimesh.Scene(); w=.56+.06*v; d=.44+.05*v; h=.62+.05*v
    add(scene,box((w,d,.075),(0,0,h),mat),'SideTableTop')
    add(scene,cyl(.13+.015*v,h-.08,(0,0,(h-.08)/2),MAT_BRASS,12),'SculpturalPedestal')
    add(scene,box((w*.78,d*.74,.05),(0,0,.08),MAT_STONE),'TableFoot')
    add(scene,sphere(.10+.01*v,(0,0,h+.11),MAT_CERAMIC,0),'DecorativeOrb')
    add(scene,box((.14,.025,.06),(w*.36,-d*.52,h-.04),MAT_LABEL),'TableMakerMark')
    return scene


def _folding_screen(v,mat):
    scene=trimesh.Scene(); w=.72+.05*v; h=1.68+.10*v
    for i in range(3):
        angle=(-1 if i==0 else 1 if i==2 else 0)*(.12+.02*v)
        x=(i-1)*w*.72
        mesh=box((w*.62,.06,h),(x,0,h/2),mat)
        _rot(mesh,angle,[0,0,1],(x,0,h/2)); add(scene,mesh,f'ScreenLeaf_{i}')
        add(scene,box((w*.60,.035,.09),(x,0,h*.10),MAT_WOOD),f'ScreenFootRail_{i}')
        add(scene,box((w*.60,.035,.09),(x,0,h*.90),MAT_BRASS),f'ScreenCrownRail_{i}')
    add(scene,box((.19,.025,.06),(0,-.06,.28),MAT_LABEL),'ScreenCollectionPlaque')
    return scene


def _art_console(v,mat):
    scene=trimesh.Scene(); w=1.12+.10*v; d=.42+.04*v; h=.78+.05*v
    add(scene,box((w,d,.10),(0,0,h),mat),'ConsoleTop')
    for i,x in enumerate((-.42*w,.42*w)):
        add(scene,box((.09,d*.72,h-.08),(x,0,(h-.08)/2),MAT_WOOD),f'ConsoleLeg_{i}')
    add(scene,box((w*.74,.06,.10),(0,d*.18,.32),MAT_WOOD),'ConsoleShelf')
    add(scene,box((w*.38,.055,.20),(0,-d*.30,h+.16),MAT_STONE),'ArtBookDisplay')
    add(scene,sphere(.10+.01*v,(w*.30,0,h+.16),MAT_BRASS,0),'ConsoleSculpture')
    add(scene,box((.22,.025,.06),(0,-d*.53,h-.08),MAT_LABEL),'ConsoleEditionPlaque')
    return scene


def _hanging_installation(v,mat):
    scene=trimesh.Scene(); w=.92+.10*v; ceiling=3.10
    add(scene,box((w*.48,.16,.07),(0,0,ceiling),MAT_STEEL),'InstallationMountRail')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.78/(3+v)
        drop=.70+.12*((i+v)%3)
        _post(scene,f'SuspensionCable_{i}',x,0,drop,.012,MAT_STEEL)
        z=ceiling-drop
        add(scene,box((.18+.015*v,.08,.11),(x,0,z-.05),mat),f'HangingArtElement_{i}')
        add(scene,sphere(.065,(x,0,z-.15),MAT_BRASS,0),f'ArtElementFinial_{i}')
    add(scene,box((.18,.025,.06),(0,-.14,.25),MAT_LABEL),'InstallationTitlePlate')
    return scene


def _decor(family,v,mat):
    return (_floor_sculpture,_botanical_divider,_lobby_mirror,_fireplace_surround,_pendant_cluster,
            _artisan_relief,_sculptural_side_table,_folding_screen,_art_console,_hanging_installation)[family](v,mat)


# Batch 53: courtyard, terrace, and planted landscape infrastructure.
def _herb_garden(v,mat):
    scene=trimesh.Scene(); w=1.02+.10*v; d=.72+.08*v; h=.52+.04*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_WOOD),'RaisedBedBase')
    add(scene,box((w*.92,d*.90,.16),(0,0,h),mat),'PlanterSoilBed')
    for side,x in enumerate((-.47*w,.47*w)):
        add(scene,box((.06,d,.42),(x,0,.33),MAT_WOOD),f'PlanterEndBoard_{side}')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.68/(3+v)
        _post(scene,f'HerbStem_{i}',x,0,h+.32,.018,MAT_STEEL)
        add(scene,sphere(.07,(x,0,h+.38),MAT_CERAMIC,0),f'HerbLeaf_{i}')
    add(scene,box((.19,.025,.06),(0,-d*.52,.38),MAT_LABEL),'PlantingPlanLabel')
    return scene


def _pergola_bay(v,mat):
    scene=trimesh.Scene(); w=1.45+.12*v; d=1.18+.10*v; h=2.60+.12*v
    for index,(x,y) in enumerate(((-1,-1),(-1,1),(1,-1),(1,1))):
        _post(scene,f'PergolaColumn_{index}',x*w*.42,y*d*.40,h,.07,mat)
        add(scene,box((.18,.18,.08),(x*w*.42,y*d*.40,.04),MAT_STONE),f'ColumnFoot_{index}')
    for i in range(6+v):
        x=(i-(5+v)/2)*w*.90/(5+v)
        add(scene,box((.08,d*.92,.10),(x,0,h+.06),MAT_WOOD),f'PergolaRafter_{i}')
    for side,y in enumerate((-.42*d,.42*d)):
        add(scene,box((w,.11,.13),(0,y,h-.02),mat),f'PergolaBeam_{side}')
    add(scene,box((.22,.025,.07),(w*.42,-d*.48,.25),MAT_LABEL),'PergolaBuildCode')
    return scene


def _retaining_seat(v,mat):
    scene=trimesh.Scene(); w=1.32+.12*v; d=.50+.05*v; h=.48+.04*v
    add(scene,box((w,d,h),(0,0,h/2),mat),'SeatRetainingWall')
    add(scene,box((w*1.04,d*1.08,.11),(0,0,h+.055),MAT_STONE),'GardenBenchCap')
    add(scene,box((w*.76,.06,.20),(0,d*.27,.24),MAT_WOOD),'SeatBackPanel')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.74/(2+v)
        add(scene,box((.035,d*.54,.06),(x,0,.18),MAT_BRASS),f'WallDrainSlot_{i}')
    add(scene,box((.18,.025,.06),(w*.38,-d*.54,.24),MAT_LABEL),'SeatLandscapeTag')
    return scene


def _terrace_trellis(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; h=2.15+.10*v
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,box((.20,.34,.08),(x,0,.04),MAT_STONE),f'TrellisFoot_{side}')
        _post(scene,f'TrellisPost_{side}',x,0,h,.05,mat)
    for i in range(5+v):
        z=.35+i*.25
        add(scene,box((w*.90,.04,.04),(0,0,z),MAT_WOOD),f'TrellisCrossRail_{i}')
    for i,x in enumerate((-.28*w,0,.28*w)):
        _post(scene,f'TrellisClimberGuide_{i}',x,0,h*.74,.018,MAT_STEEL)
    add(scene,box((.18,.025,.06),(0,-.07,.27),MAT_LABEL),'TrellisSeriesMark')
    return scene


def _bioswale_inlet(v,mat):
    scene=trimesh.Scene(); w=.92+.10*v; d=.56+.06*v
    add(scene,box((w,d,.11),(0,0,.055),MAT_STONE),'InletFrame')
    add(scene,box((w*.88,d*.82,.07),(0,0,.145),mat),'InletGrate')
    for i in range(5+v):
        x=(i-(4+v)/2)*w*.78/(4+v)
        add(scene,box((.035,d*.68,.025),(x,0,.195),MAT_STEEL),f'GrateBar_{i}')
    for side,y in enumerate((-.45*d,.45*d)):
        add(scene,box((w*.84,.045,.06),(0,y,.16),MAT_BRASS),f'PerimeterDrainEdge_{side}')
    add(scene,box((.20,.025,.05),(0,-d*.52,.18),MAT_LABEL),'StormwaterMark')
    return scene


def _landscape_light(v,mat):
    scene=trimesh.Scene(); h=.72+.08*v
    add(scene,cyl(.16,.10,(0,0,.05),MAT_STONE,12),'BollardBase')
    add(scene,cyl(.065,h,(0,0,h/2+.10),mat,12),'LandscapeLightStem')
    add(scene,cyl(.11,.12,(0,0,h+.13),MAT_STEEL,12),'LightCap')
    add(scene,box((.13,.035,.16),(0,-.07,h+.12),MAT_GLASS),'LightLens')
    for i in range(3+v):
        z=.22+i*.13
        add(scene,box((.12,.012,.018),(0,-.07,z),MAT_BRASS),f'BollardAccent_{i}')
    add(scene,box((.16,.025,.06),(0,-.085,.19),MAT_LABEL),'LuminaireRating')
    return scene


def _fountain_basin(v,mat):
    scene=trimesh.Scene(); w=1.05+.10*v; d=.88+.08*v; h=.54+.05*v
    add(scene,box((w,d,.16),(0,0,.08),MAT_STONE),'FountainFoundation')
    add(scene,cyl(.40+.035*v,.22,(0,0,.27),mat,20),'FountainBasin')
    add(scene,cyl(.12,.42,(0,0,.58),MAT_STONE,12),'FountainSpoutColumn')
    add(scene,cyl(.21,.08,(0,0,.86),MAT_BRASS,16),'WaterCrown')
    for i in range(4+v):
        angle=2*math.pi*i/(4+v); x=.28*math.cos(angle); y=.28*math.sin(angle)
        add(scene,sphere(.055,(x,y,.42),MAT_GLASS,0),f'BasinNozzle_{i}')
    add(scene,box((.20,.025,.06),(w*.40,-d*.53,.24),MAT_LABEL),'FountainCareTag')
    return scene


def _tree_guard(v,mat):
    scene=trimesh.Scene(); w=.82+.08*v; h=.78+.06*v
    add(scene,box((w,w,.07),(0,0,.035),MAT_STONE),'TreeGuardFooting')
    for index,(x,y) in enumerate(((-1,-1),(-1,1),(1,-1),(1,1))):
        _post(scene,f'GuardPost_{index}',x*w*.40,y*w*.40,h,.035,mat)
    for row,z in enumerate((.28,.53,.78)):
        for side,axis in enumerate(('x','y')):
            dims=(w*.86,.035,.04) if axis=='x' else (.035,w*.86,.04)
            center=(0,-w*.40,z) if axis=='x' else (-w*.40,0,z)
            add(scene,box(dims,center,MAT_WOOD if row==1 else MAT_STEEL),f'TreeGuardRail_{row}_{side}')
    add(scene,box((.18,.025,.06),(w*.40,-w*.47,.18),MAT_LABEL),'TreeGuardMunicipalMark')
    return scene


def _hose_reel(v,mat):
    scene=trimesh.Scene(); w=.62+.05*v; h=.96+.07*v
    add(scene,box((w,.14,h),(0,0,h/2),mat),'HoseReelBackplate')
    add(scene,cyl(.21+.02*v,.10,(0,-.13,.50),MAT_BRASS,16),'HoseDrum')
    for i in range(4+v):
        z=.35+i*.08
        add(scene,cyl(.20+.02*v,.018,(0,-.20,z),MAT_STEEL,16),f'HoseCoil_{i}')
    add(scene,box((.16,.12,.08),(w*.48,-.10,.20),MAT_STEEL),'HoseNozzleDock')
    add(scene,box((.22,.025,.08),(0,-.22,h-.12),MAT_LABEL),'HosePressureLabel')
    return scene


def _irrigation_cabinet(v,mat):
    scene=trimesh.Scene(); w=.60+.05*v; d=.34+.03*v; h=.90+.08*v
    add(scene,box((w,d,.06),(0,0,.03),MAT_STONE),'ValveCabinetPlinth')
    add(scene,box((w,d,h),(0,0,h/2+.06),mat),'IrrigationValveHousing')
    add(scene,box((w*.78,.035,h*.70),(0,-d*.52,h*.52),MAT_WOOD),'AccessDoor')
    for side,x in enumerate((-.31*w,.31*w)):
        add(scene,cyl(.025,.04,(x,-d*.56,h*.52),MAT_BRASS,8),f'DoorHinge_{side}')
    add(scene,box((.11,.05,.025),(w*.25,-d*.59,.65),MAT_STEEL),'DoorLatch')
    for i in range(2+v//2):
        add(scene,cyl(.025,.10,((i-1)*.10,d*.50,.24),MAT_STEEL,8),f'WaterFeed_{i}')
    add(scene,box((.22,.025,.07),(0,-d*.56,.20),MAT_LABEL),'ValveZoneLabel')
    return scene


def _exterior(family,v,mat):
    return (_herb_garden,_pergola_bay,_retaining_seat,_terrace_trellis,_bioswale_inlet,
            _landscape_light,_fountain_basin,_tree_guard,_hose_reel,_irrigation_cabinet)[family](v,mat)


# Batch 54: decorative floor, wall, and ceiling finish modules.
def _floor_medallion(v,mat):
    scene=trimesh.Scene(); r=.48+.05*v
    add(scene,cyl(r,.08,(0,0,.04),MAT_STONE,24),'MedallionStoneBacking')
    add(scene,cyl(r*.86,.035,(0,0,.0975),mat,24),'MedallionInlayField')
    for i in range(8+v):
        angle=2*math.pi*i/(8+v); x=r*.62*math.cos(angle); y=r*.62*math.sin(angle)
        add(scene,box((.055,.16,.025),(x,y,.13),MAT_BRASS),f'RadialBrassInlay_{i}')
    add(scene,cyl(.12+.01*v,.04,(0,0,.135),MAT_CERAMIC,16),'CenterRosette')
    return scene


def _microcement_panel(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; h=1.10+.09*v
    add(scene,box((w,.10,h),(0,0,1.55),MAT_STONE),'PanelBacking')
    add(scene,box((w*.96,.035,h*.96),(0,-.067,1.55),mat),'MicrocementFace')
    for side,x in enumerate((-.47*w,.47*w)):
        add(scene,box((.025,.06,h*.92),(x,-.09,1.55),MAT_STEEL),f'PanelEdgeTrim_{side}')
    add(scene,box((.20,.025,.06),(w*.34,-.11,1.55-h*.40),MAT_LABEL),'FinishBatchTag')
    return scene


def _ribbed_plaster_board(v,mat):
    scene=trimesh.Scene(); w=.88+.07*v; h=1.18+.10*v
    add(scene,box((w,.08,h),(0,0,1.55),MAT_STONE),'PlasterSubstrate')
    add(scene,box((w*.92,.03,h*.93),(0,-.055,1.55),mat),'PlasterFinishField')
    for i in range(6+v):
        x=(i-(5+v)/2)*w*.80/(5+v)
        add(scene,box((.022,.04,h*.82),(x,-.09,1.55),MAT_CERAMIC),f'PlasterRib_{i}')
    add(scene,box((.16,.025,.06),(w*.35,-.12,1.55-h*.40),MAT_LABEL),'PlasterSpecTag')
    return scene


def _acoustic_raft(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; d=.84+.07*v; z=3.12
    add(scene,box((w*.88,d*.88,.09),(0,0,z),MAT_STEEL),'AcousticRaftFrame')
    add(scene,box((w*.82,d*.82,.055),(0,0,z-.075),mat),'FabricRaftPanel')
    for i in range(4+v):
        x=(i-(3+v)/2)*w*.76/(3+v)
        _post(scene,f'RaftSuspension_{i}',x,0,.55,.012,MAT_STEEL)
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,box((.045,d*.84,.16),(x,0,z-.11),MAT_BRASS),f'RaftEdgeBaffle_{side}')
    add(scene,box((.16,.025,.05),(w*.35,-d*.48,z-.10),MAT_LABEL),'RaftAccessMark')
    return scene


def _vent_cassette(v,mat):
    scene=trimesh.Scene(); w=1.00+.08*v; h=.58+.05*v
    add(scene,box((w,.10,h),(0,0,1.55),MAT_STEEL),'VentCassetteFrame')
    add(scene,box((w*.90,.04,h*.84),(0,-.07,1.55),MAT_STONE),'VentBacking')
    for i in range(7+v):
        z=1.55-h*.34+i*h*.11
        add(scene,box((w*.78,.045,.028),(0,-.11,z),mat),f'VentLouver_{i}')
    for side,x in enumerate((-.44*w,.44*w)):
        add(scene,cyl(.022,.03,(x,-.13,1.55),MAT_BRASS,8),f'CassetteFixing_{side}')
    add(scene,box((.16,.025,.05),(w*.34,-.14,1.55-h*.40),MAT_LABEL),'VentFinishCode')
    return scene


def _tile_wainscot(v,mat):
    scene=trimesh.Scene(); w=1.10+.08*v; h=.78+.07*v
    add(scene,box((w,.10,h),(0,0,1.45),MAT_STONE),'WainscotBacking')
    rows,cols=3,4+v
    for row in range(rows):
        for col in range(cols):
            x=(col-(cols-1)/2)*w*.82/cols; z=1.45+(row-1)*h*.70/rows
            add(scene,box((w*.82/cols-.012,.035,h*.70/rows-.012),(x,-.07,z),mat),f'GlazedTile_{row}_{col}')
    add(scene,box((w*.92,.08,.065),(0,-.06,1.45+h*.47),MAT_BRASS),'WainscotCap')
    add(scene,box((.16,.025,.05),(w*.36,-.13,1.45-h*.38),MAT_LABEL),'TileLotLabel')
    return scene


def _parquet_threshold(v,mat):
    scene=trimesh.Scene(); w=1.12+.10*v; d=.32+.03*v
    add(scene,box((w,d,.09),(0,0,.045),MAT_STONE),'ThresholdCore')
    add(scene,box((w*.92,d*.70,.035),(0,0,.1075),mat),'ParquetTransitionField')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.70/(2+v)
        add(scene,box((.12,d*.62,.025),(x,0,.1375),MAT_WOOD),f'WoodTransitionStrip_{i}')
    for side,y in enumerate((-.45*d,.45*d)):
        add(scene,box((w*.94,.02,.035),(0,y,.13),MAT_BRASS),f'ThresholdEnd_{side}')
    add(scene,box((.18,.025,.05),(w*.35,-d*.55,.15),MAT_LABEL),'ThresholdSpec')
    return scene


def _stone_corner_guard(v,mat):
    scene=trimesh.Scene(); h=1.10+.12*v; w=.19+.015*v
    add(scene,box((w,.10,h),(w/2,0,h/2),mat),'GuardFaceA')
    add(scene,box((.10,w,h),(0,w/2,h/2),MAT_STONE),'GuardFaceB')
    add(scene,box((w*.70,w*.70,h*.94),(0,0,h/2),MAT_STEEL),'CornerCore')
    for i,z in enumerate((.18,h*.50,h*.86)):
        add(scene,cyl(.018,.025,(w*.70,-.06,z),MAT_BRASS,8),f'GuardFastener_{i}')
    add(scene,box((.12,.025,.05),(w*.54,-.07,.12),MAT_LABEL),'GuardMaterialStamp')
    return scene


def _wall_reveal(v,mat):
    scene=trimesh.Scene(); w=1.15+.10*v; h=1.40+.12*v
    add(scene,box((w,.08,h),(0,0,1.55),MAT_STONE),'RevealWallPanel')
    add(scene,box((w*.72,.025,h*.68),(0,-.06,1.55),mat),'InsetFinishField')
    for side,x in enumerate((-.42*w,.42*w)):
        add(scene,box((.035,.06,h*.88),(x,-.09,1.55),MAT_BRASS),f'VerticalReveal_{side}')
    for i,z in enumerate((1.55-h*.42,1.55+h*.42)):
        add(scene,box((w*.88,.06,.035),(0,-.09,z),MAT_STEEL),f'HorizontalReveal_{i}')
    add(scene,box((.17,.025,.05),(w*.33,-.12,1.55-h*.40),MAT_LABEL),'RevealProfileLabel')
    return scene


def _elevator_wall_cassette(v,mat):
    scene=trimesh.Scene(); w=1.15+.09*v; h=1.48+.10*v
    add(scene,box((w,.12,h),(0,0,1.55),MAT_STEEL),'ElevatorWallSubframe')
    for i in range(3+v//2):
        x=(i-(2+v//2)/2)*w*.80/(2+v//2)
        add(scene,box((w*.80/(3+v//2),.05,h*.82),(x,-.09,1.55),mat),f'CabWallPanel_{i}')
        add(scene,box((.018,.065,h*.76),(x-w*.40/(3+v//2),-.12,1.55),MAT_BRASS),f'PanelDivider_{i}')
    add(scene,box((.24,.035,.11),(0,-.16,1.55-h*.36),MAT_LABEL),'ElevatorPanelCode')
    for side,x in enumerate((-.46*w,.46*w)):
        add(scene,box((.04,.06,h*.90),(x,-.10,1.55),MAT_STONE),f'CassetteLockRail_{side}')
    return scene


def _finish(family,v,mat):
    return (_floor_medallion,_microcement_panel,_ribbed_plaster_board,_acoustic_raft,_vent_cassette,
            _tile_wainscot,_parquet_threshold,_stone_corner_guard,_wall_reveal,_elevator_wall_cassette)[family](v,mat)


# Batch 55: specialized banquet, buffet, and premium dining service fixtures.
def _carving_station(v,mat):
    scene=trimesh.Scene(); w=1.05+.10*v; d=.70+.06*v; h=.82+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'CarvingStationCounter')
    _legs(scene,w,d,h-.06,MAT_STEEL,'StationLeg')
    add(scene,box((w*.72,d*.60,.10),(0,0,h+.11),MAT_STEEL),'HeatedCarvingWell')
    add(scene,box((w*.82,.035,.34),(0,d*.30,h+.37),MAT_GLASS),'CarvingSneezeGuard')
    for side,x in enumerate((-.32*w,.32*w)):
        _post(scene,f'SneezeGuardSupport_{side}',x,d*.30,.36,.025,MAT_BRASS)
    add(scene,box((.25,.03,.07),(0,-d*.53,h-.10),MAT_LABEL),'CarvingStationCard')
    return scene


def _noodle_induction(v,mat):
    scene=trimesh.Scene(); w=.88+.08*v; d=.68+.05*v; h=.78+.06*v
    add(scene,box((w,d,.12),(0,0,h),mat),'InductionNoodleCounter')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*w*.42/(1+v//2)
        add(scene,cyl(.15,.03,(x,-.04,h+.075),MAT_STEEL,16),f'WokInductionZone_{i}')
        add(scene,cyl(.11,.045,(x,-.04,h+.11),MAT_CERAMIC,16),f'WokSupportRing_{i}')
    _legs(scene,w,d,h-.06,MAT_STEEL,'NoodleStationLeg')
    add(scene,box((w*.45,.035,.14),(0,d*.32,h+.16),MAT_STEEL),'IngredientShelf')
    add(scene,box((.22,.025,.07),(0,-d*.53,h-.09),MAT_LABEL),'InductionSafetyLabel')
    return scene


def _oyster_display(v,mat):
    scene=trimesh.Scene(); w=.92+.08*v; d=.66+.05*v; h=.72+.05*v
    add(scene,box((w,d,.12),(0,0,.06),MAT_STONE),'SeafoodDisplayBase')
    add(scene,box((w*.76,d*.70,h-.12),(0,0,h/2),mat),'DisplayPedestal')
    add(scene,box((w*.85,d*.76,.10),(0,0,h+.04),MAT_CERAMIC),'ChilledOysterBed')
    for i in range(5+v):
        x=(i-(4+v)/2)*w*.70/(4+v)
        y=((-1)**i)*d*.17
        add(scene,sphere(.065+.003*v,(x,y,h+.16),MAT_STONE,0),f'OysterShell_{i}')
    add(scene,box((w*.86,.035,.27),(0,d*.38,h+.18),MAT_GLASS),'ChilledDisplayGuard')
    add(scene,box((.22,.025,.06),(0,-d*.53,h+.03),MAT_LABEL),'OysterOriginCard')
    return scene


def _espresso_cart(v,mat):
    scene=trimesh.Scene(); w=.92+.08*v; d=.50+.05*v; h=.84+.06*v
    add(scene,box((w,d,.11),(0,0,h),mat),'EspressoCartTop')
    _legs(scene,w,d,h-.05,MAT_STEEL,'CoffeeCartLeg')
    _wheels(scene,w,d,.07,'CoffeeCartCaster')
    add(scene,box((w*.58,d*.56,.24),(0,0,h+.17),MAT_ELECTRONICS),'EspressoMachine')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*.20
        add(scene,cyl(.04,.08,(x,-d*.24,h+.13),MAT_BRASS,10),f'GroupHead_{i}')
    add(scene,box((.22,.03,.08),(0,-d*.53,h-.10),MAT_LABEL),'CoffeeCartBrandPlate')
    return scene


def _dessert_cart(v,mat):
    scene=trimesh.Scene(); w=.82+.08*v; d=.54+.04*v; h=.82+.06*v
    add(scene,box((w,d,.10),(0,0,h),mat),'DessertCartDeck')
    add(scene,box((w*.86,.07,.30),(0,d*.34,h+.20),MAT_GLASS),'DessertDisplayCanopy')
    _legs(scene,w,d,h-.04,MAT_WOOD,'DessertCartPost')
    _wheels(scene,w,d,.07,'DessertCartCaster')
    for i in range(3+v):
        x=(i-(2+v)/2)*w*.66/(2+v)
        add(scene,cyl(.075,.055,(x,0,h+.10),MAT_CERAMIC,12),f'PastryPlatter_{i}')
    add(scene,box((.20,.025,.06),(0,-d*.53,h-.10),MAT_LABEL),'DessertCartMenu')
    return scene


def _decanter_stand(v,mat):
    scene=trimesh.Scene(); w=.88+.08*v; d=.42+.04*v; h=.94+.06*v
    add(scene,box((w,d,.08),(0,0,h),mat),'DecanterDisplayTop')
    for i in range(2+v//2):
        z=.28+i*.20
        add(scene,box((w*.82,d*.70,.045),(0,0,z),MAT_WOOD),f'DecanterShelf_{i}')
    _legs(scene,w,d,h-.05,MAT_BRASS,'DecanterStandLeg')
    for i in range(2+v):
        x=(i-(1+v)/2)*w*.72/(1+v)
        add(scene,cyl(.06,.18,(x,0,.54),MAT_GLASS,12),f'DisplayDecanter_{i}')
        add(scene,cyl(.022,.07,(x,0,.67),MAT_BRASS,10),f'DecanterStopper_{i}')
    add(scene,box((.18,.025,.06),(0,-d*.53,h-.08),MAT_LABEL),'DecanterCollectionLabel')
    return scene


def _draft_tower(v,mat):
    scene=trimesh.Scene(); w=.80+.07*v; d=.54+.05*v; h=.78+.05*v
    add(scene,box((w,d,.12),(0,0,h),mat),'DraftServiceCounter')
    add(scene,box((w*.68,d*.64,.34),(0,0,h+.22),MAT_STEEL),'ChilledKegHousing')
    for i in range(2+v):
        x=(i-(1+v)/2)*w*.56/(1+v)
        add(scene,cyl(.04,.22,(x,-d*.20,h+.50),MAT_BRASS,10),f'DraftTapColumn_{i}')
        add(scene,box((.12,.035,.07),(x,-d*.20,h+.63),MAT_WOOD),f'TapHandle_{i}')
    _legs(scene,w,d,h-.06,MAT_STEEL,'DraftCounterLeg')
    add(scene,box((.22,.025,.07),(0,-d*.53,h-.09),MAT_LABEL),'DraftLineLabel')
    return scene


def _glass_polisher(v,mat):
    scene=trimesh.Scene(); w=.62+.05*v; d=.54+.05*v; h=.66+.05*v
    add(scene,box((w,d,.12),(0,0,h),mat),'PolisherBaseCabinet')
    add(scene,box((w*.82,d*.78,.09),(0,0,h+.10),MAT_STEEL),'PolisherWorkSurface')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*.18
        add(scene,cyl(.11,.06,(x,0,h+.18),MAT_STEEL,12),f'GlassPolishingCup_{i}')
    add(scene,box((w*.68,.035,.20),(0,-d*.50,h+.12),MAT_ELECTRONICS),'PolisherControlPanel')
    add(scene,cyl(.025,.04,(w*.28,-d*.55,h+.21),MAT_BRASS,8),'PolisherStartButton')
    _legs(scene,w,d,h-.06,MAT_STEEL,'PolisherFoot')
    return scene


def _cereal_tower(v,mat):
    scene=trimesh.Scene(); w=.68+.06*v; d=.50+.04*v; h=1.36+.10*v
    add(scene,box((w*.76,d*.72,.10),(0,0,.05),MAT_STONE),'DispenserFoot')
    for i in range(2+v//2):
        x=(i-(1+v//2)/2)*w*.35/(1+v//2)
        add(scene,box((w*.32,d*.34,.42),(x,0,h*.62),MAT_GLASS),f'CerealHopper_{i}')
        add(scene,box((w*.32,d*.34,.06),(x,0,h*.62+.24),mat),f'HopperLid_{i}')
        add(scene,cyl(.035,.08,(x,-d*.24,h*.30),MAT_BRASS,8),f'PortionLever_{i}')
        add(scene,box((.09,.10,.06),(x,-d*.25,h*.18),MAT_CERAMIC),f'BowlRest_{i}')
    _post(scene,'DispenserSpine',0,d*.34,h,.045,MAT_WOOD)
    add(scene,box((.22,.025,.06),(0,-d*.40,.16),MAT_LABEL),'AllergenNotice')
    return scene


def _grab_go_case(v,mat):
    scene=trimesh.Scene(); w=1.18+.10*v; d=.62+.06*v; h=.88+.07*v
    add(scene,box((w,d,.12),(0,0,h),mat),'RefrigeratedCaseBase')
    add(scene,box((w*.90,d*.78,.44),(0,0,h+.28),MAT_GLASS),'GrabGoGlassCase')
    for i in range(2+v//2):
        z=h+.14+i*.16
        add(scene,box((w*.82,d*.68,.035),(0,0,z),MAT_STEEL),f'RetailShelf_{i}')
    _legs(scene,w,d,h-.06,MAT_STEEL,'CaseSupport')
    add(scene,box((.25,.03,.08),(0,-d*.53,h-.09),MAT_LABEL),'CaseTemperatureTag')
    add(scene,box((w*.84,.025,.05),(0,d*.38,h+.45),MAT_BRASS),'GlassTopRail')
    return scene


def _restaurant(family,v,mat):
    return (_carving_station,_noodle_induction,_oyster_display,_espresso_cart,_dessert_cart,
            _decanter_stand,_draft_tower,_glass_polisher,_cereal_tower,_grab_go_case)[family](v,mat)


def build_asset(name, subcategory, mat, asset_id, profile):
    try:
        number=int(asset_id.removeprefix('HH_A'))
    except ValueError as exc:
        raise ValueError(f'destination expansion asset ID outside A2501–A2750: {asset_id}') from exc
    if not 2501 <= number <= 2750:
        raise ValueError(f'destination expansion asset ID outside A2501–A2750: {asset_id}')
    batch_index=(number-2501)//50
    family,variant=divmod((number-2501)%50,5)
    return (_events,_decor,_exterior,_finish,_restaurant)[batch_index](family,variant,mat)
