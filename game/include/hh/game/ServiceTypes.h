#pragma once

#include <cstdint>

namespace hh::game {

using ServiceId = std::uint64_t;
using RoomId = ServiceId;
using TaskId = ServiceId;
using AssetId = ServiceId;
using WorkOrderId = ServiceId;
using GuestId = ServiceId;
using RoomServiceOrderId = ServiceId;
using StorageNodeId = ServiceId;
using LaundryBatchId = ServiceId;
using PurchaseOrderId = ServiceId;

// Stable diagnostic reasons consumed by FINAL-07. Keep these semantic rather than
// presentation-specific so UI/frontends can map them without mutating simulation state.
enum class BlockReason : std::uint8_t {
  None,
  MissingCleanLinen,
  MissingTowels,
  MissingAmenities,
  MissingChemicals,
  MissingWasher,
  MissingDryer,
  MissingFoldingStation,
  MissingCleanStorage,
  MissingDirtyStorage,
  InsufficientStorageCapacity,
  MissingReceiving,
  MissingRoute,
  MissingWasteCapacity,
  AwaitingPart,
  AwaitingProduction
};

} // namespace hh::game
