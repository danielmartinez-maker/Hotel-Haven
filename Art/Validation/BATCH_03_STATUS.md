# Batch 03 Production Status

- Scope: HH_A111 through HH_A160
- Asset count: 50
- Focus: guest-room beds, casegoods, desks, seating, storage, televisions, mirrors, curtains, rug
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Automated generator tests: 4 passed locally
- Geometry reload validation: 50/50 passed locally
- Curtains HH_A157-HH_A159 expose `MOV_CurtainPanel_*` nodes for shared animation binding
- Batch status: PRODUCTION / geometry-complete / curtain-binding-ready

Generation command:

```bash
python Tools/ArtGeneration/batch03_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_03.json \
  Art/Exports/Batch03 \
  --package
```
