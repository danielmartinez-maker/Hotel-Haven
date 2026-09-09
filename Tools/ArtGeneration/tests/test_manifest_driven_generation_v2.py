from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
sys.path.insert(0, str(ART))

import generate_all_assets as generation


def test_generation_loads_active_v2_manifest_instead_of_fixed_batch_registry():
    assert hasattr(generation, 'load_active_manifest'), 'generation must expose the active manifest loader'
    manifest = generation.load_active_manifest(ROOT)
    assert manifest.asset_count == 800
    assert manifest.batch_numbers == tuple(range(1, 17))
    assert manifest.batch_path(11).name == 'asset_batch_11.json'
    assert manifest.batch_path(16).name == 'asset_batch_16.json'


def test_generation_source_has_no_500_or_10_batch_scope_guards():
    source = (ART / 'generate_all_assets.py').read_text(encoding='utf-8')
    assert 'tuple(range(1, 11))' not in source
    assert 'expected 500 gameplay assets' not in source
    assert 'expected 500 normalized gameplay sidecars' not in source


def test_manifest_metadata_is_scoped_to_declared_batches_only(tmp_path):
    assert hasattr(generation, 'AssetManifest')
    definitions = tmp_path / 'GameData' / 'AssetDefinitions'
    manifest_dir = definitions / 'Manifest'
    manifest_dir.mkdir(parents=True)
    (manifest_dir / 'asset_batch_01.json').write_text(
        '{"groups":[{"family":"x","assets":[["HH_A001","One","x","MAT_WOOD_WARM","P_SMALL_PROP",null,[]]]}]}',
        encoding='utf-8',
    )
    (manifest_dir / 'asset_batch_99.json').write_text(
        '{"groups":[{"family":"x","assets":[["HH_A999","Stray","x","MAT_WOOD_WARM","P_SMALL_PROP",null,[]]]}]}',
        encoding='utf-8',
    )
    master = definitions / 'hotel_haven_asset_manifest_v2.json'
    master.write_text(
        '{"asset_count":1,"profiles":{"P_SMALL_PROP":{"units":"meters","lod_policy":"lod_small_prop","collision_policy":"none_or_simple_proxy","cutaway_policy":"normal"}},"batches":[{"batch":1,"path":"GameData/AssetDefinitions/Manifest/asset_batch_01.json","asset_count":1}]}',
        encoding='utf-8',
    )
    manifest = generation.AssetManifest(tmp_path, master)
    assets, _profiles = generation.gameplay_manifest_metadata(tmp_path, manifest)
    assert set(assets) == {'HH_A001'}
