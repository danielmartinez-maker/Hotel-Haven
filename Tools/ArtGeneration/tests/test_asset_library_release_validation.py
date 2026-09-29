import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ContentPipeline' / 'scripts'))

from validate_asset_library import batch_entry_columns, validate


def test_active_4200_catalog_passes_strict_release_manifest_validation():
    assert validate(ROOT) == []


def test_legacy_schema1_shards_inherit_only_the_master_column_contract():
    canonical = ['asset_id', 'display_name', 'subcategory', 'material_family',
                 'profile', 'animation_set', 'interaction_anchors']
    assert batch_entry_columns({'schema': 1}, canonical) == canonical
    assert batch_entry_columns({'schema': 1, 'columns': ['wrong']}, canonical) == ['wrong']
    assert batch_entry_columns({'schema': 2}, canonical) is None
