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
};

ScaleBudget budget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return {500, 100, 50'000};
  case hh::stress::Scale::Extended:
    return {2'000, 500, 500'000};
  case hh::stress::Scale::Exhaustive:
    return {5'000, 1'000, 2'000'000};
  }
  std::abort();
}

void seedInventory(ServiceLogisticsRuntime &runtime,
                   hh::stress::RunContext &ctx) {
  auto &logistics = runtime.logistics();
  const auto clean = logistics.firstStorage(StorageKind::CleanLinen);
  const auto dirty = logistics.firstStorage(StorageKind::DirtyLinen);
  const auto closet = logistics.firstStorage(StorageKind::FloorCloset);
  const auto central = logistics.firstStorage(StorageKind::CentralStorage);
  if (!logistics.addInventory(clean, "clean_linen_set", 400) ||
      !logistics.addInventory(dirty, "dirty_linen_set", 100) ||
      !logistics.addInventory(closet, "towel_unit", 100) ||
      !logistics.addInventory(closet, "amenity_kit", 50) ||
      !logistics.addInventory(closet, "cleaning_chemical", 50) ||
      !logistics.addInventory(central, "maintenance_part", 200))
    ctx.fail("failed to seed bounded service inventory", 0);
}

void assertLogistics(const LogisticsSnapshot &snapshot,
                     hh::stress::RunContext &ctx,
                     std::uint64_t checkpoint) {
  std::unordered_set<StorageNodeId> storageIds;
  for (const auto &node : snapshot.storage) {
    storageIds.insert(node.id);
    if (node.capacityUnits <= 0 || node.usedUnits < 0 || node.reservedUnits < 0 ||
        node.usedUnits > node.capacityUnits ||
        node.reservedUnits > node.capacityUnits)
      ctx.fail("storage capacity/reservation invariant violated", checkpoint);
  }
  for (const auto &stack : snapshot.inventory) {
    if (!storageIds.contains(stack.storage) || stack.quantity < 0 ||
        stack.reservedQuantity < 0 || stack.reservedQuantity > stack.quantity)
      ctx.fail("inventory stack invariant violated", checkpoint);
  }
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
      ctx.fail("engineering work order invariant violated", checkpoint);
}

void registerFixture(ServiceLogisticsRuntime &runtime, const ScaleBudget &limits) {
  for (int room = 0; room < limits.rooms; ++room)
    runtime.registerRoom(static_cast<RoomId>(10'000 + room),
                         ServiceRoomStatus::Ready);
  for (int asset = 0; asset < limits.assets; ++asset)
    runtime.registerAsset(static_cast<AssetId>(20'000 + asset),
                          4'000 + (asset % 6'000));
}

void stressCombined(const ScaleBudget &limits,
                    const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"service-logistics", config};
  ctx.phase = "service_spike";
  ServiceLogisticsRuntime runtime(config.seed, {4, 4, 4});
  seedInventory(runtime, ctx);
  registerFixture(runtime, limits);
  const int initialLinen = runtime.laundry().totalLinenUnits();
  hh::stress::Rng rng{config.seed ^ 0xA0761D6478BD642FULL};

  for (std::size_t operation = 0; operation < limits.operations; ++operation) {
    const auto action = rng.bounded(8);
    const auto room = static_cast<RoomId>(10'000 + rng.bounded(limits.rooms));
    const auto asset = static_cast<AssetId>(20'000 + rng.bounded(limits.assets));
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
      (void)runtime.requestLaundryBatch(1 + static_cast<int>(rng.bounded(5)));
      ctx.trace.push("laundry-request");
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
          static_cast<GuestId>(30'000 + rng.bounded(limits.rooms * 4ULL)),
          order);
      if (id != 0 && (rng.next() & 1ULL))
        (void)runtime.markRoomServiceProductionReady(id);
      ctx.trace.push("room-service=" + std::to_string(id));
      break;
    }
    case 6:
      runtime.logistics().produceWaste(1 + static_cast<int>(rng.bounded(4)));
      if ((rng.next() & 15ULL) == 0)
        runtime.logistics().requestWastePickup();
      ctx.trace.push("waste");
      break;
    case 7:
      runtime.tickSeconds(1 + static_cast<std::int64_t>(rng.bounded(30)));
      ctx.trace.push("tick");
      break;
    default:
      ctx.fail("invalid service action", operation);
    }

    if ((operation & 255U) == 0U)
      assertSubsystems(runtime, ctx, operation);
    if (operation != 0 && operation % 8192U == 0U) {
      const auto encoded = runtime.save();
      auto restored = ServiceLogisticsRuntime::load(encoded);
      if (restored.save() != encoded)
        ctx.fail("service save/load round-trip diverged", operation);
      restored.tickSeconds(60);
      auto continued = ServiceLogisticsRuntime::load(encoded);
      continued.tickSeconds(60);
      if (restored.save() != continued.save())
        ctx.fail("service loaded continuation was not deterministic", operation);
      ctx.trace.push("roundtrip");
    }
  }

  runtime.tickSeconds(10'000);
  assertSubsystems(runtime, ctx, limits.operations);
  const int finalLinen = runtime.laundry().totalLinenUnits();
  if (finalLinen != initialLinen)
    ctx.fail("physical linen was minted or lost", limits.operations,
             std::to_string(initialLinen) + "/" + std::to_string(finalLinen));
}

void stressStarvationAndRecovery(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"service-logistics", config};
  ctx.phase = "inventory_starvation";
  ServiceLogisticsRuntime runtime(config.seed ^ 0xE7037ED1A0B428DBULL,
                                  {1, 1, 1});
  constexpr RoomId room = 77;
  runtime.registerRoom(room, ServiceRoomStatus::Ready);
  const auto job = runtime.requestRoomTurn(room);
  if (job == 0)
    ctx.fail("starvation fixture could not create room turn", 0);
  for (int second = 0; second < 1'000; ++second)
    (void)runtime.workRoomTurnSecond(room);
  const auto starved = runtime.housekeeping().snapshot();
  const auto blocked = std::any_of(
      starved.jobs.begin(), starved.jobs.end(), [](const HousekeepingJobView &view) {
        return view.blockedReason != BlockReason::None;
      });
  if (!blocked)
    ctx.fail("resource starvation did not block housekeeping", 1'000);

  auto &logistics = runtime.logistics();
  if (!logistics.addInventory(logistics.firstStorage(StorageKind::CleanLinen),
                              "clean_linen_set", 10) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                              "towel_unit", 10) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                              "amenity_kit", 10) ||
      !logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                              "cleaning_chemical", 10))
    ctx.fail("recovery inventory injection failed", 1'001);

  for (int second = 0; second < 10'000 &&
                       runtime.housekeeping().roomStatus(room) !=
                           ServiceRoomStatus::Ready;
       ++second) {
    (void)runtime.workRoomTurnSecond(room);
    runtime.tickSecond();
  }
  if (runtime.housekeeping().roomStatus(room) != ServiceRoomStatus::Ready)
    ctx.fail("housekeeping did not recover after replenishment", 11'000);
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "service_spike" ||
      scenario == "inventory_starvation" || scenario == "maintenance_burst" ||
      scenario == "laundry_backlog" || scenario == "room_service_wave" ||
      scenario == "save_in_crisis")
    return;
  throw std::invalid_argument("unknown service/logistics stress scenario");
}
} // namespace

int main() {
  try {
    const auto config = hh::stress::configFromEnvironment(0x5EED04045EED0404ULL);
    validateScenario(config.scenario);
    const auto limits = budget(config.scale);
    if (config.scenario.empty() || config.scenario != "inventory_starvation")
      stressCombined(limits, config);
    if (config.scenario.empty() || config.scenario == "inventory_starvation")
      stressStarvationAndRecovery(config);
    std::cout << "StressServiceLogistics PASS rooms=" << limits.rooms
              << " operations=" << limits.operations << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
