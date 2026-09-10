from pathlib import Path

path = Path("game/tests/SimulationTests.cpp")
text = path.read_text(encoding="utf-8")

old = '''  for (int day = 0; day < 30; ++day) {
    if (s.view().inventory.linen < 12) {
      const auto order = s.orderSupplies({30, 60, 30, 30, 5});
      require(order.ok, "viable tutorial could not fund routine supplies");
    }
    s.step(86400);
  }
'''
new = '''  for (int day = 0; day < 30; ++day) {
    const auto state = s.view();
    const bool pendingSupply = std::any_of(
        state.supplyOrders.begin(), state.supplyOrders.end(),
        [](const auto &order) { return !order.delivered; });
    const auto &inventory = state.inventory;
    if (!pendingSupply &&
        (inventory.linen < 12 || inventory.towels < 24 ||
         inventory.amenities < 12 || inventory.chemicals < 12 ||
         inventory.parts < 2)) {
      const SupplyOrder refill{
          std::max(0, 24 - inventory.linen),
          std::max(0, 48 - inventory.towels),
          std::max(0, 36 - inventory.amenities),
          std::max(0, 24 - inventory.chemicals),
          std::max(0, 8 - inventory.parts)};
      const auto order = s.orderSupplies(refill);
      require(order.ok, "viable tutorial could not fund physical restock");
    }
    s.step(86400);
  }
'''

if text.count(old) != 1:
    raise SystemExit(f"physical restock test anchor count={text.count(old)}")
text = text.replace(old, new, 1)
path.write_text(text, encoding="utf-8")
