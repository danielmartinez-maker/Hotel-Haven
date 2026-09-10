from pathlib import Path

path = Path("game/src/ServiceLogistics.cpp")
text = path.read_text(encoding="utf-8")

old = '''TaskId ServiceLogisticsRuntime::requestRoomTurn(RoomId room) {
  return impl_->housekeeping.requestRoomTurn(room);
}
LaundryBatchId ServiceLogisticsRuntime::requestLaundryBatch(int quantity) {
'''
new = '''TaskId ServiceLogisticsRuntime::requestRoomTurn(RoomId room) {
  return impl_->housekeeping.requestRoomTurn(room);
}
ServiceWorkResult ServiceLogisticsRuntime::workRoomTurnSecond(RoomId room) {
  const auto job = impl_->housekeeping.latestJob(room);
  return job == 0 ? ServiceWorkResult{} : impl_->housekeeping.workSecond(job);
}
LaundryBatchId ServiceLogisticsRuntime::requestLaundryBatch(int quantity) {
'''
if text.count(old) != 1:
    raise SystemExit("requestRoomTurn anchor mismatch")
text = text.replace(old, new)

old = '''WorkOrderId ServiceLogisticsRuntime::createWorkOrder(AssetId asset,
                                                     WorkOrderType type) {
  return impl_->engineering.createWorkOrder(asset, type);
}
RoomServiceOrderId ServiceLogisticsRuntime::placeRoomServiceOrder(
'''
new = '''WorkOrderId ServiceLogisticsRuntime::createWorkOrder(AssetId asset,
                                                     WorkOrderType type) {
  return impl_->engineering.createWorkOrder(asset, type);
}
ServiceWorkResult ServiceLogisticsRuntime::workEngineeringSecond(
    AssetId asset, WorkOrderType type) {
  const auto order = impl_->engineering.latestWorkOrder(asset, type);
  return order == 0 ? ServiceWorkResult{} : impl_->engineering.workSecond(order);
}
RoomServiceOrderId ServiceLogisticsRuntime::placeRoomServiceOrder(
'''
if text.count(old) != 1:
    raise SystemExit("createWorkOrder anchor mismatch")
text = text.replace(old, new)

old = '''bool ServiceLogisticsRuntime::requestRoomServiceTrayPickup(
    RoomServiceOrderId order) {
  return impl_->roomService.requestTrayPickup(order);
}

void ServiceLogisticsRuntime::tickSecond() {
'''
new = '''bool ServiceLogisticsRuntime::requestRoomServiceTrayPickup(
    RoomServiceOrderId order) {
  return impl_->roomService.requestTrayPickup(order);
}

bool ServiceLogisticsRuntime::setScenarioInventory(
    int linen, int towels, int amenities, int chemicals, int parts) {
  if (linen < 0 || towels < 0 || amenities < 0 || chemicals < 0 || parts < 0)
    return false;

  for (const auto &job : impl_->housekeeping.jobs_)
    if (job.stage != HousekeepingStage::Completed)
      return false;
  for (const auto &batch : impl_->laundry.batches_)
    if (batch.stage != LaundryStage::Completed)
      return false;
  for (const auto &order : impl_->engineering.workOrders_)
    if (order.stage != WorkOrderStage::Completed)
      return false;
  for (const auto &order : impl_->logistics.orders_)
    if (order.state != PurchaseOrderState::Completed &&
        order.state != PurchaseOrderState::Cancelled &&
        order.state != PurchaseOrderState::RejectedNoCapacity)
      return false;
  for (const auto &move : impl_->logistics.moves_)
    if (!move.completed)
      return false;

  auto &logistics = impl_->logistics;
  const auto clean = logistics.firstStorage(StorageKind::CleanLinen);
  const auto central = logistics.firstStorage(StorageKind::CentralStorage);
  const auto *cleanNode = logistics.node(clean);
  const auto *centralNode = logistics.node(central);
  if (!cleanNode || !centralNode)
    return false;

  const auto knownItem = [](const std::string &item) {
    return item == "clean_linen_set" || item == "dirty_linen_set" ||
           item == "towel_unit" || item == "amenity_kit" ||
           item == "cleaning_chemical" || item == "maintenance_part";
  };
  int cleanOther = 0;
  int centralOther = 0;
  for (const auto &stack : logistics.inventory_) {
    if (knownItem(stack.item))
      continue;
    if (stack.storage == clean)
      cleanOther += stack.quantity;
    if (stack.storage == central)
      centralOther += stack.quantity;
  }
  if (cleanOther + linen > cleanNode->capacityUnits ||
      centralOther + towels + amenities + chemicals + parts >
          centralNode->capacityUnits)
    return false;

  logistics.inventory_.erase(
      std::remove_if(logistics.inventory_.begin(), logistics.inventory_.end(),
                     [&](const LogisticsSystem::Stack &stack) {
                       return knownItem(stack.item);
                     }),
      logistics.inventory_.end());

  const auto add = [&](StorageNodeId storage, std::string_view item, int quantity) {
    return quantity == 0 || logistics.addInventory(storage, item, quantity);
  };
  return add(clean, "clean_linen_set", linen) &&
         add(central, "towel_unit", towels) &&
         add(central, "amenity_kit", amenities) &&
         add(central, "cleaning_chemical", chemicals) &&
         add(central, "maintenance_part", parts);
}

void ServiceLogisticsRuntime::tickSecond() {
'''
if text.count(old) != 1:
    raise SystemExit("tray pickup anchor mismatch")
text = text.replace(old, new)

path.write_text(text, encoding="utf-8")
