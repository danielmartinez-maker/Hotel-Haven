#include "hh/game/Logistics.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

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
  const auto index = storage_.size();
  storage_.push_back({id, spec.kind, spec.capacityUnits, 0, spec.operational});
  storageIndex_.emplace(id, index);
  return id;
}

LogisticsSystem::StorageNode *LogisticsSystem::node(StorageNodeId id) {
  const auto found = storageIndex_.find(id);
  return found == storageIndex_.end() ? nullptr : &storage_[found->second];
}

const LogisticsSystem::StorageNode *LogisticsSystem::node(StorageNodeId id) const {
  const auto found = storageIndex_.find(id);
  return found == storageIndex_.end() ? nullptr : &storage_[found->second];
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
    if (take > 0 && !consumeAt(storage.id, item, take))
      return false;
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
    if (take > 0 && !consumeAt(storage.id, item, take))
      return false;
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
  const auto index = orders_.size();
  orders_.push_back({id, std::string(item), quantity, destination,
                     PurchaseOrderState::InTransit, leadSeconds,
                     BlockReason::None});
  orderIndex_.emplace(id, index);
  activeOrders_.push_back(index);
  transitOrders_.push_back(index);
  return {id, OrderError::None};
}

void LogisticsSystem::produceWaste(int quantity) {
  if (quantity > 0)
    wasteAtSources_ += quantity;
}

void LogisticsSystem::requestWastePickup() {
  if (wastePickupRemaining_ < 0 && totalInventory("waste") > 0)
    wastePickupRemaining_ = 120;
}

void LogisticsSystem::rebuildDerivedState() {
  storageIndex_.clear();
  storageIndex_.reserve(storage_.size());
  for (std::size_t index = 0; index < storage_.size(); ++index)
    storageIndex_.emplace(storage_[index].id, index);
  orderIndex_.clear();
  activeOrders_.clear();
  transitOrders_.clear();
  activeMoves_.clear();
  orderIndex_.reserve(orders_.size());
  activeOrders_.reserve(orders_.size());
  transitOrders_.reserve(orders_.size());
  activeMoves_.reserve(moves_.size());
  for (std::size_t index = 0; index < orders_.size(); ++index) {
    orderIndex_.emplace(orders_[index].id, index);
    if (orders_[index].state != PurchaseOrderState::Completed &&
        orders_[index].state != PurchaseOrderState::Cancelled)
      activeOrders_.push_back(index);
    if (orders_[index].state == PurchaseOrderState::InTransit)
      transitOrders_.push_back(index);
  }
  for (std::size_t index = 0; index < moves_.size(); ++index)
    if (!moves_[index].completed)
      activeMoves_.push_back(index);
}

void LogisticsSystem::tickSecond() {
  ++elapsedSeconds_;

  for (const auto index : transitOrders_) {
    auto &order = orders_[index];
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
    const auto moveIndex = moves_.size();
    moves_.push_back({nextId_++, order.id, order.item, order.quantity, receiving,
                      order.destination, 120, false, BlockReason::None});
    activeMoves_.push_back(moveIndex);
  }
  transitOrders_.erase(
      std::remove_if(transitOrders_.begin(), transitOrders_.end(),
                     [&](std::size_t index) {
                       return orders_[index].state !=
                              PurchaseOrderState::InTransit;
                     }),
      transitOrders_.end());

  for (const auto index : activeMoves_) {
    auto &move = moves_[index];
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
    const auto order = orderIndex_.find(move.orderId);
    if (order != orderIndex_.end()) {
      orders_[order->second].state = PurchaseOrderState::Completed;
      orders_[order->second].blockedReason = BlockReason::None;
    }
  }
  activeMoves_.erase(
      std::remove_if(activeMoves_.begin(), activeMoves_.end(),
                     [&](std::size_t index) { return moves_[index].completed; }),
      activeMoves_.end());
  activeOrders_.erase(
      std::remove_if(activeOrders_.begin(), activeOrders_.end(),
                     [&](std::size_t index) {
                       const auto state = orders_[index].state;
                       return state == PurchaseOrderState::Completed ||
                              state == PurchaseOrderState::Cancelled;
                     }),
      activeOrders_.end());

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
      if (amount == 0 || consumeFromKind(StorageKind::Waste, "waste", amount))
        wastePickupRemaining_ = -1;
    }
  }
}

void LogisticsSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

LogisticsSnapshot LogisticsSystem::snapshot(bool includeHistory) const {
  LogisticsSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  out.wasteAtSources = wasteAtSources_;
  out.wasteInBackOfHouse = 0;
  out.wasteOverflowUnits = wasteOverflowUnits_;
  out.wastePickupActive = wastePickupRemaining_ >= 0;
  out.storage.reserve(storage_.size());
  out.inventory.reserve(inventory_.size());
  out.purchaseOrders.reserve(includeHistory ? orders_.size()
                                            : activeOrders_.size());
  out.stockMoves.reserve(includeHistory ? moves_.size()
                                        : activeMoves_.size());

  std::unordered_map<StorageNodeId, int> usedByStorage;
  usedByStorage.reserve(storage_.size());
  std::unordered_set<StorageNodeId> wasteStorage;
  wasteStorage.reserve(storage_.size());
  for (const auto &storage : storage_)
    if (storage.kind == StorageKind::Waste)
      wasteStorage.insert(storage.id);

  for (const auto &stack : inventory_) {
    usedByStorage[stack.storage] += stack.quantity;
    if (stack.item == "waste" && wasteStorage.contains(stack.storage))
      out.wasteInBackOfHouse += stack.quantity;
    out.inventory.push_back(
        {stack.storage, stack.item, stack.quantity, stack.reservedQuantity});
  }

  for (const auto &storage : storage_) {
    const auto used = usedByStorage.find(storage.id);
    out.storage.push_back(
        {storage.id, storage.kind, storage.capacityUnits,
         used == usedByStorage.end() ? 0 : used->second,
         storage.reservedUnits, storage.operational});
  }
  if (includeHistory) {
    for (const auto &order : orders_)
      out.purchaseOrders.push_back({order.id, order.item, order.quantity,
                                    order.destination, order.state,
                                    order.remainingSeconds, order.blockedReason});
    for (const auto &move : moves_)
      out.stockMoves.push_back({move.id, move.orderId, move.item, move.quantity,
                                move.from, move.to, move.remainingSeconds,
                                move.completed, move.blockedReason});
  } else {
    for (const auto index : activeOrders_) {
      const auto &order = orders_[index];
      out.purchaseOrders.push_back({order.id, order.item, order.quantity,
                                    order.destination, order.state,
                                    order.remainingSeconds, order.blockedReason});
    }
    for (const auto index : activeMoves_) {
      const auto &move = moves_[index];
      out.stockMoves.push_back({move.id, move.orderId, move.item, move.quantity,
                                move.from, move.to, move.remainingSeconds,
                                move.completed, move.blockedReason});
    }
  }
  return out;
}

} // namespace hh::game
