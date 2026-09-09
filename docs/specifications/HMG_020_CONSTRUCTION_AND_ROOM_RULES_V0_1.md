# HMG-020 — Construction and Room Rules
## Authoritative Subsystem Specification v0.1

## 1. Purpose

This system defines the physical hotel: tiles, walls, floors, doors, utilities, construction jobs, room recognition, room validity, room quality, environmental properties, and renovation.

---

## 2. Grid and Geometry

- tile footprint: 1 m × 1 m nominal.
- wall segments occupy edges between adjacent tiles.
- doors/windows occupy wall segments.
- objects occupy integer tile footprints plus interaction nodes.
- all walkable spaces require at least one navigation node per free tile; wide spaces may generate subnodes.

A tile stores:

```text
FloorID
TileCoord
FloorFinish
RoomID|null
Walkability
Ownership
ConstructionState
UtilityConnections
Contamination/Cleanliness
```

---

## 3. Build Modes

### 3.1 Planning Mode
Planning is free and produces ghost entities.

Allowed planned elements:

- walls
- doors
- room zones
- objects
- stairs
- elevators
- utility routes

### 3.2 Commit Construction
Committing a plan:

1. validates geometry,
2. calculates estimated cost,
3. reserves required funds according to payment rule,
4. creates construction tasks,
5. converts ghosts to `PlannedConstruction` state.

No object becomes functional before its construction state reaches `Complete`.

---

## 4. Construction State Machine

For structural/object work:

`Ghost -> Planned -> AwaitingMaterials -> ReadyForLabor -> UnderConstruction -> Complete`

Failure/alternate states:

- `Blocked`
- `Cancelled`
- `DemolitionQueued`
- `UnderDemolition`

Construction material delivery can be abstracted in the first playable but must retain the states necessary for physical-delivery implementation later.

---

## 5. Build Validity Rules

A placement is invalid if any of the following is true:

- footprint leaves owned/buildable area,
- collides with non-removable structural object,
- blocks the only required egress path from an occupied valid room,
- violates vertical clearance for stairs/elevator shafts,
- creates unsupported floor when structural support rules are enabled,
- object interaction node becomes permanently unreachable,
- door is placed without wall adjacency,
- window is placed on a non-wall edge.

The UI must display the exact failing rule.

---

## 6. Room Detection

A room is an enclosed connected set of interior floor tiles bounded by:

- walls,
- closed structural boundaries,
- doors/windows occupying boundary wall segments.

Doors do not break enclosure.

Room detection runs incrementally when wall topology changes.

If an enclosure splits, old `RoomID` is retained by the larger tile set; the smaller region receives a new `RoomID`. If regions merge, the oldest RoomID survives.

---

## 7. Room Type Assignment

A detected enclosure may be:

- `Unassigned`,
- manually assigned a room type,
- auto-suggested based on objects.

Auto-suggestion never changes room type without player confirmation unless a scenario/tutorial explicitly enables auto-assign.

---

## 8. Room Validity Contract

Each room type definition contains:

```text
MinimumArea
MaximumArea|null
RequiredObjects[]
RequiredObjectGroups[]
ForbiddenObjects[]
RequiredConnections[]
RequiredAccessRules[]
EnvironmentalRules[]
CapacityRule
```

A room reports:

- `Valid`
- `Invalid`
- `Degraded`
- `OutOfService`

`Degraded` means usable but below a non-safety requirement.

---

## 9. Standard Guest Room Requirements

Baseline standard room:

- enclosed area >= 20 tiles including bathroom,
- one guest-accessible door,
- one sleeping surface for booked capacity,
- one bathroom subroom or integrated bathroom zone,
- one toilet,
- one sink,
- one shower or bath,
- one controllable light source,
- electrical service,
- potable water service,
- wastewater connection,
- reachable path from room door to each required interaction object.

A standard room cannot be sold if any safety/utility requirement is invalid.

---

## 10. Bathroom Rule

Bathroom must be either:

1. a nested enclosed room tagged `GuestBathroom` connected directly to guest room, or
2. an integrated bathroom zone if scenario rules allow open-plan bathroom design.

Default base game requires enclosed bathroom.

---

## 11. Room Capacity

Capacity is the minimum of:

- bed capacity,
- room-type max occupancy,
- accessibility/safety occupancy constraint.

Examples:

- single bed: 1 adult-equivalent
- double/queen/king: 2 adult-equivalent
- sofa bed: 2 when deployed
- crib: 1 infant only

---

## 12. Guest Room Status State Machine

Room operational status:

1. `VacantReady`
2. `ReservedVacantReady`
3. `OccupiedClean`
4. `OccupiedServiceDue`
5. `VacantDirty`
6. `Cleaning`
7. `AwaitingInspection`
8. `MaintenanceHold`
9. `OutOfOrder`
10. `ConstructionHold`

Only `VacantReady` and `ReservedVacantReady` are assignable for normal arrival.

---

## 13. Room Quality Score

Guest room quality 0–100 is computed from:

```text
Quality =
  Bed            * 0.15 +
  Bathroom       * 0.15 +
  Space          * 0.10 +
  Furniture      * 0.10 +
  Decor          * 0.10 +
  Cleanliness    * 0.15 +
  Quiet          * 0.10 +
  View           * 0.05 +
  Amenities      * 0.05 +
  Maintenance    * 0.05
```

All component values are 0–100.

This score is a descriptive quality measure; guest satisfaction compares actual components against guest-specific expectations.

---

## 14. Space Score

For a room type:

```text
SpaceScore = clamp(50 + (Area - TargetArea) * AreaPointRate, 0, 100)
```

Standard guest room defaults:

- minimum area 20 m²
- target area 28 m²
- `AreaPointRate` = 2.5 points/m² above/below target.

This means an oversized room improves score only until 100 and consumes expensive floor area, creating a meaningful tradeoff.

---

## 15. Object Quality

Every furnishing object definition contains:

- purchase cost,
- quality 0–100,
- style tags,
- condition 0–100,
- age,
- footprint,
- interaction nodes,
- noise generation,
- maintenance class,
- service capability.

Effective contribution:

`EffectiveQuality = BaseQuality * (0.5 + 0.5 * Condition/100)`.

---

## 16. Decor and Style Coherence

Decor score combines:

- object visual-quality points,
- decorative density,
- style coherence,
- clutter penalty.

Style coherence uses tags such as:

- Modern
- Classic
- Industrial
- Coastal
- Rustic
- ArtDeco
- Minimal
- Luxury

A room is coherent when >=70% of decor-weighted objects share at least one compatible style family.

Clutter penalty begins when decorative occupancy exceeds 35% of walkable area or path clearance is reduced.

---

## 17. Views

Each window evaluates a view ray/cone into exterior world or large interior atrium.

View categories:

- Ocean
- Mountain
- Garden
- Skyline
- Historic Landmark
- Pool
- Courtyard
- Street
- Parking
- ServiceYard
- Wall/Obstruction

Each category has base value 0–100. Obstruction reduces value according to blocked view percentage.

Room view score is the best window value plus 25% of second-best, capped at 100.

---

## 18. Noise Simulation

Each noise source emits:

```text
NoisePower
FrequencyClass
ActiveSchedule
```

Noise propagates over the room adjacency graph and local spatial falloff.

Attenuation sources:

- distance,
- wall material,
- closed door,
- floor/ceiling separation,
- acoustic upgrade.

Room noise score is inverse of average sleeping-period effective dB-equivalent index.

The simulation may use abstract noise units rather than physical decibels, but displayed player values must remain consistent.

---

## 19. Cleanliness

Each tile and cleanable object tracks dirt 0–100.

Dirt generation sources:

- footsteps,
- food spills,
- bathroom use,
- trash overflow,
- outdoor weather ingress,
- construction.

Room cleanliness is area/object-weighted average converted to 0–100 clean score.

A guest room with cleanliness <50 is considered `ServiceDefect` and may generate complaints.

A room with cleanliness <30 cannot become `VacantReady` after inspection.

---

## 20. Maintenance Condition

Placed functional objects track condition 0–100.

- 100–70: normal
- 69–40: worn, increasing fault probability
- 39–1: degraded, strong failure probability and quality penalty
- 0: failed

Breakdown probability is evaluated only when used or at scheduled wear ticks, not every frame.

---

## 21. Utilities

Guest rooms require active:

- electricity,
- potable water,
- wastewater path,
- temperature control when climate rules demand it.

Utility interruption creates a room fault immediately. A short interruption may be tolerated operationally, but new check-ins cannot be assigned to a room missing essential utilities.

---

## 22. Accessibility

Accessible room definition requires:

- accessible route from hotel accessible entrance,
- elevator if room is not on accessible entrance floor,
- door clearance meeting accessibility rule,
- accessible bathroom fixtures,
- sufficient turning/clearance tiles.

Accessibility requirements are scenario/jurisdiction data, not hidden rules.

---

## 23. Vertical Circulation

Stairs and elevators create floor-transition navigation edges.

Elevator shafts require vertically aligned shaft tiles. Elevator doors can open only onto valid landing tiles.

Service elevators may be access-restricted to staff but remain valid emergency egress only if scenario code permits; default: elevators never count as fire egress.

---

## 24. Fire and Egress Validation

Each occupied room must have a legal path to an emergency exit according to scenario rules.

Default baseline:

- at least one path to stair/exterior exit,
- no route through locked guest-inaccessible room,
- maximum allowed egress distance configurable by jurisdiction.

Construction that would invalidate all required egress must be blocked while hotel is occupied unless the player closes affected rooms/floors first.

---

## 25. Renovation

Renovation is construction performed on existing operational space.

Renovation actions:

- replace finish,
- replace object,
- move walls,
- upgrade bathroom,
- rewire utilities,
- change room type.

Affected room enters `ConstructionHold` when required interaction or safety capability becomes unavailable.

Players can renovate one room at a time or bulk-select room sets.

---

## 26. Design Aging

Furniture has `StyleAgeYears`. Guest perception of outdatedness begins after its style’s configured freshness window.

Default:

- economy furnishing: 12 years
- standard: 10 years
- upscale: 8 years
- luxury: 6 years

Outdatedness affects decor score, not condition. A perfectly maintained old room can still feel dated.

---

## 27. Required Construction Diagnostics

Selecting any invalid/degraded room must show:

- exact missing requirements,
- inaccessible objects,
- utility failures,
- egress failures,
- area/capacity,
- room-quality component breakdown,
- noise sources,
- view source,
- cleanliness blockers,
- maintenance blockers.
