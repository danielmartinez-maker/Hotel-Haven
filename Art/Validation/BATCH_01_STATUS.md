# Batch 01 Production Status

- Scope: HH_A001 through HH_A050
- Asset count: 50
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Automated generator tests: 4 passed locally
- Geometry reload validation: 50/50 passed locally
- Animation preparation: every manifest-animated asset exposes one or more `MOV_*` nodes for animation binding
- Runtime animation clips: not yet authored; Batch 01 remains production-in-progress until mechanical clips are exported and verified in-game

Generation command:

```bash
python Tools/ArtGeneration/batch01_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_01.json \
  Art/Exports/Batch01 \
  --package
```
