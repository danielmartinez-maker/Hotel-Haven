from __future__ import annotations

import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GEN = ROOT / 'Tools' / 'ArtGeneration' / 'mechanical_animation_generate.py'

spec = importlib.util.spec_from_file_location('mechanical_animation_generate', GEN)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def test_exact_shared_mechanical_set_and_clip_counts():
    sets = mod.build_sets()
    assert set(sets) == {
        'ANSET_MECH_DOOR','ANSET_MECH_SLIDING_DOOR','ANSET_MECH_REVOLVING_DOOR',
        'ANSET_MECH_ELEVATOR','ANSET_MECH_CURTAIN','ANSET_SERVICE_CART'
    }
    assert sum(len(s['clips']) for s in sets.values()) == 20


def test_generator_emits_clip_and_set_payloads_with_sidecars(tmp_path):
    clips, sets = mod.generate(tmp_path)
    assert clips == 20 and sets == 6
    assert len(list(tmp_path.glob('AN_*.anim.json'))) == 20
    assert len(list(tmp_path.glob('AN_*.anim.asset.json'))) == 20
    assert len(list(tmp_path.glob('ANSET_*.animset.json'))) == 6
    assert len(list(tmp_path.glob('ANSET_*.animset.asset.json'))) == 6


def test_animation_sidecars_are_hmg070_compatible(tmp_path):
    mod.generate(tmp_path)
    for path in tmp_path.glob('*.asset.json'):
        data = json.loads(path.read_text())
        assert data['schema'] == 1
        assert data['units'] == 'meters'
        assert data['lod_policy'] == 'animation_shared'
        assert data['collision_policy'] == 'none'
        assert data['material_slots'] == []
        assert data['asset_type'] in ('AnimationClipAsset','AnimationSetAsset')


def test_open_close_pairs_are_reversible():
    sets = mod.build_sets()
    pairs = [
        ('ANSET_MECH_DOOR','AN_DOOR_OPEN','AN_DOOR_CLOSE'),
        ('ANSET_MECH_SLIDING_DOOR','AN_SLIDE_OPEN','AN_SLIDE_CLOSE'),
        ('ANSET_MECH_ELEVATOR','AN_ELEVATOR_OPEN','AN_ELEVATOR_CLOSE'),
        ('ANSET_MECH_CURTAIN','AN_CURTAIN_OPEN','AN_CURTAIN_CLOSE'),
    ]
    for set_id, open_id, close_id in pairs:
        o = sets[set_id]['clips'][open_id]['channels'][0]['keyframes']
        c = sets[set_id]['clips'][close_id]['channels'][0]['keyframes']
        assert o[0]['value'] == c[-1]['value']
        assert o[-1]['value'] == c[0]['value']
