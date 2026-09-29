import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from architectural_modules_factory import build_asset
from semantic_asset_quality import variant_signature_failures
from validate_generated_geometry import audited_batch_statuses, release_audit_markdown


def signature(scene):
    return tuple(sorted((name, tuple(round(float(v), 4) for v in mesh.extents))
                        for name, mesh in scene.geometry.items()))


def test_ten_structural_families_have_distinct_functional_variants():
    for family in range(10):
        variants = []
        for variant in range(5):
            number = 801 + family * 5 + variant
            scene = build_asset(f'Module {family} {variant}', 'architectural_module',
                                'MAT_STONE_LIGHT', f'HH_A{number:03d}', 'P_ARCH_STATIC')
            assert len(scene.geometry) >= 3
            assert abs(float(scene.bounds[0][2])) < 0.001
            variants.append(signature(scene))
        assert len(set(variants)) == 5, family


def test_release_variant_audit_covers_new_modules():
    failures = variant_signature_failures({'HH_A801': ('same',), 'HH_A802': ('same',)})
    assert any('HH_A801 and HH_A802' in failure for failure in failures)


def test_release_audit_enumerates_active_batches():
    statuses = audited_batch_statuses({'16': 50, '17': 50}, (16, 17))
    assert statuses == {'16': 'PRODUCTION_GENERATOR_VALIDATED',
                        '17': 'PRODUCTION_GENERATOR_VALIDATED'}
    rendered = release_audit_markdown({'gameplay_asset_count': 850}, {}, statuses, 850)
    assert '| Batch 17 | PRODUCTION_GENERATOR_VALIDATED |' in rendered
