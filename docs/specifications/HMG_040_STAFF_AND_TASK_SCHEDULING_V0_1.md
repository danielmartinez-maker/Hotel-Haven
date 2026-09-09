# HMG-040 — Staff and Task Scheduling
## Authoritative Subsystem Specification v0.1

## 1. Purpose

This system governs employees, shifts, breaks, skills, task eligibility, task priority, travel, work execution, fatigue, morale, overtime, absenteeism, and department-management automation.

---

## 2. Employee Data

```text
EmployeeID
Name
Role
Department
SkillVector
Hospitality
Accuracy
Speed
Stamina
Experience
Traits[]
BaseWage
EmploymentType
ScheduleTemplate
CurrentShift
CurrentTaskID|null
CurrentLocation
Fatigue
Stress
Morale
NeedsBreak
AttendanceState
TrainingFlags
Permissions[]
```

Core attributes are 0–100 unless explicitly boolean/enumerated.

---

## 3. Baseline Roles

### Front Office
- Receptionist
- Concierge
- Bell Attendant
- Front Office Supervisor

### Housekeeping
- Housekeeper
- Public Area Cleaner
- Housekeeping Supervisor
- Laundry Worker

### Engineering
- Maintenance Technician
- Engineer
- Chief Engineer

### Food & Beverage
- Host
- Server
- Bartender
- Cook
- Chef
- Dishwasher
- F&B Manager

### Security
- Security Guard
- Security Supervisor

### Management
- Duty Manager
- General Manager

Roles define eligible task classes, required permissions, and skill vectors.

---

## 4. Shift Model

A shift contains:

```text
StartTime
EndTime
Role
AssignedDepartment
AssignedZone|null
BreakPolicy
OvertimeAllowed
```

Canonical sample shifts:

- 06:00–14:00
- 14:00–22:00
- 22:00–06:00

Players may create arbitrary shifts with minimum duration 2 hours and maximum scheduled duration 12 hours.

---

## 5. Attendance States

- `OffDuty`
- `TravelingToWork`
- `ClockingIn`
- `OnDutyAvailable`
- `Working`
- `OnBreak`
- `ClockingOut`
- `Overtime`
- `Absent`

Employees do not accept new tasks when `OffDuty`, `OnBreak`, or `Absent` unless emergency override applies.

---

## 6. Task Object

Every operational job is a `Task`:

```text
TaskID
TaskType
CreatedAt
SourceEntityID
TargetLocation
Department
RequiredRoleSet
RequiredSkillThresholds
RequiredToolOrInventory
BasePriority
Urgency
Deadline|null
EstimatedWorkSeconds
Status
AssignedEmployeeID|null
ClaimedResources[]
Dependencies[]
FailureState|null
```

---

## 7. Task State Machine

`Created -> Blocked|Ready -> Assigned -> Traveling -> Executing -> Completed`

Alternate transitions:

- `Assigned -> Ready` when employee becomes unavailable before start
- `Traveling -> Ready` if path invalidates
- `Executing -> Blocked` if resource/dependency disappears
- any non-complete state -> `Cancelled`
- `Blocked -> Ready` when dependency resolved
- `Executing -> Failed` when task has explicit failure condition

No task may silently vanish.

---

## 8. Task Priority

Effective task priority is a numeric score.

```text
Priority =
 BasePriority
 + UrgencyPoints
 + DeadlinePoints
 + GuestImpactPoints
 + RevenueImpactPoints
 + BacklogAgePoints
 + PlayerOverridePoints
```

Suggested scale:

- routine: 20–39
- important: 40–59
- urgent: 60–79
- critical: 80–100+

Examples:

- refill housekeeping closet: 35
- clean departure room due in 3 h: 50
- clean room with arriving guest waiting: 85
- fire response: 150

---

## 9. Deadline Priority

If task has deadline:

```text
Slack = Deadline - Now - EstimatedTravel - EstimatedWork
```

Deadline points:

- >120 min slack: 0
- 60–120: +5
- 30–60: +15
- 0–30: +30
- negative slack: +45

---

## 10. Task Assignment / Bidding

For each ready task, eligible employees calculate bid cost:

```text
BidCost =
 TravelMinutes * 1.0
 + WorkMinutes / EffectiveSpeed
 + FatiguePenalty
 + ZonePenalty
 + TaskSwitchPenalty
 - SkillBonus
```

Lowest bid among eligible available employees wins, subject to priority ordering.

The scheduler evaluates higher-priority tasks first.

### Anti-Thrashing
An assigned task is not reassigned unless:

- assigned worker becomes invalid/unavailable,
- another critical task preempts it,
- current worker has not begun travel and new bid improves ETA by >=30%,
- player manually reassigns.

---

## 11. Skill Effects

Effective task duration:

```text
EffectiveDuration = BaseDuration / (0.75 + 0.75 * Skill/100) / SpeedTraitModifier
```

At skill 0: 1.333× base duration.
At skill 100: 0.667× base duration.

Quality-sensitive tasks also produce quality result:

```text
TaskQuality = clamp(50 + Skill*0.5 + Accuracy*0.3 - FatiguePenalty - StressPenalty + RNG(-5,+5), 0,100)
```

Deterministic RNG stream by task ID.

---

## 12. Fatigue

Fatigue 0–100.

While working:

- +6/hour normal work
- +10/hour heavy work
- +4/hour low-intensity desk work

While on paid break:

- -20/hour

Off duty:

- -12/hour awake abstraction
- reset toward 0 between sufficiently separated shifts according to rest model.

Effects:

- >60: work duration +10%
- >80: duration +25%, task-quality -10
- >95: strong mistake risk and employee requests relief.

---

## 13. Stress

Stress responds to:

- hostile guest interactions,
- excessive queue/backlog visible to role,
- repeated urgent task preemption,
- overtime,
- manager quality,
- understaffing.

Stress decays on breaks/off-duty.

Stress affects hospitality interactions and morale but does not directly duplicate fatigue.

---

## 14. Morale

Morale 0–100 calculated from rolling factors:

- compensation satisfaction 25%
- workload balance 25%
- breaks/rest 15%
- manager quality 15%
- staff facilities 10%
- guest interaction climate 10%

Effects:

- <40: productivity -5%, absence/turnover risk rises
- <20: productivity -15%, strong turnover risk
- >80: hospitality +5 effective points, small retention benefit

---

## 15. Break Rules

Default shift >6 h requires one 30-minute meal break.

Break scheduler attempts break during department demand trough while maintaining minimum role coverage.

If no valid break window exists, employee becomes `BreakOverdue`; morale and fatigue penalties begin after 60 minutes overdue.

---

## 16. Overtime

When scheduled shift ends:

- if employee has no task: clock out,
- if task is non-critical and policy forbids overtime: release task and clock out,
- if task is critical or policy permits: continue as `Overtime` until task completes or overtime cap reached.

Default max overtime: 4 h/day.

---

## 17. Zone Assignment

Players may assign employees to:

- whole property,
- floor,
- wing,
- room cluster,
- restaurant/department zone.

Zone assignment applies bid penalty for out-of-zone tasks rather than making them impossible unless set `StrictZone=true`.

---

## 18. Housekeeping Assignment Rules

Housekeepers may receive room-cleaning routes bundled into work packets.

Route bundling objective:

- minimize floor changes,
- minimize total travel,
- meet ready-room deadlines,
- respect cart inventory capacity.

Room with waiting arrival has precedence over occupied stayover clean unless policy explicitly prioritizes VIP/stayover.

---

## 19. Front Desk Staffing

Receptionists do not use discrete task dispatch while actively assigned to a desk station. Instead, they enter a `ServiceStation` worker pool.

Queue processor selects next guest by queue discipline:

- FIFO default,
- VIP priority lane if policy/station supports it,
- accessibility priority may apply.

Service time uses employee skill and transaction complexity.

---

## 20. Management Layer

Managers automate within explicit policy bounds.

Manager responsibilities can include:

- schedule recommendations,
- break timing,
- zone reassignment,
- overtime authorization within budget,
- routine task priority tuning,
- supply reorder approval within threshold.

Manager cannot:

- construct rooms,
- change room prices outside delegated revenue policy,
- take loans,
- close hotel,
- alter player hard constraints.

---

## 21. Staffing Forecast

The UI forecasts required labor hours by department from known workload:

```text
RequiredLaborHours = Σ(ExpectedTaskWork) + ExpectedServiceStationDemand
```

Coverage ratio:

`ScheduledLaborHours / RequiredLaborHours`.

Display by 1-hour bucket for next 24 hours and daily for next 7 days.

---

## 22. Understaffing Metric

Department utilization:

```text
Utilization = ProductiveWorkMinutes / AvailableOnDutyMinutes
```

Interpretation:

- <60%: low utilization
- 60–85%: healthy
- 85–95%: strained
- >95%: chronic overload

A department operating >95% for 4 consecutive hours receives an understaffing alert.

---

## 23. Absence and Turnover

Absence probability is evaluated before shift start based on morale, fatigue carryover, and traits.

Turnover evaluation occurs weekly, not every tick.

Leaving employees enter notice-period or immediate-quit state depending on morale severity/scenario labor rules.

---

## 24. Training

Training consumes employee time and money.

Training modifies specific skill vector values, not global levels.

Example:

- Guest Conflict Resolution: +10 Hospitality, +10 ComplaintHandling cap
- Advanced Housekeeping: +10 CleaningSkill
- Elevator Systems: enables advanced elevator repairs

---

## 25. Required Staff Diagnostics

Selecting employee shows:

- shift and overtime status,
- current/next task,
- route,
- eligible task types,
- skill breakdown,
- fatigue,
- stress,
- morale factor breakdown,
- workload utilization,
- rooms/tasks completed today,
- average task duration versus department norm.

Department view shows:

- on-duty count,
- scheduled count,
- backlog by priority,
- oldest task age,
- labor forecast,
- utilization,
- overtime projection.
