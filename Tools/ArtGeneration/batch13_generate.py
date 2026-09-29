from public_space_factory import build_asset as build_public
from service_asset_factory import build_asset as build_service
from v2_asset_common import generate_manifest_batch


def build_asset(name, subcategory, mat, asset_id, profile):
    if name == 'Luggage Cart Brass':
        return build_service(name, mat)
    return build_public(name, subcategory, mat, asset_id, profile)


def generate_package(manifest_path, output_dir, source_path):
    return generate_manifest_batch(manifest_path, output_dir, source_path, build_asset)
