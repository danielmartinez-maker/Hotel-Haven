from pathlib import Path

path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")

def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label} anchor mismatch: {count}")
    text = text.replace(old, new)

replace_once(
'''  int utilityPerRoomDayCents{350};

  int index(Position p) const { return (p.floor * height + p.y) * width + p.x; }
''',
'''  int utilityPerRoomDayCents{350};

  InventoryView serviceInventoryView() const {
    const auto &logistics = services.logistics();
    return {logistics.inventoryUsable("clean_linen_set"),
            logistics.inventoryUsable("towel_unit"),
            logistics.inventoryUsable("amenity_kit"),
            logistics.inventoryUsable("cleaning_chemical"),
            logistics.inventoryUsable("maintenance_part")};
  }

  int index(Position p) const { return (p.floor * height + p.y) * width + p.x; }
''', "inventory-view")

replace_once(
'''  void createTask(TaskKind kind, EntityId target, Position pos, double work) {
    for (auto &t : tasks)
      if (t.targetId == target && t.kind == kind &&
          t.status != TaskStatus::Completed)
        return;
    Task t;
''',
'''  void createTask(TaskKind kind, EntityId target, Position pos, double work) {
    for (auto &t : tasks)
      if (t.targetId == target && t.kind == kind &&
          t.status != TaskStatus::Completed)
        return;
    if (kind == TaskKind::Turnover && services.requestRoomTurn(target) == 0)
      return;
    if (kind == TaskKind::Repair &&
        services.createWorkOrder(target, WorkOrderType::Corrective) == 0)
      return;
    Task t;
''', "create-task")

replace_once(
'''        if (t.kind == TaskKind::Turnover)
          resources = t.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) && inventory.linen >= 1 &&
                       inventory.towels >= 2 && inventory.amenities >= 1 &&
                       inventory.chemicals >= 1);
        if (t.kind == TaskKind::Repair)
          resources = t.resourcesClaimed || inventory.parts >= 1;
''',
'''        if (t.kind == TaskKind::Turnover) {
          const auto &logistics = services.logistics();
          resources = t.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) &&
                       logistics.inventoryUsable("clean_linen_set") >= 1 &&
                       logistics.inventoryUsable("towel_unit") >= 2 &&
                       logistics.inventoryUsable("amenity_kit") >= 1 &&
                       logistics.inventoryUsable("cleaning_chemical") >= 1);
        }
        if (t.kind == TaskKind::Repair)
          resources = t.resourcesClaimed ||
                      services.logistics().inventoryUsable("maintenance_part") >= 1;
''', "resource-precheck")

replace_once(
'''          if (t.kind == TaskKind::Repair && !t.resourcesClaimed) {
            inventory.parts--;
            t.resourcesClaimed = true;
          }
''',
'''          if (t.kind == TaskKind::Repair && !t.resourcesClaimed)
            t.resourcesClaimed = true;
''', "repair-claim")

replace_once(
'''              if (inventory.linen < 1 || inventory.towels < 2 ||
                  inventory.amenities < 1 || inventory.chemicals < 1) {
                t.status = TaskStatus::Blocked;
                t.blockedReason = "Required local supplies unavailable";
                t.employeeId = 0;
                p->task = 0;
                p->state = PersonState::Idle;
                continue;
              }
              inventory.linen--;
              inventory.towels -= 2;
              inventory.amenities--;
              inventory.chemicals--;
              t.resourcesClaimed = true;
''',
'''              const auto &logistics = services.logistics();
              if (logistics.inventoryUsable("clean_linen_set") < 1 ||
                  logistics.inventoryUsable("towel_unit") < 2 ||
                  logistics.inventoryUsable("amenity_kit") < 1 ||
                  logistics.inventoryUsable("cleaning_chemical") < 1) {
                t.status = TaskStatus::Blocked;
                t.blockedReason = "Required physical supplies unavailable";
                t.employeeId = 0;
                p->task = 0;
                p->state = PersonState::Idle;
                continue;
              }
              t.resourcesClaimed = true;
''', "supply-arrival")

replace_once(
'''        } else if (p->state == PersonState::Working) {
          const double fatiguePerHour =
''',
'''        } else if (p->state == PersonState::Working) {
          if (t.kind == TaskKind::Turnover) {
            const auto serviceWork = services.workRoomTurnSecond(t.targetId);
            if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
              t.status = TaskStatus::Blocked;
              t.blockedReason = !serviceWork.valid
                                    ? "FINAL-04 room-turn job missing"
                                    : "FINAL-04 room-turn resources blocked";
              t.employeeId = 0;
              p->task = 0;
              p->state = PersonState::Idle;
              continue;
            }
          } else if (t.kind == TaskKind::Repair) {
            const auto serviceWork = services.workEngineeringSecond(
                t.targetId, WorkOrderType::Corrective);
            if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
              t.status = TaskStatus::Blocked;
              t.blockedReason = !serviceWork.valid
                                    ? "FINAL-04 engineering work order missing"
                                    : "FINAL-04 engineering resources blocked";
              t.employeeId = 0;
              p->task = 0;
              p->state = PersonState::Idle;
              continue;
            }
          }
          const double fatiguePerHour =
''', "work-hook")

replace_once(
'''          t.workRemainingSeconds -= efficiency;
          if (t.workRemainingSeconds <= 0) {
            t.status = TaskStatus::Completed;
''',
'''          t.workRemainingSeconds -= efficiency;
          const bool serviceTurnComplete =
              t.kind != TaskKind::Turnover ||
              services.housekeeping().roomStatus(t.targetId) ==
                  ServiceRoomStatus::Ready;
          bool serviceRepairComplete = t.kind != TaskKind::Repair;
          if (t.kind == TaskKind::Repair) {
            const auto engineering = services.engineering().snapshot();
            for (const auto &order : engineering.workOrders)
              if (order.assetId == t.targetId &&
                  order.type == WorkOrderType::Corrective &&
                  order.stage == WorkOrderStage::Completed)
                serviceRepairComplete = true;
          }
          if (t.workRemainingSeconds <= 0 && serviceTurnComplete &&
              serviceRepairComplete) {
            t.status = TaskStatus::Completed;
''', "completion-gate")

replace_once(
'''              if (t.kind == TaskKind::Repair) {
                r->condition = 100;
                if (!r->closed) {
''',
'''              if (t.kind == TaskKind::Repair) {
                const auto engineering = services.engineering().snapshot();
                for (const auto &asset : engineering.assets)
                  if (asset.id == r->id)
                    r->condition = asset.condition / 100.0;
                if (!r->closed) {
''', "repair-condition")

replace_once(
'''    if (hourBoundary) {
      for (auto &o : orders)
        if (!o.delivered && o.etaDay <= day) {
          inventory.linen += o.items.linen;
          inventory.towels += o.items.towels;
          inventory.amenities += o.items.amenities;
          inventory.chemicals += o.items.chemicals;
          inventory.parts += o.items.parts;
          o.delivered = true;
        }
    }
''',
'''    if (hourBoundary) {
      for (auto &o : orders)
        if (!o.delivered && o.etaDay <= day)
          o.delivered = true;
    }
''', "legacy-delivery")

replace_once(
'''    staffAndTasks();
    guests();
    compactTransientState();
''',
'''    staffAndTasks();
    guests();
    const int dirtyLinen = services.logistics().totalInventory("dirty_linen_set");
    bool laundryActive = false;
    for (const auto &batch : services.laundry().snapshot().batches)
      laundryActive |= batch.stage != LaundryStage::Completed;
    if (dirtyLinen > 0 && !laundryActive)
      (void)services.requestLaundryBatch(std::min(dirtyLinen, 8));
    compactTransientState();
''', "auto-laundry")

replace_once(
'''  impl_->services = ServiceLogisticsRuntime(seed);
  auto &serviceInventory = impl_->services.logistics();
  const auto seedServiceItem = [&](StorageKind kind, std::string_view item, int quantity) {
    if (quantity > 0 &&
        !serviceInventory.addInventory(serviceInventory.firstStorage(kind), item, quantity))
      throw std::logic_error("failed to seed service inventory");
  };
  seedServiceItem(StorageKind::CleanLinen, "clean_linen_set", impl_->inventory.linen);
  seedServiceItem(StorageKind::FloorCloset, "towel_unit", impl_->inventory.towels);
  seedServiceItem(StorageKind::FloorCloset, "amenity_kit", impl_->inventory.amenities);
  seedServiceItem(StorageKind::FloorCloset, "cleaning_chemical", impl_->inventory.chemicals);
  seedServiceItem(StorageKind::CentralStorage, "maintenance_part", impl_->inventory.parts);
''',
'''  impl_->services = ServiceLogisticsRuntime(seed);
  if (!impl_->services.setScenarioInventory(
          impl_->inventory.linen, impl_->inventory.towels,
          impl_->inventory.amenities, impl_->inventory.chemicals,
          impl_->inventory.parts))
    throw std::logic_error("failed to seed FINAL-04 physical inventory");
''', "constructor-inventory")

old_order_start = text.index('CommandResult Simulation::orderSupplies(const SupplyOrder &o) {')
old_order_end = text.index('CommandResult Simulation::loadDefinitions', old_order_start)
new_order = r'''CommandResult Simulation::orderSupplies(const SupplyOrder &o) {
  const auto valid = [](int quantity) {
    return quantity >= 0 && quantity <= 100000;
  };
  const std::int64_t total = static_cast<std::int64_t>(o.linen) + o.towels +
                             o.amenities + o.chemicals + o.parts;
  if (!valid(o.linen) || !valid(o.towels) || !valid(o.amenities) ||
      !valid(o.chemicals) || !valid(o.parts) || total == 0)
    return {false, "Order quantities invalid"};

  const auto snapshot = impl_->services.logisticsSnapshot();
  StorageNodeId receiving{};
  StorageNodeId cleanLinen{};
  StorageNodeId central{};
  int cleanCapacity{};
  int centralCapacity{};
  for (const auto &node : snapshot.storage) {
    const int available = node.capacityUnits - node.usedUnits - node.reservedUnits;
    if (node.kind == StorageKind::Receiving && node.operational)
      receiving = node.id;
    else if (node.kind == StorageKind::CleanLinen && node.operational) {
      cleanLinen = node.id;
      cleanCapacity = available;
    } else if (node.kind == StorageKind::CentralStorage && node.operational) {
      central = node.id;
      centralCapacity = available;
    }
  }
  if (receiving == 0 || cleanLinen == 0 || central == 0)
    return {false, "Required receiving or storage is unavailable"};
  const int centralNeed = o.towels + o.amenities + o.chemicals + o.parts;
  if (o.linen > cleanCapacity || centralNeed > centralCapacity)
    return {false, "Insufficient physical storage capacity"};

  std::int64_t cost = static_cast<std::int64_t>(o.linen) * 1200 +
                      static_cast<std::int64_t>(o.towels) * 500 +
                      static_cast<std::int64_t>(o.amenities) * 250 +
                      static_cast<std::int64_t>(o.chemicals) * 800 +
                      static_cast<std::int64_t>(o.parts) * 3500;
  if (impl_->economy.cashCents < cost)
    return {false, "Insufficient cash"};

  auto &logistics = impl_->services.logistics();
  constexpr int leadSeconds = 2 * 24 * 60 * 60;
  const auto place = [&](std::string_view item, int quantity,
                         StorageNodeId destination) {
    if (quantity == 0)
      return true;
    return logistics.placePurchaseOrder(item, quantity, destination, leadSeconds).ok();
  };
  if (!place("clean_linen_set", o.linen, cleanLinen) ||
      !place("towel_unit", o.towels, central) ||
      !place("amenity_kit", o.amenities, central) ||
      !place("cleaning_chemical", o.chemicals, central) ||
      !place("maintenance_part", o.parts, central))
    throw std::logic_error("preflighted FINAL-04 purchase order failed");

  PendingOrder p;
  p.id = impl_->nextId++;
  p.items = {o.linen, o.towels, o.amenities, o.chemicals, o.parts};
  p.etaDay = impl_->elapsed / 86400 + 2;
  impl_->orders.push_back(p);
  impl_->economy.cashCents -= cost;
  impl_->economy.supplyCostCents += cost;
  return {true, "Order submitted to receiving", p.id};
}
'''
text = text[:old_order_start] + new_order + text[old_order_end:]

replace_once(
'''  if (!root.is_object())
    return {false, "Definitions must be a JSON object"};
  auto number = [&](std::string_view key, double &out) {
''',
'''  if (!root.is_object())
    return {false, "Definitions must be a JSON object"};
  const bool inventoryDefinition =
      root.find("initialLinen") || root.find("initialTowels") ||
      root.find("initialAmenities") || root.find("initialChemicals") ||
      root.find("initialParts");
  auto number = [&](std::string_view key, double &out) {
''', "definition-flag")

replace_once(
'''      d.roomConditionLossPerDay < 0 || d.roomConditionLossPerDay > 100)
    return {false, "Definition values are invalid"};
  *impl_ = std::move(d);
''',
'''      d.roomConditionLossPerDay < 0 || d.roomConditionLossPerDay > 100)
    return {false, "Definition values are invalid"};
  if (inventoryDefinition &&
      !d.services.setScenarioInventory(d.inventory.linen, d.inventory.towels,
                                       d.inventory.amenities,
                                       d.inventory.chemicals,
                                       d.inventory.parts))
    return {false, "Inventory definitions require idle FINAL-04 services and physical capacity"};
  d.inventory = d.serviceInventoryView();
  *impl_ = std::move(d);
''', "definition-physical")

replace_once(
'''TaskId Simulation::requestRoomTurn(RoomId roomId) {
  if (!impl_->getRoom(roomId))
    return 0;
  return impl_->services.requestRoomTurn(roomId);
}
''',
'''TaskId Simulation::requestRoomTurn(RoomId roomId) {
  auto *room = impl_->getRoom(roomId);
  if (!room || room->reservationId != 0 || room->status == RoomStatus::Occupied ||
      room->status == RoomStatus::OutOfOrder || room->condition < 40)
    return 0;
  const auto serviceTask = impl_->services.requestRoomTurn(roomId);
  if (serviceTask == 0)
    return 0;
  room->status = RoomStatus::VacantDirty;
  impl_->createTask(TaskKind::Turnover, roomId, room->door,
                    impl_->turnoverWork);
  return serviceTask;
}
''', "public-room-turn")

replace_once('''  v.inventory = impl_->inventory;
''', '''  v.inventory = impl_->serviceInventoryView();
''', "view-inventory")
replace_once('''  inv(impl_->inventory);
''', '''  inv(impl_->serviceInventoryView());
''', "save-inventory")

replace_once(
'''    d.services = ServiceLogisticsRuntime::load(serviceState);
    if (i.get() != '\n')
      throw std::invalid_argument("invalid FINAL-04 save terminator");
  } else {
    for (const auto &room : d.rooms) {
''',
'''    d.services = ServiceLogisticsRuntime::load(serviceState);
    d.inventory = d.serviceInventoryView();
    if (i.get() != '\n')
      throw std::invalid_argument("invalid FINAL-04 save terminator");
  } else {
    if (!d.services.setScenarioInventory(
            d.inventory.linen, d.inventory.towels, d.inventory.amenities,
            d.inventory.chemicals, d.inventory.parts))
      throw std::invalid_argument("legacy inventory exceeds FINAL-04 capacity");
    for (const auto &room : d.rooms) {
''', "load-inventory-migration")

path.write_text(text, encoding="utf-8")
