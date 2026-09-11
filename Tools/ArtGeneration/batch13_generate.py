from public_space_factory import build_asset
from v2_asset_common import generate_manifest_batch


def generate_package(manifest_path, output_dir, source_path):
    return generate_manifest_batch(manifest_path, output_dir, source_path, build_asset)
