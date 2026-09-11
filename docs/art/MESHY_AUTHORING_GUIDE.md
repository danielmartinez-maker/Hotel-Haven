# Hotel Haven Meshy Authoring Guide

Meshy is an optional **offline authoring source** for Hotel Haven. It does not participate in the shipping runtime. Generated models become ordinary Hotel Haven source assets only after normalization and the same V2 quality gates applied to procedural/DCC-authored content.

## Required flow

1. Resolve the target logical ID in `hotel_haven_asset_manifest_v2.json`.
2. Generate or refine the model in Meshy with PBR enabled. Prefer GLB output.
3. Preserve the target asset's intended family, profile, approximate physical scale and management-camera silhouette.
4. Run `meshy_import_adapter.normalize_meshy_asset(...)` with the Meshy task ID.
5. Inspect the normalized asset under the canonical Hotel Haven camera: orthographic, 45-degree yaw, 35.264-degree pitch.
6. Run `HH_ASSET_QUALITY_V2` geometry, semantic, material, interaction, animation, variant and preview gates.
7. Cook the accepted source to deterministic `.hasset` content.
8. The runtime loads only the cooked `.hasset`; it never calls Meshy.

## Quality expectations

- Use readable primary masses and role-defining details; avoid geometry that disappears at management-camera distance.
- Manufactured objects use small, consistent edge treatment rather than razor-sharp or noisy micro-bevels.
- Materials use restrained Hotel Haven PBR response. Metallic is reserved for physical metals; roughness does most of the material differentiation.
- Repeated variants must differ in silhouette, proportions or semantic subcomponents. Material-only/name-only variants do not satisfy required variant groups.
- Floor-standing assets must normalize to floor contact and meter scale.
- Non-small-prop assets may not enter the pipeline as a single anonymous `Body` mesh.
- Interaction anchors, animation bindings, room logic and other gameplay authority remain in game data; Meshy authoring cannot invent gameplay behavior.

## Provenance

The authoring sidecar records only non-secret provenance:

- provider (`meshy`)
- Meshy task ID
- source format
- PBR enabled state
- quality review state

Never write API credentials, tokens or secrets into asset metadata, repository files, logs or cooked content.

## Current automation boundary

The repository adapter accepts Meshy output and normalizes it into Hotel Haven's V2 authoring pipeline. Actual Meshy task submission/download requires an authorized Meshy execution endpoint or local Meshy tool; when unavailable, the rest of the quality/cook pipeline remains fully usable with procedural or DCC source assets.
