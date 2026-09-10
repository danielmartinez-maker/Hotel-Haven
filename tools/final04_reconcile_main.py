from pathlib import Path
import subprocess


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label} anchor mismatch: {count}")
    return text.replace(old, new, 1)


def git_show(spec: str) -> str:
    return subprocess.check_output(["git", "show", spec], text=True)


path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")
main = git_show("origin/main:game/src/Simulation.cpp")

# FINAL-04 remains the physical maintenance authority while FINAL-03 owns labor.
text = replace_once(
    text,
    """    if (kind == TaskKind::Repair &&\n        services.createWorkOrder(target, WorkOrderType::Corrective) == 0)\n      return;\n""",
    """    if (kind == TaskKind::Repair) {\n      const auto workSeconds = static_cast<int>(std::clamp<std::int64_t>(\n          static_cast<std::int64_t>(std::llround(repairWork)), 1,\n          7LL * 24LL * 60LL * 60LL));\n      if (services.engineering().createWorkOrder(\n              target, WorkOrderType::Corrective, workSeconds) == 0)\n        return;\n    }\n""",
    "repair-duration-authority",
)

inventory_end = text.index("\n  int index(Position p) const")
if "void syncEngineeringState()" not in text:
    helper = r'''
  void syncEngineeringState() {
    const auto engineering = services.engineering().snapshot();
    for (const auto &asset : engineering.assets) {
      auto *room = getRoom(asset.id);
      if (!room)
        continue;
      room->condition = asset.condition / 100.0;
      if (asset.failed && room->reservationId == 0 && !room->closed &&
          room->status != RoomStatus::Occupied &&
          room->status != RoomStatus::OutOfOrder) {
        room->status = RoomStatus::OutOfOrder;
        createTask(TaskKind::Repair, room->id, room->door, repairWork);
      }
    }
  }
'''
    text = text[:inventory_end] + helper + text[inventory_end:]

# Remove the old duplicate random daily room-condition mutation. Engineering now
# owns condition and failure state; Simulation only mirrors its snapshot.
daily_anchor = text.index("if (hour == 0 && minute == 0 && hourBoundary)")
wear_start = text.index("      std::vector<EntityId> newlyFailedRooms;", daily_anchor)
wear_end = text.index("      economy.distressed", wear_start)
text = text[:wear_start] + text[wear_end:]

constructor_anchor = '    throw std::logic_error("failed to seed FINAL-04 physical inventory");\n'
if "setConditionLossPerDayHundredths" not in text[text.index("Simulation::Simulation"):text.index("Simulation::~Simulation")]:
    text = replace_once(
        text,
        constructor_anchor,
        constructor_anchor
        + "  impl_->services.engineering().setConditionLossPerDayHundredths(\n"
        + "      static_cast<int>(std::llround(impl_->roomConditionLossPerDay * 100.0)));\n",
        "constructor-engineering-wear",
    )

# Use FINAL-03's complete definition parser, then bind inventory and wear tunables
# into FINAL-04 instead of retaining a second physical ledger.
ld_start = main.index("CommandResult Simulation::loadDefinitions(std::string_view j) {")
ld_end = main.index("void Simulation::step(double seconds) {", ld_start)
load_defs = main[ld_start:ld_end]
load_defs = replace_once(
    load_defs,
    """  if (!root.is_object())\n    return {false, \"Definitions must be a JSON object\"};\n""",
    """  if (!root.is_object())\n    return {false, \"Definitions must be a JSON object\"};\n  const bool inventoryDefinition =\n      root.find(\"initialLinen\") || root.find(\"initialTowels\") ||\n      root.find(\"initialAmenities\") || root.find(\"initialChemicals\") ||\n      root.find(\"initialParts\");\n""",
    "definitions-inventory-presence",
)
load_defs = replace_once(
    load_defs,
    """  *impl_ = std::move(d);\n  return {true, \"Definitions loaded\"};\n""",
    """  if (inventoryDefinition &&\n      !d.services.setScenarioInventory(d.inventory.linen, d.inventory.towels,\n                                       d.inventory.amenities,\n                                       d.inventory.chemicals,\n                                       d.inventory.parts))\n    return {false, \"Inventory definitions require idle FINAL-04 services and physical capacity\"};\n  d.services.engineering().setConditionLossPerDayHundredths(\n      static_cast<int>(std::llround(d.roomConditionLossPerDay * 100.0)));\n  d.inventory = d.serviceInventoryView();\n  *impl_ = std::move(d);\n  return {true, \"Definitions loaded\"};\n""",
    "definitions-final04-bindings",
)
cur_ld_start = text.index("CommandResult Simulation::loadDefinitions(std::string_view j) {")
cur_ld_end = text.index("void Simulation::step(double seconds) {", cur_ld_start)
text = text[:cur_ld_start] + load_defs + text[cur_ld_end:]

text = replace_once(
    text,
    """    impl_->minute();\n    impl_->services.tickSecond();\n""",
    """    impl_->minute();\n    impl_->services.tickSecond();\n    impl_->syncEngineeringState();\n""",
    "engineering-snapshot-sync",
)

# Rebuild serialization from current main's complete FINAL-03 v8 format, move the
# combined format to v9, and append an explicit FINAL-04 section. Main v8 remains
# the supported migration baseline; the unmerged FINAL-04 v8 was development-only.
save_start = main.index("std::string Simulation::save() const {")
namespace_start = main.rindex("\n} // namespace hh::game")
tail = main[save_start:namespace_start]
tail = replace_once(tail, '<< "HHGS 8 "', '<< "HHGS 9 "', "save-version-9")
tail = replace_once(
    tail,
    "  inv(impl_->inventory);\n",
    "  inv(impl_->serviceInventoryView());\n",
    "save-physical-inventory",
)
tail = replace_once(
    tail,
    """  o << '\\n';\n  return o.str();\n}\nSimulation Simulation::load(std::string_view data) {\n""",
    """  o << '\\n';\n  const auto serviceState = impl_->services.save();\n  o << \"FINAL04 \" << serviceState.size() << '\\n';\n  o.write(serviceState.data(), static_cast<std::streamsize>(serviceState.size()));\n  o << '\\n';\n  return o.str();\n}\nSimulation Simulation::load(std::string_view data) {\n""",
    "save-final04-section",
)
tail = replace_once(
    tail,
    'if (magic != "HHGS" || version < 2 || version > 8)',
    'if (magic != "HHGS" || version < 2 || version > 9)',
    "load-version-9",
)
service_load = r'''  if (version >= 9) {
    std::string final04Tag;
    std::size_t serviceBytes{};
    i >> final04Tag >> serviceBytes;
    if (!i || final04Tag != "FINAL04" || serviceBytes > 16 * 1024 * 1024)
      throw std::invalid_argument("invalid FINAL-04 save section");
    if (i.get() != '\n')
      throw std::invalid_argument("invalid FINAL-04 save delimiter");
    std::string serviceState(serviceBytes, '\0');
    i.read(serviceState.data(), static_cast<std::streamsize>(serviceBytes));
    if (!i || static_cast<std::size_t>(i.gcount()) != serviceBytes)
      throw std::invalid_argument("truncated FINAL-04 save section");
    d.services = ServiceLogisticsRuntime::load(serviceState);
    if (i.get() != '\n')
      throw std::invalid_argument("invalid FINAL-04 save terminator");
  } else {
    if (!d.services.setScenarioInventory(
            d.inventory.linen, d.inventory.towels, d.inventory.amenities,
            d.inventory.chemicals, d.inventory.parts))
      throw std::invalid_argument("legacy inventory exceeds FINAL-04 capacity");
    for (const auto &room : d.rooms) {
      d.services.registerRoom(
          room.id, room.status == RoomStatus::VacantReady
                       ? ServiceRoomStatus::Ready
                       : ServiceRoomStatus::Blocked);
      d.services.registerAsset(
          room.id,
          std::clamp(static_cast<int>(std::llround(room.condition * 100.0)),
                     0, 10000));
    }
  }
  d.services.engineering().setConditionLossPerDayHundredths(
      static_cast<int>(std::llround(d.roomConditionLossPerDay * 100.0)));
  d.inventory = d.serviceInventoryView();
'''
tail = replace_once(
    tail,
    """  if (!i)\n    throw std::invalid_argument(\"corrupt simulation save\");\n  i >> std::ws;\n""",
    service_load
    + """  if (!i)\n    throw std::invalid_argument(\"corrupt simulation save\");\n  i >> std::ws;\n""",
    "load-final04-section",
)
cur_save_start = text.index("std::string Simulation::save() const {")
cur_namespace = text.rindex("\n} // namespace hh::game")
text = text[:cur_save_start] + tail + text[cur_namespace:]
path.write_text(text, encoding="utf-8")

# Update legacy acceptance tests where FINAL-04 intentionally changes the physical
# model: gradual authoritative engineering wear/partial repair, and consumable
# restocking independent of recyclable linen stock.
tests = Path("game/tests/SimulationTests.cpp")
test_text = tests.read_text(encoding="utf-8")
test_text = test_text.replace('saved.find("HHGS 8 ', 'saved.find("HHGS 9 ')
test_text = replace_once(
    test_text,
    """    if (s.view().inventory.linen < 12) {\n      const auto order = s.orderSupplies({30, 60, 30, 30, 5});\n""",
    """    const auto inventory = s.view().inventory;\n    if (inventory.linen < 12 || inventory.towels < 24 ||\n        inventory.amenities < 12 || inventory.chemicals < 12 ||\n        inventory.parts < 2) {\n      const auto order = s.orderSupplies({30, 60, 30, 30, 5});\n""",
    "campaign-physical-reorder",
)
test_text = replace_once(
    test_text,
    """  s.step(10 * 3600);\n  auto failed = s.view();\n""",
    """  s.step(24 * 3600);\n  auto failed = s.view();\n""",
    "engineering-gradual-wear-window",
)
test_text = replace_once(
    test_text,
    '"daily wear did not create one repair task per failed room"',
    '"engineering wear did not create one repair task per failed room"',
    "engineering-wear-message",
)
test_text = replace_once(
    test_text,
    """  s.step(12 * 3600);\n  auto serviced = s.view();\n""",
    """  s.step(22 * 3600);\n  auto serviced = s.view();\n""",
    "engineering-repair-window",
)
test_text = replace_once(
    test_text,
    """    restored |=\n        roomView.condition == 100 && roomView.status != RoomStatus::OutOfOrder;\n""",
    """    restored |=\n        roomView.condition >= 80 && roomView.status != RoomStatus::OutOfOrder;\n""",
    "corrective-partial-restoration",
)
tests.write_text(test_text, encoding="utf-8")
