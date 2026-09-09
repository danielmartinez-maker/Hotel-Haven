# HMG-060 — Art Direction Options
## Three Visual Identity Pitches v0.1

These are deliberately distinct directions. The gameplay simulation and camera remain the same; the rendering, proportions, materials, palette discipline, animation language, and asset-production burden differ.

---

# Direction A — Architectural Diorama Realism

## Pitch

A luxurious miniature architectural model brought to life. Rooms look like carefully constructed dollhouse interiors with physically convincing materials, soft global illumination, subtle depth of field at extreme zoom levels, and slightly simplified people. The hotel should look desirable enough that players enjoy simply watching it operate.

## Visual DNA

- orthographic 2.5D presentation
- realistic room proportions
- stylized but physically plausible furniture
- soft PBR materials
- warm practical lighting
- miniature/diorama scale cues
- clean geometry and restrained texture noise
- characters with simplified facial detail and moderately enlarged heads/hands for readability

## Reference Feel

Architectural visualization + premium board-game miniature + management simulation readability.

## Character Proportions

- approximately 5.5–6 heads tall
- slightly oversized head: +10–15% from realistic proportion
- hands/tools enlarged enough to read at gameplay zoom
- uniforms use strong silhouettes and department-specific shapes

## Material Strategy

- wood, stone, carpet, metal, glass visibly distinct
- roughness variation is more important than noisy albedo textures
- guest rooms should remain readable from 10–20 m virtual camera distance

## Lighting

- baked/static environment contribution where possible
- dynamic practical lights for lamps/fixtures
- soft shadows
- time-of-day tinting
- no extreme bloom

## Strengths

- strongest hotel fantasy
- screenshots market well
- makes renovation and room design emotionally rewarding
- good fit for boutique/luxury gameplay

## Risks

- highest asset-production cost
- easiest direction to make visually cluttered
- requires excellent cutaway and contrast design
- realistic characters can become uncanny if animation budget is insufficient

## Best Use

Recommended if the game is positioned as a premium, highly polished management simulator with strong interior-design appeal.

---

# Direction B — Graphic Cutaway / Modern Management Illustration

## Pitch

A crisp, highly readable 3D interpretation of management-game illustration. Surfaces use broad color regions, restrained outlines, simplified geometry, and highly expressive characters. The hotel resembles an animated architectural infographic rather than a miniature photograph.

## Visual DNA

- clean orthographic cutaway
- flat-to-soft shaded materials
- controlled outline treatment on characters and important props
- limited per-room color palette
- exaggerated silhouettes
- very low texture noise
- iconographic animation poses

## Character Proportions

- approximately 4.5–5 heads tall
- larger heads and hands
- strong uniform color blocking
- facial expressions readable through brows/mouth/pose rather than detailed facial rigs

## Environment

- walls and furniture use simplified forms
- decorative assets use strong shapes instead of surface detail
- each service department has subtle palette coding without turning the hotel into a rainbow

## Lighting

- soft directional light
- ambient occlusion emphasized for depth
- minimal specular noise
- lighting never obscures gameplay state

## Strengths

- best readability at large hotel scale
- lower production cost than realism
- supports 1,000+ characters visually
- easiest for overlays, statuses, and animation clarity
- ages well stylistically

## Risks

- can feel generic if shape language is weak
- less aspirational for players who want realistic interior design
- must avoid resembling any one existing management title too closely

## Best Use

Recommended if simulation legibility and broad appeal are the highest priorities.

---

# Direction C — Neo-Art-Deco Hotel Storybook

## Pitch

The entire game is presented as an elegant contemporary interpretation of 1920s–1950s hotel posters, Art Deco interiors, and illustrated travel advertising, while still allowing modern hotel scenarios. Architecture uses clean geometry, brass accents, rich upholstery, patterned floors, and graphic shadows. Characters have fashion-illustration silhouettes and expressive animation.

This is the most distinctive option.

## Visual DNA

- Art Deco geometry without requiring historical setting
- strong vertical lines and geometric ornament
- brass, dark timber, stone, velvet, terrazzo
- stylized graphic lighting
- travel-poster-inspired skies and exterior backdrops
- elegant UI that looks like hotel stationery, brass signage, luggage tags, and reservation cards

## Character Proportions

- approximately 6 heads tall
- elegant elongated silhouettes
- simplified faces
- exaggerated jackets, dresses, luggage, service uniforms
- strong animation posing rather than realism

## Environment

The same construction kit supports multiple property identities:

- classic grand hotel
- modern neo-Deco tower
- Miami resort
- alpine lodge variant
- contemporary boutique property

The art direction applies through shape language rather than forcing every hotel into identical decor.

## Lighting

- stronger contrast than Directions A/B
- pools of warm interior light
- stylized shafts and reflected highlights
- night scenes become especially attractive

## Strengths

- strongest standalone visual identity
- hotel theme is immediately recognizable
- excellent UI/art cohesion opportunity
- creates strong marketing screenshots
- lets the game feel sophisticated without photorealism

## Risks

- art direction can overpower player-created style if every asset is too Deco-specific
- requires disciplined modular asset families
- some players may perceive it as a period game unless marketing communicates otherwise

## Best Use

Recommended if the project needs a recognizable visual identity that does not resemble existing construction-management games.

---

# Recommendation

## Primary recommendation: Direction A + selective graphic discipline from B

Use Architectural Diorama Realism as the world-rendering foundation, then borrow Direction B's readability rules:

- simplified character geometry,
- restrained texture noise,
- strong department silhouettes,
- readable object shapes,
- aggressive wall cutaway,
- clean overlays.

This produces an aspirational hotel that is attractive at room scale without sacrificing management readability at 500-room scale.

## Secondary recommendation: Direction C

Choose C if visual differentiation in the market is more important than broad realism. It has the strongest identity and could make the UI, marketing, rooms, and characters feel unusually cohesive.
