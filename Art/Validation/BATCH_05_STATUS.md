# Batch 05 Production Status

- Scope: HH_A201 through HH_A250
- Asset count: 50
- Focus: lobby/public-area furniture, kiosks and interaction props, luggage/bell carts, first restaurant tables and banquettes
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Automated generator tests: 4 passed locally
- Geometry reload validation: 50/50 passed locally
- HH_A211-HH_A213 expose four `MOV_Wheel_*` nodes each and bind to shared `ANSET_SERVICE_CART`
- Batch status: PRODUCTION / geometry-complete / cart-animation-binding-ready

Generation command:

```bash
python Tools/ArtGeneration/batch05_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_05.json \
  Art/Exports/Batch05 \
  --package
```
