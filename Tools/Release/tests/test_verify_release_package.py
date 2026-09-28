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


SYNTHETIC_RUNTIME_ASSET_IDS = tuple(
    f"HH_TEST_A{index:03d}" for index in range(1, 746)
)


def write_exports(exports_root: Path, asset_ids=SYNTHETIC_RUNTIME_ASSET_IDS):
    exports_root.mkdir(parents=True, exist_ok=True)
    for index, asset_id in enumerate(asset_ids):
        sidecar = exports_root / f"{asset_id}.asset.json"
        sidecar.write_text(
            json.dumps(
                {
                    "schema": 1,
                    "asset_id": asset_id,
                    "asset_type": (
                        "SkinnedMeshAsset" if index % 11 == 0 else "StaticMeshAsset"
                    ),
                }
            ),
            encoding="utf-8",
        )


def write_package(path: Path, *, omit=(), extras=None, duplicate=None):
    omit = set(omit)
    extras = extras or {}
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, payload in REQUIRED_FILES.items():
            if name not in omit:
                archive.writestr(name, payload)
        for asset_id in SYNTHETIC_RUNTIME_ASSET_IDS:
            name = f"data/assets/{asset_id}.hasset"
            if name not in omit:
                archive.writestr(name, b"asset")
        for name, payload in extras.items():
            archive.writestr(name, payload)
        if duplicate:
            archive.writestr(duplicate, b"duplicate")


class ReleasePackageIntegrityTests(unittest.TestCase):
    def validate(self, *, asset_ids=SYNTHETIC_RUNTIME_ASSET_IDS, **kwargs):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package = root / "Hotel-Haven-0.1.0-Windows.zip"
            exports_root = root / "exports"
            write_exports(exports_root, asset_ids)
            write_package(package, **kwargs)
            return validate_archive(package, exports_root)

    def test_accepts_complete_shipping_package(self):
        self.assertEqual(745, len(SYNTHETIC_RUNTIME_ASSET_IDS))
        self.assertEqual([], self.validate())

    def test_rejects_missing_runtime_file(self):
        errors = self.validate(omit={"shaders/Mesh.hlsl"})
        self.assertTrue(any("shaders/Mesh.hlsl" in error for error in errors))

    def test_rejects_incomplete_asset_set(self):
        missing_asset = SYNTHETIC_RUNTIME_ASSET_IDS[-1]
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
