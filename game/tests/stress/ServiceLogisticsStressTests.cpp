#include "StressHarness.h"
#include "hh/game/ServiceLogistics.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
using namespace hh::game;

struct ScaleBudget {
  int rooms;
  int assets;
  std::size_t operations;
  int simulatedDays;
};

ScaleBudget budget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return {500, 100, 10'000, 30};
  case hh::stress::Scale::Extended:
    return {2'000, 500, 100'000, 365};
  case hh::stress::Scale::Exhaustive:
    return {5'000, 1'000, 500'000, 2'000};
  }
  std::abort();
}

void seedInventory(ServiceLogisticsRuntime &runtime,
                   hh::stress::RunContext &ctx) {
  auto &logistics = runtime.logistics();
  if (!logistics.addInventory(logistics.firstStorage(StorageKind::CleanLinen),
                              "clean_linen_set", 400) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::DirtyLinen),
                              "dirty_linen_set", 100) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                              "towel_unit", 100) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                              "amenity_kit", 50) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                              "cleaning_chemical", 50) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::CentralStorage),
                              "maintenance_part", 200))
    ctx.fail("failed to seed bounded service inventory", 0);
}

void assertLogistics(const LogisticsSnapshot &snapshot,
                     hh::stress::RunContext &ctx,
                     std::uint64_t checkpoint) {
  std::unordered_set<StorageNodeId> storageIds;
  for (const auto &node : snapshot.storage) {
    storageIds.insert(node.id);
    if (node.capacityUnits <= 0 || node.usedUnits < 0 || node.reservedUnits < 0 ||
        node.usedUnits > node.capacityUnits || node.reservedUnits > node.capacityUnits)
      ctx.fail("storage capacity/reservation invariant violated", checkpoint);
  }
  for (const auto &stack : snapshot.inventory)
    if (!storageIds.contains(stack.storage) || stack.quantity < 0 ||
        stack.reservedQuantity < 0 || stack.reservedQuantity > stack.quantity)
      ctx.fail("inventory stack invariant violated", checkpoint);
  for (const auto &order : snapshot.purchaseOrders)
    if (order.quantity <= 0 || !storageIds.contains(order.destination) ||
        order.remainingSeconds < 0)
      ctx.fail("purchase order invariant violated", checkpoint);
  for (const auto &move : snapshot.stockMoves)
    if (move.quantity <= 0 || !storageIds.contains(move.from) ||
        !storageIds.contains(move.to) || move.remainingSeconds < 0)
      ctx.fail("stock move invariant violated", checkpoint);
  if (snapshot.wasteAtSources < 0 || snapshot.wasteInBackOfHouse < 0 ||
      snapshot.wasteOverflowUnits < 0)
    ctx.fail("negative waste accounting", checkpoint);
}

void assertSubsystems(ServiceLogisticsRuntime &runtime,
                      hh::stress::RunContext &ctx,
                      std::uint64_t checkpoint) {
  assertLogistics(runtime.logisticsSnapshot(), ctx, checkpoint);
  for (const auto &job : runtime.housekeeping().snapshot().jobs)
    if (job.id == 0 || job.roomId == 0 || job.remainingSeconds < 0)
      ctx.fail("housekeeping job invariant violated", checkpoint);
  for (const auto &batch : runtime.laundry().snapshot().batches)
    if (batch.id == 0 || batch.quantity <= 0 || batch.remainingSeconds < 0)
      ctx.fail("laundry batch invariant violated", checkpoint);
  for (const auto &asset : runtime.engineering().snapshot().assets)
    if (asset.id == 0 || asset.condition < 0 || asset.condition > 10'000 ||
        asset.failurePressure < 0)
      ctx.fail("engineering asset invariant violated", checkpoint);
  for (const auto &order : runtime.engineering().snapshot().workOrders)
    if (order.id == 0 || order.assetId == 0 || order.remainingSeconds < 0)
      ctx.fail("engineering work-order invariant violated", checkpoint);
}

void registerFixture(ServiceLogisticsRuntime &runtime, const ScaleBudget &limits) {
  for (int room = 0; room < limits.rooms; ++room)
    runtime.registerRoom(static_cast<RoomId>(10'000 + room), ServiceRoomStatus::Ready);
  for (int asset = 0; asset < limits.assets; ++asset)
    runtime.registerAsset(static_cast<AssetId>(20'000 + asset),
                          4'000 + (asset % 6'000));
}

void crisisOperation(ServiceLogisticsRuntime &runtime, const ScaleBudget &limits,
                     std::string_view scenario, hh::stress::Rng &rng,
                     std::size_t operation, hh::stress::RunContext &ctx) {
  const auto room = static_cast<RoomId>(10'000 + rng.bounded(limits.rooms));
  const auto asset = static_cast<AssetId>(20'000 + rng.bounded(limits.assets));
  std::uint64_t action = rng.bounded(9);
  if (scenario == "checkout_storm") action = operation % 3;
  if (scenario == "receiving_saturation") action = 6 + operation % 2;
  if (scenario == "maintenance_storm") action = 3 + operation % 2;
  if (scenario == "room_service_burst") action = 5;

  switch (action) {
  case 0:
    (void)runtime.requestRoomTurn(room);
    ctx.trace.push("room-turn=" + std::to_string(room));
    break;
  case 1:
    (void)runtime.workRoomTurnSecond(room);
    ctx.trace.push("room-work=" + std::to_string(room));
    break;
  case 2:
    runtime.tickSeconds(1 + static_cast<std::int64_t>(rng.bounded(30)));
    ctx.trace.push("service-tick");
    break;
  case 3:
    (void)runtime.createWorkOrder(
        asset, (rng.next() & 1ULL) ? WorkOrderType::Preventive
                                  : WorkOrderType::Corrective);
    ctx.trace.push("work-order=" + std::to_string(asset));
    break;
  case 4:
    (void)runtime.workEngineeringSecond(
        asset, (rng.next() & 1ULL) ? WorkOrderType::Preventive
                                  : WorkOrderType::Corrective);
    ctx.trace.push("engineering-work=" + std::to_string(asset));
    break;
  case 5: {
    RoomServiceOrder order;
    const auto id = runtime.placeRoomServiceOrder(
        static_cast<GuestId>(30'000 + rng.bounded(limits.rooms * 4ULL)), order);
    if (id != 0)
      (void)runtime.markRoomServiceProductionReady(id);
    ctx.trace.push("room-service=" + std::to_string(id));
    break;
  }
  case 6: {
    auto &logistics = runtime.logistics();
    const auto result = logistics.placePurchaseOrder(
        "maintenance_part", 1 + static_cast<int>(rng.bounded(25)),
        logistics.firstStorage(StorageKind::CentralStorage),
        1 + static_cast<int>(rng.bounded(120)));
    ctx.trace.push("receiving-po=" + std::to_string(result.id));
    break;
  }
  case 7:
    runtime.logistics().produceWaste(1 + static_cast<int>(rng.bounded(4)));
    if ((rng.next() & 15ULL) == 0)
      runtime.logistics().requestWastePickup();
    ctx.trace.push("waste");
    break;
  case 8:
    (void)runtime.requestLaundryBatch(1 + static_cast<int>(rng.bounded(5)));
    ctx.trace.push("laundry-request");
    break;
  default:
    ctx.fail("invalid service action", operation);
  }
}

void stressCrisis(const ScaleBudget &limits, const hh::stress::Config &config,
                  std::string_view scenario) {
  hh::stress::RunContext ctx{"service-logistics", config};
  ctx.phase = std::string(scenario);
  ServiceLogisticsRuntime runtime(config.seed, {4, 4, 4});
  seedInventory(runtime, ctx);
  registerFixture(runtime, limits);
  const int initialLinen = runtime.laundry().totalLinenUnits();
  hh::stress::Rng rng{config.seed ^ 0xA0761D6478BD642FULL};

  for (std::size_t operation = 0; operation < limits.operations; ++operation) {
    crisisOperation(runtime, limits, scenario, rng, operation, ctx);
    if ((operation & 255U) == 0U)
      assertSubsystems(runtime, ctx, operation);
    if ((scenario == "save_in_crisis" || scenario == "combined_crisis") &&
        operation != 0 && operation % 2048U == 0U) {
      const auto encoded = runtime.save();
      auto restored = ServiceLogisticsRuntime::load(encoded);
      if (restored.save() != encoded)
        ctx.fail("service save/load round-trip diverged", operation);
      restored.tickSeconds(60);
      auto replay = ServiceLogisticsRuntime::load(encoded);
      replay.tickSeconds(60);
      if (restored.save() != replay.save())
        ctx.fail("loaded service continuation was not deterministic", operation);
      ctx.trace.push("crisis-roundtrip");
    }
  }

  for (int day = 0; day < limits.simulatedDays; ++day) {
    for (int window = 0; window < 4; ++window) {
      runtime.tickSeconds(6 * 3600);
      assertSubsystems(runtime, ctx,
                       limits.operations + static_cast<std::uint64_t>(day * 4 + window));
    }
  }

  const int finalLinen = runtime.laundry().totalLinenUnits();
  if (finalLinen != initialLinen)
    ctx.fail("physical linen was minted or lost", limits.simulatedDays,
             std::to_string(initialLinen) + "/" + std::to_string(finalLinen));
}

void stressLaundryStarvation(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"service-logistics", config};
  ctx.phase = "laundry_starvation";
  ServiceLogisticsRuntime runtime(config.seed ^ 0xE7037ED1A0B428DBULL,
                                  {0, 0, 0});
  auto &logistics = runtime.logistics();
  if (!logistics.addInventory(logistics.firstStorage(StorageKind::DirtyLinen),
                              "dirty_linen_set", 30))
    ctx.fail("failed to seed dirty linen starvation fixture", 0);
  const int initialLinen = runtime.laundry().totalLinenUnits();
  const auto batch = runtime.requestLaundryBatch(30);
  if (batch == 0)
    ctx.fail("valid laundry batch rejected", 0);
  runtime.tickSeconds(10'000);
  const auto snapshot = runtime.laundry().snapshot();
  const auto blocked = std::find_if(
      snapshot.batches.begin(), snapshot.batches.end(),
      [batch](const LaundryBatchView &view) { return view.id == batch; });
  if (blocked == snapshot.batches.end() ||
      blocked->stage != LaundryStage::AwaitingWasher ||
      blocked->blockedReason != BlockReason::MissingWasher)
    ctx.fail("laundry starvation did not remain safely blocked", 10'000);
  if (runtime.laundry().totalLinenUnits() != initialLinen)
    ctx.fail("blocked laundry lost physical linen", 10'000);
  const auto encoded = runtime.save();
  if (ServiceLogisticsRuntime::load(encoded).save() != encoded)
    ctx.fail("blocked laundry state did not round-trip", 10'001);
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "checkout_storm" ||
      scenario == "laundry_starvation" || scenario == "receiving_saturation" ||
      scenario == "maintenance_storm" || scenario == "room_service_burst" ||
      scenario == "combined_crisis" || scenario == "save_in_crisis")
    return;
  throw std::invalid_argument("unknown service/logistics stress scenario");
}
} // namespace

int main() {
  try {
    const auto config = hh::stress::configFromEnvironment(0x5EED04045EED0404ULL);
    validateScenario(config.scenario);
    const auto limits = budget(config.scale);

    if (config.scenario.empty()) {
      for (const auto scenario : {"checkout_storm", "receiving_saturation",
                                  "maintenance_storm", "room_service_burst",
                                  "combined_crisis", "save_in_crisis"})
        stressCrisis(limits, config, scenario);
      stressLaundryStarvation(config);
    } else if (config.scenario == "laundry_starvation") {
      stressLaundryStarvation(config);
    } else {
      stressCrisis(limits, config, config.scenario);
    }

    std::cout << "StressServiceLogistics PASS rooms=" << limits.rooms
              << " operations=" << limits.operations
              << " simulated_days=" << limits.simulatedDays << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
