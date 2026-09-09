# Batch 03 Production Status

- Scope: HH_A111 through HH_A160
- Asset count: 50
- Focus: guest-room beds, casegoods, desks, seating, storage, televisions, mirrors, curtains, rug
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Material encoding: uint8 PBR base-color factors; palette validated without white clamping
- Geometry reload validation: 50/50 generated GLBs reload successfully
- Floor-contact validation: applicable beds, crib, sofas, tables, chairs, casegoods, and interactive furniture reach the placement plane through modeled supports or service geometry
- Curtains HH_A157-HH_A159 expose `MOV_CurtainPanel_*` nodes with pleats, rings, blackout backing, and sheer overlays parented to the moving panels
- Curtain sidecars declare the shared `ANSET_MECH_CURTAIN` dependency directly
- Casegoods include camera-readable doors, handles, shelves, hardware, minibar/coffee features, and TV-console details
- Art-generation regression suite at completion: 58/58 passed in GitHub Actions
- Full 500-asset manifest validation: passed
- Full 500-asset generation and geometry/palette/placement QA: passed
- Windows native content-pipeline build, tests, and generated-asset validation: passed
- Batch status: PRODUCTION / generator-complete / CI-validated

Generation command:

```bash
python Tools/ArtGeneration/batch03_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_03.json \
  Art/Exports/Batch03 \
  --package
```
