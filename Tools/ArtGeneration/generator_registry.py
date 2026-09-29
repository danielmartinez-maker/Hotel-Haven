"""Shared batch generators for manifest-declared content families."""

from architecture_factory import build_asset as build_architecture
from architectural_modules_factory import build_asset as build_architectural_modules
from catalog_expansion_factory import build_asset as build_catalog_expansion
from core_hotel_expansion_factory import build_asset as build_core_hotel_expansion
from design_completion_factory import build_asset as build_design_completion
from hotel_continuation_factory import build_asset as build_hotel_continuation
from curated_hotel_factory import build_asset as build_curated_hotel
from destination_expansion_factory import build_asset as build_destination_expansion
from extended_hotel_factory import build_asset as build_extended_hotel
from experience_expansion_factory import build_asset as build_experience_expansion
from operations_expansion_factory import build_asset as build_operations_expansion
from property_expansion_factory import build_asset as build_property_expansion
from service_expansion_factory import build_asset as build_service_expansion
from v2_asset_common import generate_manifest_batch


class FamilyGenerator:
    def __init__(self, builder):
        self.builder = builder

    def generate_package(self, manifest_path, output_dir, source_path):
        return generate_manifest_batch(manifest_path, output_dir, source_path, self.builder)


GENERATOR_FAMILIES = {
    'architecture': FamilyGenerator(build_architecture),
    'architectural_modules': FamilyGenerator(build_architectural_modules),
    'curated_hotel': FamilyGenerator(build_curated_hotel),
    'extended_hotel': FamilyGenerator(build_extended_hotel),
    'catalog_expansion': FamilyGenerator(build_catalog_expansion),
    'property_expansion': FamilyGenerator(build_property_expansion),
    'operations_expansion': FamilyGenerator(build_operations_expansion),
    'service_expansion': FamilyGenerator(build_service_expansion),
    'experience_expansion': FamilyGenerator(build_experience_expansion),
    'destination_expansion': FamilyGenerator(build_destination_expansion),
    'core_hotel_expansion': FamilyGenerator(build_core_hotel_expansion),
    'hotel_continuation': FamilyGenerator(build_hotel_continuation),
    'design_completion': FamilyGenerator(build_design_completion),
}
