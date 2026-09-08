import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
import generate_all_assets as g

def test_batch_registry_is_complete():
    assert g.BATCHES==tuple(range(1,11))
    assert [g.generator_source(i) for i in g.BATCHES]==[f'Tools/ArtGeneration/batch{i:02d}_generate.py' for i in range(1,11)]

def test_sidecar_normalization_matches_exact_export_filename(tmp_path):
    export=tmp_path/'HH_A001.glb'; export.write_bytes(b'x')
    side=tmp_path/'HH_A001.asset.json'; side.write_text('{}')
    assert g.normalize_sidecars(tmp_path)==1
    assert not side.exists()
    assert (tmp_path/'HH_A001.glb.asset.json').exists()

def test_animation_sidecar_normalization(tmp_path):
    export=tmp_path/'AN_IDLE.anim.json'; export.write_text('{}')
    side=tmp_path/'AN_IDLE.anim.asset.json'; side.write_text('{}')
    assert g.normalize_sidecars(tmp_path)==1
    assert (tmp_path/'AN_IDLE.anim.json.asset.json').exists()
