"""Shared batch generators for manifest-declared content families."""

from architecture_factory import build_asset as build_architecture
from architectural_modules_factory import build_asset as build_architectural_modules
from v2_asset_common import generate_manifest_batch


class FamilyGenerator:
    def __init__(self, builder):
        self.builder = builder

    def generate_package(self, manifest_path, output_dir, source_path):
        return generate_manifest_batch(manifest_path, output_dir, source_path, self.builder)


GENERATOR_FAMILIES = {
    'architecture': FamilyGenerator(build_architecture),
    'architectural_modules': FamilyGenerator(build_architectural_modules),
}
