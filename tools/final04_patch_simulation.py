from pathlib import Path

path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {count}")
    text = text.replace(old, new, 1)


replace_once(
    "  InventoryView inventory{24, 48, 36, 24, 8};\n  EconomyView economy{2500000};",
    "  InventoryView inventory{24, 48, 36, 24, 8};\n  ServiceLogisticsRuntime services{1};\n  EconomyView economy{2500000};",
    "service field",
)

replace_once(
    "  impl_->seed = seed;\n  impl_->rng.seed(seed);\n  impl_->width = w;",
    "  impl_->seed = seed;\n  impl_->rng.seed(seed);\n  impl_->services = ServiceLogisticsRuntime(seed);\n  auto &serviceInventory = impl_->services.logistics();\n  const auto seedServiceItem = [&](StorageKind kind, std::string_view item, int quantity) {\n    if (quantity > 0 &&\n        !serviceInventory.addInventory(serviceInventory.firstStorage(kind), item, quantity))\n      throw std::logic_error(\"failed to seed service inventory\");\n  };\n  seedServiceItem(StorageKind::CleanLinen, \"clean_linen_set\", impl_->inventory.linen);\n  seedServiceItem(StorageKind::FloorCloset, \"towel_unit\", impl_->inventory.towels);\n  seedServiceItem(StorageKind::FloorCloset, \"amenity_kit\", impl_->inventory.amenities);\n  seedServiceItem(StorageKind::FloorCloset, \"cleaning_chemical\", impl_->inventory.chemicals);\n  seedServiceItem(StorageKind::CentralStorage, \"maintenance_part\", impl_->inventory.parts);\n  impl_->width = w;",
    "constructor service init",
)

replace_once(
    "  impl_->rooms.push_back(r);\n  impl_->economy.cashCents -= cost;",
    "  impl_->rooms.push_back(r);\n  impl_->services.registerRoom(\n      r.id, r.status == RoomStatus::VacantReady ? ServiceRoomStatus::Ready\n                                                 : ServiceRoomStatus::Blocked);\n  impl_->services.registerAsset(\n      r.id, std::clamp(static_cast<int>(std::llround(r.condition * 100.0)), 0, 10000));\n  impl_->economy.cashCents -= cost;",
    "room service registration",
)

replace_once(
    "  while (impl_->remainderMillis >= 1000) {\n    impl_->remainderMillis -= 1000;\n    impl_->minute();\n  }\n}\nbool Simulation::isReachable",
    "  while (impl_->remainderMillis >= 1000) {\n    impl_->remainderMillis -= 1000;\n    impl_->minute();\n    impl_->services.tickSecond();\n  }\n}\n\nLogisticsSnapshot Simulation::logisticsSnapshot() const {\n  return impl_->services.logisticsSnapshot();\n}\nTaskId Simulation::requestRoomTurn(RoomId roomId) {\n  if (!impl_->getRoom(roomId))\n    return 0;\n  return impl_->services.requestRoomTurn(roomId);\n}\nLaundryBatchId Simulation::requestLaundryBatch(int quantity) {\n  return impl_->services.requestLaundryBatch(quantity);\n}\nWorkOrderId Simulation::createWorkOrder(AssetId assetId, WorkOrderType type) {\n  return impl_->services.createWorkOrder(assetId, type);\n}\nRoomServiceOrderId Simulation::placeRoomServiceOrder(\n    GuestId guestId, const RoomServiceOrder &order) {\n  return impl_->services.placeRoomServiceOrder(guestId, order);\n}\nbool Simulation::markRoomServiceProductionReady(RoomServiceOrderId orderId) {\n  return impl_->services.markRoomServiceProductionReady(orderId);\n}\nbool Simulation::requestRoomServiceTrayPickup(RoomServiceOrderId orderId) {\n  return impl_->services.requestRoomServiceTrayPickup(orderId);\n}\n\nbool Simulation::isReachable",
    "simulation service seam",
)

replace_once(
    '  o << std::setprecision(17) << "HHGS 7 " << impl_->seed << \' \' << impl_->width',
    '  o << std::setprecision(17) << "HHGS 8 " << impl_->seed << \' \' << impl_->width',
    "save version",
)

replace_once(
    "  for (auto &p : impl_->orders) {\n    o << p.id << ' ';\n    inv(p.items);\n    o << ' ' << p.etaDay << ' ' << p.delivered << '\\n';\n  }\n  return o.str();\n}\nSimulation Simulation::load",
    "  for (auto &p : impl_->orders) {\n    o << p.id << ' ';\n    inv(p.items);\n    o << ' ' << p.etaDay << ' ' << p.delivered << '\\n';\n  }\n  const auto serviceState = impl_->services.save();\n  o << \"FINAL04 \" << serviceState.size() << '\\n';\n  o.write(serviceState.data(), static_cast<std::streamsize>(serviceState.size()));\n  o << '\\n';\n  return o.str();\n}\nSimulation Simulation::load",
    "save service state",
)

replace_once(
    '  if (magic != "HHGS" || version < 2 || version > 7)',
    '  if (magic != "HHGS" || version < 2 || version > 8)',
    "load version",
)

replace_once(
    "  if (!i)\n    throw std::invalid_argument(\"corrupt simulation save\");\n  i >> std::ws;\n  if (!i.eof())",
    "  if (version >= 8) {\n    std::string final04Tag;\n    std::size_t serviceBytes{};\n    i >> final04Tag >> serviceBytes;\n    if (!i || final04Tag != \"FINAL04\" || serviceBytes > 16 * 1024 * 1024)\n      throw std::invalid_argument(\"invalid FINAL-04 save section\");\n    if (i.get() != '\\n')\n      throw std::invalid_argument(\"invalid FINAL-04 save delimiter\");\n    std::string serviceState(serviceBytes, '\\0');\n    i.read(serviceState.data(), static_cast<std::streamsize>(serviceBytes));\n    if (!i || static_cast<std::size_t>(i.gcount()) != serviceBytes)\n      throw std::invalid_argument(\"truncated FINAL-04 save section\");\n    d.services = ServiceLogisticsRuntime::load(serviceState);\n    if (i.get() != '\\n')\n      throw std::invalid_argument(\"invalid FINAL-04 save terminator\");\n  } else {\n    for (const auto &room : d.rooms) {\n      d.services.registerRoom(\n          room.id, room.status == RoomStatus::VacantReady ? ServiceRoomStatus::Ready\n                                                          : ServiceRoomStatus::Blocked);\n      d.services.registerAsset(\n          room.id,\n          std::clamp(static_cast<int>(std::llround(room.condition * 100.0)), 0, 10000));\n    }\n  }\n  if (!i)\n    throw std::invalid_argument(\"corrupt simulation save\");\n  i >> std::ws;\n  if (!i.eof())",
    "load service state",
)

path.write_text(text, encoding="utf-8")
print("FINAL-04 Simulation.cpp integration patch applied")
