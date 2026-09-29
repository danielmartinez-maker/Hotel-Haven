"""Purpose-built structural recipes for room, dining, service and garden collections.

Five product classes per recipe vary capacity, proportions and actual functional
components. Product keys are fixed by the canonical A851–A1050 catalog.
"""

import trimesh

from v2_asset_common import add, box, cyl, sphere


# (product name, geometry recipe, detail material)
COLLECTIONS = (
    (
        ('Upholstered Bed', 'bed', 'MAT_LINEN'),
        ('Bedside Cabinet', 'cabinet', 'MAT_BRASS_POLISHED'),
        ('Guest Wardrobe', 'wardrobe', 'MAT_WOOD_WARM'),
        ('Guest Lounge Sofa', 'sofa', 'MAT_UPHOLSTERY'),
        ('Guest Work Desk', 'desk', 'MAT_STAINLESS'),
        ('Minibar Console', 'cabinet', 'MAT_ELECTRONICS'),
        ('Luggage Bench', 'bench', 'MAT_LINEN'),
        ('Vanity Station', 'vanity', 'MAT_GLASS_CLEAR'),
        ('Bathroom Storage', 'wardrobe', 'MAT_CERAMIC_FIXTURE'),
        ('Reading Floor Lamp', 'lamp', 'MAT_EMISSIVE_WARM'),
    ),
    (
        ('Restaurant Dining Table', 'table', 'MAT_WOOD_WARM'),
        ('Dining Banquette', 'sofa', 'MAT_UPHOLSTERY'),
        ('Bar Service Counter', 'counter', 'MAT_BRASS_POLISHED'),
        ('Buffet Island', 'buffet', 'MAT_STAINLESS'),
        ('Kitchen Prep Station', 'counter', 'MAT_STAINLESS'),
        ('Coffee Service Station', 'coffee', 'MAT_ELECTRONICS'),
        ('Pastry Display Case', 'display', 'MAT_GLASS_CLEAR'),
        ('Wine Bottle Rack', 'rack', 'MAT_WOOD_WARM'),
        ('Host Reception Podium', 'podium', 'MAT_SIGNAGE'),
        ('Dish Return Station', 'buffet', 'MAT_CERAMIC_FIXTURE'),
    ),
    (
        ('Linen Storage Shelf', 'rack', 'MAT_LINEN'),
        ('Cleaning Cabinet', 'wardrobe', 'MAT_STAINLESS'),
        ('Repair Workbench', 'desk', 'MAT_BLACKENED_STEEL'),
        ('Laundry Folding Station', 'table', 'MAT_LINEN'),
        ('Chemical Supply Locker', 'cabinet', 'MAT_SIGNAGE'),
        ('Engineer Tool Rack', 'rack', 'MAT_BLACKENED_STEEL'),
        ('Waste Sorting Station', 'bins', 'MAT_SIGNAGE'),
        ('Receiving Parcel Cage', 'cage', 'MAT_BLACKENED_STEEL'),
        ('Housekeeping Supply Island', 'buffet', 'MAT_LINEN'),
        ('Staff Check In Kiosk', 'podium', 'MAT_ELECTRONICS'),
    ),
    (
        ('Courtyard Planter', 'planter', 'MAT_VEGETATION'),
        ('Garden Seat', 'bench', 'MAT_WOOD_WARM'),
        ('Pergola Bay', 'pergola', 'MAT_WOOD_WARM'),
        ('Poolside Side Table', 'table', 'MAT_STONE_LIGHT'),
        ('Exterior Wayfinding Post', 'post', 'MAT_SIGNAGE'),
        ('Bicycle Parking Stand', 'bike', 'MAT_STAINLESS'),
        ('Garden Boundary Wall', 'wall', 'MAT_STONE_LIGHT'),
        ('Hedge Planter Row', 'hedge', 'MAT_VEGETATION'),
        ('Exterior Lamp Post', 'lamp', 'MAT_EMISSIVE_WARM'),
        ('Entry Canopy Bay', 'canopy', 'MAT_GLASS_CLEAR'),
    ),
)


def _four_legs(scene, width, depth, height):
    for index, (x, y) in enumerate(((-1, -1), (-1, 1), (1, -1), (1, 1))):
        add(scene, box((.055, .055, height),
                       (x * width * .42, y * depth * .38, height / 2),
                       'MAT_BLACKENED_STEEL'), f'Leg_{index}')


def build_asset(name, subcategory, mat, asset_id, profile):
    number = int(asset_id.removeprefix('HH_A'))
    if not 851 <= number <= 1050:
        raise ValueError(f'curated hotel asset ID outside A851–A1050: {asset_id}')
    collection, offset = divmod(number - 851, 50)
    product, variant = divmod(offset, 5)
    _, recipe, detail_mat = COLLECTIONS[collection][product]
    width = .86 + variant * .18 + (product % 3) * .08
    depth = .48 + variant * .065
    scene = trimesh.Scene()

    if recipe in ('bed', 'sofa', 'bench'):
        seat_h = .44 + variant * .018
        add(scene, box((width + (.35 if recipe == 'bed' else 0), depth + (.3 if recipe == 'bed' else 0), .17),
                       (0, 0, seat_h), mat), 'PrimaryForm')
        _four_legs(scene, width, depth, seat_h - .075)
        if recipe != 'bench':
            add(scene, box((width, .12, .45 + variant * .03),
                           (0, -depth * .45, seat_h + .25), mat), 'Backrest')
        for i in range(1 + variant):
            x = width * ((i + .5) / (1 + variant) - .5) * .8
            add(scene, box((width * .75 / (1 + variant), depth * .75, .09),
                           (x, depth * .08, seat_h + .13), detail_mat), f'FunctionalCushion_{i}')
        if recipe == 'bed':
            add(scene, box((width * .85, depth * .28, .07),
                           (0, -depth * .3, seat_h + .21), 'MAT_LINEN'), 'Pillow')
    elif recipe in ('table', 'desk', 'counter', 'buffet', 'vanity', 'coffee'):
        top_h = .72 if recipe in ('table', 'desk', 'vanity') else 1.0
        add(scene, box((width, depth, .08), (0, 0, top_h), mat), 'PrimaryForm')
        _four_legs(scene, width, depth, top_h - .04)
        if recipe in ('counter', 'buffet', 'coffee'):
            add(scene, box((width * .83, depth * .75, .44),
                           (0, 0, top_h * .53), 'MAT_STAINLESS'), 'ServiceCabinet')
        if recipe == 'vanity':
            add(scene, box((width * .65, .035, .58),
                           (0, depth * .42, top_h + .36), 'MAT_GLASS_CLEAR'), 'VanityMirror')
        for i in range(1 + variant):
            x = width * ((i + .5) / (1 + variant) - .5) * .75
            station_h = .18 + variant * .035
            add(scene, box((width * .53 / (1 + variant), .12, station_h),
                           (x, depth * .08, top_h + .04 + station_h / 2), detail_mat), f'FunctionalStation_{i}')
        if recipe in ('coffee', 'buffet'):
            add(scene, cyl(.095, .28 + variant * .025,
                           (width * .33, 0, top_h + .22), 'MAT_BLACKENED_STEEL', 16), 'ServiceDispenser')
        if recipe == 'counter':
            add(scene, box((width * .8, .045, .3),
                           (0, depth * .46, top_h + .1), 'MAT_BRASS_POLISHED'), 'CounterFascia')
    elif recipe in ('cabinet', 'wardrobe', 'rack', 'display', 'cage', 'bins'):
        height = 1.05 + variant * .14
        if recipe in ('wardrobe', 'rack', 'cage'):
            height += .45
        add(scene, box((width, depth, .08), (0, 0, .04),
                       'MAT_BLACKENED_STEEL'), 'Base')
        if recipe in ('rack', 'cage'):
            for side, x in enumerate((-width * .46, width * .46)):
                add(scene, box((.06, depth, height), (x, 0, height / 2), mat), f'PrimaryForm_{side}')
        else:
            add(scene, box((width, depth, height), (0, 0, height / 2 + .08), mat), 'PrimaryForm')
        for i in range(2 + variant):
            z = .25 + i * (height - .34) / (2 + variant)
            if recipe in ('rack', 'cage'):
                add(scene, box((width * .9, depth * .86, .045),
                               (0, 0, z), detail_mat), f'FunctionalShelf_{i}')
            elif recipe == 'bins':
                x = width * ((i + .5) / (2 + variant) - .5) * .75
                add(scene, box((width * .7 / (2 + variant), .03, .19),
                               (x, depth / 2 + .02, .44), detail_mat), f'SortLabel_{i}')
            else:
                add(scene, box((width * .8, .045, .09),
                               (0, depth / 2 + .03, z), detail_mat), f'FunctionalDrawer_{i}')
                add(scene, box((width * .15, .055, .025),
                               (0, depth / 2 + .065, z), 'MAT_BRASS_POLISHED'), f'DrawerPull_{i}')
        if recipe == 'display':
            add(scene, box((width * .83, .03, height * .6),
                           (0, depth / 2 + .045, height * .6), 'MAT_GLASS_CLEAR'), 'DisplayGlass')
        if recipe in ('wardrobe', 'cabinet'):
            add(scene, box((width * .93, .06, .09),
                           (0, depth / 2 + .02, height + .045), detail_mat), 'UpperCrown')
    elif recipe in ('planter', 'hedge'):
        height = .42 + variant * .065
        add(scene, box((width, depth, height), (0, 0, height / 2), mat), 'PrimaryForm')
        add(scene, box((width + .06, depth + .06, .07), (0, 0, .035),
                       'MAT_STONE_LIGHT'), 'Base')
        for i in range(2 + variant):
            x = width * ((i + .5) / (2 + variant) - .5) * .75
            add(scene, sphere(.12 + variant * .012, (x, 0, height + .12),
                              'MAT_VEGETATION', 1), f'FunctionalPlant_{i}')
    elif recipe in ('pergola', 'canopy', 'wall'):
        height = 2.0 + variant * .12
        for side, x in enumerate((-width * .46, width * .46)):
            add(scene, box((.09, depth, height), (x, 0, height / 2), mat), f'PrimaryForm_{side}')
            add(scene, box((.16, depth + .06, .07), (x, 0, .035),
                           'MAT_STONE_LIGHT'), f'Foot_{side}')
        for i in range(2 + variant):
            x = width * ((i + .5) / (2 + variant) - .5) * .85
            add(scene, box((.08 if recipe != 'wall' else width / (3 + variant), depth, .07 if recipe != 'wall' else height * .65),
                           (x, 0, height if recipe != 'wall' else height * .45), detail_mat), f'FunctionalSpan_{i}')
    elif recipe in ('lamp', 'post', 'podium'):
        height = (1.5 if recipe != 'podium' else 1.05) + variant * .13
        add(scene, cyl(.17 + variant * .02, .07, (0, 0, .035), mat, 16), 'Base')
        add(scene, cyl(.045 + variant * .008, height - .08,
                       (0, 0, height / 2), mat, 16), 'PrimaryForm')
        add(scene, box((.36 + variant * .065, .12, .2),
                       (0, 0, height), detail_mat), 'FunctionalHead')
        if recipe == 'lamp':
            for i in range(variant + 1):
                x = (i - variant / 2) * .10
                add(scene, box((.06, .12, .06), (x, .075, height + .14),
                               'MAT_EMISSIVE_WARM'), f'LampEmitter_{i}')
        for i in range(variant + 1):
            x = (i - variant / 2) * .055
            add(scene, box((.025, .025, .04),
                           (x, -.065, height), 'MAT_BRASS_POLISHED'), f'Indicator_{i}')
    else:  # Bicycle stand: full-height loops with variable parking capacity.
        add(scene, box((width, .27, .07), (0, 0, .035), mat), 'Base')
        for i in range(2 + variant):
            x = width * ((i + .5) / (2 + variant) - .5) * .84
            add(scene, box((.055, .055, .85), (x, -.08, .425),
                           'MAT_STAINLESS'), f'PrimaryForm_{i}')
            add(scene, box((.055, .22, .055), (x, 0, .85),
                           'MAT_STAINLESS'), f'WheelLoop_{i}')
    return scene
