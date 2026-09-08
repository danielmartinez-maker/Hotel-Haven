from __future__ import annotations

import json
from pathlib import Path
import sys

import pytest

ROOT = Path(__file__).resolve().parents[2]
ART_TOOLS = ROOT / "Tools" / "ArtGeneration"
sys.path.insert(0, str(ART_TOOLS))

import batch01_generate  # noqa: E402
import batch02_generate  # noqa: E402


PROFILE_ASSET_TYPES = {
    "P_ARCH_STATIC": "StaticMeshAsset",
    "P_ARCH_ANIMATED": "SkinnedMeshAsset",
    "P_FURNITURE_STATIC": "StaticMeshAsset",
    "P_SERVICE_PROP_ANIMATED": "SkinnedMeshAsset",
    "P_INTERACTIVE_PREFAB": "PrefabAsset",
    "P_INTERACTIVE_ANIMATED": "SkinnedMeshAsset",
    "P_SMALL_PROP": "StaticMeshAsset",
    "P_CHARACTER": "SkinnedMeshAsset",
}


def _manifest(batch: int) -> Path:
    return ROOT / "GameData" / "AssetDefinitions" / "Manifest" / f"asset_batch_{batch:02d}.json"


def _data(batch: int) -> dict:
    return json.loads(_manifest(batch).read_text(encoding="utf-8"))


def _rows(data: dict) -> list[list]:
    return [row for group in data["groups"] for row in group["assets"]]


def _write_manifest(path: Path, data: dict) -> Path:
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def test_batch01_package_asset_type_matches_declared_profile(tmp_path: Path) -> None:
    outputs = batch01_generate.generate_package(
        _manifest(1), tmp_path, "Tools/ArtGeneration/batch01_generate.py"
    )
    rows = {row[0]: row for row in _rows(_data(1))}
    for glb in outputs:
        metadata = json.loads(glb.with_suffix(".asset.json").read_text(encoding="utf-8"))
        declared_profile = rows[glb.stem][4]
        assert metadata["asset_type"] == PROFILE_ASSET_TYPES[declared_profile], glb.stem


def test_batch02_package_asset_type_matches_declared_profile(tmp_path: Path) -> None:
    outputs = batch02_generate.generate(_manifest(2), tmp_path, package=True)
    rows = {row[0]: row for row in _rows(_data(2))}
    for glb in outputs:
        metadata = json.loads(glb.with_suffix(".asset.json").read_text(encoding="utf-8"))
        declared_profile = rows[glb.stem][4]
        assert metadata["asset_type"] == PROFILE_ASSET_TYPES[declared_profile], glb.stem


@pytest.mark.parametrize("batch", [1, 2])
def test_generator_rejects_path_traversal_asset_ids(batch: int, tmp_path: Path) -> None:
    data = _data(batch)
    data["groups"][0]["assets"][0][0] = "../HH_ESCAPE"
    manifest = _write_manifest(tmp_path / "malicious.json", data)
    output = tmp_path / "out"
    escaped = tmp_path / "HH_ESCAPE.glb"

    with pytest.raises(ValueError, match="asset_id"):
        if batch == 1:
            batch01_generate.generate_batch(manifest, output)
        else:
            batch02_generate.generate(manifest, output, package=False)

    assert not escaped.exists(), "generator wrote outside the requested output directory"


@pytest.mark.parametrize("batch", [1, 2])
def test_generator_rejects_duplicate_asset_ids(batch: int, tmp_path: Path) -> None:
    data = _data(batch)
    rows = _rows(data)
    rows[1][0] = rows[0][0]
    manifest = _write_manifest(tmp_path / "duplicate.json", data)

    with pytest.raises(ValueError, match="duplicate"):
        if batch == 1:
            batch01_generate.generate_batch(manifest, tmp_path / "out")
        else:
            batch02_generate.generate(manifest, tmp_path / "out", package=False)
