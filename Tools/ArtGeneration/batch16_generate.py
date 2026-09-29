from amenity_v2_factory import build_asset as build_amenity
from character_v2_factory import build_asset as build_character
from v2_asset_common import generate_manifest_batch
from v2_asset_common import add, cyl


def build_asset(name, subcategory, mat, asset_id, profile):
    if profile == 'P_CHARACTER':
        return build_character(name, subcategory, mat, asset_id, profile)
    scene = build_amenity(name, subcategory, mat, asset_id, profile)
    if asset_id == 'HH_A764':
        scene.apply_translation((0, 0, -float(scene.bounds[0][2])))
        for i, x in enumerate((-0.22, 0.22)):
            add(scene, cyl(0.055, 0.04, (x, 0, 0.055), 'MAT_BLACKENED_STEEL', 14), f'MOV_Wheel_{i}')
    return scene


def generate_package(manifest_path, output_dir, source_path):
    return generate_manifest_batch(manifest_path, output_dir, source_path, build_asset)
