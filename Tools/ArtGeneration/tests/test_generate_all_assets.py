import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
import generate_all_assets as g

def test_batch_registry_is_complete():
    assert g.BATCHES==tuple(range(1,11))
