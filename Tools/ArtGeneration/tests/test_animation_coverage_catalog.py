import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from asset_manifest import load_active_manifest
from generator_registry import GENERATOR_FAMILIES


MOVING_PRODUCT_GROUPS = (
    (3501, 'P_ARCH_ANIMATED', 'ANSET_MECH_SLIDING_DOOR', 'MOV_SlidingPanel'),
    (3541, 'P_ARCH_ANIMATED', 'ANSET_MECH_ELEVATOR', 'MOV_ElevatorDoor'),
    (3656, 'P_SERVICE_PROP_ANIMATED', 'ANSET_SERVICE_CART', 'MOV_Wheel'),
    (3796, 'P_SERVICE_PROP_ANIMATED', 'ANSET_SERVICE_CART', 'MOV_Wheel'),
    (3951, 'P_ARCH_ANIMATED', 'ANSET_MECH_SLIDING_DOOR', 'MOV_PocketPanel'),
    (4026, 'P_SERVICE_PROP_ANIMATED', 'ANSET_SERVICE_CART', 'MOV_Wheel'),
)


def _manifest_rows():
    return {
        int(row[0].removeprefix('HH_A')): (path, row)
        for _family, path, row in load_active_manifest(ROOT).iter_rows()
    }


def test_moving_catalog_variants_declare_compatible_animation_profiles_and_sets():
    rows = _manifest_rows()
    for start, profile, animation_set, _node_prefix in MOVING_PRODUCT_GROUPS:
        for number in range(start, start + 5):
            _path, row = rows[number]
            assert row[4] == profile, f'{row[0]} must use the animated profile {profile}'
            assert row[5] == animation_set, f'{row[0]} must bind {animation_set}'


def test_moving_catalog_variants_emit_nodes_targeted_by_their_animation_sets():
    active = load_active_manifest(ROOT)
    entries = {entry.batch: entry for entry in active.batch_entries}
    rows = _manifest_rows()
    for start, _profile, _animation_set, node_prefix in MOVING_PRODUCT_GROUPS:
        for number in range(start, start + 5):
            path, row = rows[number]
            batch = int(Path(path).stem.rsplit('_', 1)[1])
            generator = GENERATOR_FAMILIES[entries[batch].generator_family].builder
            scene = generator(row[1], row[2], row[3], row[0], row[4])
            assert any(str(name).startswith(node_prefix) for name in scene.geometry), (
                f'{row[0]} has no node matched by {row[5]}'
            )
