# Batch 05 Production Status

- Scope: HH_A201 through HH_A250
- Asset count: 50
- Focus: lobby/public-area furniture, kiosks and interaction props, luggage/bell carts, front-desk props, restaurant tables and banquettes
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- PBR encoding: corrected fractional RGBA -> uint8 path; sampled library palette remains fully non-white
- Lobby/self-check-in/information kiosk hardware now has role-specific readable details
- HH_A211-HH_A213 expose four `MOV_Wheel_*` nodes each and declare direct `ANSET_SERVICE_CART` dependencies
- Restaurant table/banquette silhouettes retain distinct capacity/corner profiles with physical floor supports
- Batch 05 completion contracts are covered by the shared 64-test art-generation suite
- Full-library CI baseline: 500/500 gameplay assets; 10 x 50 batches; 87/87 animation links; 0 deferred
- Geometry QA baseline: 500 assets; max_faces=3104; colors=86; nonwhite=1.0; anchors=184/184; contracts=PASS; placement=PASS
- Preview generation: 10/10 batch sheets
- Native Windows content pipeline: configure/build PASS; CTest 1/1 PASS; 591 generated asset records validated
- Verification implementation commit: `5ed45fbbb3640b03a6a2f885508806142c1ae4bb`
- Batch status: **PRODUCTION / generator-complete / CI-validated**

Generation command:

```bash
python Tools/ArtGeneration/batch05_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_05.json \
  Art/Exports/Batch05 \
  --package
```
