# Batch 02 Production Status

- Scope: HH_A051 through HH_A100
- Asset count: 50
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Automated generator tests: 4 passed locally
- Geometry reload validation: 50/50 passed locally
- Animated architectural assets expose `MOV_*` nodes for animation binding
- Runtime mechanical animation clips remain shared-library work, not per-mesh duplication
- Batch status: PRODUCTION / geometry-complete / animation-binding-ready

Generation command:

```bash
python Tools/ArtGeneration/batch02_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_02.json \
  Art/Exports/Batch02 \
  --package
```
