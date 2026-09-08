# Hotel Haven 500-Asset Library Design

## Purpose

Define and produce the first canonical 500-asset content library for Hotel Haven while preserving one unified visual language, deterministic metadata, reusable animation systems, and compatibility with the existing HMG-070 asset production pipeline.

This specification is authoritative for the 500-asset library. HMG-070 remains authoritative for pipeline mechanics, cooking, validation, runtime/source separation, lifecycle states, and supported runtime asset types.

## Visual Direction Lock

All assets must follow the previously approved Hotel Haven visual direction:

- Architectural Diorama Realism with restrained stylization.
- Orthographic 2.5D/isometric presentation designed for readable cutaway interiors.
- Soft simplified PBR materials rather than photorealistic scanning.
- Clean silhouettes and controlled geometry density.
- Warm practical lighting, soft shadows, readable ambient occlusion.
- Restrained texture noise and high material separation.
- Slightly stylized human proportions with readable role silhouettes.
- Outlines only when readability materially improves.
- No mixing of unrelated painterly, pixel-art, low-poly, toon, or hyperreal styles.

Every asset must be reviewable against a shared lighting rig, camera, neutral background, scale reference, and material calibration scene.

## Count and Scope

The production library contains exactly 500 primary asset entries. Animation clips, shared skeletons, material instances, textures, VFX helpers, and generated intermediate files do not consume the 500-primary-asset count unless they are explicitly listed as a primary catalog entry.

| Family | Count |
| --- | ---: |
| Architecture & construction | 70 |
| Floors, walls & finish systems | 40 |
| Guest-room furniture & fixtures | 75 |
| Lobby / front-of-house / public areas | 60 |
| Restaurant, bar & food service | 45 |
| Housekeeping, maintenance & logistics | 55 |
| Amenities & event spaces | 35 |
| Decor, clutter & signage | 45 |
| Exterior & landscaping | 25 |
| Guests & staff | 50 |
| **Total** | **500** |

## Production Philosophy

The library uses a modular authored-library approach.

- Assets are authored as reusable components, not 500 unrelated one-off meshes.
- Shared materials and trim systems are preferred over unique shaders.
- Modular architectural pieces must tile cleanly and preserve world-scale consistency.
- Furniture families may share silhouettes, materials, and construction language while remaining visually distinct.
- Character variants must derive from shared skeleton families and reusable animation sets.
- Mechanical animation is authored only where gameplay or visual feedback requires it.

## Repository Placement

Existing repository structure is preserved:

```text
/Art
  /Source
  /Reference
  /Exports
  /Generated
  /Validation
/GameData
  /AssetDefinitions
/Tools
  /ContentPipeline
```

This work adds:

```text
/GameData/AssetDefinitions
  hotel_haven_asset_manifest_v1.json
  animation_sets_v1.json
  material_families_v1.json
  art_style_contract_v1.json
  production_batches_v1.json

/Art/Reference
  /StyleLock
  /MaterialCalibration
  /CharacterProportions
  /LightingCalibration
```

Source assets never bypass HMG-070 cooking.

## Asset Identity

Primary assets use stable IDs:

`HH_<FAMILY>_<NNN>_<NAME>`

Examples:

- `HH_ARCH_001_INTERIOR_WALL_CREAM`
- `HH_ROOM_014_KING_BED_BRASS`
- `HH_FOH_022_RECEPTION_DESK_GRAND`
- `HH_CHAR_003_GUEST_BUSINESS_A`

The numeric index is stable after assignment. Renaming display labels must not silently reassign IDs.

## Shared Art Contract

### Scale

- World units are meters for all 3D classes.
- Architectural dimensions must align to the canonical construction grid used by gameplay.
- Furniture dimensions must remain believable relative to human reference proportions.
- Character scale is fixed across all guest and staff variants.

### Pivot Rules

- Floor-standing props: pivot centered on footprint at floor level.
- Wall-mounted props: pivot at wall contact plane and logical placement center.
- Doors: pivot at hinge axis unless mechanically incompatible.
- Rotating props: pivot on mechanical rotation axis.
- Character root: centered between feet at ground plane.
- Pivot exceptions require metadata justification.

### Geometry Language

- Moderate bevels on exposed hard edges.
- No razor-sharp major furniture edges unless materially justified.
- Curvature and bevel width should read consistently at management-camera distance.
- Small decorative geometry that cannot be read at expected zoom should move into textures or normal detail.

### Material Language

Canonical families include:

- warm oak
- dark walnut
- painted cream wood
- brass
- brushed steel
- blackened metal
- polished marble light
- polished marble dark
- limestone
- ceramic tile
- carpet standard
- carpet luxury
- leather warm brown
- upholstery burgundy
- upholstery muted blue
- hospitality white linen
- glass clear
- glass smoked
- greenery foliage
- service plastic/utility polymer

Material instances may vary hue/value/roughness within family limits but must not establish incompatible shading models.

### Texture Language

- Detail frequency must remain readable at normal gameplay zoom.
- Avoid high-frequency photographic noise.
- Fabric, stone, and wood should read through broad forms first, microdetail second.
- Wear and dirt are restrained and context-sensitive; default hotel spaces are maintained rather than distressed.

### Lighting Reference

Every asset review render uses the same neutral calibration scene with:

- orthographic/isometric gameplay-compatible camera;
- neutral mid-value backdrop;
- warm key light;
- cool-neutral fill;
- soft contact shadow/AO;
- no cinematic depth-of-field;
- no grading that hides material defects.

## Architecture Rules

Architecture assets include walls, partitions, doors, windows, facade modules, stairs, railings, arches, columns, elevators, service access, vents, trim, canopies, and structural decorative modules.

Requirements:

- modular snapping;
- consistent wall thickness;
- valid cutaway metadata;
- floor/ceiling seams hidden or intentionally trimmed;
- doors and windows align to canonical wall openings;
- foreground pieces comply with renderer cutaway policies.

## Furniture and Prop Rules

Furniture must have:

- credible hotel-scale footprint;
- clear gameplay silhouette;
- canonical placement pivot;
- collision policy appropriate to navigation and interaction;
- interaction anchors when used by guests/staff;
- optional socket points for carried/placed objects.

Variants should reuse material families but avoid palette monotony.

## Character System

The 50 primary character assets are divided between guest archetypes and staff roles.

### Shared Skeleton Strategy

Use as few compatible skeleton families as practical:

- adult human standard;
- adult human broad/tall variant only if deformation quality requires it;
- child skeleton only if children are in shipping scope.

Clothing, hair, accessories, and body variation must preserve animation compatibility wherever possible.

### Guest Archetype Coverage

Guest production should visually support at least:

- business traveler;
- leisure tourist;
- couple;
- family adult;
- family child if shipping scope requires children;
- backpacker/budget traveler;
- luxury/VIP guest;
- elderly guest;
- influencer/content traveler;
- long-stay guest.

### Staff Role Coverage

Staff visuals should cover:

- receptionist/front desk;
- concierge;
- bell staff;
- housekeeping;
- laundry;
- waiter/server;
- bartender;
- chef/kitchen;
- maintenance/engineering;
- security;
- manager;
- spa/amenity staff where applicable.

## Animation Architecture

Animations do not consume the 500-primary-asset count. They are reusable runtime animation assets referenced by character and mechanical assets.

### Core Human Locomotion

Required shared clips:

- idle neutral variants;
- idle impatient/tired variants where psychology needs them;
- walk;
- brisk walk;
- turn-in-place;
- sit down;
- seated idle;
- stand up;
- sleep/rest posture transitions where applicable.

### Guest Interaction Clips

Reusable clips include:

- carry suitcase;
- roll suitcase;
- use door;
- press elevator control;
- enter/exit elevator;
- check-in/check-out counter interaction;
- wait in queue;
- inspect room/object;
- talk/listen;
- eat;
- drink;
- use phone;
- sit lounge;
- react happy;
- react annoyed;
- react angry;
- react surprised;
- complain;
- celebrate/satisfied gesture.

### Staff Interaction Clips

Reusable role clips include:

- type/use desk terminal;
- hand over key/card;
- carry luggage;
- push bell cart;
- push cleaning cart;
- vacuum;
- mop;
- make bed;
- replace towels/linen;
- clean surface;
- collect trash;
- carry tray;
- serve food/drink;
- cook/prep;
- bartend/pour;
- repair/inspect equipment;
- use toolbox;
- security observe/intervene;
- manager inspect/clipboard interaction.

### Mechanical Animation

Mechanical animation is required only where state readability benefits gameplay. Candidates include:

- hinged doors;
- sliding doors;
- revolving doors;
- guest elevators;
- service elevators;
- curtains/blinds;
- luggage carts where wheel motion is visible;
- selected kitchen/service equipment;
- fountains/water features;
- television/display state loops;
- ceiling fans or equivalent rotating fixtures if used.

Each animated asset declares its animation set, state names, loop policy, interaction anchor, and synchronization requirements.

## Interaction Metadata

Interactive assets may declare:

- standing anchor;
- sitting anchor;
- service-side anchor;
- guest-side anchor;
- pickup socket;
- placement socket;
- queue anchor;
- maintenance anchor;
- animation facing vector.

Art metadata must not define gameplay economics, service times, room validity, or simulation authority.

## LOD and Readability

LOD policy should preserve silhouette and material identity before microdetail.

- Hero/public-space assets can retain additional detail.
- Repeated room furniture and service props prioritize instancing efficiency.
- Characters preserve head, torso, limb, clothing, and carried-item readability at management zoom.
- Tiny clutter may collapse to simplified geometry at distance.

Exact triangle and texture budgets remain governed by later asset-type specifications or measured performance budgets; this design does not invent unsupported hard limits.

## Cutaway and Occlusion

Assets use HMG-070 legal cutaway values:

- `normal`
- `fade_when_foreground`
- `hide_upper_section`
- `never_cut`

Architecture and tall furniture must be tagged deliberately. Foreground occluders that commonly block room readability should prefer cutaway-compatible construction.

## Manifest Fields

Every primary manifest entry must include at minimum:

- primary index 1-500;
- stable asset ID;
- family;
- subtype;
- display name;
- runtime asset type;
- source path target;
- export path target;
- style contract version;
- material family references;
- dimensions or expected scale class;
- pivot policy;
- collision policy;
- LOD policy;
- cutaway policy where relevant;
- animation requirement;
- animation set reference where relevant;
- interaction anchors required;
- lifecycle state;
- production batch;
- tags;
- dependencies.

## Material Families File

`material_families_v1.json` centralizes approved visual materials and prevents uncontrolled one-off material proliferation. It records canonical family IDs, intended surfaces, base-value ranges, roughness character, metallic behavior, and permitted variation notes.

## Animation Sets File

`animation_sets_v1.json` defines shared animation groups rather than copying clip lists into every character. Character entries reference sets by stable ID.

## Style Contract File

`art_style_contract_v1.json` contains machine-readable style constraints that can be validated automatically where practical, including:

- contract version;
- world units;
- canonical pivot policies;
- allowed cutaway policies;
- allowed material family IDs;
- naming grammar;
- review-camera identifier;
- calibration-scene identifier;
- character skeleton-family IDs.

Creative quality remains a human review responsibility.

## Production Batches

The 500 assets are produced in ten batches of 50.

### Batch 01 — Construction Core

Core floors, walls, doors, windows, stairs, elevators, railings, columns, facade modules, vents, entrance pieces, and foundational hotel construction parts.

### Batch 02 — Guest Room Core

Beds, casegoods, seating, lighting, wardrobes, desks, televisions, bathroom fixtures, room-service interaction pieces, and standard room decor.

### Batch 03 — Guest Room Expansion

Deluxe, suite, luxury, accessibility, family, and long-stay furniture/fixture variants.

### Batch 04 — Lobby & Front of House

Reception, concierge, bell service, lobby seating, queue infrastructure, luggage handling, directory/signage, and entrance furnishing.

### Batch 05 — Restaurant & Bar

Dining furniture, bars, counters, buffets, service stations, table dressing, kitchen-adjacent service props, and food-service equipment.

### Batch 06 — Back of House

Housekeeping, laundry, maintenance, storage, carts, bins, shelving, cleaning gear, engineering/service props, and logistics equipment.

### Batch 07 — Amenities & Events

Gym, spa, pool-adjacent, conference, ballroom, business-center, lounge, and event-space assets.

### Batch 08 — Decor & Hotel Life

Art, plants, lamps, rugs, clocks, signage, brochures, magazines, trays, toiletries, luggage, table clutter, and environmental storytelling props.

### Batch 09 — Exterior & Grounds

Facade complements, paving, lamps, trees, hedges, planters, fountain elements, benches, exterior seating, entrance landscaping, and service-yard visuals.

### Batch 10 — Characters & Animation Integration

Guest and staff character primary assets, skeleton integration, wardrobe/accessory variants, shared animation-set validation, and interaction-anchor verification.

## Batch Gates

A batch cannot advance to APPROVED until:

1. all 50 primary entries exist in the manifest;
2. IDs are unique;
3. category count is correct;
4. style contract validation passes;
5. all required metadata is present;
6. required dependencies resolve;
7. calibration renders have been reviewed;
8. animated entries reference valid skeleton/animation sets;
9. pivots, scale, collision, and cutaway policy are reviewed;
10. no BLOCKER, CRITICAL, or known MAJOR validation issue remains.

## Validation

Automated validation should check:

- exact primary count equals 500;
- category counts equal the approved allocation;
- stable ID uniqueness;
- valid naming grammar;
- required metadata completeness;
- valid style contract version;
- allowed material family references only;
- valid cutaway policy;
- valid lifecycle state;
- valid animation/skeleton dependencies;
- valid production batch 01-10;
- no missing dependency IDs;
- no unsupported runtime asset types;
- source/export path separation.

Human art review checks silhouette, material consistency, lighting response, proportion, visual hierarchy, cutaway readability, and animation quality.

## Definition of Done

The 500-asset library milestone is complete when:

- the canonical manifest contains exactly 500 approved primary entries;
- every primary asset is represented by valid source/export metadata;
- all required shared materials, skeletons, and animation sets resolve;
- all assets pass HMG-070 pipeline validation;
- every production batch passes its quality gate;
- calibration renders confirm visual consistency across the full library;
- runtime cooking is deterministic;
- representative scenes demonstrate guest rooms, lobby, food service, back-of-house, amenities, exterior, and populated guest/staff operation without obvious stylistic drift.

## Non-Goals

This design does not define game balance, construction costs, guest needs, service durations, room validation rules, hotel economy values, pathfinding authority, or renderer implementation details. Those remain owned by their respective Hotel Haven simulation and renderer specifications.
