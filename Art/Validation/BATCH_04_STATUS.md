# Batch 04 Production Status

- Scope: HH_A101-HH_A110 plus HH_A161-HH_A200
- Asset count: 50
- Focus: premium finish modules, bathroom/in-room fixtures, reception and lobby furniture
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Material encoding: uint8 PBR base-color factors; ceramic, wood, metal, linen, upholstery, glass, and finish colors validated without white clamping
- Geometry reload validation: 50/50 generated GLBs reload successfully
- Floor-contact validation: applicable furniture and service fixtures reach the placement plane through modeled supports, rails, legs, cords, or service chases
- Bathroom assets include readable fixture hardware and dedicated ceramic/electronics/glass material treatment
- Reception desks include counters, monitors, work surfaces, storage, and premium trim appropriate to each desk variant
- Concierge, bell, and host stations have distinct camera-readable service objects and silhouettes
- Lobby seating variants are differentiated; the curved sofa includes complete segmented seat, back, arm, cushion, and support geometry
- Art-generation regression suite at completion: 58/58 passed in GitHub Actions
- Full 500-asset manifest validation: passed
- Full 500-asset generation and geometry/palette/placement QA: passed
- Windows native content-pipeline build, tests, and generated-asset validation: passed
- Batch status: PRODUCTION / generator-complete / CI-validated

Generation command:

```bash
python Tools/ArtGeneration/batch04_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_04.json \
  Art/Exports/Batch04 \
  --package
```
