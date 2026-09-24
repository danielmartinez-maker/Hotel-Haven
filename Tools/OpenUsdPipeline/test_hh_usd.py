import copy
import json
import tempfile
import unittest
from pathlib import Path

import hh_usd


def manifest_fixture():
    return {
        "schema": 1,
        "hotel_id": "beaumont",
        "assets": [
            {"asset_id": "asset.prop.bed.king.modern_01", "usd_path": "Assets/bed_king_modern_01.usda", "runtime_asset_id": "asset.prop.bed.king.modern_01", "instance_mode": "scenegraph", "expected_instances": 250},
            {"asset_id": "asset.prop.ceiling_light.simple_01", "usd_path": "Assets/ceiling_light_simple_01.usda", "runtime_asset_id": "asset.prop.ceiling_light.simple_01", "instance_mode": "point_instancer", "expected_instances": 500},
        ],
        "floors": [
            {"floor_id": 1, "instances": [
                {"instance_id": "bed_102", "asset_id": "asset.prop.bed.king.modern_01", "transform": [4, 0, 8, 0, 90, 0, 1, 1, 1]},
                {"instance_id": "light_102", "asset_id": "asset.prop.ceiling_light.simple_01", "transform": [4, 2.7, 8, 0, 0, 0, 1, 1, 1]},
            ]},
            {"floor_id": 0, "instances": [
                {"instance_id": "light_001", "asset_id": "asset.prop.ceiling_light.simple_01", "transform": [1, 2.7, 1, 0, 0, 0, 1, 1, 1]},
                {"instance_id": "bed_001", "asset_id": "asset.prop.bed.king.modern_01", "transform": [1, 0, 1, 0, 0, 0, 1, 1, 1]},
            ]},
        ],
    }


class OpenUsdPipelineTests(unittest.TestCase):
    def test_validation_rejects_unknown_assets_and_high_count_unique_duplication(self):
        manifest = manifest_fixture()
        manifest["floors"][0]["instances"][0]["asset_id"] = "asset.missing"
        with self.assertRaisesRegex(ValueError, "unknown asset"):
            hh_usd.validate_manifest(manifest)
        manifest = manifest_fixture()
        manifest["assets"][0]["instance_mode"] = "unique"
        with self.assertRaisesRegex(ValueError, "more than 100"):
            hh_usd.validate_manifest(manifest)

    def test_generation_is_deterministic_and_floor_payloads_are_sorted(self):
        a = manifest_fixture()
        b = copy.deepcopy(a)
        b["assets"].reverse()
        b["floors"].reverse()
        for floor in b["floors"]:
            floor["instances"].reverse()
        with tempfile.TemporaryDirectory() as t1, tempfile.TemporaryDirectory() as t2:
            hh_usd.generate(a, Path(t1))
            hh_usd.generate(b, Path(t2))
            self.assertEqual((Path(t1) / "hotel.usda").read_text(), (Path(t2) / "hotel.usda").read_text())
            self.assertEqual((Path(t1) / "floors" / "F_00.usda").read_text(), (Path(t2) / "floors" / "F_00.usda").read_text())
            hotel = (Path(t1) / "hotel.usda").read_text()
            self.assertLess(hotel.index('"F_00"'), hotel.index('"F_01"'))
            self.assertIn('prepend payload = @floors/F_00.usda@</Floor>', hotel)

    def test_scenegraph_instances_and_point_instancers_are_emitted(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            hh_usd.generate(manifest_fixture(), root)
            floor = (root / "floors" / "F_00.usda").read_text()
            self.assertIn("instanceable = true", floor)
            self.assertIn("prepend references = @../Assets/bed_king_modern_01.usda@</Asset>", floor)
            self.assertIn('def PointInstancer "PI_asset_prop_ceiling_light_simple_01"', floor)
            self.assertIn("int[] protoIndices = [0]", floor)

    def test_cook_manifest_maps_used_usd_assets_to_runtime_asset_ids(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            hh_usd.generate(manifest_fixture(), root)
            cook = json.loads((root / "hotel_haven_usd_cook_manifest.json").read_text())
            self.assertEqual([a["asset_id"] for a in cook["assets"]], ["asset.prop.bed.king.modern_01", "asset.prop.ceiling_light.simple_01"])
            self.assertEqual(cook["floors"], ["floors/F_00.usda", "floors/F_01.usda"])


if __name__ == "__main__":
    unittest.main()
