# HMG-010 — Guest AI and Psychology
## Authoritative Subsystem Specification v0.1

## 1. Purpose

This system determines who guests are, why they book, what they do while present, how they evaluate experiences, when they complain, and what they report after departure.

The system must create behavior that is:

- individually differentiated,
- deterministic from simulation state and RNG stream,
- computationally bounded,
- causally explainable to the player,
- sufficiently expressive to generate emergent stories without requiring scripted story events.

---

## 2. Guest Lifecycle State Machine

Every guest occupies exactly one primary lifecycle state:

1. `Prospective`
2. `Reserved`
3. `TravelingToHotel`
4. `Arriving`
5. `AwaitingCheckIn`
6. `CheckedIn`
7. `InStay`
8. `PreparingCheckout`
9. `AwaitingCheckout`
10. `Departing`
11. `CompletedStay`
12. `Cancelled`
13. `NoShow`
14. `WalkedRelocated`

Illegal transitions must be rejected.

Canonical transitions:

`Prospective -> Reserved`
`Reserved -> TravelingToHotel | Cancelled | NoShow`
`TravelingToHotel -> Arriving`
`Arriving -> AwaitingCheckIn`
`AwaitingCheckIn -> CheckedIn | WalkedRelocated`
`CheckedIn -> InStay`
`InStay -> PreparingCheckout`
`PreparingCheckout -> AwaitingCheckout`
`AwaitingCheckout -> Departing`
`Departing -> CompletedStay`

---

## 3. Guest Identity Data

Each guest stores:

```text
GuestID
GroupID|null
DisplayName
AgeBand
TravelPurpose
WealthBand
BudgetPerNight
PriceSensitivity
ServiceSensitivity
CleanlinessSensitivity
NoiseSensitivity
PrivacySensitivity
SafetySensitivity
ComfortSensitivity
FoodSensitivity
Patience
SocialPreference
ActivityPreferenceVector
RoomPreferenceVector
TraitSet
Needs
Mood
Stress
ExpectationProfile
Memories[]
ActiveGoal
ActiveAction
ReservationID|null
AssignedRoomID|null
ArrivalTime
PlannedDepartureTime
ActualDepartureTime|null
ComplaintHistory[]
```

All sensitivity and personality factors are normalized 0.0–1.0.

---

## 4. Guest Segments

Baseline archetypes:

1. Budget Leisure
2. Backpacker
3. Business Traveler
4. Executive Business
5. Couple Leisure
6. Family Leisure
7. Luxury Leisure
8. Conference Delegate
9. Group/Tour Traveler
10. Airport/Transit Traveler
11. Wellness Traveler
12. VIP/Celebrity
13. Critic/Reviewer

An archetype is a probability distribution, not a fixed template. Individual guests receive variation around archetype means.

Example Business Traveler defaults (`BALANCE_TUNABLE`):

- price sensitivity: 0.45
- service sensitivity: 0.75
- cleanliness sensitivity: 0.80
- noise sensitivity: 0.85
- patience: 0.45
- Wi-Fi preference: 0.95
- desk preference: 0.80
- breakfast preference: 0.70
- spa preference: 0.10
- pool preference: 0.15

---

## 5. Trait System

Each adult guest receives 0–3 traits. Baseline traits:

- Patient
- Impatient
- Neat
- Messy
- Light Sleeper
- Heavy Sleeper
- Foodie
- Workaholic
- Social
- Private
- Frugal
- Status Conscious
- Fitness Focused
- Early Riser
- Night Owl
- Complaint Prone
- Forgiving

Traits apply explicit modifiers. Example:

`Impatient`: queue tolerance × 0.65.

`Forgiving`: negative-memory final review weight × 0.80.

No trait may use vague behavior without a numeric implementation effect.

---

## 6. Needs Model

Needs are stored 0–100 where 100 means fully satisfied.

Core needs:

- Energy
- Hunger
- Hygiene
- Comfort
- Entertainment
- Social
- Privacy
- Safety

Operational perception tracks separately:

- ServiceConfidence
- CleanlinessConfidence
- EnvironmentComfort
- ValuePerception

### Need Decay
Each need has a per-simulation-hour decay rate affected by state and activity.

Example defaults (`BALANCE_TUNABLE`):

- Hunger: -10/hour awake
- Energy: -5/hour awake; +22/hour sleeping
- Hygiene: -2/hour base; additional -5 after exercise/swim
- Entertainment: -4/hour while idle
- Social: -3/hour for social guests; -1/hour for private guests

Need values clamp to 0–100.

---

## 7. Goal Generation

AI uses hierarchical utility selection.

### 7.1 Goal Classes

- ReachHotel
- CheckIn
- ReachRoom
- Sleep
- Eat
- Drink
- Bathe
- Work
- Exercise
- Swim
- Socialize
- Relax
- AttendEvent
- RequestService
- ResolveComplaint
- Checkout
- LeaveHotel

Mandatory lifecycle goals override discretionary utility goals.

### 7.2 Utility Formula

For discretionary goal `g`:

```text
U(g) =
  NeedPressure(g)
  * Preference(g)
  * Availability(g)
  * TimeCompatibility(g)
  * BudgetCompatibility(g)
  * GroupCompatibility(g)
  * DistanceUtility(g)
  * MoodModifier(g)
```

Each factor is 0.0–1.5 except `NeedPressure`, which is 0.0–2.0.

### 7.3 Need Pressure

For a need score `N`:

```text
NeedPressure = ((100 - N) / 100)^2 * 2
```

This makes urgent needs disproportionately important.

### 7.4 Distance Utility

```text
DistanceUtility = 1 / (1 + ExpectedTravelMinutes / 10)
```

Expected travel includes elevator wait.

### 7.5 Reassessment

A guest reevaluates goals when:

- current goal completes,
- current target becomes invalid,
- wait exceeds personal abort threshold,
- mandatory lifecycle state changes,
- need reaches critical threshold (<15),
- group leader changes group plan,
- event invitation starts.

Otherwise, discretionary reassessment occurs every 60 simulation seconds.

---

## 8. Queue Psychology

Each service queue exposes expected wait.

Guest personal queue tolerance:

```text
ToleranceMinutes = BaseServiceTolerance
  * (0.5 + Patience)
  * SegmentModifier
  * UrgencyModifier
```

Default base tolerances (`BALANCE_TUNABLE`):

- reception check-in: 8 min
- checkout: 6 min
- restaurant host: 12 min
- bar: 8 min
- elevator: 6 min
- concierge: 10 min

If projected wait > tolerance, the guest may:

1. choose alternate service,
2. abandon action,
3. complain if service is considered promised/essential,
4. continue waiting while accumulating negative memory if no substitute exists.

Queue dissatisfaction rate after tolerance is exceeded:

`-0.6 satisfaction points per excess simulated minute * ServiceSensitivity`.

---

## 9. Expectations

Each guest computes expected standards at booking and revalidates on arrival.

```text
Expectation =
  SegmentBaseExpectation
  + StarClassModifier
  + ReputationModifier
  + PricePositionModifier
  + MarketingClaimModifier
```

Each category expectation is clamped 0–100.

Categories:

- Room
- Cleanliness
- Service
- Food
- Amenities
- Quiet
- Convenience
- Value

Price position is calculated relative to comparable competitors, not absolute price alone.

High expectations increase downside when service underperforms but increase satisfaction only modestly when merely met.

---

## 10. Experience Events

Guest satisfaction changes through discrete `ExperienceEvent` records.

Each event contains:

```text
EventType
Timestamp
LocationID
SourceEntityID|null
Category
ObservedValue
ExpectedValue
RawImpact
MemorySalience
ResolvedFlag
```

Examples:

- FastCheckIn
- LongCheckInQueue
- RoomNotReady
- FreeUpgrade
- DirtyBathroom
- ExcellentRoomCleanliness
- BrokenAC
- QuickMaintenanceRecovery
- GreatMeal
- SlowRoomService
- ElevatorDelay
- NoiseDisturbance
- StaffRudeness
- StaffExceptionalService

---

## 11. Satisfaction Calculation

Guest category satisfaction begins at 70 on arrival, then updates from experience events.

Final overall satisfaction:

```text
Overall = Σ(CategoryScore[c] * GuestCategoryWeight[c])
```

Default global category weights before guest-specific modulation:

- Room: 0.28
- Service: 0.24
- Cleanliness: 0.16
- Food: 0.10
- Amenities: 0.08
- Convenience: 0.06
- Value: 0.05
- ArrivalDeparture: 0.03

Weights are normalized to sum to 1 after archetype modifiers.

---

## 12. Memory Model

Each memory has:

```text
Valence: -1..+1
Magnitude: 0..100
Salience: 0..1
Category
DecayHalfLifeHours
ResolvedFlag
```

Current memory contribution:

```text
Contribution = Valence * Magnitude * Salience * 0.5^(AgeHours / HalfLifeHours)
```

Critical events may have half-lives longer than the stay and persist unchanged until checkout.

Examples:

- dirty towel: 12 h
- slow elevator: 6 h
- free suite upgrade: 48 h
- room unavailable at arrival: 72 h
- theft/security incident: non-decaying during stay

---

## 13. Complaint Decision

A guest creates a complaint when all are true:

1. a negative experience event is complaint-eligible,
2. unresolved negative magnitude >= 25,
3. guest is not already complaining about the same incident,
4. one of the following applies:
   - category sensitivity >= 0.55,
   - magnitude >= 50,
   - `ComplaintProne` trait.

Complaint urgency:

- Low: impact 25–39
- Medium: 40–59
- High: 60–79
- Critical: 80–100

---

## 14. Service Recovery

Resolution options modify the original memory and may add a recovery memory.

Examples (`BALANCE_TUNABLE`):

- apology only: reduce unresolved magnitude by 10 if issue already fixed
- free drink: reduce by 15 for low/medium complaints
- partial room refund: reduce by 25
- room upgrade: reduce by 35 if target room is superior
- full night refund: reduce by 45

Recovery effectiveness is multiplied by:

`StaffHospitality * GuestForgiveness * ResponseSpeedFactor`.

ResponseSpeedFactor:

- <=5 min: 1.25
- <=15 min: 1.00
- <=30 min: 0.80
- <=60 min: 0.60
- >60 min: 0.40

---

## 15. Group AI

A `GuestGroup` stores:

- members,
- leader,
- shared reservation,
- cohesion 0–1,
- shared itinerary,
- group budget where applicable.

Families and couples prefer synchronized discretionary activities. Group leader proposes actions; members accept when individual utility is within 30% of their best alternative.

Children never independently perform hotel lifecycle actions such as check-in or room reassignment.

---

## 16. Sleep and Noise

A sleeping guest samples effective room noise every 5 sim minutes.

Sleep disruption probability per sample:

```text
P = clamp((Noise - PersonalNoiseThreshold) / 50, 0, 1) * LightSleepModifier
```

On disruption:

- Energy gain pauses for 5 sim minutes.
- negative noise memory is added.
- repeated disruptions may generate complaint after 3 disruptions within 60 sim minutes.

---

## 17. Reviews

At `CompletedStay`, review generation probability defaults to 0.65.

Modifiers:

- satisfaction <=40: +0.20
- satisfaction >=90: +0.10
- critic/reviewer archetype: forced 1.00
- group tour guest: ×0.50 unless group leader

Overall review score 1.0–10.0:

```text
ReviewScore = clamp(1 + OverallSatisfaction * 0.09 + Noise, 1, 10)
```

`Noise` is deterministic RNG in range -0.3..+0.3.

Text generation must select statements tied to the guest’s strongest 1–3 positive/negative memories. Reviews must never invent services or incidents not experienced.

---

## 18. VIP and Critic Behavior

VIPs do not use a separate satisfaction system. They use:

- higher expectations,
- stronger privacy/security weighting,
- lower tolerance for visible operational failure,
- greater reputation multiplier on review/publicity.

A critic’s review reputation impact multiplier defaults to 5× a normal guest review.

---

## 19. AI Performance Rules

- Guests with no visible/urgent interaction may use coarse updates.
- Path queries are cached until target/path-invalidating event.
- A guest may not recompute every venue utility every second; candidate venues are prefiltered by service type and reachable floor zone.
- Full decision recalculation is event-triggered or 60-second cadence.

---

## 20. Required Player Diagnostics

Selecting a guest must show:

- archetype,
- budget band,
- current lifecycle state,
- current goal,
- current need scores,
- satisfaction by category,
- expectation by category,
- active memories with source and timestamp,
- current queue tolerance if queued,
- likely review range,
- complaints and resolution status.
