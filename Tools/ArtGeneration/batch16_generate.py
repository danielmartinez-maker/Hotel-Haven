from amenity_v2_factory import build_asset as build_amenity
from character_v2_factory import build_asset as build_character
from v2_asset_common import generate_manifest_batch


def build_asset(name, subcategory, mat, asset_id, profile):
    if profile == 'P_CHARACTER':
        return build_character(name, subcategory, mat, asset_id, profile)
    return build_amenity(name, subcategory, mat, asset_id, profile)


def generate_package(manifest_path, output_dir, source_path):
    return generate_manifest_batch(manifest_path, output_dir, source_path, build_asset)
