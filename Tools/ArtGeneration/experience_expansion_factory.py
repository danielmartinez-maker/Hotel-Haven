"""Purpose-built accessible guest, heritage display, grounds, spa and finish assets for A2251–A2500."""

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


# Batch 46: accessible guest-room equipment and room-control fixtures.
def _transfer_bench(v, mat):
    scene = trimesh.Scene(); w, d, h = .92 + .07 * v, .48 + .04 * v, .48 + .025 * v
    add(scene, box((w * .70, d, .10), (-w * .14, 0, h), mat), 'ShowerTransferSeat')
    for side, x in enumerate((-.40 * w, .40 * w)):
        add(scene, box((.06, .06, h), (x, 0, h / 2), 'MAT_STAINLESS'), f'BenchSupport_{side}')
        add(scene, box((.055, d * .72, .06), (x, 0, .03), 'MAT_BLACKENED_STEEL'), f'BenchFoot_{side}')
    add(scene, box((w * .32, .07, .10), (w * .34, 0, h + .05), 'MAT_STAINLESS'), 'TransferWing')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .52 / (3 + v)
        add(scene, box((.028, d * .72, .018), (x, 0, h + .052), 'MAT_STONE_LIGHT'), f'SeatDrainGroove_{i}')
    add(scene, box((w * .62, .04, .08), (0, -d * .52, .19), 'MAT_SIGNAGE'), 'TransferBenchLabel')
    return scene


def _fold_down_support_rail(v, mat):
    scene = trimesh.Scene(); length = .72 + .08 * v
    add(scene, box((length * 1.14, .11, .16), (0, 0, 1.08), mat), 'SupportRailWallPlate')
    for side, x in enumerate((-.36, .36)):
        add(scene, cyl(.035, .16, (x, -.07, 1.08), 'MAT_BRASS_POLISHED', 12), f'RailHinge_{side}')
    rail = cyl(.045, length, (0, -.28, 1.08), 'MAT_STAINLESS', 16)
    _rot(rail, math.pi / 2, [1, 0, 0], (0, -.28, 1.08)); add(scene, rail, 'FoldDownSupportBar')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * length / (2 + v)
        add(scene, cyl(.055, .035, (x, -.28, 1.08), 'MAT_PLASTIC_RUBBER', 12), f'GripSleeve_{i}')
    add(scene, box((.20, .10, .06), (length * .52, -.08, 1.02), 'MAT_BLACKENED_STEEL'), 'RailLatch')
    add(scene, box((.26, .035, .08), (0, -.10, .95), 'MAT_SIGNAGE'), 'RailLoadLabel')
    return scene


def _visual_alert_panel(v, mat):
    scene = trimesh.Scene(); w, h = .42 + .04 * v, .36 + .03 * v
    add(scene, box((w, .10, h), (0, 0, 1.55), mat), 'AlertPanelHousing')
    add(scene, box((w * .68, .025, h * .42), (0, -.065, 1.62), 'MAT_GLASS_CLEAR'), 'AlertDiffuser')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .44 / (1 + v // 2)
        add(scene, sphere(.045 + .004 * v, (x, -.09, 1.63), 'MAT_EMISSIVE_WARM', 1), f'AlertBeacon_{i}')
    add(scene, box((w * .78, .035, .06), (0, -.07, 1.39), 'MAT_SIGNAGE'), 'AlertSystemLabel')
    for i in range(4):
        add(scene, cyl(.018, .025, ((i - 1.5) * w * .30, -.07, 1.76), 'MAT_STAINLESS', 8), f'PanelFastener_{i}')
    return scene


def _bed_shaker_alarm(v, mat):
    scene = trimesh.Scene(); w, d = .44 + .035 * v, .32 + .025 * v
    add(scene, box((w, d, .14), (0, 0, .07), mat), 'BedsideAlarmBase')
    add(scene, box((w * .84, d * .74, .20 + .015 * v), (0, .02, .24), 'MAT_ELECTRONICS'), 'AlarmClockBody')
    add(scene, box((w * .55, .025, .09), (0, -.12, .25), 'MAT_GLASS_CLEAR'), 'ClockDisplay')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * .085
        add(scene, cyl(.028, .035, (x, -.14, .18), 'MAT_BRASS_POLISHED', 10), f'AlarmControl_{i}')
    shaker = cyl(.11 + .006 * v, .055, (w * .38, .06, .48), 'MAT_BLACKENED_STEEL', 16)
    _rot(shaker, math.pi / 2, [1, 0, 0], (w * .38, .06, .48)); add(scene, shaker, 'UnderPillowVibrationDisc')
    add(scene, box((.16, .045, .05), (-w * .35, -.13, .14), 'MAT_SIGNAGE'), 'AlarmModeLabel')
    return scene


def _curtain_cassette(v, mat):
    scene = trimesh.Scene(); w, h = 1.12 + .10 * v, 1.86 + .12 * v
    add(scene, box((w, .18, .20), (0, 0, h), mat), 'MotorizedShadeCassette')
    add(scene, box((w * .94, .035, h * .82), (0, -.10, h * .51), 'MAT_LINEN'), 'BlackoutShade')
    add(scene, box((w * .98, .10, .075), (0, 0, h * .08), 'MAT_STAINLESS'), 'ShadeBottomBar')
    add(scene, box((.14, .08, .22), (w * .55, -.10, h * .90), 'MAT_ELECTRONICS'), 'ShadeMotorHousing')
    for side, x in enumerate((-.44 * w, .44 * w)):
        add(scene, box((.045, .06, h * .84), (x, 0, h * .48), 'MAT_BLACKENED_STEEL'), f'ShadeGuideRail_{side}')
    add(scene, box((.24, .045, .075), (w * .58, -.10, h * .80), 'MAT_SIGNAGE'), 'ShadeServiceTag')
    return scene


def _call_pendant_station(v, mat):
    scene = trimesh.Scene(); w, h = .58 + .05 * v, .92 + .06 * v
    add(scene, box((w, .42, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'CallStationFoot')
    add(scene, box((.16, .20, h * .74), (0, .02, h * .37 + .08), mat), 'CallStationPedestal')
    add(scene, box((w * .86, .08, .12), (0, 0, h * .82), 'MAT_STAINLESS'), 'PendantDockShelf')
    add(scene, sphere(.11, (0, -.07, h * .82 + .14), 'MAT_ELECTRONICS', 2), 'AssistancePendant')
    add(scene, cyl(.032, .34 + .02 * v, (0, -.07, h * .82 + .34), 'MAT_STAINLESS', 12), 'PendantCord')
    for i, material in enumerate(('MAT_EMISSIVE_WARM', 'MAT_BRASS_POLISHED')):
        add(scene, cyl(.035, .03, ((i - .5) * .13, -.19, .54), material, 12), f'CallButton_{i}')
    add(scene, box((w * .72, .035, .12), (0, -.22, .23), 'MAT_SIGNAGE'), 'CallStationInstruction')
    return scene


def _roll_under_nightstand(v, mat):
    scene = trimesh.Scene(); w, d, h = .58 + .05 * v, .44 + .04 * v, .68 + .05 * v
    add(scene, box((w * .78, d * .76, h * .76), (0, 0, h * .46), mat), 'NightstandCase')
    add(scene, box((w, d, .07), (0, 0, .04), 'MAT_STAINLESS'), 'NightstandPlinth')
    add(scene, box((w * .94, d * .90, .075), (0, 0, h * .87), 'MAT_WOOD_WARM'), 'NightstandTop')
    for i in range(2 + v // 2):
        z = .26 + i * .17
        add(scene, box((w * .65, .025, .12), (0, -d * .39, z), 'MAT_WOOD_WARM'), f'OpenDrawerFront_{i}')
        add(scene, cyl(.023, .04, (0, -d * .42, z), 'MAT_BRASS_POLISHED', 10), f'DrawerPull_{i}')
    for i in range(4):
        add(scene, cyl(.085, .06, ((i % 2 - .5) * w * .74, (i // 2 - .5) * d * .70, .09), 'MAT_BLACKENED_STEEL', 14), f'NightstandCaster_{i}')
    add(scene, box((w * .78, .035, .06), (0, -d * .51, .15), 'MAT_SIGNAGE'), 'MobilityClearanceTag')
    return scene


def _desk_knee_return(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.22 + .10 * v, .62 + .05 * v, .76 + .04 * v
    add(scene, box((w, d, .09), (0, 0, h), mat), 'AccessibleDeskTop')
    add(scene, box((w * .40, .16, h - .10), (-w * .28, 0, (h - .10) / 2), 'MAT_WOOD_WARM'), 'DeskPedestal')
    add(scene, box((.07, d * .88, h - .12), (w * .46, 0, (h - .12) / 2), 'MAT_STAINLESS'), 'OpenSideSupport')
    add(scene, box((w * .48, .14, .08), (w * .22, d * .28, .04), 'MAT_STONE_LIGHT'), 'FootrestBar')
    add(scene, box((.24, .035, .09), (-w * .32, -d * .53, .20), 'MAT_SIGNAGE'), 'AccessibleWorksurfaceLabel')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * .11
        add(scene, cyl(.018, .02, (x, 0, h + .055), 'MAT_BRASS_POLISHED', 8), f'DeskEdgeFastener_{i}')
    return scene


def _tactile_room_control(v, mat):
    scene = trimesh.Scene(); w, h = .42 + .035 * v, .56 + .04 * v
    add(scene, box((w, .10, h), (0, 0, 1.20), mat), 'RoomControlPanel')
    add(scene, box((w * .78, .03, h * .25), (0, -.07, 1.38), 'MAT_GLASS_CLEAR'), 'ControlDisplay')
    for row in range(3 + v // 2):
        for col in range(2):
            x = (col - .5) * w * .48
            z = 1.12 + row * .09
            add(scene, box((.09, .04, .06), (x, -.08, z), 'MAT_STAINLESS'), f'TactileKey_{row}_{col}')
            add(scene, sphere(.014, (x, -.105, z), 'MAT_BRASS_POLISHED', 1), f'BrailleDot_{row}_{col}')
    add(scene, box((w * .72, .035, .07), (0, -.07, .88), 'MAT_SIGNAGE'), 'RoomControlLegend')
    for i in range(4):
        add(scene, cyl(.016, .025, ((i - 1.5) * w * .34, -.07, 1.48), 'MAT_BLACKENED_STEEL', 8), f'PanelScrew_{i}')
    return scene


def _transfer_aid_seat(v, mat):
    scene = trimesh.Scene(); w, d, h = .72 + .06 * v, .56 + .05 * v, .58 + .03 * v
    add(scene, box((w * .68, d, .11), (-w * .12, 0, h), mat), 'TransferAidSeat')
    for side, x in enumerate((-.34 * w, .34 * w)):
        add(scene, box((.06, .06, h), (x, 0, h / 2), 'MAT_STAINLESS'), f'TransferSeatLeg_{side}')
        add(scene, box((.11, d * .86, .07), (x, 0, .035), 'MAT_BLACKENED_STEEL'), f'TransferSeatFoot_{side}')
    add(scene, box((.08, .08, h * .70), (w * .43, 0, h * .39), 'MAT_STAINLESS'), 'SupportHandlePost')
    add(scene, cyl(.028, .46 + .04 * v, (w * .43, -.03, h * .78), 'MAT_PLASTIC_RUBBER', 12), 'SupportHandGrip')
    add(scene, box((w * .60, .04, .07), (0, -d * .52, .18), 'MAT_SIGNAGE'), 'TransferAidLabel')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * w * .38 / (2 + v)
        add(scene, box((.025, d * .70, .015), (x, 0, h + .062), 'MAT_STONE_LIGHT'), f'SeatDrainLine_{i}')
    return scene


def _accessible_guest(family, v, mat):
    return (_transfer_bench, _fold_down_support_rail, _visual_alert_panel, _bed_shaker_alarm,
            _curtain_cassette, _call_pendant_station, _roll_under_nightstand, _desk_knee_return,
            _tactile_room_control, _transfer_aid_seat)[family](v, mat)


# Batch 47: hotel heritage, curated collections and lobby display systems.
def _heritage_vitrine(v, mat):
    scene = trimesh.Scene(); w, d, h = .92 + .08 * v, .58 + .05 * v, 1.48 + .10 * v
    add(scene, box((w * 1.08, d * 1.06, .09), (0, 0, .045), 'MAT_STONE_LIGHT'), 'VitrinePlinth')
    add(scene, box((w, d, h * .70), (0, 0, h * .45), mat), 'DisplayCabinetBase')
    add(scene, box((w * .90, d * .90, h * .56), (0, 0, h * .83), 'MAT_GLASS_CLEAR'), 'GlassShowcase')
    for side, x in enumerate((-.43 * w, .43 * w)):
        add(scene, box((.05, d * .88, h * .58), (x, 0, h * .82), 'MAT_STAINLESS'), f'VitrineCornerPost_{side}')
    add(scene, box((w * .62, .08, .06), (0, -d * .53, h * .13), 'MAT_SIGNAGE'), 'VitrineCollectionPlaque')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * w * .55 / (2 + v)
        add(scene, cyl(.085 + .006 * v, .12, (x, 0, h * .60), 'MAT_BRASS_POLISHED', 14), f'ArtifactStand_{i}')
    return scene


def _artifact_plinth_case(v, mat):
    scene = trimesh.Scene(); w, h = .72 + .06 * v, 1.05 + .08 * v
    add(scene, box((w, .72, .10), (0, 0, .05), mat), 'ArtifactPlinthBase')
    add(scene, box((w * .78, .50, h * .62), (0, 0, h * .36), 'MAT_STONE_LIGHT'), 'DisplayPedestal')
    add(scene, box((w * .90, .62, .06), (0, 0, h * .68), 'MAT_BRASS_POLISHED'), 'ArtifactShelf')
    add(scene, box((w * .72, .44, .34 + .02 * v), (0, 0, h * .88), 'MAT_GLASS_CLEAR'), 'ProtectiveGlassDome')
    add(scene, cyl(.14 + .01 * v, .20, (0, 0, h * .77), mat, 18), 'ArtifactMount')
    add(scene, box((w * .66, .04, .07), (0, -.36, .16), 'MAT_SIGNAGE'), 'ArtifactTitlePlaque')
    return scene


def _timeline_panel(v, mat):
    scene = trimesh.Scene(); w, h = 1.32 + .12 * v, 1.60 + .10 * v
    add(scene, box((w * 1.04, .16, h), (0, 0, h / 2), mat), 'HistoryPanelBacking')
    add(scene, box((w * .86, .05, h * .72), (0, -.11, h * .52), 'MAT_WOOD_WARM'), 'TimelineRail')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .76 / (3 + v)
        add(scene, sphere(.055 + .004 * v, (x, -.16, h * .52), 'MAT_BRASS_POLISHED', 2), f'TimelineMilestone_{i}')
        add(scene, box((.15, .035, .22), (x, -.16, h * .34), 'MAT_SIGNAGE'), f'TimelineDateCard_{i}')
        add(scene, box((.12, .04, .28), (x, -.16, h * .68), 'MAT_LINEN'), f'TimelineImage_{i}')
    add(scene, box((w * .68, .045, .10), (0, -.13, h * .90), 'MAT_BRASS_POLISHED'), 'HotelHeritageTitle')
    add(scene, box((w * 1.10, .24, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'TimelineBase')
    return scene


def _textile_display(v, mat):
    scene = trimesh.Scene(); w, h = .92 + .08 * v, 1.24 + .10 * v
    add(scene, box((w, .12, h), (0, 0, h / 2), 'MAT_WOOD_WARM'), 'TextileDisplayFrame')
    add(scene, box((w * .78, .035, h * .76), (0, -.08, h * .52), mat), 'RegionalTextile')
    for side, x in enumerate((-.44 * w, .44 * w)):
        add(scene, box((.06, .06, h * .88), (x, 0, h / 2), 'MAT_BRASS_POLISHED'), f'FrameStile_{side}')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .74 / (4 + v)
        add(scene, box((.028, .045, h * .70), (x, -.11, h * .52), 'MAT_STONE_LIGHT'), f'TextileWeave_{i}')
    add(scene, box((w * .58, .045, .08), (0, -.10, .16), 'MAT_SIGNAGE'), 'TextileProvenanceCard')
    add(scene, box((w * 1.10, .24, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'TextileDisplayBase')
    return scene


def _rotating_curio(v, mat):
    scene = trimesh.Scene(); w, h = .70 + .06 * v, 1.50 + .10 * v
    add(scene, cyl(w * .70, .10, (0, 0, .05), 'MAT_STONE_LIGHT', 24), 'CurioTurntableBase')
    add(scene, cyl(.08, h * .54, (0, 0, h * .29 + .10), 'MAT_BLACKENED_STEEL', 16), 'RotatingSpindle')
    add(scene, cyl(w * .44, .07, (0, 0, h * .56), 'MAT_WOOD_WARM', 22), 'CurioShelfLower')
    add(scene, cyl(w * .44, .07, (0, 0, h * .83), 'MAT_WOOD_WARM', 22), 'CurioShelfUpper')
    for side, angle in enumerate((0, math.pi / 2, math.pi, 3 * math.pi / 2)):
        x, y = w * .40 * math.cos(angle), w * .40 * math.sin(angle)
        add(scene, box((.05, .05, h * .78), (x, y, h * .48), mat), f'CurioGlassPost_{side}')
    add(scene, box((w * .82, .04, .08), (0, -w * .42, .16), 'MAT_SIGNAGE'), 'CurioCollectionLabel')
    return scene


def _heritage_map_stand(v, mat):
    scene = trimesh.Scene(); w, h = .80 + .06 * v, 1.18 + .08 * v
    add(scene, box((w * .84, .56, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'MapStandFoot')
    add(scene, box((.18, .18, h * .72), (0, 0, h * .37), mat), 'MapStandPedestal')
    panel = box((w, .10, .70 + .03 * v), (0, -.02, h * .83), 'MAT_WOOD_WARM')
    _rot(panel, -.13, [1, 0, 0], (0, -.02, h * .83)); add(scene, panel, 'HeritageMapPanel')
    add(scene, box((w * .78, .04, .45), (0, -.09, h * .83), 'MAT_LINEN'), 'PrintedHotelMap')
    add(scene, box((w * .86, .06, .07), (0, -.28, h * .56), 'MAT_BRASS_POLISHED'), 'MapRetainerRail')
    add(scene, box((.20, .12, .10), (w * .32, -.12, .26), 'MAT_SIGNAGE'), 'MapKeyCard')
    return scene


def _wall_medallion(v, mat):
    scene = trimesh.Scene(); r = .44 + .04 * v
    medallion = cyl(r, .10, (0, 0, 1.42), mat, 32)
    _rot(medallion, math.pi / 2, [1, 0, 0], (0, 0, 1.42)); add(scene, medallion, 'HeritageMedallion')
    ring = cyl(r * .80, .07, (0, -.08, 1.42), 'MAT_BRASS_POLISHED', 32)
    _rot(ring, math.pi / 2, [1, 0, 0], (0, -.08, 1.42)); add(scene, ring, 'MedallionBorder')
    for i in range(6 + v):
        angle = 2 * math.pi * i / (6 + v)
        x, z = r * .52 * math.cos(angle), 1.42 + r * .52 * math.sin(angle)
        add(scene, sphere(.045 + .003 * v, (x, -.13, z), 'MAT_STONE_LIGHT', 1), f'MosaicInlay_{i}')
    add(scene, cyl(.10, .05, (0, -.14, 1.42), 'MAT_BRASS_POLISHED', 18), 'HotelCrestBoss')
    for i in range(4):
        x = (i - 1.5) * r * .50
        add(scene, cyl(.018, .03, (x, -.09, 1.42), 'MAT_STAINLESS', 8), f'MedallionMount_{i}')
    return scene


def _acoustic_art_screen(v, mat):
    scene = trimesh.Scene(); w, h = 1.08 + .10 * v, 1.42 + .10 * v
    add(scene, box((w, .16, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'ArtScreenBase')
    add(scene, box((w * .94, .10, h), (0, 0, h / 2 + .08), 'MAT_BLACKENED_STEEL'), 'ArtScreenFrame')
    add(scene, box((w * .82, .035, h * .82), (0, -.07, h * .48), mat), 'AcousticArtwork')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .74 / (4 + v)
        add(scene, box((.035, .05, h * .70), (x, -.10, h * .48), 'MAT_WOOD_WARM'), f'ArtworkSlat_{i}')
    add(scene, box((w * .70, .04, .12), (0, -.09, h + .10), 'MAT_BRASS_POLISHED'), 'ArtScreenHeader')
    for side, x in enumerate((-.40 * w, .40 * w)):
        add(scene, cyl(.07, .09, (x, 0, .13), 'MAT_BLACKENED_STEEL', 12), f'ScreenCaster_{side}')
    return scene


def _donor_wall(v, mat):
    scene = trimesh.Scene(); w, h = 1.28 + .12 * v, 1.52 + .10 * v
    add(scene, box((w, .14, h), (0, 0, h / 2), mat), 'RecognitionWallBacking')
    add(scene, box((w * .78, .04, .10), (0, -.09, h * .88), 'MAT_BRASS_POLISHED'), 'DonorWallTitle')
    for i in range(6 + v):
        x = (i - (5 + v) / 2) * w * .74 / (5 + v)
        z = .38 + (i % 3) * .36
        add(scene, box((.16 + .01 * v, .04, .11), (x, -.10, z), 'MAT_SIGNAGE'), f'DonorNamePlate_{i}')
        add(scene, cyl(.020, .03, (x, -.13, z), 'MAT_BRASS_POLISHED', 8), f'PlateMount_{i}')
    add(scene, box((w * 1.06, .26, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'DonorWallBase')
    return scene


def _gallery_label_stand(v, mat):
    scene = trimesh.Scene(); w, h = .68 + .06 * v, 1.02 + .08 * v
    add(scene, box((w * .90, .48, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'LabelStandFoot')
    add(scene, cyl(.055, h * .68, (0, 0, h * .34 + .08), mat, 14), 'LabelStandColumn')
    add(scene, box((w, .12, .36 + .02 * v), (0, -.02, h * .80), 'MAT_WOOD_WARM'), 'GalleryLabelPanel')
    add(scene, box((w * .80, .05, .24), (0, -.10, h * .80), 'MAT_LINEN'), 'ExhibitTextCard')
    add(scene, box((w * .94, .05, .08), (0, -.12, h * .60), 'MAT_BRASS_POLISHED'), 'LabelCardStop')
    for i in range(4):
        add(scene, cyl(.018, .024, ((i - 1.5) * w * .34, -.13, h * .80), 'MAT_STAINLESS', 8), f'LabelFastener_{i}')
    return scene


def _heritage_displays(family, v, mat):
    return (_heritage_vitrine, _artifact_plinth_case, _timeline_panel, _textile_display,
            _rotating_curio, _heritage_map_stand, _wall_medallion, _acoustic_art_screen,
            _donor_wall, _gallery_label_stand)[family](v, mat)


# Batch 48: parking, accessible arrival and grounds infrastructure.
def _parking_barrier(v, mat):
    scene = trimesh.Scene(); length = 1.65 + .14 * v
    add(scene, box((.46, .52, .18), (0, 0, .09), 'MAT_STONE_LIGHT'), 'BarrierBase')
    add(scene, box((.36, .40, .56 + .04 * v), (0, 0, .46 + .02 * v), mat), 'BarrierMotorCabinet')
    arm = box((length, .09, .09), (length * .46, 0, .76 + .03 * v), 'MAT_STAINLESS')
    _rot(arm, -.025 - .008 * v, [0, 1, 0], (0, 0, .76 + .03 * v)); add(scene, arm, 'GateArm')
    for i in range(5 + v):
        x = (i + .5) * length / (5 + v)
        add(scene, box((.10, .10, .095), (x, -.005, .77 + .03 * v - .025 * x), 'MAT_SIGNAGE'), f'BarrierReflector_{i}')
    add(scene, cyl(.07, .08, (.20, -.25, .52), 'MAT_BRASS_POLISHED', 14), 'ManualReleaseKnob')
    add(scene, box((.28, .04, .14), (-.02, -.24, .62), 'MAT_ELECTRONICS'), 'GateController')
    return scene


def _ev_charger(v, mat):
    scene = trimesh.Scene(); w, h = .42 + .04 * v, 1.48 + .10 * v
    add(scene, box((w * 1.60, .48, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'ChargerFoot')
    add(scene, box((w, .28, h * .74), (0, 0, h * .37 + .10), mat), 'EVChargerColumn')
    add(scene, box((w * .72, .04, .32), (0, -.16, h * .75), 'MAT_ELECTRONICS'), 'ChargerDisplay')
    add(scene, box((w * .60, .025, .24), (0, -.19, h * .75), 'MAT_GLASS_CLEAR'), 'DisplayGlass')
    cable = cyl(.028, .62 + .04 * v, (w * .55, .06, h * .55), 'MAT_BLACKENED_STEEL', 12)
    _rot(cable, .60, [0, 1, 0], (w * .55, .06, h * .55)); add(scene, cable, 'ChargingCable')
    add(scene, cyl(.07, .15, (w * .58, -.04, h * .42), 'MAT_STAINLESS', 12), 'CableDock')
    add(scene, box((w * .82, .04, .12), (0, -.18, .30), 'MAT_SIGNAGE'), 'EVServiceLabel')
    return scene


def _accessible_parking_totem(v, mat):
    scene = trimesh.Scene(); w, h = .52 + .05 * v, 1.72 + .10 * v
    add(scene, box((w * 1.35, .46, .12), (0, 0, .06), 'MAT_STONE_LIGHT'), 'ParkingTotemFoot')
    add(scene, box((w, .28, h * .68), (0, 0, h * .34 + .10), mat), 'AccessibleParkingPost')
    add(scene, box((w * 1.22, .08, .58 + .04 * v), (0, -.02, h * .77), 'MAT_SIGNAGE'), 'ParkingSignPanel')
    add(scene, sphere(.12, (0, -.08, h * .78), 'MAT_GLASS_CLEAR', 2), 'AccessSymbolDisc')
    add(scene, cyl(.08, .045, (0, -.09, h * .78), 'MAT_BRASS_POLISHED', 18), 'AccessSymbolCenter')
    add(scene, box((w * .82, .05, .18), (0, -.17, .44), 'MAT_ELECTRONICS'), 'ParkingAvailabilitySensor')
    add(scene, box((w * 1.08, .34, .06), (0, 0, .03), 'MAT_BLACKENED_STEEL'), 'TotemAnchorPlate')
    return scene


def _pedestrian_bollard(v, mat):
    scene = trimesh.Scene(); h, r = .86 + .08 * v, .12 + .01 * v
    add(scene, cyl(r * 1.62, .10, (0, 0, .05), 'MAT_STONE_LIGHT', 20), 'BollardFoot')
    add(scene, cyl(r, h, (0, 0, h / 2 + .10), mat, 20), 'PedestrianBollardBody')
    add(scene, cyl(r * 1.08, .07, (0, 0, h + .135), 'MAT_BRASS_POLISHED', 20), 'BollardCap')
    for i in range(3 + v):
        z = .30 + i * .14
        add(scene, box((r * 1.48, .035, .055), (0, -r * .82, z), 'MAT_SIGNAGE'), f'ReflectiveBand_{i}')
    add(scene, sphere(.055, (0, 0, h + .19), 'MAT_EMISSIVE_WARM', 1), 'BollardMarkerLamp')
    return scene


def _bicycle_shelter(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.90 + .14 * v, 1.20 + .10 * v, 1.92 + .10 * v
    for side, x in enumerate((-.44 * w, .44 * w)):
        add(scene, box((.07, .08, h), (x, 0, h / 2), mat), f'ShelterPost_{side}')
    roof = box((w, d, .10), (0, 0, h), 'MAT_STONE_LIGHT')
    _rot(roof, -.04, [0, 1, 0], (0, 0, h)); add(scene, roof, 'CycleShelterRoof')
    add(scene, box((w * .94, .06, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'ShelterBaseRail')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * w * .80 / (2 + v)
        add(scene, box((.05, .05, h * .72), (x, 0, h * .36), 'MAT_STAINLESS'), f'BicycleLockPost_{i}')
        loop = cyl(.18 + .01 * v, .035, (x, -.10, .52), 'MAT_BRASS_POLISHED', 16)
        _rot(loop, math.pi / 2, [1, 0, 0], (x, -.10, .52)); add(scene, loop, f'BicycleLockLoop_{i}')
    add(scene, box((w * .62, .045, .10), (0, -.20, .22), 'MAT_SIGNAGE'), 'CycleShelterLabel')
    return scene


def _exterior_parcel_drop(v, mat):
    scene = trimesh.Scene(); w, h = .76 + .06 * v, 1.34 + .10 * v
    add(scene, box((w * 1.08, .60, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'ParcelDropFoot')
    add(scene, box((w, .48, h), (0, 0, h / 2 + .10), mat), 'WeatherproofParcelBox')
    add(scene, box((w * .72, .04, .40), (0, -.25, h * .72), 'MAT_STAINLESS'), 'ParcelSlotDoor')
    add(scene, box((w * .42, .035, .12), (0, -.28, h * .72), 'MAT_BLACKENED_STEEL'), 'DepositSlot')
    add(scene, box((.15, .045, .32), (w * .55, -.26, h * .76), 'MAT_ELECTRONICS'), 'ParcelAccessKeypad')
    add(scene, cyl(.035, .04, (w * .32, -.28, h * .64), 'MAT_BRASS_POLISHED', 10), 'ParcelDoorPull')
    for i in range(4 + v):
        z = .32 + i * .06
        add(scene, box((w * .62, .025, .025), (0, -.25, z), 'MAT_BLACKENED_STEEL'), f'ParcelDrainSlot_{i}')
    return scene


def _valet_key_kiosk(v, mat):
    scene = trimesh.Scene(); w, h = .72 + .06 * v, 1.38 + .10 * v
    add(scene, box((w * 1.26, .56, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'ValetKioskFoot')
    add(scene, box((w, .42, h), (0, 0, h / 2 + .10), mat), 'ValetKeyCabinet')
    add(scene, box((w * .72, .04, h * .62), (0, -.23, h * .54), 'MAT_STAINLESS'), 'KeyDrawerBank')
    for i in range(3 + v):
        x = (i - (2 + v) / 2) * w * .60 / (2 + v)
        add(scene, box((w * .12, .04, .28), (x, -.26, h * .50), 'MAT_WOOD_WARM'), f'KeyDrawer_{i}')
        add(scene, cyl(.022, .035, (x, -.29, h * .50), 'MAT_BRASS_POLISHED', 10), f'DrawerPull_{i}')
    add(scene, box((.16, .05, .34), (w * .55, -.25, h * .72), 'MAT_ELECTRONICS'), 'KeyIssueTerminal')
    add(scene, box((w * .66, .04, .08), (0, -.25, .22), 'MAT_SIGNAGE'), 'KioskIdentityPanel')
    return scene


def _cable_guard(v, mat):
    scene = trimesh.Scene(); w, d = .72 + .06 * v, .42 + .04 * v
    add(scene, box((w, d, .12), (0, 0, .06), mat), 'ChargingCableBridge')
    add(scene, box((w * .92, d * .72, .11), (0, 0, .175), 'MAT_BLACKENED_STEEL'), 'CableChannelCover')
    for side, y in enumerate((-.38 * d, .38 * d)):
        add(scene, box((w * 1.02, .045, .07), (0, y, .035), 'MAT_SIGNAGE'), f'HighVisibilityEdge_{side}')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .86 / (4 + v)
        add(scene, box((.025, d * .62, .018), (x, 0, .236), 'MAT_STAINLESS'), f'AntiSlipRidge_{i}')
    add(scene, box((.22, .035, .08), (0, -d * .52, .12), 'MAT_SIGNAGE'), 'CableRouteMarking')
    return scene


def _vehicle_clearance_portal(v, mat):
    scene = trimesh.Scene(); w, h = 2.32 + .14 * v, 2.48 + .12 * v
    for side, x in enumerate((-w / 2, w / 2)):
        add(scene, box((.11, .16, h), (x, 0, h / 2), mat), f'ClearancePost_{side}')
        add(scene, box((.46, .40, .10), (x, 0, .05), 'MAT_STONE_LIGHT'), f'PostFoot_{side}')
    add(scene, box((w + .16, .18, .16), (0, 0, h - .08), 'MAT_STAINLESS'), 'ClearanceHeader')
    add(scene, box((w * .70, .04, .46 + .03 * v), (0, -.11, h - .42), 'MAT_SIGNAGE'), 'VehicleHeightSign')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .86 / (4 + v)
        add(scene, cyl(.025, .12, (x, -.13, h - .52), 'MAT_BRASS_POLISHED', 10), f'HeightIndicator_{i}')
    for side, x in enumerate((-.38 * w, .38 * w)):
        add(scene, box((.16, .20, .32), (x, 0, .21), 'MAT_PLASTIC_RUBBER'), f'BumperGuard_{side}')
    return scene


def _courtyard_drinking_fountain(v, mat):
    scene = trimesh.Scene(); h, r = .92 + .08 * v, .20 + .015 * v
    add(scene, cyl(r * 1.55, .12, (0, 0, .06), 'MAT_STONE_LIGHT', 20), 'DrinkingFountainFoot')
    add(scene, cyl(r, h * .54, (0, 0, h * .27 + .12), mat, 18), 'FountainPedestal')
    add(scene, box((r * 1.92, r * 1.66, .14), (0, 0, h * .64), 'MAT_STAINLESS'), 'FountainBasin')
    add(scene, cyl(.035, .22, (0, 0, h * .64 + .15), 'MAT_STAINLESS', 12), 'BubblerStem')
    add(scene, cyl(.07, .055, (0, 0, h * .64 + .28), 'MAT_BRASS_POLISHED', 14), 'BubblerHead')
    add(scene, box((.12, .05, .10), (r * .90, -.02, h * .72), 'MAT_BLACKENED_STEEL'), 'BottleFillerSpout')
    add(scene, box((.24, .04, .18), (r * 1.55, -.06, h * .48), 'MAT_SIGNAGE'), 'FountainUseLabel')
    return scene


def _grounds_infrastructure(family, v, mat):
    return (_parking_barrier, _ev_charger, _accessible_parking_totem, _pedestrian_bollard,
            _bicycle_shelter, _exterior_parcel_drop, _valet_key_kiosk, _cable_guard,
            _vehicle_clearance_portal, _courtyard_drinking_fountain)[family](v, mat)


# Batch 49: spa recovery, sensory wellness and private treatment amenities.
def _hot_stone_warmer(v, mat):
    scene = trimesh.Scene(); w, d, h = .72 + .06 * v, .54 + .045 * v, .74 + .06 * v
    add(scene, box((w, d, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'WarmerFoot')
    add(scene, box((w * .86, d * .82, h * .60), (0, 0, h * .37), mat), 'HotStoneCabinet')
    add(scene, box((w * .92, d * .90, .10), (0, 0, h * .72), 'MAT_STAINLESS'), 'StoneWarmerTray')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .52 / (1 + v // 2)
        add(scene, sphere(.11 + .01 * v, (x, 0, h * .82), 'MAT_BLACKENED_STEEL', 2), f'TherapyStone_{i}')
    add(scene, box((.14, .045, .26), (w * .52, -d * .48, h * .54), 'MAT_ELECTRONICS'), 'WarmerControlPanel')
    add(scene, box((w * .60, .04, .07), (0, -d * .50, .16), 'MAT_SIGNAGE'), 'StoneWarmerLabel')
    return scene


def _aromatherapy_cabinet(v, mat):
    scene = trimesh.Scene(); w, h = .76 + .06 * v, 1.22 + .08 * v
    add(scene, box((w, .46, h), (0, 0, h / 2), mat), 'AromaCabinetBody')
    add(scene, box((w * .72, .04, h * .58), (0, -.24, h * .48), 'MAT_GLASS_CLEAR'), 'AromaDisplayDoor')
    for i in range(3 + v // 2):
        z = .28 + i * .22
        add(scene, box((w * .60, .28, .035), (0, 0, z), 'MAT_WOOD_WARM'), f'EssentialOilShelf_{i}')
        for j in range(2 + v % 2):
            x = (j - .5) * .20
            add(scene, cyl(.045, .12, (x, -.06, z + .10), 'MAT_GLASS_CLEAR', 12), f'OilBottle_{i}_{j}')
    add(scene, box((.12, .04, .28), (w * .55, -.26, h * .66), 'MAT_ELECTRONICS'), 'AromaTimer')
    add(scene, box((w * .64, .035, .075), (0, -.25, .12), 'MAT_SIGNAGE'), 'AromaRecipeLabel')
    return scene


def _treatment_oil_warmer(v, mat):
    scene = trimesh.Scene(); w, h = .62 + .05 * v, .86 + .06 * v
    add(scene, box((w, .44, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'OilWarmerBase')
    add(scene, box((w * .84, .32, h * .64), (0, 0, h * .34), mat), 'OilWarmerHousing')
    for i in range(2 + v // 2):
        x = (i - (1 + v // 2) / 2) * w * .54 / (1 + v // 2)
        add(scene, cyl(.085 + .006 * v, .24, (x, 0, h * .74), 'MAT_CERAMIC_FIXTURE', 16), f'OilBottleCup_{i}')
        add(scene, cyl(.09 + .006 * v, .045, (x, 0, h * .88), 'MAT_BRASS_POLISHED', 16), f'CupRim_{i}')
    add(scene, box((.14, .04, .26), (w * .54, -.18, h * .46), 'MAT_ELECTRONICS'), 'OilTemperatureControl')
    add(scene, box((w * .68, .04, .075), (0, -.23, .16), 'MAT_SIGNAGE'), 'OilWarmerLabel')
    return scene


def _hydrotherapy_basin(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.12 + .10 * v, .80 + .07 * v, .62 + .04 * v
    add(scene, box((w * 1.06, d * 1.04, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'HydroBasinFoot')
    add(scene, box((w, d, h), (0, 0, h / 2 + .10), mat), 'HydrotherapyShell')
    add(scene, box((w * .80, d * .72, .16), (0, 0, h + .02), 'MAT_CERAMIC_FIXTURE'), 'SoakingBasin')
    for i in range(4 + v):
        angle = 2 * math.pi * i / (4 + v)
        x, y = w * .34 * math.cos(angle), d * .30 * math.sin(angle)
        jet = cyl(.045, .04, (x, y, h + .11), 'MAT_STAINLESS', 12)
        _rot(jet, math.pi / 2, [1, 0, 0], (x, y, h + .11)); add(scene, jet, f'HydrotherapyJet_{i}')
    add(scene, box((.18, .06, .30), (w * .54, -d * .54, .48), 'MAT_ELECTRONICS'), 'HydrotherapyControl')
    add(scene, cyl(.04, .12, (0, -d * .46, h + .19), 'MAT_BRASS_POLISHED', 12), 'BasinWaterSpout')
    return scene


def _infrared_sauna(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.14 + .10 * v, .92 + .08 * v, 1.92 + .12 * v
    add(scene, box((w, d, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'SaunaFloor')
    add(scene, box((w, d, h), (0, 0, h / 2 + .04), mat), 'InfraredSaunaCabin')
    add(scene, box((w * .80, .045, h * .78), (0, -d * .51, h * .48), 'MAT_GLASS_CLEAR'), 'SaunaDoorGlass')
    add(scene, box((w * .74, d * .36, .10), (0, d * .20, .48), 'MAT_WOOD_WARM'), 'SaunaBench')
    for side, x in enumerate((-.37 * w, .37 * w)):
        add(scene, box((.045, .055, h * .70), (x, d * .32, h * .39), 'MAT_BLACKENED_STEEL'), f'InfraredEmitter_{side}')
    add(scene, box((.14, .045, .36), (w * .55, -.53 * d, h * .68), 'MAT_ELECTRONICS'), 'SaunaControlPanel')
    for i in range(4 + v):
        add(scene, box((w * .72, .025, .02), (0, -.52 * d, .24 + i * .075), 'MAT_STAINLESS'), f'AirIntake_{i}')
    return scene


def _float_pod(v, mat):
    scene = trimesh.Scene(); w, d, h = 1.42 + .12 * v, 1.08 + .08 * v, .98 + .06 * v
    add(scene, box((w * 1.06, d * 1.04, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'FloatPodFoot')
    add(scene, box((w, d, h * .68), (0, 0, h * .36 + .10), mat), 'FloatPodBase')
    dome = box((w * .96, d * .92, h * .54), (0, 0, h * .88), 'MAT_GLASS_CLEAR')
    _rot(dome, .12, [1, 0, 0], (0, 0, h * .88)); add(scene, dome, 'FloatPodCanopy')
    add(scene, box((w * .72, d * .56, .13), (0, 0, h * .49), 'MAT_CERAMIC_FIXTURE'), 'FloatPodBath')
    add(scene, box((w * .78, .06, .055), (0, -d * .50, h * .38), 'MAT_BRASS_POLISHED'), 'CanopyHingeRail')
    add(scene, box((.18, .045, .28), (w * .53, -.52 * d, h * .56), 'MAT_ELECTRONICS'), 'FloatPodControl')
    add(scene, box((w * .54, .04, .065), (0, -.52 * d, .16), 'MAT_SIGNAGE'), 'FloatPodUseLabel')
    return scene


def _salt_therapy_panel(v, mat):
    scene = trimesh.Scene(); w, h = 1.12 + .10 * v, 1.58 + .10 * v
    add(scene, box((w, .16, h), (0, 0, h / 2), mat), 'SaltWallFrame')
    add(scene, box((w * .84, .06, h * .78), (0, -.12, h * .52), 'MAT_STONE_LIGHT'), 'SaltBlockField')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .76 / (4 + v)
        for row in range(3 + v // 2):
            z = .30 + row * .31
            add(scene, box((.14 + .01 * v, .035, .25), (x, -.17, z), 'MAT_STONE_LIGHT'), f'SaltBrick_{row}_{i}')
    add(scene, box((w * .66, .04, .08), (0, -.14, .14), 'MAT_BRASS_POLISHED'), 'SaltPanelBaseRail')
    add(scene, box((w * 1.06, .22, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'SaltPanelFoot')
    return scene


def _wellness_tea_stand(v, mat):
    scene = trimesh.Scene(); w, h = .82 + .06 * v, .98 + .07 * v
    add(scene, box((w * 1.10, .58, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'TeaStandFoot')
    add(scene, box((w, .46, h * .66), (0, 0, h * .34), mat), 'WellnessTeaCabinet')
    add(scene, box((w * 1.08, .54, .08), (0, 0, h * .68), 'MAT_WOOD_WARM'), 'TeaServingTop')
    add(scene, cyl(.17 + .01 * v, .34, (-w * .24, 0, h * .90), 'MAT_CERAMIC_FIXTURE', 18), 'TeaInfuserPot')
    add(scene, cyl(.18 + .01 * v, .045, (-w * .24, 0, h * 1.10), 'MAT_BRASS_POLISHED', 18), 'TeapotLid')
    for i in range(3 + v // 2):
        x = (i - (2 + v // 2) / 2) * w * .60 / (2 + v // 2)
        add(scene, cyl(.055, .075, (x, -.18, h * .75), 'MAT_CERAMIC_FIXTURE', 14), f'TeaCup_{i}')
    add(scene, box((w * .74, .04, .12), (0, -.25, .22), 'MAT_SIGNAGE'), 'TeaMenuPlaque')
    return scene


def _meditation_cushion_rack(v, mat):
    scene = trimesh.Scene(); w, d, h = .98 + .08 * v, .50 + .04 * v, 1.34 + .10 * v
    for side, x in enumerate((-.43 * w, .43 * w)):
        add(scene, box((.05, .06, h), (x, 0, h / 2), mat), f'CushionRackPost_{side}')
    for i in range(3 + v // 2):
        z = .24 + i * (h - .34) / (2 + v // 2)
        add(scene, box((w, d, .055), (0, 0, z), 'MAT_WOOD_WARM'), f'CushionShelf_{i}')
        for j in range(2 + v // 2):
            x = (j - (1 + v // 2) / 2) * w * .60 / (1 + v // 2)
            add(scene, cyl(.16 + .008 * v, .20, (x, 0, z + .13), 'MAT_UPHOLSTERY', 18), f'MeditationZafu_{i}_{j}')
    add(scene, box((w * 1.06, d * 1.10, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'CushionRackBase')
    add(scene, box((w * .62, .04, .09), (0, -d * .56, .18), 'MAT_SIGNAGE'), 'MeditationPracticeLabel')
    return scene


def _spa_foot_bath(v, mat):
    scene = trimesh.Scene(); w, d, h = .78 + .06 * v, .64 + .05 * v, .42 + .035 * v
    add(scene, box((w * 1.06, d * 1.04, .09), (0, 0, .045), 'MAT_STONE_LIGHT'), 'FootBathFoot')
    add(scene, box((w, d, h), (0, 0, h / 2 + .10), mat), 'FootBathCabinet')
    add(scene, box((w * .78, d * .70, .13), (0, 0, h + .015), 'MAT_CERAMIC_FIXTURE'), 'SoakingWell')
    for i in range(4 + v):
        angle = 2 * math.pi * i / (4 + v)
        x, y = w * .26 * math.cos(angle), d * .23 * math.sin(angle)
        add(scene, cyl(.028, .025, (x, y, h + .09), 'MAT_BRASS_POLISHED', 10), f'FootBathJet_{i}')
    add(scene, box((.14, .05, .26), (w * .54, -d * .53, .44), 'MAT_ELECTRONICS'), 'FootBathControls')
    add(scene, box((w * .62, .035, .08), (0, -d * .53, .18), 'MAT_SIGNAGE'), 'FootBathLabel')
    return scene


def _spa_wellness(family, v, mat):
    return (_hot_stone_warmer, _aromatherapy_cabinet, _treatment_oil_warmer, _hydrotherapy_basin,
            _infrared_sauna, _float_pod, _salt_therapy_panel, _wellness_tea_stand,
            _meditation_cushion_rack, _spa_foot_bath)[family](v, mat)


# Batch 50: finish-system modules for hotel rooms, public areas and vertical circulation.
def _terrazzo_floor_module(v, mat):
    scene = trimesh.Scene(); w, d = 1.10 + .10 * v, 1.10 + .10 * v
    add(scene, box((w, d, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'TerrazzoSubstrate')
    add(scene, box((w * .96, d * .96, .045), (0, 0, .1225), mat), 'TerrazzoSurface')
    for i in range(4 + v):
        for j in range(4 + v):
            x = (i - (3 + v) / 2) * w * .78 / (3 + v)
            y = (j - (3 + v) / 2) * d * .78 / (3 + v)
            add(scene, sphere(.025 + .002 * v, (x, y, .149), 'MAT_BRASS_POLISHED' if (i + j) % 3 == 0 else 'MAT_BLACKENED_STEEL', 0), f'TerrazzoChip_{i}_{j}')
    for side, x in enumerate((-.48 * w, .48 * w)):
        add(scene, box((.025, d * .90, .018), (x, 0, .151), 'MAT_BRASS_POLISHED'), f'TerrazzoEdgeInlay_{side}')
    return scene


def _mosaic_border_inlay(v, mat):
    scene = trimesh.Scene(); w, d = 1.22 + .10 * v, .70 + .06 * v
    add(scene, box((w, d, .09), (0, 0, .045), 'MAT_STONE_LIGHT'), 'MosaicBorderBase')
    add(scene, box((w * .92, d * .84, .04), (0, 0, .11), mat), 'MosaicField')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .76 / (4 + v)
        add(scene, box((.07 + .004 * v, d * .10, .028), (x, -d * .38, .144), 'MAT_BRASS_POLISHED'), f'BorderTileFront_{i}')
        add(scene, box((.07 + .004 * v, d * .10, .028), (x, d * .38, .144), 'MAT_BRASS_POLISHED'), f'BorderTileBack_{i}')
    for side, y in enumerate((-.46 * d, .46 * d)):
        add(scene, box((w * .96, .025, .035), (0, y, .13), 'MAT_BLACKENED_STEEL'), f'BorderDivider_{side}')
    add(scene, box((w * .42, .035, .06), (0, 0, .15), 'MAT_SIGNAGE'), 'MosaicInstallationTag')
    return scene


def _elevator_ceiling_cassette(v, mat):
    scene = trimesh.Scene(); w, d = 1.12 + .08 * v, 1.18 + .08 * v
    add(scene, box((w, d, .10), (0, 0, .05), 'MAT_BLACKENED_STEEL'), 'CeilingCassetteFrame')
    add(scene, box((w * .90, d * .90, .08), (0, 0, .14), mat), 'ElevatorCeilingPanel')
    for i, x in enumerate((-.30, .30)):
        add(scene, box((.10, d * .66, .035), (x, 0, .20), 'MAT_BRASS_POLISHED'), f'CeilingLightTrim_{i}')
        add(scene, box((.065, d * .60, .022), (x, 0, .226), 'MAT_EMISSIVE_WARM'), f'CeilingLightDiffuser_{i}')
    for side, x in enumerate((-.45 * w, .45 * w)):
        add(scene, box((.045, d * .86, .06), (x, 0, .18), 'MAT_STAINLESS'), f'CeilingRetentionRail_{side}')
    add(scene, box((.22, .14, .025), (0, d * .34, .203), 'MAT_SIGNAGE'), 'ServiceAccessLabel')
    return scene


def _acoustic_ceiling_grid(v, mat):
    scene = trimesh.Scene(); w, d = 1.44 + .12 * v, 1.10 + .10 * v
    add(scene, box((w, d, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'AcousticGridFrame')
    add(scene, box((w * .94, d * .94, .09), (0, 0, .125), mat), 'AcousticCeilingTile')
    for row in range(3 + v // 2):
        y = (row - (2 + v // 2) / 2) * d * .76 / (2 + v // 2)
        add(scene, box((w * .92, .035, .06 + .004 * v), (0, y, .20), 'MAT_LINEN'), f'CeilingAbsorber_{row}')
    for col in range(2 + v // 2):
        x = (col - (1 + v // 2) / 2) * w * .78 / (1 + v // 2)
        add(scene, box((.025, d * .86, .035), (x, 0, .22), 'MAT_BRASS_POLISHED'), f'GridDivider_{col}')
    for i in range(4):
        x, y = ((-1) ** i * w * .42, ((i // 2) * 2 - 1) * d * .40)
        add(scene, cyl(.025, .14 + .01 * v, (x, y, .24), 'MAT_STAINLESS', 10), f'CeilingSuspensionPoint_{i}')
    return scene


def _stone_sill_return(v, mat):
    scene = trimesh.Scene(); w, d = 1.18 + .10 * v, .34 + .03 * v
    add(scene, box((w, d, .16), (0, 0, .08), mat), 'StoneWindowSill')
    add(scene, box((w * .92, d * .82, .04), (0, 0, .18), 'MAT_STONE_LIGHT'), 'SillWearSurface')
    for side, x in enumerate((-.44 * w, .44 * w)):
        add(scene, box((.14, d * .90, .26 + .02 * v), (x, 0, .22), 'MAT_STONE_LIGHT'), f'SillEndReturn_{side}')
    for i in range(4 + v):
        x = (i - (3 + v) / 2) * w * .84 / (3 + v)
        add(scene, box((.035, d * .64, .018), (x, 0, .204), 'MAT_BLACKENED_STEEL'), f'SillDrainGroove_{i}')
    add(scene, box((w * .68, .035, .07), (0, -d * .52, .12), 'MAT_SIGNAGE'), 'SillModuleIdentifier')
    return scene


def _expansion_joint(v, mat):
    scene = trimesh.Scene(); w, d = 1.24 + .12 * v, .70 + .06 * v
    add(scene, box((w, d, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'FloorJointSubstrate')
    add(scene, box((w * .94, d * .90, .045), (0, 0, .1225), mat), 'FloorPanelLeft')
    add(scene, box((w * .94, d * .90, .045), (0, 0, .1225), 'MAT_WOOD_WARM'), 'FloorPanelRight')
    add(scene, box((w * .92, .11 + .008 * v, .035), (0, 0, .1625), 'MAT_STAINLESS'), 'ExpansionJointCover')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .82 / (4 + v)
        add(scene, cyl(.022, .028, (x, 0, .185), 'MAT_BRASS_POLISHED', 8), f'JointAnchor_{i}')
    for side, y in enumerate((-.44 * d, .44 * d)):
        add(scene, box((w * .96, .025, .03), (0, y, .16), 'MAT_BLACKENED_STEEL'), f'JointEdgeSeal_{side}')
    return scene


def _timber_slat_ceiling(v, mat):
    scene = trimesh.Scene(); w, d = 1.32 + .12 * v, .94 + .08 * v
    add(scene, box((w, d, .08), (0, 0, .04), 'MAT_BLACKENED_STEEL'), 'TimberCeilingCarrier')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .90 / (4 + v)
        add(scene, box((.07 + .004 * v, d * .88, .10), (x, 0, .13), mat), f'CeilingTimberSlat_{i}')
    for side, y in enumerate((-.40 * d, .40 * d)):
        add(scene, box((w * .96, .045, .12), (0, y, .12), 'MAT_BRASS_POLISHED'), f'PerimeterTrim_{side}')
    for i in range(3 + v // 2):
        x = (i - (2 + v // 2) / 2) * w * .70 / (2 + v // 2)
        add(scene, box((.035, d * .74, .03), (x, 0, .20), 'MAT_STONE_LIGHT'), f'CeilingLightSlot_{i}')
    return scene


def _feature_mosaic_wall(v, mat):
    scene = trimesh.Scene(); w, h = 1.24 + .10 * v, 1.52 + .10 * v
    add(scene, box((w * 1.06, .16, h), (0, 0, h / 2), 'MAT_STONE_LIGHT'), 'MosaicWallBacking')
    add(scene, box((w * .88, .045, h * .82), (0, -.11, h * .50), mat), 'FeatureMosaicField')
    for row in range(4 + v // 2):
        z = .26 + row * h * .72 / (3 + v // 2)
        for col in range(4 + v):
            x = (col - (3 + v) / 2) * w * .76 / (3 + v)
            size = .13 + .005 * ((row + col + v) % 3)
            add(scene, box((size, .026, size), (x, -.15, z), 'MAT_BRASS_POLISHED' if (row + col) % 5 == 0 else 'MAT_GLASS_CLEAR'), f'MosaicTile_{row}_{col}')
    add(scene, box((w * 1.10, .28, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'MosaicWallBase')
    return scene


def _cove_base_trim(v, mat):
    scene = trimesh.Scene(); w, d = 1.36 + .12 * v, .54 + .05 * v
    add(scene, box((w, d, .10), (0, 0, .05), 'MAT_STONE_LIGHT'), 'CoveTrimFloorSample')
    add(scene, box((w * .92, .12 + .008 * v, .44 + .03 * v), (0, d * .32, .22 + .015 * v), mat), 'CoveBaseProfile')
    add(scene, box((w * .92, .045, .08), (0, d * .32, .48 + .03 * v), 'MAT_BRASS_POLISHED'), 'TopCoveCap')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .86 / (4 + v)
        add(scene, box((.028, .035, .36 + .025 * v), (x, d * .39, .25 + .0125 * v), 'MAT_BLACKENED_STEEL'), f'CoveVerticalReveal_{i}')
    add(scene, box((w * .48, .035, .07), (0, -d * .52, .14), 'MAT_SIGNAGE'), 'TrimProfileCode')
    return scene


def _carpeted_stair_nosing(v, mat):
    scene = trimesh.Scene(); w, d = 1.12 + .10 * v, .48 + .04 * v
    add(scene, box((w, d, .16), (0, 0, .08), 'MAT_STONE_LIGHT'), 'StairNosingSubstrate')
    add(scene, box((w * .94, d * .62, .055), (0, d * .16, .1875), mat), 'CarpetTread')
    add(scene, box((w * .96, .10, .07), (0, -d * .40, .195), 'MAT_BLACKENED_STEEL'), 'NosingEdge')
    add(scene, box((w * .88, .07, .028), (0, -d * .40, .245), 'MAT_BRASS_POLISHED'), 'NosingContrastStrip')
    for i in range(5 + v):
        x = (i - (4 + v) / 2) * w * .82 / (4 + v)
        add(scene, box((.028, d * .48, .014), (x, d * .16, .218), 'MAT_STONE_LIGHT'), f'TreadGripRib_{i}')
    for side, x in enumerate((-.46 * w, .46 * w)):
        add(scene, box((.035, d * .88, .10), (x, 0, .13), 'MAT_STAINLESS'), f'NosingEndClip_{side}')
    return scene


def _finish_modules(family, v, mat):
    return (_terrazzo_floor_module, _mosaic_border_inlay, _elevator_ceiling_cassette,
            _acoustic_ceiling_grid, _stone_sill_return, _expansion_joint,
            _timber_slat_ceiling, _feature_mosaic_wall, _cove_base_trim,
            _carpeted_stair_nosing)[family](v, mat)


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 2251 <= number <= 2500:
        raise ValueError(f'experience expansion asset ID outside A2251–A2500: {asset_id}')
    batch_index = (number - 2251) // 50
    family, variant = divmod((number - 2251) % 50, 5)
    builders = (_accessible_guest, _heritage_displays, _grounds_infrastructure,
                _spa_wellness, _finish_modules)
    return builders[batch_index](family, variant, mat)
