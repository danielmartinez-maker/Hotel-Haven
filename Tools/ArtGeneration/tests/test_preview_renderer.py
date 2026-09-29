import sys
from pathlib import Path

import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'Tools' / 'ArtGeneration'))

from render_asset_previews import render_asset
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
