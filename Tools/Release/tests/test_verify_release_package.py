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


def write_package(path: Path, *, omit=(), extras=None, duplicate=None):
    omit = set(omit)
    extras = extras or {}
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, payload in REQUIRED_FILES.items():
            if name not in omit:
                archive.writestr(name, payload)
        for index in range(1, 501):
            name = f"data/assets/HH_A{index:03d}.hasset"
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
        self.assertEqual([], self.validate())

    def test_rejects_missing_runtime_file(self):
        errors = self.validate(omit={"shaders/Mesh.hlsl"})
        self.assertTrue(any("shaders/Mesh.hlsl" in error for error in errors))

    def test_rejects_incomplete_asset_set(self):
        errors = self.validate(omit={"data/assets/HH_A500.hasset"})
        self.assertTrue(any("500" in error and "asset" in error.lower() for error in errors))

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
