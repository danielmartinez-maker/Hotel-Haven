import sys
from pathlib import Path
import numpy as np
import trimesh

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
import generate_all_assets as g


def test_generator_source_uses_stable_batch_module_naming():
    assert g.generator_source(1) == 'Tools/ArtGeneration/batch01_generate.py'
    assert g.generator_source(16) == 'Tools/ArtGeneration/batch16_generate.py'


def test_sidecar_normalization_matches_exact_export_filename(tmp_path):
    export = tmp_path / 'HH_A001.glb'
    export.write_bytes(b'x')
    side = tmp_path / 'HH_A001.asset.json'
    side.write_text('{}')
    assert g.normalize_sidecars(tmp_path) == 1
    assert not side.exists()
    assert (tmp_path / 'HH_A001.glb.asset.json').exists()


def test_animation_sidecar_normalization(tmp_path):
    export = tmp_path / 'AN_IDLE.anim.json'
    export.write_text('{}')
    side = tmp_path / 'AN_IDLE.anim.asset.json'
    side.write_text('{}')
    assert g.normalize_sidecars(tmp_path) == 1
    assert (tmp_path / 'AN_IDLE.anim.json.asset.json').exists()


def test_trimesh_color_guard_preserves_float_255_palette_values():
    g.install_trimesh_color_guard()
    mat = trimesh.visual.material.PBRMaterial(baseColorFactor=np.array([112.0, 64.0, 32.0, 255.0]))
    assert mat.baseColorFactor.tolist() == [112, 64, 32, 255]
