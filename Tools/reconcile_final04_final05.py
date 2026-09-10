from pathlib import Path

path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one anchor, found {count}")
    text = text.replace(old, new, 1)


def insert_before(marker: str, payload: str, label: str) -> None:
    global text
    count = text.count(marker)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one marker, found {count}")
    text = text.replace(marker, payload + marker, 1)


replace_once(
    "  ServiceLogisticsRuntime services{1};\n  EconomyView economy{2500000};",
    "  ServiceLogisticsRuntime services{1};\n"
    "  FoodServiceSystem food;\n"
    "  EventsSystem events;\n"
    "  AmenitiesSystem amenities;\n"
    "  EconomyView economy{2500000};",
    "FINAL-05 subsystem members",
)

helpers = r'''  void configureTutorialFinal05() {
    food.addRecipe({1, "Classic Breakfast", {{"eggs", 2}, {"bread", 2}},
                    FoodStationClass::Oven, 30, 120, 20, 8500, 1800});
    food.addRecipe({2, "Grilled Dinner", {{"protein", 1}, {"produce", 2}},
                    FoodStationClass::Range, 60, 240, 30, 8800, 3200});
    food.addRecipe({3, "House Cocktail", {{"beverage_base", 1}, {"garnish", 1}},
                    FoodStationClass::Bar, 45, 0, 10, 8400, 1600});
    food.setIngredientStock("eggs", 200);
    food.setIngredientStock("bread", 200);
    food.setIngredientStock("protein", 100);
    food.setIngredientStock("produce", 200);
    food.setIngredientStock("beverage_base", 100);
    food.setIngredientStock("garnish", 100);
    food.setStaffCapacity(8);
    food.addStation({1, FoodStationClass::Oven, 3, true});
    food.addStation({2, FoodStationClass::Range, 4, true});
    food.addStation({3, FoodStationClass::Bar, 3, true});
    food.addStation({4, FoodStationClass::Plating, 4, true});
    food.addStation({5, FoodStationClass::ServicePass, 4, true});
    food.addVenue({101, FoodOrderChannel::Breakfast, 48, 360, 630, 20, 45, 30, 20, true});
    food.addVenue({102, FoodOrderChannel::Restaurant, 64, 660, 1380, 20, 60, 30, 30, true});
    food.addVenue({103, FoodOrderChannel::Bar, 32, 960, 120, 15, 30, 20, 20, true});
    events.addFunctionSpace({301, 200, true});
    events.setFoodServiceCapacity(120);
    events.setStaffCapacity(24);
    amenities.addAmenity({201, AmenityType::Gym, 300, 1380, 24, 0, 10000, 10000, 0, 3600, true});
    amenities.addAmenity({202, AmenityType::Spa, 540, 1260, 4, 4, 10000, 10000, 15000, 3600, true});
    amenities.addAmenity({203, AmenityType::Pool, 420, 1320, 36, 2, 10000, 10000, 0, 3600, true});
    food.setElapsedSeconds(elapsed);
    events.setElapsedSeconds(elapsed);
    amenities.setElapsedSeconds(elapsed);
  }

  [[nodiscard]] std::int64_t final05RevenueCents() const {
    return food.snapshot().revenueCents + events.snapshot().revenueCents +
           amenities.snapshot().revenueCents;
  }

'''
insert_before(
    "  int index(Position p) const { return (p.floor * height + p.y) * width + p.x; }\n",
    helpers,
    "FINAL-05 helpers",
)

replace_once(
    "  s.impl_->elapsed = 14 * 3600;\n  return s;",
    "  s.impl_->elapsed = 14 * 3600;\n"
    "  s.impl_->configureTutorialFinal05();\n"
    "  return s;",
    "tutorial FINAL-05 initialization",
)

old_step = (
    "  while (impl_->remainderMillis >= 1000) {\n"
    "    impl_->remainderMillis -= 1000;\n"
    "    impl_->minute();\n"
    "    impl_->services.tickSecond();\n"
    "  }"
)
new_step = (
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
    "  }"
)
replace_once(old_step, new_step, "authoritative FINAL-05 tick")

public_api = r'''FoodOrderId Simulation::createFoodOrder(GuestId guestId,
                                        const MenuOrder &order) {
  RoomServiceOrderId deliveryId{};
  if (order.channel == FoodOrderChannel::RoomService) {
    RoomServiceOrder delivery;
    delivery.itemCount = std::max(1, order.quantity);
    deliveryId = impl_->services.placeRoomServiceOrder(guestId, delivery);
    if (deliveryId == 0)
      return 0;
  }
  const auto id = impl_->food.createFoodOrder(guestId, order, deliveryId);
  if (id != 0 && (order.channel == FoodOrderChannel::RoomService ||
                  order.channel == FoodOrderChannel::Banquet))
    (void)impl_->food.beginProduction(id);
  return id;
}
EventQuote Simulation::quoteEvent(const EventRequest &request) const {
  return impl_->events.quoteEvent(request);
}
EventBookingId Simulation::confirmEvent(const EventRequest &request) {
  return impl_->events.confirmEvent(request);
}
AmenityReservationResult Simulation::reserveAmenity(
    GuestId guestId, const AmenityRequest &request) {
  return impl_->amenities.reserveAmenity(guestId, request);
}
FoodServiceSnapshot Simulation::foodServiceSnapshot() const {
  return impl_->food.snapshot();
}
EventsSnapshot Simulation::eventsSnapshot() const {
  return impl_->events.snapshot();
}
AmenitiesSnapshot Simulation::amenitiesSnapshot() const {
  return impl_->amenities.snapshot();
}

'''
insert_before(
    "bool Simulation::isReachable(Position a, Position b) const {\n",
    public_api,
    "FINAL-05 public API",
)

replace_once(
    '  o << std::setprecision(17) << "HHGS 8 "',
    '  o << std::setprecision(17) << "HHGS 9 "',
    "HHGS schema version",
)

final05_save = r'''  const auto foodState = impl_->food.save();
  o << "FINAL05_FOOD " << foodState.size() << '\n';
  o.write(foodState.data(), static_cast<std::streamsize>(foodState.size()));
  o << '\n';
  const auto eventState = impl_->events.save();
  o << "FINAL05_EVENTS " << eventState.size() << '\n';
  o.write(eventState.data(), static_cast<std::streamsize>(eventState.size()));
  o << '\n';
  const auto amenityState = impl_->amenities.save();
  o << "FINAL05_AMENITIES " << amenityState.size() << '\n';
  o.write(amenityState.data(), static_cast<std::streamsize>(amenityState.size()));
  o << '\n';
'''
insert_before(
    "  return o.str();\n}\nSimulation Simulation::load",
    final05_save,
    "FINAL-05 save sections",
)

replace_once(
    '  if (magic != "HHGS" || version < 2 || version > 8)\n',
    '  if (magic != "HHGS" || version < 2 || version > 9)\n',
    "HHGS load range",
)

final05_load = r'''  if (version >= 9) {
    auto readFinal05Section = [&](std::string_view expected) {
      std::string tag;
      std::size_t bytes{};
      i >> tag >> bytes;
      if (!i || tag != expected || bytes > 16 * 1024 * 1024)
        throw std::invalid_argument("invalid FINAL-05 save section");
      if (i.get() != '\n')
        throw std::invalid_argument("invalid FINAL-05 save delimiter");
      std::string state(bytes, '\0');
      i.read(state.data(), static_cast<std::streamsize>(bytes));
      if (!i || static_cast<std::size_t>(i.gcount()) != bytes)
        throw std::invalid_argument("truncated FINAL-05 save section");
      if (i.get() != '\n')
        throw std::invalid_argument("invalid FINAL-05 save terminator");
      return state;
    };
    d.food = FoodServiceSystem::load(readFinal05Section("FINAL05_FOOD"));
    d.events = EventsSystem::load(readFinal05Section("FINAL05_EVENTS"));
    d.amenities = AmenitiesSystem::load(
        readFinal05Section("FINAL05_AMENITIES"));
    if (d.food.snapshot().elapsedSeconds != d.elapsed ||
        d.events.snapshot().elapsedSeconds != d.elapsed ||
        d.amenities.snapshot().elapsedSeconds != d.elapsed)
      throw std::invalid_argument("FINAL-05 clock does not match simulation");
  } else {
    d.food.setElapsedSeconds(d.elapsed);
    d.events.setElapsedSeconds(d.elapsed);
    d.amenities.setElapsedSeconds(d.elapsed);
  }
'''
insert_before(
    '  if (!i)\n    throw std::invalid_argument("corrupt simulation save");\n',
    final05_load,
    "FINAL-05 load sections",
)

if any(marker in text for marker in ("<<<<<<<", "=======", ">>>>>>>")):
    raise SystemExit("merge markers remain in transformed Simulation.cpp")

required = (
    "FoodServiceSystem food;",
    "EventsSystem events;",
    "AmenitiesSystem amenities;",
    "HHGS 9 ",
    "FINAL05_FOOD",
    "FINAL05_EVENTS",
    "FINAL05_AMENITIES",
    "Simulation::createFoodOrder",
    "pendingRoomServiceHandoffs",
)
for token in required:
    if token not in text:
        raise SystemExit(f"required FINAL-05 token missing after transform: {token}")

path.write_text(text, encoding="utf-8")
