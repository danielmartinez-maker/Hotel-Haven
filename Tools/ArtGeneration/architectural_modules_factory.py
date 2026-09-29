"""Grounded modular architecture for the A801–A850 corridor collection."""

import trimesh

from v2_asset_common import add, box, cyl


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 801 <= number <= 850:
        raise ValueError(f'architectural module ID outside A801–A850: {asset_id}')
    family, variant = divmod(number - 801, 5)
    width = 1.2 + variant * 0.16
    height = 1.9 + variant * 0.13
    depth = 0.12 + variant * 0.018
    scene = trimesh.Scene()

    if family == 0:  # Corridor floor with an inset wear strip and edge joints.
        add(scene, box((width, width, 0.08), (0, 0, 0.04), mat), 'PrimaryStructure')
        add(scene, box((width * .62, .13 + variant * .018, .012),
                       (0, 0, .086), 'MAT_BLACKENED_STEEL'), 'WearStrip')
        for side, x in enumerate((-width * .46, width * .46)):
            add(scene, box((.018, width, .012), (x, 0, .086), 'MAT_BRASS_POLISHED'), f'Joint_{side}')
    elif family == 1:  # Wall with distinct wainscot heights and panel widths.
        add(scene, box((width, depth, height), (0, 0, height / 2), mat), 'PrimaryStructure')
        add(scene, box((width, depth + .03, .09), (0, 0, .045), 'MAT_WOOD_WARM'), 'BaseTrim')
        panel_h = .63 + variant * .105
        add(scene, box((width * (.62 + variant * .04), .026, panel_h),
                       (0, -depth / 2 - .016, panel_h / 2 + .12), 'MAT_WOOD_WARM'), 'WainscotPanel')
    elif family == 2:  # Glazed corridor partition with variable mullion count.
        add(scene, box((width, .18, .09), (0, 0, .045), 'MAT_STONE_LIGHT'), 'Base')
        add(scene, box((width, .04, height - .09), (0, 0, height / 2 + .045), 'MAT_GLASS_CLEAR'), 'PrimaryStructure')
        for i in range(variant + 2):
            x = -width / 2 + width * i / (variant + 1)
            add(scene, box((.035, .09, height), (x, 0, height / 2), mat), f'Mullion_{i}')
    elif family == 3:  # Corner protection column with distinct rounded bumpers.
        add(scene, box((.35 + variant * .06, .35 + variant * .06, height),
                       (0, 0, height / 2), mat), 'PrimaryStructure')
        add(scene, box((.5 + variant * .06, .5 + variant * .06, .08),
                       (0, 0, .04), 'MAT_STONE_LIGHT'), 'Base')
        for i in range(2 + variant):
            z = .4 + i * (height - .6) / (2 + variant)
            add(scene, box((.48 + variant * .06, .05, .045),
                           (0, -.2 - variant * .03, z), 'MAT_BRASS_POLISHED'), f'Bumper_{i}')
    elif family == 4:  # Floor threshold with a tactile strip.
        add(scene, box((width, .42 + variant * .055, .07),
                       (0, 0, .035), mat), 'PrimaryStructure')
        add(scene, box((width, .06, .015), (0, -.13, .077), 'MAT_BLACKENED_STEEL'), 'TactileStrip')
        for i in range(3 + variant):
            x = width * (i / (2 + variant) - .5) * .8
            add(scene, cyl(.018, .017, (x, -.13, .094), 'MAT_BRASS_POLISHED', 12), f'TactileStud_{i}')
    elif family == 5:  # Service riser casing with inspection panel and vents.
        add(scene, box((.75 + variant * .09, .35, height),
                       (0, 0, height / 2), mat), 'PrimaryStructure')
        add(scene, box((.85 + variant * .09, .45, .08), (0, 0, .04), 'MAT_STONE_LIGHT'), 'Base')
        add(scene, box((.56 + variant * .09, .025, .72 + variant * .08),
                       (0, -.19, 1.1), 'MAT_STAINLESS'), 'InspectionPanel')
        for i in range(2 + variant):
            add(scene, box((.38, .029, .016), (0, -.209, .55 + i * .075),
                           'MAT_BLACKENED_STEEL'), f'Vent_{i}')
    elif family == 6:  # Open archway with posts and variable lintel rise.
        span = width * .72
        for side, x in enumerate((-span / 2, span / 2)):
            add(scene, box((.15, .22, height), (x, 0, height / 2), mat), f'PrimaryStructure_{side}')
            add(scene, box((.22, .28, .07), (x, 0, .035), 'MAT_STONE_LIGHT'), f'Foot_{side}')
        add(scene, box((span + .19, .22, .18 + variant * .04),
                       (0, 0, height - .09), 'MAT_WOOD_WARM'), 'Lintel')
    elif family == 7:  # Recessed planter partition with variable compartments.
        add(scene, box((width, .4, .48 + variant * .06),
                       (0, 0, (.48 + variant * .06) / 2), mat), 'PrimaryStructure')
        add(scene, box((width + .05, .45, .07), (0, 0, .035), 'MAT_STONE_LIGHT'), 'Base')
        for i in range(2 + variant):
            x = width * ((i + .5) / (2 + variant) - .5) * .82
            add(scene, cyl(.08, .19, (x, 0, .58 + variant * .06),
                           'MAT_VEGETATION', 12), f'Planter_{i}')
    elif family == 8:  # Acoustic baffle wall with spaced fins.
        add(scene, box((width, .1, height), (0, 0, height / 2),
                       'MAT_LINEN'), 'PrimaryStructure')
        add(scene, box((width, .16, .07), (0, 0, .035), mat), 'Base')
        for i in range(3 + variant):
            x = width * ((i + .5) / (3 + variant) - .5) * .9
            add(scene, box((.07, .26 + variant * .02, height * .86),
                           (x, -.11, height * .48), 'MAT_WOOD_WARM'), f'AcousticFin_{i}')
    else:  # Guard rail with a growing number of vertical balusters.
        add(scene, box((width, .18, .07), (0, 0, .035), mat), 'Base')
        for i in range(2 + variant):
            x = width * (i / (1 + variant) - .5) * .88
            add(scene, box((.045, .055, .95 + variant * .04),
                           (x, 0, (.95 + variant * .04) / 2),
                           'MAT_BLACKENED_STEEL'), f'PrimaryStructure_{i}')
        add(scene, box((width, .09, .07),
                       (0, 0, 1.0 + variant * .04), 'MAT_WOOD_WARM'), 'Handrail')
    return scene
