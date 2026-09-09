# Hotel Haven Unified Art Style — V1

**Authoritative contract:** `GameData/AssetDefinitions/art_style_contract_v1.json`

Hotel Haven uses **Architectural Diorama Realism**: a clean, warm, readable 2.5D hotel-management presentation designed for an orthographic/isometric cutaway camera.

## Visual rules

- Work from real hotel architecture and furnishings, then simplify detail for management-camera readability.
- Large shapes must read before surface texture.
- Use small, consistent bevels on manufactured edges.
- Keep PBR response soft and restrained; roughness/value separation carries most material identity.
- Use warm hospitality neutrals as the base: ivory, walnut, stone grey, charcoal, muted navy, and brass.
- Reserve burgundy, deep green, terracotta, and muted blue for controlled accents.
- Avoid photoreal scan noise, strong grunge, toy/chibi proportions, heavy outlines, neon palettes, or generic fantasy ornament.
- Characters are slightly stylized realistic adults, with modestly enlarged heads/hands for readability.
- Warm practical fixtures are allowed, but assets must remain compatible with day/night lighting.

## Camera and scale

- Canonical preview: orthographic, 45-degree yaw, 35.264-degree pitch.
- Units: meters.
- Human scale reference: 1.75 m.
- Art dimensions are production hints only and never override gameplay placement, room, economy, or simulation authority.

## Production review

Every asset passes four gates:

1. **Style:** silhouette, palette, material response, detail density, camera readability.
2. **Technical:** scale, pivot, naming, material mapping, cutaway policy.
3. **Simulation usability:** interaction affordance, role readability, cutaway compatibility.
4. **Completion:** manifest entry, reference preview, validation record, zero unresolved blocker.

All ten production batches use this exact contract. Batch-to-batch style changes require a new versioned style contract rather than informal drift.
