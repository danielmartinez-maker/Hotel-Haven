from __future__ import annotations

import importlib.util
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
VALIDATOR = ROOT / 'Tools' / 'ContentPipeline' / 'scripts' / 'validate_asset_library.py'


def load_validator():
    spec = importlib.util.spec_from_file_location('validate_asset_library_hardening_v3', VALIDATOR)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def append_string(out: bytearray, value: str) -> None:
    encoded = value.encode('utf-8')
    out.extend(struct.pack('<I', len(encoded)))
    out.extend(encoded)


def hasset_v2(policy: dict, anchors: list[str]) -> bytes:
    out = bytearray(b'HHASSET\0')
    out.extend(struct.pack('<I', 2))
    out.extend(struct.pack('<I', 0))
    append_string(out, 'HH_A001')
    append_string(out, 'fingerprint')
    out.extend(struct.pack('<I', 0))
    append_string(out, 'Tools/ArtGeneration/batch01_generate.py')
    append_string(out, 'Art/Exports/Batch01/HH_A001.glb.asset.json')
    for key in ('units', 'lod_policy', 'collision_policy', 'cutaway_policy', 'pivot_profile'):
        append_string(out, policy[key])
    canonical_anchors = sorted(set(anchors))
    out.extend(struct.pack('<I', len(canonical_anchors)))
    for anchor in canonical_anchors:
        append_string(out, anchor)
    out.extend(struct.pack('<Q', 0))
    return bytes(out)


def sidecar(policy: dict, anchors: list[str]) -> dict:
    return {
        'schema': 1,
        'asset_id': 'HH_A001',
        'asset_type': 'StaticMeshAsset',
        'source': 'Tools/ArtGeneration/batch01_generate.py',
        'material_slots': [],
        'tags': [],
        'dependencies': [],
        **policy,
        'interaction_anchors': anchors,
    }


def test_cooked_policy_parity_detects_exact_match_and_mismatch(tmp_path: Path):
    validator = load_validator()
    validate = getattr(validator, 'validate_cooked_policy_parity', None)
    assert callable(validate)

    exports = tmp_path / 'Art' / 'Exports' / 'Batch01'
    cooked = tmp_path / 'Build' / 'CookedAssets'
    exports.mkdir(parents=True)
    cooked.mkdir(parents=True)

    policy = {
        'units': 'meters',
        'lod_policy': 'lod_furniture',
        'collision_policy': 'simple_proxy',
        'cutaway_policy': 'normal',
        'pivot_profile': 'floor_contact_center',
    }
    anchors = ['INT_USE_01', 'INT_REPAIR_01']
    (exports / 'HH_A001.glb.asset.json').write_text(
        json.dumps(sidecar(policy, anchors)), encoding='utf-8'
    )
    (cooked / 'HH_A001.hasset').write_bytes(hasset_v2(policy, anchors))

    errors, counts = validate(tmp_path / 'Art' / 'Exports', cooked)
    assert errors == []
    assert counts['cooked_policy_assets'] == 1
    assert counts['cooked_interaction_anchor_bindings'] == 2

    wrong = {**policy, 'lod_policy': 'wrong_lod'}
    (cooked / 'HH_A001.hasset').write_bytes(hasset_v2(wrong, anchors))
    errors, _ = validate(tmp_path / 'Art' / 'Exports', cooked)
    assert any('lod_policy' in error and 'HH_A001' in error for error in errors)


def test_cooked_policy_parity_rejects_legacy_container_without_envelope(tmp_path: Path):
    validator = load_validator()
    validate = getattr(validator, 'validate_cooked_policy_parity', None)
    assert callable(validate)

    exports = tmp_path / 'Art' / 'Exports' / 'Batch01'
    cooked = tmp_path / 'Build' / 'CookedAssets'
    exports.mkdir(parents=True)
    cooked.mkdir(parents=True)

    policy = {
        'units': 'meters',
        'lod_policy': 'lod_furniture',
        'collision_policy': 'simple_proxy',
        'cutaway_policy': 'normal',
        'pivot_profile': 'floor_contact_center',
    }
    (exports / 'HH_A001.glb.asset.json').write_text(
        json.dumps(sidecar(policy, [])), encoding='utf-8'
    )

    legacy = bytearray(b'HHASSET\0')
    legacy.extend(struct.pack('<I', 1))
    legacy.extend(struct.pack('<I', 0))
    append_string(legacy, 'HH_A001')
    append_string(legacy, 'fingerprint')
    legacy.extend(struct.pack('<I', 0))
    append_string(legacy, 'Tools/ArtGeneration/batch01_generate.py')
    append_string(legacy, 'Art/Exports/Batch01/HH_A001.glb.asset.json')
    legacy.extend(struct.pack('<Q', 0))
    (cooked / 'HH_A001.hasset').write_bytes(legacy)

    errors, counts = validate(tmp_path / 'Art' / 'Exports', cooked)
    assert counts['cooked_policy_assets'] == 0
    assert any('version 2 policy envelope' in error for error in errors)
