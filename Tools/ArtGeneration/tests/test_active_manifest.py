import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))
from asset_manifest import AssetManifest, load_active_manifest
from generate_all_assets import gameplay_manifest_metadata, expected_animation_bindings
from link_animation_dependencies import manifest_animation_bindings
from batch16_generate import build_asset as build_batch16


class ActiveManifestTests(unittest.TestCase):
    def test_850_canonical_ids_and_declared_batches(self):
        active = load_active_manifest(ROOT)
        self.assertEqual(active.asset_count, 850)
        self.assertEqual(active.batch_numbers, tuple(range(1, 18)))
        ids = [row[0] for _, _, row in active.iter_rows()]
        self.assertEqual(sorted(ids), [f'HH_A{i:03d}' for i in range(1, 851)])
        self.assertEqual(active.batch_entries[-1].generator_family, 'architectural_modules')
        metadata, _ = gameplay_manifest_metadata(ROOT)
        self.assertEqual(set(metadata), set(ids))
        self.assertEqual(len(manifest_animation_bindings(ROOT / 'GameData/AssetDefinitions/Manifest')),
                         expected_animation_bindings(active))

    def test_unlisted_shard_is_ignored(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            shard = root / 'declared.json'
            shard.write_text(json.dumps({'groups': [{'family': 'test', 'assets': [[f'HH_A{i:03d}', '', '', '', '', None, []] for i in range(1, 51)]}]}))
            (root / 'asset_batch_99.json').write_text(json.dumps({'groups': [{'family': 'test', 'assets': [['HH_A999', '', '', '', '', None, []]]}]}))
            master = root / 'master.json'
            master.write_text(json.dumps({'asset_count': 50, 'batches': [{'batch': 1, 'path': 'declared.json', 'asset_count': 50}]}))
            active = AssetManifest(root, master)
            self.assertEqual([row[0] for _, _, row in active.iter_rows()], [f'HH_A{i:03d}' for i in range(1, 51)])

    def test_missing_or_skipped_batch_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            master = root / 'master.json'
            master.write_text(json.dumps({'asset_count': 50, 'batches': [{'batch': 2, 'path': 'missing.json', 'asset_count': 50}]}))
            with self.assertRaisesRegex(ValueError, 'contiguous'):
                AssetManifest(root, master)

    def test_imported_floorstanding_assets_reach_placement_floor(self):
        for asset_id, name, category, material in (
            ('HH_A756', 'Gym Stationary Bike', 'gym', 'MAT_ELECTRONICS'),
            ('HH_A764', 'Conference Mobile Whiteboard', 'conference', 'MAT_SIGNAGE'),
            ('HH_A775', 'Accessibility Wheelchair', 'accessible', 'MAT_BLACKENED_STEEL'),
        ):
            with self.subTest(asset_id=asset_id):
                scene = build_batch16(name, category, material, asset_id, 'P_INTERACTIVE_PREFAB')
                self.assertAlmostEqual(float(scene.bounds[0][2]), 0.0, places=6)


if __name__ == '__main__':
    unittest.main()
