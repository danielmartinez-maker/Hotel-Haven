import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
DEFINITIONS = ROOT / 'GameData' / 'AssetDefinitions'
MANIFEST_DIR = DEFINITIONS / 'Manifest'


def _rows(batch_data):
    return [row for group in batch_data['groups'] for row in group['assets']]


def test_v2_master_manifest_declares_800_assets_in_16_batches():
    master = json.loads((DEFINITIONS / 'hotel_haven_asset_manifest_v2.json').read_text(encoding='utf-8'))
    assert master['schema'] == 2
    assert master['asset_count'] == 800
    assert len(master['batches']) == 16
    assert [entry['batch'] for entry in master['batches']] == list(range(1, 17))
    assert all(entry['asset_count'] == 50 for entry in master['batches'])
    assert master['quality_contract'] == 'GameData/AssetDefinitions/asset_quality_contract_v2.json'


def test_v2_batch_rows_cover_exactly_hh_a001_through_hh_a800():
    master = json.loads((DEFINITIONS / 'hotel_haven_asset_manifest_v2.json').read_text(encoding='utf-8'))
    ids = []
    for entry in master['batches']:
        batch_path = ROOT / entry['path']
        data = json.loads(batch_path.read_text(encoding='utf-8'))
        rows = _rows(data)
        assert data['batch'] == entry['batch']
        assert data['asset_count'] == 50
        assert len(rows) == 50
        ids.extend(row[0] for row in rows)
    assert len(ids) == 800
    assert len(set(ids)) == 800
    assert ids == [f'HH_A{i:03d}' for i in range(1, 801)]


def test_v2_rows_reference_known_profiles_and_quality_contract_is_resolvable():
    master = json.loads((DEFINITIONS / 'hotel_haven_asset_manifest_v2.json').read_text(encoding='utf-8'))
    contract_path = ROOT / master['quality_contract']
    contract = json.loads(contract_path.read_text(encoding='utf-8'))
    assert contract['schema'] == 2
    assert contract['contract_id'] == 'HH_ASSET_QUALITY_V2'
    assert contract['style_contract'] == master['style_contract']
    assert contract['preview']['canonical_camera']['projection'] == 'orthographic'
    assert 'meshy_authoring' in contract

    profiles = set(master['profiles'])
    for entry in master['batches']:
        data = json.loads((ROOT / entry['path']).read_text(encoding='utf-8'))
        for row in _rows(data):
            assert row[4] in profiles, row


def test_v2_preserves_v1_logical_ids_and_adds_300_new_ids():
    v1 = json.loads((DEFINITIONS / 'hotel_haven_asset_manifest_v1.json').read_text(encoding='utf-8'))
    v2 = json.loads((DEFINITIONS / 'hotel_haven_asset_manifest_v2.json').read_text(encoding='utf-8'))
    assert v1['asset_count'] == 500
    assert v2['asset_count'] - v1['asset_count'] == 300
    assert [entry['path'] for entry in v2['batches'][:10]] == [entry['path'] for entry in v1['batches']]
