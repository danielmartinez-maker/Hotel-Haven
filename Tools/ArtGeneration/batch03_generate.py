from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import trimesh

PALETTE = {
    'MAT_LINEN': ((0.88,0.86,0.80,1.0),0.0,0.88),
    'MAT_LINEN_BLACKOUT': ((0.24,0.27,0.31,1.0),0.0,0.92),
    'MAT_LINEN_SHEER': ((0.90,0.91,0.88,0.38),0.0,0.74),
    'MAT_WOOD_WARM': ((0.42,0.22,0.10,1.0),0.0,0.52),
    'MAT_WOOD_DARK': ((0.22,0.12,0.08,1.0),0.0,0.48),
    'MAT_EMISSIVE_WARM': ((0.90,0.68,0.34,1.0),0.0,0.36),
    'MAT_UPHOLSTERY': ((0.42,0.31,0.25,1.0),0.0,0.84),
    'MAT_LEATHER': ((0.34,0.18,0.10,1.0),0.0,0.48),
    'MAT_STAINLESS': ((0.58,0.61,0.62,1.0),0.85,0.42),
    'MAT_ELECTRONICS': ((0.08,0.09,0.10,1.0),0.12,0.36),
    'MAT_GLASS_CLEAR': ((0.34,0.55,0.64,0.42),0.0,0.10),
    'MAT_CARPET_STANDARD': ((0.53,0.47,0.39,1.0),0.0,0.90),
    'MAT_BRASS_POLISHED': ((0.62,0.38,0.10,1.0),0.88,0.30),
    'MAT_PLASTIC_RUBBER': ((0.18,0.18,0.18,1.0),0.0,0.64),
}


def mat(name):
    rgba, metallic, rough = PALETTE.get(name, PALETTE['MAT_WOOD_WARM'])
    base_color = np.clip(np.rint(np.asarray(rgba, dtype=float) * 255.0), 0, 255).astype(np.uint8)
    return trimesh.visual.material.PBRMaterial(
        name=name,
        baseColorFactor=base_color,
        metallicFactor=metallic,
        roughnessFactor=rough,
    )


def box(extents, center=(0,0,0), material='MAT_WOOD_WARM'):
    m = trimesh.creation.box(extents=extents)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=mat(material))
    return m


def cyl(radius, height, center=(0,0,0), material='MAT_WOOD_WARM', sections=16):
    m = trimesh.creation.cylinder(radius=radius, height=height, sections=sections)
    m.apply_translation(center)
    m.visual = trimesh.visual.TextureVisuals(material=mat(material))
    return m


def add(scene, mesh, name, *, parent=None, transform=None):
    kwargs = {}
    if parent is not None:
        kwargs['parent_node_name'] = parent
    if transform is not None:
        kwargs['transform'] = transform
    scene.add_geometry(mesh, node_name=name, geom_name=name, **kwargs)


def bed(name):
    s = trimesh.Scene(); w,l,h = 1.0,2.0,0.52
    if 'Double' in name: w = 1.4
    if 'Queen' in name: w = 1.6
    if 'King' in name or 'Four Poster' in name: w = 1.9
    if 'Twin' in name: w = 0.95
    if 'Accessible' in name: h = 0.40
    if 'Rollaway' in name: w,l,h = 0.85,1.9,0.42
    add(s, box((w,l,0.18),(0,0,h-0.20),'MAT_WOOD_WARM'), 'BedFrame')
    add(s, box((w*0.96,l*0.95,0.24),(0,0,h),'MAT_LINEN'), 'Mattress')
    add(s, box((w,0.10,0.85),(0,l/2-0.03,h+0.32),'MAT_WOOD_WARM'), 'Headboard')
    for x in (-w*0.24,w*0.24):
        add(s, box((w*0.34,0.34,0.10),(x,l*0.30,h+0.17),'MAT_LINEN'), f'Pillow_{x}')
    add(s, box((w*0.92,l*0.43,0.06),(0,-l*0.20,h+0.16),'MAT_LINEN'), 'DuvetFold')
    if 'Four Poster' in name:
        for x in (-w/2+0.05,w/2-0.05):
            for y in (-l/2+0.05,l/2-0.05):
                add(s, cyl(0.035,2.0,(x,y,1.0),'MAT_WOOD_DARK',12), f'Poster_{x}_{y}')
        add(s, box((w,0.055,0.055),(0,-l/2+0.05,1.96),'MAT_WOOD_DARK'), 'PosterRailFoot')
        add(s, box((w,0.055,0.055),(0,l/2-0.05,1.96),'MAT_WOOD_DARK'), 'PosterRailHead')
    if 'Accessible' in name:
        add(s, box((0.32,0.10,0.10),(w/2+0.12,0,0.34),'MAT_STAINLESS'), 'TransferClearanceMarker')
    if 'Rollaway' in name:
        for x in (-w*0.35,w*0.35):
            for y in (-l*0.38,l*0.38):
                add(s, cyl(0.05,0.04,(x,y,0.05),'MAT_STAINLESS',12), f'Caster_{x}_{y}')
        add(s, box((w*0.72,0.05,0.30),(0,l/2+0.02,0.48),'MAT_STAINLESS'), 'FoldHandle')
    return s


def crib():
    s = trimesh.Scene(); w,l = 0.75,1.30
    add(s, box((w,l,0.10),(0,0,0.38),'MAT_WOOD_WARM'), 'CribBase')
    for x in (-w/2,w/2):
        for y in np.linspace(-l/2,l/2,8):
            add(s, cyl(0.018,0.72,(x,y,0.72),'MAT_WOOD_WARM',10), f'Rail_{x}_{y}')
    for y in (-l/2,l/2):
        add(s, box((w,0.06,0.72),(0,y,0.72),'MAT_WOOD_WARM'), f'End_{y}')
    add(s, box((w*0.88,l*0.85,0.12),(0,0,0.48),'MAT_LINEN'), 'Mattress')
    return s


def _case_doors(s, w, d, h, material):
    door_w = w * 0.43
    for label, x, handle_x in (
        ('Left', -w*0.235, -w*0.055),
        ('Right', w*0.235, w*0.055),
    ):
        add(s, box((door_w,0.026,h*0.82),(x,-d/2-0.016,h*0.51),material), f'Door_{label}')
        add(s, box((0.026,0.035,0.24),(handle_x,-d/2-0.045,h*0.54),'MAT_BRASS_POLISHED'), f'Handle_{label}')


def case_piece(name, material):
    s = trimesh.Scene(); w,d,h = 0.55,0.45,0.58
    if 'Wardrobe' in name or 'Armoire' in name: w,d,h = (0.95 if 'Double' not in name else 1.35),0.55,1.95
    elif 'Dresser' in name: w,d,h = (0.95 if 'Three' in name else 1.45),0.50,0.82
    elif 'Minibar' in name or 'Coffee Station' in name or 'Safe Cabinet' in name: w,d,h = 0.75,0.52,0.95
    elif 'TV Console Compact' in name: w,d,h = 1.20,0.42,0.58
    elif 'TV Console Luxury' in name: w,d,h = 1.75,0.48,0.66
    elif 'Nightstand' in name or 'Charging Table' in name: w,d,h = 0.55,0.45,0.58
    if 'Floating' in name: h = 0.26
    add(s, box((w,d,h),(0,0,h/2),material), 'CabinetBody')

    if 'Open Closet' in name:
        add(s, box((w*0.92,d*0.08,h*0.05),(0,-d*0.42,h*0.86),'MAT_STAINLESS'), 'HangingRail')
        for x in (-w*0.34, 0, w*0.34):
            add(s, box((0.035,d*0.70,h*0.70),(x,0,h*0.45),'MAT_WOOD_DARK'), f'ClosetDivider_{x}')
    elif 'Wardrobe' in name or 'Armoire' in name:
        _case_doors(s,w,d,h,material)
        add(s, box((w*0.92,0.04,0.08),(0,-d/2-0.02,h*0.93),'MAT_WOOD_DARK'), 'TopRail')
        if 'Armoire' in name:
            add(s, box((w+0.16,d+0.08,0.12),(0,0,h+0.04),'MAT_WOOD_DARK'), 'CrownMolding')
            add(s, box((w+0.10,d+0.04,0.10),(0,0,0.05),'MAT_WOOD_DARK'), 'ArmoirePlinth')
    elif 'Refrigerator' in name:
        add(s, box((w*0.94,0.03,h*0.92),(0,-d/2-0.015,h*0.52),'MAT_STAINLESS'), 'FridgeDoor')
        add(s, box((0.035,0.035,h*0.46),(w*0.35,-d/2-0.045,h*0.55),'MAT_BLACKENED_STEEL' if 'MAT_BLACKENED_STEEL' in PALETTE else 'MAT_ELECTRONICS'), 'FridgeHandle')
    elif 'Safe' in name:
        add(s, box((w*0.82,0.03,h*0.72),(0,-d/2-0.016,h*0.55),'MAT_STAINLESS'), 'SafeDoor')
        add(s, box((0.16,0.025,0.14),(w*0.22,-d/2-0.035,h*0.58),'MAT_ELECTRONICS'), 'Keypad')
    elif 'Minibar' in name:
        add(s, box((w*0.90,0.03,h*0.76),(0,-d/2-0.016,h*0.48),material), 'MinibarDoor')
        add(s, box((0.035,0.035,0.28),(w*0.35,-d/2-0.045,h*0.51),'MAT_BRASS_POLISHED'), 'MinibarHandle')
        add(s, box((w*0.72,d*0.72,0.035),(0,0,h*0.78),'MAT_STAINLESS'), 'MinibarShelf')
    elif 'Coffee Station' in name:
        add(s, box((w*0.52,d*0.42,0.28),(-w*0.12,-d*0.03,h+0.14),'MAT_ELECTRONICS'), 'CoffeeMachine')
        add(s, cyl(0.07,0.18,(w*0.25,-d*0.08,h+0.09),'MAT_STAINLESS',16), 'CupStack')
        add(s, box((w*0.88,d*0.80,0.045),(0,0,h+0.02),'MAT_WOOD_DARK'), 'CoffeeWorktop')
    elif 'TV Console Luxury' in name:
        add(s, box((w*0.86,d*0.72,0.045),(0,-0.01,h*0.47),'MAT_WOOD_DARK'), 'ConsoleShelf')
        add(s, box((0.13,0.025,0.10),(0,-d/2-0.02,h*0.23),'MAT_ELECTRONICS'), 'CablePort')
        for x in (-w*0.30,w*0.30):
            add(s, box((w*0.26,0.026,h*0.50),(x,-d/2-0.016,h*0.32),material), f'ConsoleDoor_{x}')
    else:
        drawers = 2 if 'Drawer Nightstand' in name else 3 if 'Dresser Three' in name else 6 if 'Dresser Six' in name else 0
        if drawers:
            rows = 3 if drawers >= 3 else drawers
            cols = 2 if drawers == 6 else 1
            for r in range(rows):
                for c in range(cols):
                    dw = w*0.42 if cols == 2 else w*0.86
                    x = (-w*0.23 if c == 0 else w*0.23) if cols == 2 else 0
                    z = h*(0.22+r*0.24)
                    add(s, box((dw,0.025,h*0.17),(x,-d/2-0.016,z),material), f'Drawer_{r}_{c}')
                    add(s, box((dw*0.28,0.02,0.025),(x,-d/2-0.045,z),'MAT_BRASS_POLISHED'), f'DrawerPull_{r}_{c}')
        elif 'Charging Table' in name:
            add(s, box((0.18,0.08,0.025),(0,-d*0.22,h+0.015),'MAT_ELECTRONICS'), 'ChargingPad')
    return s


def lamp():
    s = trimesh.Scene()
    add(s,cyl(0.16,0.05,(0,0,0.025),'MAT_BRASS_POLISHED',20),'Base')
    add(s,cyl(0.025,0.42,(0,0,0.26),'MAT_BRASS_POLISHED',12),'Stem')
    shade = trimesh.creation.cone(radius=0.24,height=0.30,sections=24)
    shade.apply_translation((0,0,0.62))
    shade.visual = trimesh.visual.TextureVisuals(material=mat('MAT_EMISSIVE_WARM'))
    add(s,shade,'Shade')
    return s


def desk(name):
    s = trimesh.Scene(); w,d,h = 1.2,0.55,0.75
    if 'Executive' in name: w,d = 1.55,0.70
    if 'Classic' in name: w,d = 1.35,0.62
    add(s,box((w,d,0.08),(0,0,h),'MAT_WOOD_WARM'),'Desktop')
    for x in (-w/2+0.08,w/2-0.08):
        for y in (-d/2+0.08,d/2-0.08):
            add(s,box((0.08,0.08,h),(x,y,h/2),'MAT_WOOD_DARK'),f'Leg_{x}_{y}')
    if 'Executive' in name:
        add(s,box((0.35,d*0.82,0.46),(w*0.32,0,0.46),'MAT_WOOD_WARM'),'Pedestal')
    if 'Classic' in name:
        add(s,box((w*0.70,0.035,0.08),(0,-d/2-0.02,h*0.76),'MAT_BRASS_POLISHED'),'ClassicApron')
    return s


def chair(name, material):
    s = trimesh.Scene(); seat_w,seat_d,seat_h = 0.52,0.50,0.48
    if 'Armchair' in name or 'Lounge' in name: seat_w,seat_d,seat_h = 0.70,0.68,0.46
    add(s,box((seat_w,seat_d,0.16),(0,0,seat_h),material),'Seat')
    back_h = 0.72 if 'Wingback' not in name else 1.05
    add(s,box((seat_w,0.14,back_h),(0,seat_d/2-0.03,seat_h+back_h/2-0.04),material),'Back')
    for x in (-seat_w*0.38,seat_w*0.38):
        for y in (-seat_d*0.35,seat_d*0.35):
            add(s,box((0.055,0.055,seat_h),(x,y,seat_h/2),'MAT_WOOD_DARK'),f'Leg_{x}_{y}')
    if 'Armchair' in name or 'Lounge' in name:
        for x in (-seat_w/2-0.04,seat_w/2+0.04):
            add(s,box((0.13,seat_d*0.92,0.35),(x,0,seat_h+0.10),material),f'Arm_{x}')
    if 'Wingback' in name:
        for x in (-seat_w*0.47,seat_w*0.47):
            add(s,box((0.12,0.20,0.48),(x,seat_d*0.32,seat_h+0.72),material),f'Wing_{x}')
    return s


def sofa(name):
    s = trimesh.Scene(); w = 1.55 if 'Two Seat' in name else 2.20; d = 0.82; h = 0.47
    add(s,box((w,d,0.18),(0,0,h),'MAT_UPHOLSTERY'),'SeatBase')
    add(s,box((w,0.18,0.72),(0,d/2-0.04,h+0.32),'MAT_UPHOLSTERY'),'Back')
    for x in (-w/2-0.04,w/2+0.04):
        add(s,box((0.18,d*0.95,0.38),(x,0,h+0.08),'MAT_UPHOLSTERY'),f'Arm_{x}')
    seats = 2 if 'Two Seat' in name else 3
    for i,x in enumerate(np.linspace(-w*0.30,w*0.30,seats)):
        add(s,box((w/seats*0.78,d*0.72,0.12),(x,-0.02,h+0.12),'MAT_UPHOLSTERY'),f'Cushion_{i}')
    return s


def table_or_bench(name, material):
    s = trimesh.Scene()
    if 'Bench' in name:
        w,d,h = 1.20,0.42,0.48
        add(s,box((w,d,0.16),(0,0,h),material),'BenchSeat')
        for x in (-w*0.38,w*0.38):
            add(s,box((0.08,d*0.72,h),(x,0,h/2),'MAT_WOOD_DARK'),f'Leg_{x}')
        if 'Luggage' in name:
            for y in (-d*0.26,0,d*0.26):
                add(s,box((w*0.72,0.04,0.025),(0,y,h+0.10),'MAT_LINEN'),f'LuggageStrap_{y}')
        return s
    round_table = 'Round' in name
    w,d,h = (0.82,0.82,0.46) if 'Coffee' in name else (0.55,0.55,0.58)
    if 'Rectangular' in name: w,d = 1.10,0.62
    if round_table:
        add(s,cyl(w/2,0.08,(0,0,h),material,28),'Top')
        add(s,cyl(0.07,h,(0,0,h/2),'MAT_WOOD_DARK',16),'Pedestal')
        add(s,cyl(0.24,0.05,(0,0,0.025),'MAT_WOOD_DARK',20),'Base')
    else:
        add(s,box((w,d,0.08),(0,0,h),material),'Top')
        for x in (-w*0.38,w*0.38):
            for y in (-d*0.34,d*0.34):
                add(s,box((0.06,0.06,h),(x,y,h/2),'MAT_WOOD_DARK'),f'Leg_{x}_{y}')
    return s


def television(name):
    s = trimesh.Scene(); w,h = 1.10,0.68
    add(s,box((w,0.055,h),(0,0,h/2+0.55),'MAT_ELECTRONICS'),'TVBody')
    add(s,box((w*0.92,0.018,h*0.82),(0,-0.037,h/2+0.55),'MAT_GLASS_CLEAR'),'Screen')
    if 'Floor Standing' in name:
        add(s,cyl(0.05,0.55,(0,0,0.275),'MAT_STAINLESS',14),'Stand')
        add(s,box((0.45,0.25,0.06),(0,0,0.03),'MAT_STAINLESS'),'Base')
    else:
        add(s,box((0.48,0.05,0.12),(0,0,0.34),'MAT_STAINLESS'),'WallBracket')
    return s


def mirror(name):
    s = trimesh.Scene(); w,h = (0.65,1.75) if 'Full Length' in name else (0.90,0.75)
    add(s,box((w+0.10,0.06,h+0.10),(0,0,(h+0.10)/2),'MAT_WOOD_WARM'),'Frame')
    add(s,box((w,0.018,h),(0,-0.04,h/2+0.05),'MAT_GLASS_CLEAR'),'MirrorGlass')
    if 'Decorative' in name:
        for x in (-w/2-0.06,w/2+0.06):
            add(s,cyl(0.035,h+0.18,(x,0,(h+0.10)/2),'MAT_BRASS_POLISHED',12),f'DecorativeRail_{x}')
    return s


def curtain(name):
    s = trimesh.Scene(); w,h = 1.80,2.35
    add(s,box((w+0.18,0.045,0.045),(0,0,h+0.12),'MAT_BRASS_POLISHED'),'CurtainRod')
    panel_material = 'MAT_LINEN_BLACKOUT' if 'Blackout' in name else 'MAT_LINEN_SHEER' if 'Sheer' in name else 'MAT_LINEN'
    for i,x in enumerate((-0.48,0.48)):
        parent = f'MOV_CurtainPanel_{i}'
        add(s,box((0.78,0.055,h),(x,0,h/2),panel_material),parent)
        for j,dx in enumerate(np.linspace(-0.30,0.30,5)):
            add(s,box((0.025,0.025,h*0.94),(x+dx,-0.035,h*0.48),panel_material),f'Pleat_{i}_{j}',parent=parent)
        for j,dx in enumerate(np.linspace(-0.30,0.30,5)):
            add(s,cyl(0.028,0.026,(x+dx,0,h+0.055),'MAT_BRASS_POLISHED',12),f'CurtainRing_{i}_{j}',parent=parent)
        if 'Blackout' in name:
            add(s,box((0.72,0.020,h*0.94),(x,0.045,h*0.48),'MAT_LINEN_BLACKOUT'),f'BlackoutBacking_{i}',parent=parent)
        if 'Sheer' in name:
            add(s,box((0.70,0.012,h*0.90),(x,-0.044,h*0.46),'MAT_LINEN_SHEER'),f'SheerOverlay_{i}',parent=parent)
    return s


def rug():
    s = trimesh.Scene()
    add(s,box((1.45,0.95,0.025),(0,0,0.0125),'MAT_CARPET_STANDARD'),'Rug')
    add(s,box((1.25,0.75,0.008),(0,0,0.029),'MAT_CARPET_STANDARD'),'Inset')
    for x in (-0.60,0.60):
        for y in np.linspace(-0.34,0.34,5):
            add(s,box((0.035,0.07,0.018),(x,y,0.030),'MAT_CARPET_STANDARD'),f'Fringe_{x}_{y}')
    return s


def build(name, material):
    if 'Bed' in name and 'Bedside' not in name: return bed(name)
    if name == 'Baby Crib': return crib()
    if 'Nightstand' in name or 'Charging Table' in name or 'Wardrobe' in name or 'Closet' in name or 'Armoire' in name or 'Dresser' in name or 'Cabinet' in name or 'Refrigerator' in name or 'Safe' in name or 'TV Console' in name: return case_piece(name, material)
    if 'Lamp' in name: return lamp()
    if 'Desk' in name and 'Chair' not in name: return desk(name)
    if 'Chair' in name or 'Armchair' in name: return chair(name, material)
    if 'Sofa' in name: return sofa(name)
    if 'Table' in name or 'Bench' in name: return table_or_bench(name, material)
    if 'Television' in name: return television(name)
    if 'Mirror' in name: return mirror(name)
    if 'Curtain' in name: return curtain(name)
    if 'Rug' in name: return rug()
    return trimesh.Scene(box((0.6,0.6,0.6),(0,0,0.3),material))


def make_sidecar(asset_id, subcategory, material, animset):
    tags = ['hotel-haven','batch_03',subcategory]
    if animset:
        tags += ['animated-binding',animset]
    return {
        'schema':1,
        'asset_id':asset_id,
        'asset_type':'StaticMeshAsset',
        'source':'Tools/ArtGeneration/batch03_generate.py',
        'units':'meters',
        'lod_policy':'lod_furniture',
        'collision_policy':'simple_proxy',
        'material_slots':[material],
        'tags':tags,
        'dependencies':[animset] if animset else [],
        'cutaway_policy':'normal',
        'source_revision':2,
        'metadata_revision':2,
        'cooker_schema':1,
        'lifecycle_state':'PRODUCTION',
    }


def generate(manifest:Path, output:Path, package:bool):
    data = json.loads(manifest.read_text())
    rows = [a for g in data['groups'] for a in g['assets']]
    if len(rows) != 50:
        raise ValueError(f'Batch 03 must contain 50 assets; got {len(rows)}')
    output.mkdir(parents=True,exist_ok=True)
    outputs = []
    for asset_id,name,subcategory,material,profile,animset,anchors in rows:
        scene = build(name,material)
        path = output/f'{asset_id}.glb'
        path.write_bytes(scene.export(file_type='glb'))
        outputs.append(path)
        if package:
            (output/f'{asset_id}.asset.json').write_text(json.dumps(make_sidecar(asset_id,subcategory,material,animset),indent=2)+'\n')
        print(path)
    return outputs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('manifest',type=Path)
    ap.add_argument('output',type=Path)
    ap.add_argument('--package',action='store_true')
    a = ap.parse_args()
    generate(a.manifest,a.output,a.package)


if __name__ == '__main__':
    main()
