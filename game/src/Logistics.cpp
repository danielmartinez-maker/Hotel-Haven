#include "hh/game/Logistics.h"
#include <algorithm>
#include <limits>

namespace hh::game {

LogisticsSystem LogisticsSystem::standardHotel() {
  LogisticsSystem out;
  out.addStorage({StorageKind::Receiving, 1000, true});
  out.addStorage({StorageKind::CentralStorage, 1000, true});
  out.addStorage({StorageKind::CleanLinen, 500, true});
  out.addStorage({StorageKind::DirtyLinen, 500, true});
  out.addStorage({StorageKind::FloorCloset, 250, true});
  out.addStorage({StorageKind::Waste, 500, true});
  return out;
}

StorageNodeId LogisticsSystem::addStorage(const StorageNodeSpec &spec) {
  if (spec.capacityUnits <= 0)
    return 0;
  const auto id = nextId_++;
  storage_.push_back({id, spec.kind, spec.capacityUnits, 0, spec.operational});
  return id;
}

LogisticsSystem::StorageNode *LogisticsSystem::node(StorageNodeId id) {
  for (auto &entry : storage_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

const LogisticsSystem::StorageNode *LogisticsSystem::node(StorageNodeId id) const {
  for (const auto &entry : storage_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

int LogisticsSystem::usedUnits(StorageNodeId id) const {
  int used = 0;
  for (const auto &stack : inventory_)
    if (stack.storage == id)
      used += stack.quantity;
  return used;
}

StorageNodeId LogisticsSystem::firstStorage(StorageKind kind) const {
  for (const auto &entry : storage_)
    if (entry.kind == kind && entry.operational)
      return entry.id;
  return 0;
}

bool LogisticsSystem::addInventory(StorageNodeId storage, std::string_view item,
                                   int quantity) {
  if (quantity <= 0 || item.empty())
    return false;
  auto *destination = node(storage);
  if (!destination || !destination->operational ||
      usedUnits(storage) + quantity > destination->capacityUnits)
    return false;
  for (auto &stack : inventory_)
    if (stack.storage == storage && stack.item == item) {
      stack.quantity += quantity;
      return true;
    }
  inventory_.push_back({storage, std::string(item), quantity, 0});
  return true;
}

int LogisticsSystem::inventoryAt(StorageNodeId storage,
                                 std::string_view item) const {
  int quantity = 0;
  for (const auto &stack : inventory_)
    if (stack.storage == storage && stack.item == item)
      quantity += stack.quantity;
  return quantity;
}

bool LogisticsSystem::isUsable(StorageKind kind) const {
  return kind == StorageKind::CentralStorage || kind == StorageKind::CleanLinen ||
         kind == StorageKind::FloorCloset;
}

int LogisticsSystem::inventoryUsable(std::string_view item) const {
  int quantity = 0;
  for (const auto &stack : inventory_) {
    const auto *storage = node(stack.storage);
    if (storage && storage->operational && isUsable(storage->kind) &&
        stack.item == item)
      quantity += stack.quantity - stack.reservedQuantity;
  }
  return quantity;
}

int LogisticsSystem::totalInventory(std::string_view item) const {
  int quantity = 0;
  for (const auto &stack : inventory_)
    if (stack.item == item)
      quantity += stack.quantity;
  return quantity;
}

bool LogisticsSystem::consumeAt(StorageNodeId storage, std::string_view item,
                                int quantity) {
  if (quantity <= 0 || inventoryAt(storage, item) < quantity)
    return false;
  int remaining = quantity;
  for (auto &stack : inventory_) {
    if (stack.storage != storage || stack.item != item)
      continue;
    const int take = std::min(remaining, stack.quantity);
    stack.quantity -= take;
    remaining -= take;
    if (remaining == 0)
      break;
  }
  inventory_.erase(std::remove_if(inventory_.begin(), inventory_.end(),
                                  [](const Stack &stack) {
                                    return stack.quantity == 0 &&
                                           stack.reservedQuantity == 0;
                                  }),
                   inventory_.end());
  return remaining == 0;
}

bool LogisticsSystem::consumeUsable(std::string_view item, int quantity) {
  if (quantity <= 0 || inventoryUsable(item) < quantity)
    return false;
  int remaining = quantity;
  for (const auto &storage : storage_) {
    if (!storage.operational || !isUsable(storage.kind))
      continue;
    const int available = inventoryAt(storage.id, item);
    const int take = std::min(remaining, available);
    if (take > 0)
      consumeAt(storage.id, item, take);
    remaining -= take;
    if (remaining == 0)
      return true;
  }
  return false;
}

bool LogisticsSystem::consumeFromKind(StorageKind kind, std::string_view item,
                                      int quantity) {
  if (quantity <= 0)
    return false;
  int available = 0;
  for (const auto &storage : storage_)
    if (storage.operational && storage.kind == kind)
      available += inventoryAt(storage.id, item);
  if (available < quantity)
    return false;
  int remaining = quantity;
  for (const auto &storage : storage_) {
    if (!storage.operational || storage.kind != kind)
      continue;
    const int take = std::min(remaining, inventoryAt(storage.id, item));
    if (take > 0)
      consumeAt(storage.id, item, take);
    remaining -= take;
    if (remaining == 0)
      return true;
  }
  return false;
}

bool LogisticsSystem::canAddToKind(StorageKind kind, int quantity) const {
  if (quantity <= 0)
    return false;
  for (const auto &storage : storage_)
    if (storage.operational && storage.kind == kind &&
        usedUnits(storage.id) + storage.reservedUnits + quantity <=
            storage.capacityUnits)
      return true;
  return false;
}

bool LogisticsSystem::addToKind(StorageKind kind, std::string_view item,
                                int quantity) {
  if (quantity <= 0)
    return false;
  for (const auto &storage : storage_) {
    if (!storage.operational || storage.kind != kind)
      continue;
    if (usedUnits(storage.id) + quantity <= storage.capacityUnits)
      return addInventory(storage.id, item, quantity);
  }
  return false;
}

OrderResult LogisticsSystem::placePurchaseOrder(std::string_view item, int quantity,
                                                StorageNodeId destination,
                                                int leadSeconds) {
  if (quantity <= 0 || item.empty() || leadSeconds < 0)
    return {0, OrderError::InvalidQuantity};
  auto *target = node(destination);
  if (!target || !target->operational)
    return {0, OrderError::UnknownDestination};
  if (firstStorage(StorageKind::Receiving) == 0)
    return {0, OrderError::MissingReceiving};
  if (usedUnits(destination) + target->reservedUnits + quantity >
      target->capacityUnits)
    return {0, OrderError::InsufficientStorageCapacity};

  const auto id = nextId_++;
  target->reservedUnits += quantity;
  orders_.push_back({id, std::string(item), quantity, destination,
                     PurchaseOrderState::InTransit, leadSeconds,
                     BlockReason::None});
  return {id, OrderError::None};
}

void LogisticsSystem::produceWaste(int quantity) {
  if (quantity > 0)
    wasteAtSources_ += quantity;
}

void LogisticsSystem::requestWastePickup() {
  if (wastePickupRemaining_ < 0 &&
      totalInventory("waste") > 0)
    wastePickupRemaining_ = 120;
}

void LogisticsSystem::tickSecond() {
  ++elapsedSeconds_;

  for (auto &order : orders_) {
    if (order.state != PurchaseOrderState::InTransit)
      continue;
    if (order.remainingSeconds > 0)
      --order.remainingSeconds;
    if (order.remainingSeconds > 0)
      continue;
    const auto receiving = firstStorage(StorageKind::Receiving);
    if (receiving == 0 || !addInventory(receiving, order.item, order.quantity)) {
      order.state = PurchaseOrderState::Arrived;
      order.blockedReason = BlockReason::MissingReceiving;
      continue;
    }
    order.state = PurchaseOrderState::Unloading;
    moves_.push_back({nextId_++, order.id, order.item, order.quantity, receiving,
                      order.destination, 120, false, BlockReason::None});
  }

  for (auto &move : moves_) {
    if (move.completed)
      continue;
    auto *destination = node(move.to);
    if (!destination || !destination->operational) {
      move.blockedReason = BlockReason::MissingRoute;
      continue;
    }
    if (move.remainingSeconds > 0)
      --move.remainingSeconds;
    if (move.remainingSeconds > 0)
      continue;
    if (inventoryAt(move.from, move.item) < move.quantity ||
        usedUnits(move.to) + move.quantity > destination->capacityUnits) {
      move.blockedReason = BlockReason::InsufficientStorageCapacity;
      continue;
    }
    if (!consumeAt(move.from, move.item, move.quantity) ||
        !addInventory(move.to, move.item, move.quantity)) {
      move.blockedReason = BlockReason::MissingRoute;
      continue;
    }
    destination->reservedUnits = std::max(0, destination->reservedUnits - move.quantity);
    move.completed = true;
    move.blockedReason = BlockReason::None;
    for (auto &order : orders_)
      if (order.id == move.orderId) {
        order.state = PurchaseOrderState::Completed;
        order.blockedReason = BlockReason::None;
        break;
      }
  }

  if (wasteAtSources_ > 0 && wasteCollectionRemaining_ < 0)
    wasteCollectionRemaining_ = 120;
  if (wasteCollectionRemaining_ >= 0) {
    if (wasteCollectionRemaining_ > 0)
      --wasteCollectionRemaining_;
    if (wasteCollectionRemaining_ == 0) {
      const int amount = wasteAtSources_;
      if (amount > 0 && addToKind(StorageKind::Waste, "waste", amount))
        wasteAtSources_ = 0;
      else if (amount > 0) {
        wasteOverflowUnits_ += amount;
        wasteAtSources_ = 0;
      }
      wasteCollectionRemaining_ = -1;
    }
  }

  if (wastePickupRemaining_ >= 0) {
    if (wastePickupRemaining_ > 0)
      --wastePickupRemaining_;
    if (wastePickupRemaining_ == 0) {
      const int amount = totalInventory("waste");
      if (amount > 0)
        consumeFromKind(StorageKind::Waste, "waste", amount);
      wastePickupRemaining_ = -1;
    }
  }
}

void LogisticsSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

LogisticsSnapshot LogisticsSystem::snapshot() const {
  LogisticsSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  out.wasteAtSources = wasteAtSources_;
  out.wasteInBackOfHouse = 0;
  out.wasteOverflowUnits = wasteOverflowUnits_;
  out.wastePickupActive = wastePickupRemaining_ >= 0;
  for (const auto &storage : storage_) {
    out.storage.push_back({storage.id, storage.kind, storage.capacityUnits,
                           usedUnits(storage.id), storage.reservedUnits,
                           storage.operational});
    if (storage.kind == StorageKind::Waste)
      out.wasteInBackOfHouse += inventoryAt(storage.id, "waste");
  }
  for (const auto &stack : inventory_)
    out.inventory.push_back(
        {stack.storage, stack.item, stack.quantity, stack.reservedQuantity});
  for (const auto &order : orders_)
    out.purchaseOrders.push_back({order.id, order.item, order.quantity,
                                  order.destination, order.state,
                                  order.remainingSeconds, order.blockedReason});
  for (const auto &move : moves_)
    out.stockMoves.push_back({move.id, move.orderId, move.item, move.quantity,
                              move.from, move.to, move.remainingSeconds,
                              move.completed, move.blockedReason});
  return out;
}

} // namespace hh::game
