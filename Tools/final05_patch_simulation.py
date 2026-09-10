from pathlib import Path

path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")


def replace_once(old: str, new: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"expected exactly one match, got {count}: {old[:80]!r}")
    text = text.replace(old, new, 1)


replace_once(
    "  ServiceLogisticsRuntime services{1};\n  EconomyView economy{2500000};",
    "  ServiceLogisticsRuntime services{1};\n"
    "  FoodServiceSystem food;\n"
    "  EventsSystem events;\n"
    "  AmenitiesSystem amenities;\n"
    "  EconomyView economy{2500000};",
)

replace_once(
    "  int utilityPerRoomDayCents{350};\n\n  int index(Position p) const { return (p.floor * height + p.y) * width + p.x; }",
    "  int utilityPerRoomDayCents{350};\n\n"
    "  void configureTutorialFinal05() {\n"
    "    food.addRecipe({1, \"Classic Breakfast\", {{\"eggs\", 2}, {\"bread\", 2}},\n"
    "                    FoodStationClass::Oven, 30, 120, 20, 8500, 1800});\n"
    "    food.addRecipe({2, \"Grilled Dinner\", {{\"protein\", 1}, {\"produce\", 2}},\n"
    "                    FoodStationClass::Range, 60, 240, 30, 8800, 3200});\n"
    "    food.addRecipe({3, \"House Cocktail\", {{\"beverage_base\", 1}, {\"garnish\", 1}},\n"
    "                    FoodStationClass::Bar, 45, 0, 10, 8400, 1600});\n"
    "    food.setIngredientStock(\"eggs\", 200);\n"
    "    food.setIngredientStock(\"bread\", 200);\n"
    "    food.setIngredientStock(\"protein\", 100);\n"
    "    food.setIngredientStock(\"produce\", 200);\n"
    "    food.setIngredientStock(\"beverage_base\", 100);\n"
    "    food.setIngredientStock(\"garnish\", 100);\n"
    "    food.setStaffCapacity(8);\n"
    "    food.addStation({1, FoodStationClass::Oven, 3, true});\n"
    "    food.addStation({2, FoodStationClass::Range, 4, true});\n"
    "    food.addStation({3, FoodStationClass::Bar, 3, true});\n"
    "    food.addStation({4, FoodStationClass::Plating, 4, true});\n"
    "    food.addStation({5, FoodStationClass::ServicePass, 4, true});\n"
    "    food.addVenue({101, FoodOrderChannel::Breakfast, 48, 360, 630, 20, 45, 30, 20, true});\n"
    "    food.addVenue({102, FoodOrderChannel::Restaurant, 64, 660, 1380, 20, 60, 30, 30, true});\n"
    "    food.addVenue({103, FoodOrderChannel::Bar, 32, 960, 120, 15, 30, 20, 20, true});\n"
    "    events.addFunctionSpace({301, 200, true});\n"
    "    events.setFoodServiceCapacity(120);\n"
    "    events.setStaffCapacity(24);\n"
    "    amenities.addAmenity({201, AmenityType::Gym, 300, 1380, 24, 0, 10000, 10000, 0, 3600, true});\n"
    "    amenities.addAmenity({202, AmenityType::Spa, 540, 1260, 4, 4, 10000, 10000, 15000, 3600, true});\n"
    "    amenities.addAmenity({203, AmenityType::Pool, 420, 1320, 36, 2, 10000, 10000, 0, 3600, true});\n"
    "    food.setElapsedSeconds(elapsed);\n"
    "    events.setElapsedSeconds(elapsed);\n"
    "    amenities.setElapsedSeconds(elapsed);\n"
    "  }\n\n"
    "  [[nodiscard]] std::int64_t final05RevenueCents() const {\n"
    "    return food.snapshot().revenueCents + events.snapshot().revenueCents +\n"
    "           amenities.snapshot().revenueCents;\n"
    "  }\n\n"
    "  int index(Position p) const { return (p.floor * height + p.y) * width + p.x; }",
)

replace_once(
    "  s.hireStaff({\"Casey\", PersonKind::Maintenance, 10, 12, 25});\n  s.impl_->elapsed = 14 * 3600;\n  return s;",
    "  s.hireStaff({\"Casey\", PersonKind::Maintenance, 10, 12, 25});\n"
    "  s.impl_->elapsed = 14 * 3600;\n"
    "  s.impl_->configureTutorialFinal05();\n"
    "  return s;",
)

replace_once(
    "  while (impl_->remainderMillis >= 1000) {\n    impl_->remainderMillis -= 1000;\n    impl_->minute();\n    impl_->services.tickSecond();\n  }",
    "  while (impl_->remainderMillis >= 1000) {\n"
    "    impl_->remainderMillis -= 1000;\n"
    "    const auto revenueBefore = impl_->final05RevenueCents();\n"
    "    impl_->minute();\n"
    "    impl_->services.tickSecond();\n"
    "    impl_->food.tickSecond();\n"
    "    impl_->events.tickSecond();\n"
    "    impl_->amenities.tickSecond();\n"
    "    for (const auto &handoff : impl_->food.pendingRoomServiceHandoffs())\n"
    "      if (impl_->services.markRoomServiceProductionReady(\n"
    "              handoff.roomServiceOrderId))\n"
    "        (void)impl_->food.acknowledgeRoomServiceHandoff(\n"
    "            handoff.foodOrderId);\n"
    "    const auto revenueAfter = impl_->final05RevenueCents();\n"
    "    if (revenueAfter < revenueBefore)\n"
    "      throw std::logic_error(\"FINAL-05 revenue moved backwards\");\n"
    "    const auto revenueDelta = revenueAfter - revenueBefore;\n"
    "    impl_->economy.revenueCents += revenueDelta;\n"
    "    impl_->economy.cashCents += revenueDelta;\n"
    "  }",
)

replace_once(
    "bool Simulation::requestRoomServiceTrayPickup(RoomServiceOrderId orderId) {\n  return impl_->services.requestRoomServiceTrayPickup(orderId);\n}\n\nbool Simulation::isReachable",
    "bool Simulation::requestRoomServiceTrayPickup(RoomServiceOrderId orderId) {\n"
    "  return impl_->services.requestRoomServiceTrayPickup(orderId);\n"
    "}\n"
    "FoodOrderId Simulation::createFoodOrder(GuestId guestId,\n"
    "                                        const MenuOrder &order) {\n"
    "  RoomServiceOrderId deliveryId{};\n"
    "  if (order.channel == FoodOrderChannel::RoomService) {\n"
    "    RoomServiceOrder delivery;\n"
    "    delivery.itemCount = std::max(1, order.quantity);\n"
    "    deliveryId = impl_->services.placeRoomServiceOrder(guestId, delivery);\n"
    "    if (deliveryId == 0)\n"
    "      return 0;\n"
    "  }\n"
    "  const auto id = impl_->food.createFoodOrder(guestId, order, deliveryId);\n"
    "  if (id != 0 && (order.channel == FoodOrderChannel::RoomService ||\n"
    "                  order.channel == FoodOrderChannel::Banquet))\n"
    "    (void)impl_->food.beginProduction(id);\n"
    "  return id;\n"
    "}\n"
    "EventQuote Simulation::quoteEvent(const EventRequest &request) const {\n"
    "  return impl_->events.quoteEvent(request);\n"
    "}\n"
    "EventBookingId Simulation::confirmEvent(const EventRequest &request) {\n"
    "  return impl_->events.confirmEvent(request);\n"
    "}\n"
    "AmenityReservationResult Simulation::reserveAmenity(\n"
    "    GuestId guestId, const AmenityRequest &request) {\n"
    "  return impl_->amenities.reserveAmenity(guestId, request);\n"
    "}\n"
    "FoodServiceSnapshot Simulation::foodServiceSnapshot() const {\n"
    "  return impl_->food.snapshot();\n"
    "}\n"
    "EventsSnapshot Simulation::eventsSnapshot() const {\n"
    "  return impl_->events.snapshot();\n"
    "}\n"
    "AmenitiesSnapshot Simulation::amenitiesSnapshot() const {\n"
    "  return impl_->amenities.snapshot();\n"
    "}\n\n"
    "bool Simulation::isReachable",
)

replace_once('  o << std::setprecision(17) << "HHGS 8 "',
             '  o << std::setprecision(17) << "HHGS 9 "')
replace_once(
    "  o << '\\n';\n  return o.str();\n}\nSimulation Simulation::load",
    "  o << '\\n';\n"
    "  const auto foodState = impl_->food.save();\n"
    "  o << \"FINAL05_FOOD \" << foodState.size() << '\\n';\n"
    "  o.write(foodState.data(), static_cast<std::streamsize>(foodState.size()));\n"
    "  o << '\\n';\n"
    "  const auto eventState = impl_->events.save();\n"
    "  o << \"FINAL05_EVENTS \" << eventState.size() << '\\n';\n"
    "  o.write(eventState.data(), static_cast<std::streamsize>(eventState.size()));\n"
    "  o << '\\n';\n"
    "  const auto amenityState = impl_->amenities.save();\n"
    "  o << \"FINAL05_AMENITIES \" << amenityState.size() << '\\n';\n"
    "  o.write(amenityState.data(), static_cast<std::streamsize>(amenityState.size()));\n"
    "  o << '\\n';\n"
    "  return o.str();\n"
    "}\nSimulation Simulation::load",
)
replace_once(
    '  if (magic != "HHGS" || version < 2 || version > 8)\n',
    '  if (magic != "HHGS" || version < 2 || version > 9)\n',
)

replace_once(
    "  } else {\n    for (const auto &room : d.rooms) {\n      d.services.registerRoom(\n          room.id, room.status == RoomStatus::VacantReady ? ServiceRoomStatus::Ready\n                                                          : ServiceRoomStatus::Blocked);\n      d.services.registerAsset(\n          room.id,\n          std::clamp(static_cast<int>(std::llround(room.condition * 100.0)), 0, 10000));\n    }\n  }\n  if (!i)\n    throw std::invalid_argument(\"corrupt simulation save\");",
    "  } else {\n"
    "    for (const auto &room : d.rooms) {\n"
    "      d.services.registerRoom(\n"
    "          room.id, room.status == RoomStatus::VacantReady ? ServiceRoomStatus::Ready\n"
    "                                                          : ServiceRoomStatus::Blocked);\n"
    "      d.services.registerAsset(\n"
    "          room.id,\n"
    "          std::clamp(static_cast<int>(std::llround(room.condition * 100.0)), 0, 10000));\n"
    "    }\n"
    "  }\n"
    "  if (version >= 9) {\n"
    "    auto readFinal05Section = [&](std::string_view expected) {\n"
    "      std::string tag;\n"
    "      std::size_t bytes{};\n"
    "      i >> tag >> bytes;\n"
    "      if (!i || tag != expected || bytes > 16 * 1024 * 1024)\n"
    "        throw std::invalid_argument(\"invalid FINAL-05 save section\");\n"
    "      if (i.get() != '\\n')\n"
    "        throw std::invalid_argument(\"invalid FINAL-05 save delimiter\");\n"
    "      std::string state(bytes, '\\0');\n"
    "      i.read(state.data(), static_cast<std::streamsize>(bytes));\n"
    "      if (!i || static_cast<std::size_t>(i.gcount()) != bytes)\n"
    "        throw std::invalid_argument(\"truncated FINAL-05 save section\");\n"
    "      if (i.get() != '\\n')\n"
    "        throw std::invalid_argument(\"invalid FINAL-05 save terminator\");\n"
    "      return state;\n"
    "    };\n"
    "    d.food = FoodServiceSystem::load(readFinal05Section(\"FINAL05_FOOD\"));\n"
    "    d.events = EventsSystem::load(readFinal05Section(\"FINAL05_EVENTS\"));\n"
    "    d.amenities = AmenitiesSystem::load(\n"
    "        readFinal05Section(\"FINAL05_AMENITIES\"));\n"
    "    if (d.food.snapshot().elapsedSeconds != d.elapsed ||\n"
    "        d.events.snapshot().elapsedSeconds != d.elapsed ||\n"
    "        d.amenities.snapshot().elapsedSeconds != d.elapsed)\n"
    "      throw std::invalid_argument(\"FINAL-05 clock does not match simulation\");\n"
    "  } else {\n"
    "    d.food.setElapsedSeconds(d.elapsed);\n"
    "    d.events.setElapsedSeconds(d.elapsed);\n"
    "    d.amenities.setElapsedSeconds(d.elapsed);\n"
    "  }\n"
    "  if (!i)\n"
    "    throw std::invalid_argument(\"corrupt simulation save\");",
)

path.write_text(text, encoding="utf-8")
print("FINAL-05 Simulation integration applied")
