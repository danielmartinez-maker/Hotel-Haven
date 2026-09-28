# Hotel Haven 4,200-Asset Library Expansion Design

## Status

Approved conversational design translated into repository authority on 2026-09-28.

## Goal

Expand Hotel Haven from the current A700 runtime-integrated asset milestone to a minimum of 4,200 canonical gameplay assets while preserving deterministic generation, semantic quality gates, animation dependency integrity, selective runtime loading, and exact release-package auditing.

The finished canonical gameplay namespace is **HH_A001 through HH_A4200**, with no gaps or duplicate IDs.

## Baseline and lineage

Implementation starts from `assets/runtime-700-integration-2026-09-24`, because that lineage already provides:

- the A001-A700 runtime milestone;
- deterministic V2 semantic asset factories;
- exact 700-asset cooking, audit, staging, smoke, and package contracts;
- selective shipping runtime loading;
- bounded runtime catalog loading for audit/smoke;
- the v2 quality contract and variant checks;
- runtime bindings and a scalable `WorldAssetSet` catalog.

The stronger reusable content-pipeline pieces from `feature/asset-pipeline-v2-800-assets` are ported forward where they improve this lineage without importing unrelated stale game code. In particular:

- manifest-driven batch enumeration;
- the canonical A701-A800 batch definitions;
- the manifest loader abstraction;
- quality-enrichment hooks;
- service/amenity/character V2 generation patterns.

No unrelated simulation, UI, economy, persistence, or service code is to be transplanted from the older branch.

## Canonical library size and family targets

The minimum final gameplay library is exactly **4,200 assets** for this milestone. Animation support assets such as skeletons, clips, and animation-set records are tracked separately and do not change the 4,200 gameplay-asset count.

Family targets:

| Family | Final target |
| --- | ---: |
| architecture_construction | 650 |
| finish_systems | 200 |
| guest_room_furniture_fixtures | 700 |
| front_of_house_public | 500 |
| restaurant_bar_food_service | 500 |
| housekeeping_maintenance_logistics | 450 |
| amenities_events | 400 |
| decor_clutter_signage | 350 |
| exterior_landscaping | 250 |
| guests_staff | 200 |
| **Total** | **4,200** |

These counts include existing assets. They are targets for canonical manifest membership, not runtime preload counts.

## Batch structure

Production remains sharded into deterministic batches of 50 gameplay assets.

- A001-A700: existing batches 01-14.
- A701-A800: ported batches 15-16 from the V2 800-asset branch, reconciled against the A700 runtime lineage.
- A801-A4200: batches 17-84.
- Each batch contains exactly 50 gameplay assets.
- Master-manifest enumeration is authoritative. Merely placing a shard file in the directory must not make it active.

The active manifest records:

- total gameplay asset count;
- ordered batch list;
- exact path and expected count for each batch;
- profile contracts;
- family targets;
- style and quality contracts;
- runtime and authority notes.

## Generator architecture

The expansion must not require one bespoke Python module per new batch.

A generator registry maps a manifest-declared `generator_family` to a focused factory implementation. Batch shards remain content data; generator behavior remains code.

Required generator families:

1. architecture
2. finish_system
3. guestroom
4. bathroom
5. public_space
6. office_business
7. restaurant_dining
8. bar_beverage
9. kitchen_back_of_house
10. housekeeping
11. engineering_maintenance
12. logistics_receiving
13. amenities_spa
14. amenities_gym
15. amenities_pool
16. conference_events
17. decor_clutter
18. signage_wayfinding
19. exterior_landscaping
20. character_guest
21. character_staff

Factories may share lower-level primitive helpers, but each generator family owns recognizable silhouettes and semantic details for its domain.

`generate_all_assets.py` reads only the active master manifest and dispatches batches through the registry. No hardcoded `range(1, N)` or hardcoded gameplay count is allowed in the generation path.

## Asset-quality standard

The existing `HH_ASSET_QUALITY_V2` philosophy remains binding and may be strengthened but not weakened.

Every gameplay asset must have:

- a canonical manifest entry;
- one generated/exported payload;
- one normalized sidecar;
- valid units, LOD, collision, cutaway, pivot, material, profile, and interaction metadata;
- semantic nodes appropriate to its asset family;
- non-empty, finite, non-degenerate geometry;
- deterministic provenance;
- a generated preview;
- release-audit participation.

Manufactured objects must preserve small, consistent edge treatment and management-camera readability. Microdetail that does not survive the intended camera scale belongs in material response rather than geometry.

No asset may pass solely because its filename or material differs from another asset.

## Variant integrity

Variant groups must remain visibly and structurally distinct.

Required comparisons include:

- geometry signature;
- bounds ratio;
- semantic-node set;
- silhouette signature;
- perceptual preview signature where available.

Required variants must not be:

- name-only variants;
- material-only variants;
- scale-only variants unless scale itself represents a real product class with distinct proportions and functional use;
- trivial primitive rearrangements that are not visibly distinguishable from the gameplay camera.

High-volume families must be built from curated parameter spaces with constraints that force meaningful differences in silhouette, footprint, role detail, and semantic structure.

## Curated hybrid production model

The library uses three content tiers.

### Tier 1: deterministic modular assets

Architecture, finish systems, standard furniture, fixtures, shelving, tables, chairs, counters, doors, storage, signage, landscaping modules, and ordinary service props are produced by deterministic semantic factories.

### Tier 2: purpose-built procedural hero assets

Important silhouettes and frequently seen objects receive dedicated builders rather than generic composites. This includes reception furniture, premium beds, lobby statement pieces, restaurant/bar hero pieces, kitchen equipment, major service equipment, spa/gym/pool equipment, and distinctive exterior features.

### Tier 3: optional external authoring

External 3D generation/DCC workflows may be used for selected hero assets only after explicit authorization for any paid generation. External authoring is offline-only and must normalize into the same sidecar, quality, provenance, and deterministic `.hasset` cooking contracts. External authoring never becomes a runtime dependency.

The 4,200-asset milestone must be achievable without paid external generation.

## Animation design

Animation support is dependency-driven and separate from the gameplay asset count.

Any asset whose profile or manifest declares an animation set must resolve to a generated animation-set asset with zero deferred dependencies at release.

Mechanical coverage expands to include, where applicable:

- hinged, sliding, pocket, revolving, overhead, and service doors;
- elevator doors and related moving assemblies;
- curtains and movable partitions;
- cart and trolley wheels;
- luggage and receiving equipment;
- laundry washer/dryer moving assemblies;
- housekeeping machines;
- engineering equipment with readable moving parts;
- gym cardio/rowing/cable equipment;
- selected kitchen/service equipment;
- pool/spa equipment where movement is visually required.

Humanoid animation coverage expands beyond generic locomotion to role/context sets for:

- guest locomotion and idle families;
- seated guest behavior;
- luggage handling;
- reception/concierge;
- bell service;
- housekeeping/cleaning;
- maintenance/inspection/repair;
- restaurant service;
- bar service;
- kitchen work;
- banquet/event work;
- spa service;
- fitness/pool attendants;
- security;
- valet;
- office/admin work.

Characters continue to use deterministic skeleton and animation-set IDs. Required bone/node contracts remain release-blocking.

## Character expansion

The canonical `guests_staff` family target is 200 assets.

New character variants must increase actual visual and role diversity through combinations of:

- age band;
- body-proportion family within supported bounds;
- clothing silhouette;
- role uniform;
- luggage/accessory attachment set;
- hair/head silhouette;
- palette;
- guest segment or staff role.

Character variants must still satisfy the shared hierarchy and height contracts. Variation cannot rely on palette changes alone.

## Runtime policy

The shipping client does **not** decode all 4,200 gameplay assets at startup.

Runtime behavior remains:

- selective loading for assets reachable from current shipping world bindings or currently instantiated hotel content;
- bounded/catalog loading for smoke tests and renderer audits;
- full cook/audit/stage/package coverage for the canonical milestone;
- no art metadata taking simulation, construction, economy, placement, task, or interaction authority away from owning gameplay systems.

The runtime asset registry must remain atomic on failed loads and deterministic in enumeration.

## Manifest-derived count authority

Hardcoded release counts such as 500, 700, 745, 800, or 4200 must not be duplicated throughout scripts and CMake.

A single authoritative manifest-derived gameplay count is used to validate:

`manifest count == generated gameplay assets == normalized gameplay sidecars == geometry-QA gameplay population == cooked gameplay assets == staged gameplay assets == packaged gameplay assets`

Animation support records are validated separately.

CMake staging and release verification receive or derive the expected gameplay count from authoritative generated metadata rather than embedding an independent number.

## Validation and CI

CI must continue to fail closed.

Required gates include:

1. master manifest schema and batch-order validation;
2. exact contiguous unique ID set HH_A001-HH_A4200;
3. exact 50 assets per declared batch;
4. family-target reconciliation;
5. deterministic generation;
6. sidecar/profile normalization;
7. animation dependency linking with zero deferred dependencies;
8. semantic-node quality validation;
9. geometry/pivot/floor-contact validation;
10. variant distinctness validation;
11. preview nonblank/frame-occupancy validation;
12. cooking of the full canonical gameplay library;
13. renderer-safe runtime library audit;
14. staging exact-count verification;
15. selective runtime loading tests;
16. bounded/full smoke catalog coverage;
17. package-integrity verification of the canonical gameplay count.

Tests or audits must not be weakened, bypassed, made advisory, or given larger tolerances merely to make the expansion pass.

## Performance constraints

Scaling the content library must not make ordinary game startup decode the complete library.

Generation and validation may process the full library offline. Runtime performance protection includes:

- selective asset decode;
- catalog metadata separated from decoded geometry;
- deterministic bounded loading for diagnostics;
- no per-frame manifest parsing;
- no repeated directory scans in steady-state gameplay.

CI may shard expensive asset-generation or validation work by declared batch ranges if the final release gate still verifies the complete 4,200-asset set.

## Repository/storage policy

Generated GLB binaries, cooked `.hasset` payloads, and preview boards remain generated artifacts unless the repository's existing policy explicitly stores them. The repository stores the deterministic source of truth:

- canonical manifests;
- generator code;
- animation definitions/generators;
- validation policy;
- release audit summaries;
- selected visual references where intentionally versioned.

## Integration sequence

Implementation order:

1. establish manifest-driven authority on the A700 runtime branch;
2. port and validate A701-A800;
3. add generator-family registry and scalable batch dispatch;
4. define A801-A4200 canonical catalog;
5. implement/extend generator families;
6. expand mechanical and humanoid animation sets;
7. run quality enrichment and variant checks across all batches;
8. make cooking/staging/package counts manifest-derived;
9. preserve selective runtime loading while exposing the full catalog to audits;
10. run full generation, geometry, animation, cook, runtime, smoke, and package verification.

## Acceptance criteria

The milestone is complete only when all of the following are true:

- canonical gameplay namespace is exactly HH_A001-HH_A4200;
- all 84 declared batches contain exactly 50 assets;
- family targets reconcile to 4,200;
- all gameplay assets generate successfully with valid normalized sidecars;
- every declared animation dependency resolves;
- zero release-blocking geometry, semantic, placement, profile, variant, or animation failures remain;
- all 4,200 gameplay assets cook into renderer-safe runtime form;
- staging and release package contain exactly the manifest-declared gameplay count;
- shipping runtime still selectively decodes only required assets;
- smoke/audit paths can enumerate and validate the complete milestone;
- no simulation/gameplay authority boundary is weakened;
- no CI quality gate is weakened to achieve the count.

## Explicit non-goals

This expansion does not by itself:

- add new simulation mechanics;
- make every new asset placeable before the owning construction/venue system exposes placement authority;
- preload all assets at startup;
- require paid external asset generation;
- replace deterministic generation with opaque online dependencies;
- redesign unrelated UI, economy, persistence, or service systems.
