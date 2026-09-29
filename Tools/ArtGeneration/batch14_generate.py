from food_beverage_factory import build_asset as build_food
from service_asset_factory import build_asset as build_service
from v2_asset_common import cart, generate_manifest_batch


def build_asset(name, subcategory, mat, asset_id, profile):
    if name == 'Banquet Chair Cart':
        return cart(name, mat)
    if name == 'Room Service Cart':
        return build_service(name, mat)
    return build_food(name, subcategory, mat, asset_id, profile)


def generate_package(manifest_path, output_dir, source_path):
    return generate_manifest_batch(manifest_path, output_dir, source_path, build_asset)
