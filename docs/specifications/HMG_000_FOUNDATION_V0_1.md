# HMG-000 — Foundation
## Authoritative Game Design Specification v0.1

## 0. Document Status

- **Status:** Foundation baseline
- **Genre:** 2.5D hotel construction + hospitality management simulation
- **Platform target:** Windows 11 x64
- **Primary input:** mouse + keyboard
- **Player count:** single-player only
- **Simulation:** deterministic, pauseable, variable speed
- **Camera:** orthographic 2.5D, rotatable, multi-floor cutaway
- **World model:** tile-based building + entity-based people/items/tasks

This file defines the shared assumptions used by all subsystem specs.

---

## 1. Design Thesis

The game must simulate a hotel as a physical operating institution. Revenue is generated because real simulated guests arrive, receive rooms, consume services, experience delays and failures, and leave with memories that influence reviews and future demand. Staff must physically perform most operational work. Building layout must therefore materially affect service quality, cost, throughput, and reputation.

The player fantasy is to progress from manually operating a small property to designing and controlling a complex, multi-department hotel whose internal systems remain understandable despite scale.

---

## 2. Primary Gameplay Pillars

### P1 — Construct the Property
The player places structure, rooms, infrastructure, furniture, and circulation. Spatial design changes travel distance, queue formation, service capacity, noise, views, and usable floor area.

### P2 — Operate the Property
Every stay creates work. Work must be staffed, scheduled, supplied, completed, inspected where required, and paid for.

### P3 — Serve Distinct Guests
Guests have budgets, trip purposes, preferences, tolerance thresholds, and expectations. Identical service can satisfy one guest and disappoint another.

### P4 — Manage Service Chains
Hotel output is produced through chains, not isolated buttons. Example: a room is sellable only if housekeeping labor, linen inventory, utilities, maintenance state, and room-state transitions all succeed.

### P5 — Manage Reputation and Demand
Guest outcomes create reviews. Reviews affect market conversion, allowable pricing, segment mix, contracts, and future occupancy.

### P6 — Scale Through Organization
Growth increases coordination burden. The late game shifts from direct task manipulation toward policies, department managers, routing design, staffing standards, and automation.

---

## 3. Core Player Loop

1. **Plan:** choose target segment, service level, price position, and expansion objective.
2. **Build:** create or modify rooms, facilities, circulation, and back-of-house capacity.
3. **Staff:** recruit, schedule, and assign employees.
4. **Sell:** publish room inventory and prices into the market-demand simulation.
5. **Operate:** process arrivals, stays, service consumption, cleaning, food, logistics, maintenance, and departures.
6. **Observe:** inspect queues, heatmaps, department backlogs, guest thoughts, financial metrics, and reviews.
7. **Correct:** alter staffing, policies, layout, pricing, inventory, and maintenance.
8. **Grow:** reinvest profit or debt into additional capacity, quality, and automation.

The loop has no mandatory round boundary. The simulation runs continuously.

---

## 4. Simulation Time

### 4.1 Base Time
At speed 1×:

- 1 simulation minute = 1 real second
- 1 simulation hour = 60 real seconds
- 1 simulation day = 24 real minutes

### 4.2 Player Speeds

| Setting | Multiplier |
|---|---:|
| Pause | 0× |
| Normal | 1× |
| Fast | 3× |
| Very Fast | 8× |
| Maximum | 20× |

### 4.3 Simulation Update Domains

- movement integration: 20 Hz logical target
- local steering / avoidance: 10 Hz
- action execution progress: 5 Hz
- needs decay: 1 Hz
- task bidding / reassessment: 1 Hz unless event-triggered
- room environment update: every 5 sim minutes
- department aggregate statistics: every 15 sim minutes
- market booking batch: every 1 sim hour
- daily review generation: checkout-triggered, summarized at 03:00
- financial day close: 00:00

Movement may render at display-frame rate using interpolation without changing authoritative simulation state.

---

## 5. Spatial Model

### 5.1 Tile Scale
- 1 tile = 1 m × 1 m nominal.
- walls occupy tile edges.
- objects reserve one or more tile footprints.
- character navigation uses walkable subnodes generated from tile occupancy.

### 5.2 Standard Map
- standard buildable area: 192 × 192 tiles.
- large scenario maximum: 256 × 256 tiles.
- baseline supported floors: 20 above/below combined, scenario-configurable.

### 5.3 Floor IDs
Ground floor is `0`. Floors above are positive integers. Basements are negative integers.

---

## 6. Camera and 2.5D Presentation

- projection: orthographic.
- pitch target: 55° default; allowed range 45–65°.
- yaw: free 360° rotation in 90° snap increments by default; optional smooth rotation setting.
- zoom: must maintain legible characters from hotel overview to individual-room inspection.
- active floor: one full-detail floor.
- lower floors: hidden by default.
- higher floors: hidden by default.
- optional context mode: adjacent floors rendered as translucent shells.

### Wall Visibility
Walls support three render states:
1. full height,
2. cutaway,
3. blueprint/outline.

The visibility controller must prioritize selected room visibility and prevent foreground walls from obscuring the current focus.

---

## 7. Primary Entity Classes

- `Guest`
- `GuestGroup`
- `Employee`
- `Task`
- `Room`
- `ObjectInstance`
- `InventoryItemStack`
- `Reservation`
- `BookingRequest`
- `SupplierOrder`
- `Delivery`
- `Complaint`
- `Review`
- `Department`
- `ServiceRequest`
- `Contract`
- `EventBooking`
- `ElevatorCar`
- `UtilityNode`

Every persistent entity requires a stable unique 64-bit ID.

---

## 8. Target Simulation Scale

The engine must be architected for the following high-end normal gameplay target:

- 500 guest rooms
- 1,000 simultaneous guests
- 300 employees
- 1,300+ moving characters
- 20 floors
- 10,000+ placed objects
- 5,000+ queued/open operational tasks during severe backlog conditions

The target does not imply every entity receives full-rate AI updates at all times. Scheduling and update throttling are mandatory.

---

## 9. Hotel Day Rhythm

Default demand profile:

| Time | Dominant systems |
|---|---|
| 05:00–07:00 | breakfast prep, night audit, early departures |
| 07:00–10:00 | breakfast rush, departures, transport requests |
| 10:00–15:00 | housekeeping peak, maintenance access, room turnover |
| 14:00–18:00 | arrival/check-in peak |
| 18:00–22:00 | restaurant, bar, event, leisure peak |
| 22:00–02:00 | room service, noise complaints, late arrivals |
| 02:00–05:00 | low-demand night operations |

Scenarios may override profiles.

---

## 10. Progression Structure

Progression uses operational milestones and property capability. No abstract character XP is required for the player.

Example unlock classes:

- facility capability,
- guest segment access,
- manager automation,
- supplier tier,
- financing tier,
- star-classification eligibility,
- advanced revenue-management controls.

Unlock conditions must be observable. Example: `RestaurantOperations` may require 25 completed stays, a valid kitchen room, and positive cash balance rather than an opaque level number.

---

## 11. Success and Failure Philosophy

### 11.1 No Sudden Arbitrary Failure
Cash below zero starts financial distress, not instant game over.

Financial distress stages:
1. working-capital warning,
2. revolving credit draw,
3. lender covenant warning,
4. emergency financing / asset liquidation options,
5. receivership or bankruptcy.

### 11.2 Operational Failure Must Be Traceable
Every material negative outcome must expose a causal path to the player.

Example:
`Late room ready` → `linen shortage` → `laundry dryers saturated` → `hotel expanded without dryer capacity`.

---

## 12. Information and Diagnostics

Required global overlays:

- cleanliness
- guest satisfaction
- guest traffic
- staff traffic
- room quality
- room status
- noise
- temperature
- electrical load
- water/plumbing demand
- maintenance condition
- open-task density
- staff utilization
- queue length / wait time
- elevator congestion
- fire safety
- security coverage
- revenue per room/area

Every overlay requires a legend and exact numeric inspection on hover/select.

---

## 13. Global State Conventions

### 13.1 Normalized Scores
Unless otherwise specified, human-readable scores use integer 0–100.

### 13.2 Probability
Probability values are stored as floating-point 0.0–1.0 but displayed as percentages where user-facing.

### 13.3 Money
Authoritative money uses integer smallest currency units to avoid floating-point drift.

### 13.4 Time
Authoritative simulation timestamps use integer simulation seconds from campaign epoch.

---

## 14. Data-Driven Requirement

The following must be external data definitions rather than hard-coded switch statements:

- guest archetypes
- room types
- object definitions
- menu items
- staff roles
- task types
- service standards
- supplier catalogs
- events
- scenarios
- star requirements
- hotel policy presets
- review phrase pools

---

## 15. Cross-System Contract

The five subsystem specs have the following authority boundaries:

- **HMG-010 Guest AI:** determines guest goals, expectations, reactions, memories, satisfaction, complaints, and reviews.
- **HMG-020 Construction:** determines what physical spaces exist, whether rooms are valid, and environmental/quality properties.
- **HMG-030 Economics:** determines demand, reservations, room pricing, revenue, variable/fixed costs, and market response.
- **HMG-040 Staff:** determines employee availability, task assignment, schedules, skill impact, fatigue, morale, and department labor capacity.
- **HMG-050 Logistics:** determines operational task generation and physical material/service chains.

No subsystem may silently duplicate another subsystem’s authority.

---

## 16. First Playable Scope

The minimum vertically integrated game must include:

- one map,
- up to 3 floors,
- walls/doors/floors,
- lobby,
- reception,
- standard guest room,
- bathroom,
- staff room,
- housekeeping closet,
- receptionists,
- housekeepers,
- maintenance technician,
- reservations,
- guest arrival/check-in/stay/checkout,
- dirty/clean room cycle,
- basic maintenance failure,
- room rate and payroll,
- guest satisfaction,
- reviews,
- cash flow,
- 2.5D camera and cutaway.

This build is considered valid only if a poor layout demonstrably creates longer travel, longer waits, lower satisfaction, and lower profit than an otherwise equivalent efficient layout.
