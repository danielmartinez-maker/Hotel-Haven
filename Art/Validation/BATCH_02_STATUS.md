# Batch 02 Production Status

- Scope: HH_A051 through HH_A100
- Asset count: 50
- Generated export format: GLB 2.0
- Units: meters
- Unified style: Architectural Diorama Realism v1
- Sidecars: HMG-070 schema 1, one per generated GLB
- Batch 02 generator tests: 8 coverage tests; full art-generation suite: 47/47 passed in CI
- Geometry reload validation: 50/50 Batch 02 assets; full-library geometry QA: 500/500 passed
- Finish-system differentiation: 30 named finish colorways with shared family roughness/metallic response
- Animated architectural assets: 7/7 expose `MOV_*` binding nodes, hinge/slide/overhead hierarchy, and direct shared animation-set dependencies
- Runtime animation assets remain shared-library data rather than per-mesh duplication; full-library dependency linking: 87/87, deferred: 0
- Full-library palette QA: 83 sampled RGB colors, non-white material ratio 1.0
- Native HMG-070 content pipeline: build/tests/asset validation passed; 591 generated asset records validated
- Preview generation: 10/10 batch preview sheets generated successfully
- Batch status: PRODUCTION / generator-complete / CI-validated

Generation command:

```bash
python Tools/ArtGeneration/batch02_generate.py \
  GameData/AssetDefinitions/Manifest/asset_batch_02.json \
  Art/Exports/Batch02 \
  --package
```

Verification baseline: Content Pipeline CI run 152 on commit `bb10edfd0c941c996a0bacdb6efde02304c794a1`.
