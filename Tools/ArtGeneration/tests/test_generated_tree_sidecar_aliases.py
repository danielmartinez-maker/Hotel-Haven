import json
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from generate_all_assets import validate_generated_tree


def _add_mesh(root, export_stem, asset_id, source_stem, dependencies=None):
    exports = root / 'Art' / 'Exports'
    source = root / 'Art' / 'Source' / f'{source_stem}.blend'
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_bytes(b'source')
    export = exports / f'{export_stem}.glb'
    export.parent.mkdir(parents=True, exist_ok=True)
    export.write_bytes(b'glb')
    sidecar = export.with_name(export.name + '.asset.json')
    sidecar.write_text(json.dumps({
        'asset_id': asset_id,
        'source': f'Art/Source/{source_stem}.blend',
        'dependencies': list(dependencies or []),
    }))
    return exports, sidecar


def test_compatible_plain_stem_sidecar_is_recognized_as_the_glb_alias(tmp_path):
    exports, canonical = _add_mesh(tmp_path, 'desk', 'HH_A001', 'desk')
    (exports / 'desk.asset.json').write_text(canonical.read_text())

    result = validate_generated_tree(tmp_path, exports)

    assert result['generated_asset_records'] == 1


@pytest.mark.parametrize(('field', 'conflicting_value'), [
    ('asset_id', 'HH_A002'),
    ('source', 'Art/Source/other.blend'),
    ('dependencies', ['HH_A999']),
])
def test_incompatible_plain_stem_alias_is_rejected(tmp_path, field, conflicting_value):
    exports, canonical = _add_mesh(tmp_path, 'desk', 'HH_A001', 'desk')
    legacy = json.loads(canonical.read_text())
    legacy[field] = conflicting_value
    (exports / 'desk.asset.json').write_text(json.dumps(legacy))

    with pytest.raises(RuntimeError, match='alias|sidecar|conflict|duplicate'):
        validate_generated_tree(tmp_path, exports)


def test_dotted_glb_name_does_not_hide_an_unpaired_short_name_sidecar(tmp_path):
    exports, _canonical = _add_mesh(tmp_path, 'cart.wheels', 'HH_A001', 'cart-wheels')
    (exports / 'cart.asset.json').write_text(json.dumps({
        'asset_id': 'HH_A002',
        'source': 'Art/Source/cart.blend',
        'dependencies': [],
    }))
    source = tmp_path / 'Art' / 'Source' / 'cart.blend'
    source.write_bytes(b'source')

    with pytest.raises(RuntimeError, match='missing export'):
        validate_generated_tree(tmp_path, exports)
