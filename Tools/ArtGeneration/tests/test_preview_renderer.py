import sys
from pathlib import Path

import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from render_asset_previews import preview_quality_failures, render_asset
from v2_asset_common import add, box


def test_preview_depth_order_keeps_raised_detail_in_front_of_its_base(tmp_path):
    scene = trimesh.Scene()
    add(scene, box((1.0, 1.0, .06), (0, 0, .03), 'MAT_STONE_LIGHT'), 'Backing')
    detail = box((.8, .05, .012), (0, 0, .067), 'MAT_BRASS_POLISHED')
    detail.apply_transform(trimesh.transformations.rotation_matrix(.785398, [0, 0, 1]))
    add(scene, detail, 'RaisedDetail')
    asset_path = tmp_path / 'stacked_detail.glb'
    asset_path.write_bytes(scene.export(file_type='glb'))

    pixels = np.asarray(render_asset(asset_path, 600), dtype=np.int16)
    brass_pixels = pixels[:, :, 0] > pixels[:, :, 2] * 1.5
    assert int(brass_pixels.sum()) > 2000


def test_guestroom_completion_variant_previews_reject_duplicate_visual_signatures():
    from PIL import Image

    image = Image.new('RGB', (150, 150), (150, 120, 90))
    rendered = {f'HH_A{i:04d}': image.copy() for i in range(4101, 4106)}

    failures = preview_quality_failures(rendered, expected_count=5)

    assert any('guestroom_completion_4101' in failure for failure in failures)


def test_preview_gate_rejects_tiny_changes_that_only_pass_the_finer_fingerprint():
    from PIL import Image, ImageDraw

    rendered = {}
    for variant, x in enumerate((40, 43, 46, 49, 52), start=1):
        image = Image.new('RGB', (150, 150), (150, 120, 90))
        ImageDraw.Draw(image).rectangle((x, 70, x + 2, 72), fill=(0, 0, 0))
        rendered[f'HH_A{4100 + variant:04d}'] = image

    failures = preview_quality_failures(rendered, expected_count=5)

    assert any('guestroom_completion_4101' in failure for failure in failures)


def test_preview_gate_accepts_variant_details_visible_at_the_coarse_resolution():
    from PIL import Image, ImageDraw

    rendered = {}
    for variant, x in enumerate((12, 40, 68, 96, 124), start=1):
        image = Image.new('RGB', (150, 150), (150, 120, 90))
        ImageDraw.Draw(image).rectangle((x, 64, x + 19, 83), fill=(0, 0, 0))
        rendered[f'HH_A{4100 + variant:04d}'] = image

    failures = preview_quality_failures(rendered, expected_count=5)

    assert not any('guestroom_completion_4101' in failure for failure in failures)
