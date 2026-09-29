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

    def test_existing_batches_preserve_legacy_generators(self):
        active = load_active_manifest(ROOT)
        for entry in active.batch_entries:
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


if __name__ == '__main__':
    unittest.main()
