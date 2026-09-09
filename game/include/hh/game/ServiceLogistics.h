#pragma once

#include "hh/game/Engineering.h"
#include "hh/game/Housekeeping.h"
#include "hh/game/Laundry.h"
#include "hh/game/Logistics.h"
#include "hh/game/RoomService.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace hh::game {

// Authoritative FINAL-04 aggregate. The individual domains remain separately
// testable, while this seam provides one clock, persistence boundary and the
// stable commands/snapshot consumed by Simulation and later UI diagnostics.
class ServiceLogisticsRuntime {
public:
  explicit ServiceLogisticsRuntime(std::uint64_t seed = 1,
                                   LaundryStations stations = {1, 1, 1});
  ~ServiceLogisticsRuntime();
  ServiceLogisticsRuntime(ServiceLogisticsRuntime &&) noexcept;
  ServiceLogisticsRuntime &operator=(ServiceLogisticsRuntime &&) noexcept;
  ServiceLogisticsRuntime(const ServiceLogisticsRuntime &);
  ServiceLogisticsRuntime &operator=(const ServiceLogisticsRuntime &);

  [[nodiscard]] LogisticsSystem &logistics();
  [[nodiscard]] const LogisticsSystem &logistics() const;
  [[nodiscard]] HousekeepingSystem &housekeeping();
  [[nodiscard]] const HousekeepingSystem &housekeeping() const;
  [[nodiscard]] LaundrySystem &laundry();
  [[nodiscard]] const LaundrySystem &laundry() const;
  [[nodiscard]] EngineeringSystem &engineering();
  [[nodiscard]] const EngineeringSystem &engineering() const;
  [[nodiscard]] RoomServiceSystem &roomService();
  [[nodiscard]] const RoomServiceSystem &roomService() const;

  void registerRoom(RoomId room,
                    ServiceRoomStatus status = ServiceRoomStatus::Ready);
  void registerAsset(AssetId asset, int condition = 10000);
  [[nodiscard]] LogisticsSnapshot logisticsSnapshot() const;
  [[nodiscard]] TaskId requestRoomTurn(RoomId room);
  [[nodiscard]] LaundryBatchId requestLaundryBatch(int quantity);
  [[nodiscard]] WorkOrderId createWorkOrder(AssetId asset, WorkOrderType type);
  [[nodiscard]] RoomServiceOrderId placeRoomServiceOrder(
      GuestId guest, const RoomServiceOrder &order);
  [[nodiscard]] bool markRoomServiceProductionReady(RoomServiceOrderId order);
  [[nodiscard]] bool requestRoomServiceTrayPickup(RoomServiceOrderId order);

  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] std::int64_t elapsedSeconds() const;

  [[nodiscard]] std::string save() const;
  static ServiceLogisticsRuntime load(std::string_view data);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::game
