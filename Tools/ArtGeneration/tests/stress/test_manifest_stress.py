from __future__ import annotations

import importlib.util
import json
import shutil
import sys
from pathlib import Path

from .stress_support import SplitMix64, iterations, load_stress_config

ROOT = Path(__file__).resolve().parents[4]
ART = ROOT / 'Tools' / 'ArtGeneration'
VALIDATOR_PATH = ROOT / 'Tools' / 'ContentPipeline' / 'scripts' / 'validate_asset_library.py'
if str(ART) not in sys.path:
    sys.path.insert(0, str(ART))

from asset_manifest import AssetManifest, load_active_manifest


def _load_validator():
    spec = importlib.util.spec_from_file_location('hh_validate_asset_library_stress', VALIDATOR_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _rows(manifest: AssetManifest):
    return list(manifest.iter_rows())


def _copy_definitions(tmp_path: Path) -> Path:
    target = tmp_path / 'GameData' / 'AssetDefinitions'
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(ROOT / 'GameData' / 'AssetDefinitions', target)
    return target


def test_splitmix64_is_replayable():
    a = SplitMix64(0x123456789ABCDEF0)
    b = SplitMix64(0x123456789ABCDEF0)
    assert [a.bounded(37) for _ in range(1000)] == [b.bounded(37) for _ in range(1000)]


def test_default_scale_is_pr(monkeypatch):
    monkeypatch.delenv('HH_STRESS_SCALE', raising=False)
    monkeypatch.delenv('HH_STRESS_SEED', raising=False)
    monkeypatch.delenv('HH_STRESS_SCENARIO', raising=False)
    config = load_stress_config()
    assert config.scale == 'pr'
    assert config.seed == 0x5EED5EED5EED5EED
    assert config.scenario == ''


def test_bounded_respects_range():
    rng = SplitMix64(123)
    assert all(0 <= rng.bounded(7) < 7 for _ in range(10_000))


def test_active_v2_manifest_repeatedly_preserves_exact_800_asset_contract(monkeypatch):
    monkeypatch.setenv('HH_STRESS_SCALE', 'pr')
    config = load_stress_config()
    expected_ids = {f'HH_A{i:03d}' for i in range(1, 801)}
    passes = iterations(config, 100, 1_000, 5_000)

    for _ in range(passes):
        manifest = load_active_manifest(ROOT)
        assert manifest.path.name == 'hotel_haven_asset_manifest_v2.json'
        assert manifest.asset_count == 800
        assert manifest.batch_numbers == tuple(range(1, 17))
        assert len(manifest.batch_entries) == 16
        assert all(entry.asset_count == 50 for entry in manifest.batch_entries)
        rows = _rows(manifest)
        ids = [row[2][0] for row in rows]
        assert len(ids) == 800
        assert len(set(ids)) == 800
        assert set(ids) == expected_ids
        assert manifest.quality_contract_path is not None
        assert manifest.quality_contract_path.is_file()
        profiles = manifest.profiles
        assert all(row[2][4] in profiles for row in rows)


def test_release_validator_uses_active_v2_manifest_even_without_v1(tmp_path):
    definitions = _copy_definitions(tmp_path)
    (definitions / 'hotel_haven_asset_manifest_v1.json').unlink()
    validator = _load_validator()
    errors = validator.validate(tmp_path)
    assert errors == [], '\n'.join(errors)


def test_release_validator_rejects_duplicate_v2_asset_id(tmp_path):
    definitions = _copy_definitions(tmp_path)
    (definitions / 'hotel_haven_asset_manifest_v1.json').unlink()
    batch = definitions / 'Manifest' / 'asset_batch_16.json'
    data = json.loads(batch.read_text(encoding='utf-8'))
    rows = [row for group in data['groups'] for row in group['assets']]
    assert len(rows) == 50
    rows[-1][0] = rows[0][0]
    batch.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')

    validator = _load_validator()
    errors = validator.validate(tmp_path)
    assert errors
    assert any('duplicate asset' in error.lower() for error in errors), errors


def test_release_validator_rejects_missing_declared_v2_batch(tmp_path):
    definitions = _copy_definitions(tmp_path)
    (definitions / 'hotel_haven_asset_manifest_v1.json').unlink()
    (definitions / 'Manifest' / 'asset_batch_16.json').unlink()
    validator = _load_validator()
    errors = validator.validate(tmp_path)
    assert errors
    assert any('asset_batch_16.json' in error for error in errors), errors


def test_asset_manifest_rejects_duplicate_and_unsorted_batch_numbers(tmp_path):
    definitions = tmp_path / 'GameData' / 'AssetDefinitions'
    definitions.mkdir(parents=True)
    manifest_path = definitions / 'hotel_haven_asset_manifest_v2.json'
    manifest_path.write_text(
        json.dumps({
            'asset_count': 2,
            'profiles': {'P_SMALL_PROP': {}},
            'batches': [
                {'batch': 2, 'path': 'b.json', 'asset_count': 1},
                {'batch': 2, 'path': 'a.json', 'asset_count': 1},
            ],
        }),
        encoding='utf-8',
    )
    try:
        AssetManifest(tmp_path, manifest_path)
    except ValueError as exc:
        assert 'duplicate batch' in str(exc)
    else:
        raise AssertionError('duplicate batch numbers were accepted')

    manifest_path.write_text(
        json.dumps({
            'asset_count': 2,
            'profiles': {'P_SMALL_PROP': {}},
            'batches': [
                {'batch': 2, 'path': 'b.json', 'asset_count': 1},
                {'batch': 1, 'path': 'a.json', 'asset_count': 1},
            ],
        }),
        encoding='utf-8',
    )
    try:
        AssetManifest(tmp_path, manifest_path)
    except ValueError as exc:
        assert 'sorted' in str(exc)
    else:
        raise AssertionError('unsorted batch numbers were accepted')
