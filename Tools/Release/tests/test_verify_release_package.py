import json
import tempfile
import unittest
import zipfile
from pathlib import Path

from Tools.Release.verify_release_package import validate_archive


REQUIRED_FILES = {
    "hotel_haven.exe": b"client",
    "hotel_haven_headless.exe": b"headless",
    "shaders/InstancedBox.hlsl": b"shader",
    "shaders/Mesh.hlsl": b"shader",
    "data/balance.json": b"{}",
    "README.md": b"readme",
    "IMPLEMENTATION_STATUS.md": b"status",
}


RUNTIME_ASSET_TYPES = {"StaticMeshAsset", "SkinnedMeshAsset"}


def current_runtime_asset_ids():
    repo_root = Path(__file__).resolve().parents[3]
    manifest_path = (
        repo_root
        / "GameData"
        / "AssetDefinitions"
        / "hotel_haven_asset_manifest_v2.json"
    )
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    runtime_profiles = {
        profile_name
        for profile_name, profile in manifest["profiles"].items()
        if profile["asset_type"] in RUNTIME_ASSET_TYPES
    }

    ids = []
    for batch in manifest["batches"]:
        batch_path = repo_root / batch["path"]
        catalog = json.loads(batch_path.read_text(encoding="utf-8"))
        columns = catalog["columns"]
        asset_id_index = columns.index("asset_id")
        profile_index = columns.index("profile")
        for group in catalog["groups"]:
            for asset in group["assets"]:
                if asset[profile_index] in runtime_profiles:
                    ids.append(asset[asset_id_index])
    return tuple(sorted(ids))


CURRENT_RUNTIME_ASSET_IDS = current_runtime_asset_ids()


def write_package(path: Path, *, omit=(), extras=None, duplicate=None):
    omit = set(omit)
    extras = extras or {}
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, payload in REQUIRED_FILES.items():
            if name not in omit:
                archive.writestr(name, payload)
        for asset_id in CURRENT_RUNTIME_ASSET_IDS:
            name = f"data/assets/{asset_id}.hasset"
            if name not in omit:
                archive.writestr(name, b"asset")
        for name, payload in extras.items():
            archive.writestr(name, payload)
        if duplicate:
            archive.writestr(duplicate, b"duplicate")


class ReleasePackageIntegrityTests(unittest.TestCase):
    def validate(self, **kwargs):
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp) / "Hotel-Haven-0.1.0-Windows.zip"
            write_package(package, **kwargs)
            return validate_archive(package)

    def test_accepts_complete_shipping_package(self):
        self.assertEqual(745, len(CURRENT_RUNTIME_ASSET_IDS))
        self.assertEqual([], self.validate())

    def test_rejects_missing_runtime_file(self):
        errors = self.validate(omit={"shaders/Mesh.hlsl"})
        self.assertTrue(any("shaders/Mesh.hlsl" in error for error in errors))

    def test_rejects_incomplete_asset_set(self):
        missing_asset = CURRENT_RUNTIME_ASSET_IDS[-1]
        errors = self.validate(omit={f"data/assets/{missing_asset}.hasset"})
        self.assertTrue(any("asset" in error.lower() for error in errors))

    def test_rejects_duplicate_archive_entry(self):
        errors = self.validate(duplicate="data/balance.json")
        self.assertTrue(any("duplicate" in error.lower() for error in errors))

    def test_rejects_unsafe_archive_path(self):
        errors = self.validate(extras={"../escape.txt": b"bad"})
        self.assertTrue(any("unsafe" in error.lower() for error in errors))

    def test_rejects_windows_drive_archive_path(self):
        errors = self.validate(extras={"C:/escape.txt": b"bad"})
        self.assertTrue(any("unsafe" in error.lower() for error in errors))

    def test_rejects_development_only_artifacts(self):
        errors = self.validate(extras={"debug/hotel_haven.pdb": b"symbols"})
        self.assertTrue(any("development" in error.lower() for error in errors))


if __name__ == "__main__":
    unittest.main()
