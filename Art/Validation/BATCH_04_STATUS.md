# Batch 04 Production Status

- Scope: HH_A101-HH_A110 plus HH_A161-HH_A200
- Asset count: 50
- Focus: premium finish modules, bathroom/in-room fixtures, reception and lobby furniture
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Automated generator tests: 4 passed locally
- Geometry reload validation: 50/50 passed locally
- Bathroom fixtures use dedicated ceramic/electronics/glass families; lobby furniture reuses Batch 03 proportion grammar
- Batch status: PRODUCTION / geometry-complete

Generation command:

```bash
python Tools/ArtGeneration/batch04_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_04.json \
  Art/Exports/Batch04 \
  --package
```
