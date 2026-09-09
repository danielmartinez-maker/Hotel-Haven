# HMG-050 — Service and Logistics Simulation
## Authoritative Subsystem Specification v0.1

## 1. Purpose

This system defines the physical operational chains that convert staff time, rooms, equipment, supplies, and infrastructure into guest service. It is the primary bridge between building layout and hotel performance.

---

## 2. Logistics Principle

A service may complete only when its explicit dependencies are available. Example housekeeping requires:

- eligible housekeeper,
- reachable room,
- cleaning tools,
- required linen/amenities,
- room access permission,
- enough work time.

A missing dependency creates a visible blocked task or degraded service; the simulation must never fabricate completion.

---

## 3. Physical Flow Domains

Primary flows:

1. guests
2. staff
3. clean linen
4. dirty linen
5. food inventory
6. beverages
7. guest consumables
8. maintenance parts
9. waste/trash
10. room-service orders/dishes
11. deliveries

Each flow uses paths and storage nodes.

---

## 4. Inventory Stack Model

Inventory stack:

```text
ItemType
Quantity
StorageLocationID
Quality/Freshness|null
ReservedQuantity
OwnerDepartment
```

No negative inventory is permitted.

Task resource claims reserve quantity before worker begins pickup when race conditions are possible.

---

## 5. Storage Capacity

Storage furniture/rooms define capacity by item class and volume units.

If storage is full:

- deliveries cannot unload affected items,
- produced clean linen cannot be placed there,
- staff may seek alternate compatible storage.

Items may never teleport to global inventory.

---

## 6. Supplier Orders

Order states:

`Draft -> Submitted -> Scheduled -> InTransit -> Arrived -> Unloading -> Completed`

Possible failures:

- Delayed
- PartialDelivery
- RejectedNoCapacity
- Cancelled

Each supplier has:

- item catalog,
- unit price,
- lead time,
- delivery windows,
- minimum order,
- reliability.

---

## 7. Loading Dock

Physical deliveries require a loading dock/service entrance unless scenario explicitly abstracts deliveries.

Dock throughput is limited by:

- number of bays,
- receiving staff,
- cart/pallet capacity,
- route congestion,
- storage availability.

Guest-facing entrances may accept emergency small deliveries at a service-quality penalty if policy allows.

---

## 8. Housekeeping Turnover Chain

Checkout triggers room transition:

`Occupied -> VacantDirty`.

System creates `TurnoverClean` task containing required subtasks based on dirt and room inventory:

1. collect trash,
2. strip used linen,
3. clean bathroom,
4. clean surfaces/floor,
5. make bed,
6. replace towels,
7. refill amenities,
8. inspect visible defects,
9. final room check.

Subtasks may be represented as timed phases inside one task for performance, but resource usage and failure points must remain explicit.

---

## 9. Housekeeping Supply Consumption

Default standard turnover consumes (`BALANCE_TUNABLE`):

- clean sheet set: 1
- pillowcase set: 1 per configured bed set
- bath towels: 2
- hand towels: 2
- toilet paper refill if current stock below threshold
- soap/shampoo units according to replacement policy
- cleaning chemical charge: abstract 1 unit

Stayover cleans use a reduced replacement policy unless guest requests full linen change.

---

## 10. Housekeeping Cart

A cart is a mobile inventory container.

Typical capacity supports 6 standard room turnovers before refill.

Cart may contain:

- linen,
- towels,
- amenities,
- cleaning chemicals,
- trash capacity.

A housekeeper without required resources must route to nearest compatible closet/cart supply point.

---

## 11. Housekeeping Closets

Closets act as local buffers between central stores/laundry and room floors.

Automatic replenishment task is generated when item quantity falls below player-defined par level.

Example:

`Floor 8 towel par = 80; reorder threshold = 30; target refill = 80`.

Poor closet placement increases labor travel.

---

## 12. Room Inspection

Inspection policy options:

- none,
- random percentage,
- all rooms,
- only low-skill cleaner rooms,
- only VIP arrivals.

Inspection task checks task quality and maintenance defects.

If inspection score <70 default:

- room remains not ready,
- corrective clean task created.

---

## 13. Laundry Chain

Dirty linen flow:

`Room -> DirtyCart -> DirtyLinenStorage -> Washer -> Dryer -> Folding -> CleanLinenStorage -> Closet -> Room`

Laundry item states:

- Dirty
- Washing
- WetClean
- Drying
- CleanUnfolded
- CleanReady

No clean linen may skip processing states.

---

## 14. Laundry Equipment Throughput

Each machine has:

- batch capacity,
- cycle time,
- utility demand,
- condition,
- supported linen classes.

Example (`BALANCE_TUNABLE`):

- commercial washer: 30 linen units / 35 min
- commercial dryer: 30 units / 40 min

Bottleneck throughput is visible to player.

---

## 15. Outsourced Laundry

Outsourcing removes dirty linen at scheduled pickup and returns clean linen after lead time.

Tradeoffs:

- no in-house equipment/staff,
- higher per-unit cost,
- lead-time risk,
- larger clean-linen buffer required.

---

## 16. Front Desk Service Chain

Check-in requires:

- guest arrival,
- reservation or walk-in availability,
- reception workstation,
- receptionist on duty,
- assignable room or resolution path.

Check-in service time baseline:

- standard reservation: 3 sim min
- complex/payment/problem reservation: 5–10 sim min
- group handling: specialized batch process

Employee skill modifies duration per HMG-040.

---

## 17. Room Assignment

Room assignment scores valid candidate rooms based on:

```text
CategoryMatch
BedMatch
AccessibilityMatch
PreferenceMatch
ViewMatch
NoiseFit
GroupAdjacency
OperationalReadiness
UpgradeCost
```

Player may allow automatic free upgrades under policy conditions.

System must expose why a particular room was selected.

---

## 18. Service Requests

Guests can create requests:

- extra towels
- toiletries
- room cleaning
- maintenance
- luggage assistance
- room service
- wake-up call abstraction
- room change

Request state:

`Requested -> Accepted -> TaskCreated -> InProgress -> Delivered/Resolved -> Closed`

Guest wait-time memory starts at `Requested`.

---

## 19. Maintenance Chain

Object condition produces preventive or corrective tasks.

Corrective maintenance:

1. fault detected,
2. repair ticket created,
3. technician assigned,
4. parts/tools checked,
5. technician travels,
6. diagnosis phase,
7. repair phase,
8. test phase,
9. ticket closes or escalates.

If part unavailable, task blocks with explicit `AwaitingPart` reason.

---

## 20. Preventive Maintenance

Player sets condition thresholds by equipment class.

Default:

- critical infrastructure preventive threshold 70
- standard equipment 55
- cosmetic/noncritical 40

Preventive work reduces breakdown probability by restoring condition before failure.

---

## 21. Food Storage

Food inventory classes:

- dry
- refrigerated
- frozen
- beverage

Fresh items have remaining shelf life. Spoiled items become waste and cannot be cooked.

Storage room/equipment must support required temperature class.

---

## 22. Restaurant Service Chain

Dine-in flow:

1. guest requests restaurant,
2. host queue,
3. table assignment,
4. server greeting,
5. order entry,
6. kitchen ticket,
7. ingredient reservation,
8. prep,
9. cook,
10. plate,
11. server pickup,
12. delivery,
13. eating,
14. payment/posting,
15. table clearing,
16. dishwashing,
17. table ready.

Each step can create bottlenecks.

---

## 23. Menu Item Production

Menu item definition:

```text
Ingredients[]
PrepStation
CookStation|null
PrepSeconds
CookSeconds
PlateSeconds
RequiredCookSkill
BaseQuality
SalePrice
```

Final food quality considers:

- ingredient freshness,
- cook skill,
- equipment condition,
- hold time before serving.

---

## 24. Room Service

Room service flow:

`GuestOrder -> KitchenTicket -> Production -> TrayAssembly -> RunnerPickup -> Elevator/Route -> RoomDelivery -> DirtyTrayPickup -> Dishwashing`.

Delivery-time expectation begins at order confirmation.

If food hold time exceeds menu threshold, quality falls.

---

## 25. Bar Service

Bar uses station queue rather than kitchen production for simple drinks.

Complex cocktails have preparation durations and bartender skill requirements.

Alcohol/legal systems may remain abstract depending on rating and region; intoxication can be represented as guest-state modifier where included.

---

## 26. Waste Flow

Waste sources:

- guest rooms,
- public bins,
- kitchen,
- maintenance,
- construction.

Flow:

`Source -> CollectionTask -> BackOfHouseWasteStorage -> ScheduledRemoval`.

Overflow increases dirt, odor/atmosphere penalty, and hygiene risk.

---

## 27. Public Area Cleaning

Traffic creates dirt on public tiles.

Cleaner tasks are generated when:

- dirt exceeds zone threshold,
- spill event occurs,
- scheduled cleaning window begins.

Spills create high-priority localized cleaning tasks because of guest-perception and safety impact.

---

## 28. Elevator Operations

Elevator car has:

- capacity persons/weight abstraction,
- current floor,
- direction,
- target stops,
- speed,
- door time,
- condition.

Dispatch objective minimizes weighted passenger wait while respecting service/guest restrictions.

Expected elevator wait contributes to path cost before a character commits to elevator versus stairs.

---

## 29. Service Elevators

Service elevators can be restricted to staff/logistics.

Staff carrying:

- dirty linen,
- trash,
- delivery carts,
- large maintenance equipment

prefer service elevators with strong path-cost penalty for guest elevators if service route exists.

This is the primary mechanism making back-of-house vertical design valuable.

---

## 30. Back-of-House Exposure

Guests receive a small negative atmosphere event when they encounter incompatible service traffic in premium guest areas.

Examples:

- dirty linen cart through luxury lobby,
- trash cart in restaurant corridor,
- major supplier unloading through guest entrance.

Penalty magnitude depends on guest segment and hotel star expectations.

---

## 31. Congestion

Each navigation edge accumulates occupancy. Movement speed begins reducing above target density.

Congestion affects:

- guest travel,
- staff travel,
- delivery times,
- elevator access,
- perceived convenience.

Traffic heatmap records traversals per 15 sim minutes.

---

## 32. Service-Level Timers

Operational services track explicit response-time targets.

Default examples (`BALANCE_TUNABLE`):

- extra towels: 15 min
- simple maintenance response: 20 min
- critical room fault response: 10 min
- room service advertised delivery: 35 min
- luggage assistance: 10 min

Hotels may advertise better service levels; doing so raises guest expectation.

---

## 33. Event/Banquet Logistics

Event contracts generate bulk demand for:

- setup labor,
- chairs/tables,
- food production,
- service staff,
- room blocks,
- cleanup,
- audiovisual equipment if enabled.

Event setup and teardown are physical tasks with deadlines. Failure to complete before event start reduces contract satisfaction.

---

## 34. Failure Propagation Example

Linen shortage chain:

1. occupancy rises,
2. dirty linen volume rises,
3. dryer becomes throughput bottleneck,
4. clean-linen store falls below par,
5. floor closets fail replenishment,
6. housekeepers block on linen,
7. departure rooms remain dirty,
8. arrivals wait for rooms,
9. front desk queue grows,
10. negative memories and reviews increase,
11. reputation and future demand fall.

Every link must be inspectable through task/resource diagnostics.

---

## 35. Required Logistics Diagnostics

The player must be able to inspect:

- current inventory by storage node,
- incoming orders and ETA,
- blocked tasks and exact missing dependency,
- housekeeping room backlog,
- linen flow and machine utilization,
- kitchen tickets by stage,
- table queue/wait,
- room-service queue,
- maintenance tickets by severity,
- elevator wait by floor,
- guest/staff traffic heatmaps,
- back-of-house exposure incidents,
- service-level compliance percentage by department.
