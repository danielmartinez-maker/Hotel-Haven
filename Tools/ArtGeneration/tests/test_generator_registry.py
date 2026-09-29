import sys
import json
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import AssetManifest, load_active_manifest
from generate_all_assets import resolve_batch_generator, generator_source


class GeneratorRegistryTests(unittest.TestCase):
    def test_declared_shard_with_49_rows_rejected_before_generation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'batch.json').write_text(json.dumps({'groups': [{'family': 'architecture_construction',
                'assets': [[f'HH_A{i:03d}'] for i in range(1, 50)]}]}))
            (root / 'manifest.json').write_text(json.dumps({'asset_count': 50,
                'batches': [{'batch': 1, 'path': 'batch.json', 'asset_count': 50}]}))
            with self.assertRaisesRegex(ValueError, '49 rows'):
                AssetManifest(root, root / 'manifest.json')

    def test_wrong_or_duplicate_canonical_id_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rows = [[f'HH_A{i:03d}'] for i in range(1, 51)]
            rows[24] = ['HH_A001']
            (root / 'batch.json').write_text(json.dumps({'batch': 1, 'groups': [
                {'family': 'architecture_construction', 'assets': rows}]}))
            (root / 'manifest.json').write_text(json.dumps({'asset_count': 50,
                'batches': [{'batch': 1, 'path': 'batch.json', 'asset_count': 50}]}))
            with self.assertRaisesRegex(ValueError, 'canonical IDs'):
                AssetManifest(root, root / 'manifest.json')

    def test_existing_batches_preserve_legacy_generators(self):
        active = load_active_manifest(ROOT)
        for entry in active.batch_entries[:16]:
            self.assertEqual(resolve_batch_generator(entry).__name__, f'batch{entry.batch:02d}_generate')

    def test_unknown_family_fails_closed(self):
        from asset_manifest import BatchEntry
        with self.assertRaisesRegex(ValueError, 'unknown generator family'):
            resolve_batch_generator(BatchEntry(17, 'unused.json', 50, 'nonexistent'))

    def test_registry_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(17, 'unused.json', 50, 'architecture'))
        self.assertTrue(callable(generator.generate_package))
        self.assertEqual(generator_source(17, 'architecture'),
                         'Tools/ArtGeneration/generator_registry.py')

    def test_extended_hotel_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(22, 'unused.json', 50, 'extended_hotel'))
        self.assertTrue(callable(generator.generate_package))

    def test_catalog_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(26, 'unused.json', 50, 'catalog_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_property_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(31, 'unused.json', 50, 'property_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_operations_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(36, 'unused.json', 50, 'operations_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_service_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(41, 'unused.json', 50, 'service_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_experience_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(46, 'unused.json', 50, 'experience_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_destination_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(51, 'unused.json', 50, 'destination_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_core_hotel_expansion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(56, 'unused.json', 50, 'core_hotel_expansion'))
        self.assertTrue(callable(generator.generate_package))

    def test_hotel_continuation_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(61, 'unused.json', 50, 'hotel_continuation'))
        self.assertTrue(callable(generator.generate_package))

    def test_design_completion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(66, 'unused.json', 50, 'design_completion'))
        self.assertTrue(callable(generator.generate_package))

    def test_hotel_finalization_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(71, 'unused.json', 50, 'hotel_finalization'))
        self.assertTrue(callable(generator.generate_package))

    def test_hotel_catalog_completion_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(75, 'unused.json', 50, 'hotel_catalog_completion'))
        self.assertTrue(callable(generator.generate_package))

    def test_hotel_final_catalog_family_resolves_shared_generator(self):
        from asset_manifest import BatchEntry
        generator = resolve_batch_generator(BatchEntry(79, 'unused.json', 50, 'hotel_final_catalog'))
        self.assertTrue(callable(generator.generate_package))

if __name__ == '__main__':
    unittest.main()
