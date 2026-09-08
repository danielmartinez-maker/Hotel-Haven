from __future__ import annotations

import json
from pathlib import Path
import shutil
import sys

import pytest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT_DIR = ROOT / "Tools" / "ContentPipeline" / "scripts"
sys.path.insert(0, str(SCRIPT_DIR))

import validate_asset_library  # noqa: E402


def _copy_repo(tmp_path: Path) -> Path:
    repo = tmp_path / "repo"
    shutil.copytree(ROOT / "GameData", repo / "GameData")
    return repo


def _load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _save(path: Path, data: dict) -> None:
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def _defs(repo: Path) -> Path:
    return repo / "GameData" / "AssetDefinitions"


def _first_static_row(repo: Path) -> tuple[Path, list]:
    master = _load(_defs(repo) / "hotel_haven_asset_manifest_v1.json")
    animated = validate_asset_library.ANIMATED_PROFILES
    for spec in master["batches"]:
        path = repo / spec["path"]
        batch = _load(path)
        for group in batch["groups"]:
            for row in group["assets"]:
                if row[4] not in animated:
                    return path, row
    raise AssertionError("test fixture has no static asset")


def test_validator_accepts_current_canonical_library(tmp_path: Path) -> None:
    repo = _copy_repo(tmp_path)
    assert validate_asset_library.validate(repo) == []


def test_validator_rejects_animation_metadata_on_static_profiles(tmp_path: Path) -> None:
    repo = _copy_repo(tmp_path)
    animations = _load(_defs(repo) / "animation_sets_v1.json")
    animation_id = animations["sets"][0]["animation_set_id"]
    batch_path, target_row = _first_static_row(repo)
    batch = _load(batch_path)
    target_id = target_row[0]
    for group in batch["groups"]:
        for row in group["assets"]:
            if row[0] == target_id:
                row[5] = animation_id
    _save(batch_path, batch)

    errors = validate_asset_library.validate(repo)
    assert any(target_id in error and "animation" in error.lower() for error in errors)


def test_validator_rejects_duplicate_interaction_anchors(tmp_path: Path) -> None:
    repo = _copy_repo(tmp_path)
    master = _load(_defs(repo) / "hotel_haven_asset_manifest_v1.json")
    batch_path = repo / master["batches"][0]["path"]
    batch = _load(batch_path)
    target = batch["groups"][0]["assets"][0]
    target[6] = ["INT_TEST", "INT_TEST"]
    _save(batch_path, batch)

    errors = validate_asset_library.validate(repo)
    assert any(target[0] in error and "anchor" in error.lower() for error in errors)


def test_validator_rejects_duplicate_skeleton_definitions(tmp_path: Path) -> None:
    repo = _copy_repo(tmp_path)
    path = _defs(repo) / "animation_sets_v1.json"
    data = _load(path)
    data["skeletons"].append(dict(data["skeletons"][0]))
    _save(path, data)

    errors = validate_asset_library.validate(repo)
    assert any("skeleton" in error.lower() and "unique" in error.lower() for error in errors)


def test_validator_reports_bad_family_target_types_without_crashing(tmp_path: Path) -> None:
    repo = _copy_repo(tmp_path)
    path = _defs(repo) / "hotel_haven_asset_manifest_v1.json"
    data = _load(path)
    first_key = next(iter(data["family_targets"]))
    data["family_targets"][first_key] = "fifty"
    _save(path, data)

    try:
        errors = validate_asset_library.validate(repo)
    except Exception as exc:  # pragma: no cover - this is the behavior the regression forbids
        pytest.fail(f"validator crashed on malformed family target: {exc}")
    assert errors
    assert any("family_targets" in error for error in errors)


def test_validator_reports_non_object_batch_spec_without_crashing(tmp_path: Path) -> None:
    repo = _copy_repo(tmp_path)
    path = _defs(repo) / "hotel_haven_asset_manifest_v1.json"
    data = _load(path)
    data["batches"][0] = "not-an-object"
    _save(path, data)

    try:
        errors = validate_asset_library.validate(repo)
    except Exception as exc:  # pragma: no cover
        pytest.fail(f"validator crashed on malformed batch spec: {exc}")
    assert errors
    assert any("batch" in error.lower() for error in errors)
