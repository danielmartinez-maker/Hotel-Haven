# Hotel Haven 500-Asset Library Release Audit V1

## Release gate

- Gameplay-facing assets: **500 / 500**
- Generated asset records (gameplay + animation support): **591**
- Animation dependencies linked: **87 / 87**
- Deferred animation dependencies: **0**
- Interaction anchors normalized: **184 / 184**
- Profile contract conformance: **PASS** with **0** failures
- Placement/pivot QC: **PASS** with **0** failures
- Semantic identity QC: **PASS** with **0** failures
- Preview QA: **PASS** across **10 / 10** production batches
- Contextual placement exceptions: **10** documented assets
- Floor-support hardening applied during current generation: **1** generated asset
- Mechanical animation coverage: **24 clips / 7 sets**
- Humanoid animation coverage: **47 clips / 11 sets / 2 skeletons**
- Geometry QC: **PASS** with **0** failures
- Maximum generated face count: **3968**
- Material palette: **86** unique sampled RGB colors; non-white ratio **1.0**
- Unresolved BLOCKER/CRITICAL/MAJOR issues: **0**

## Profile face budgets

| Profile | Maximum faces |
| --- | ---: |
| P_ARCH_ANIMATED | 5000 |
| P_ARCH_STATIC | 5000 |
| P_CHARACTER | 2500 |
| P_FURNITURE_STATIC | 4200 |
| P_INTERACTIVE_ANIMATED | 4200 |
| P_INTERACTIVE_PREFAB | 4200 |
| P_SERVICE_PROP_ANIMATED | 4200 |
| P_SMALL_PROP | 3000 |

## Batch status

| Batch | Status |
| --- | --- |
| Batch 01 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 02 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 03 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 04 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 05 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 06 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 07 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 08 | PRODUCTION_GENERATOR_SEMANTIC_PREVIEW_VALIDATED |
| Batch 09 | PRODUCTION_GENERATOR_SEMANTIC_PREVIEW_VALIDATED |
| Batch 10 | PRODUCTION_GENERATOR_SEMANTIC_PREVIEW_VALIDATED |

## Final hardening stacks

- **Q1 Semantic Library Hardening:** semantic-node contracts, generic-fallback rejection, and required variant geometry differentiation are active in the release validator.
- **Q2 Visual Release Hardening:** per-batch preview completeness, blank-preview detection, and required visual-variant differentiation are active in preview generation.

## Verification baseline

Implementation baseline: `084e9b96646ff9e22bb4c7225346988aa887d448`

GitHub Actions run: `34367200413`

- Python tests: **69 / 69 PASS**
- Windows MSVC build: **PASS**
- Native tests: **100% PASS**
- Native generated-asset validation: **591 assets**
- CI artifact package: **1196 files**, SHA-256 `b0faa00514ca747b554ed5b7f703755777d61dca23cf205b9c3248a3ea544ec1`

## Operational note

Generated GLB binaries and preview boards remain CI artifacts by design; the repository stores deterministic generators, manifests, validation policy, semantic/visual release gates, and this audit record.
