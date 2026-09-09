#pragma once

#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class StorageKind : std::uint8_t {
  Receiving,
  CentralStorage,
  CleanLinen,
  DirtyLinen,
  FloorCloset,
  Waste
};

struct StorageNodeSpec {
  StorageKind kind{StorageKind::CentralStorage};
  int capacityUnits{};
  bool operational{true};
};

struct StorageNodeView {
  StorageNodeId id{};
  StorageKind kind{StorageKind::CentralStorage};
  int capacityUnits{};
  int usedUnits{};
  int reservedUnits{};
  bool operational{};
};

struct InventoryStackView {
  StorageNodeId storage{};
  std::string item;
  int quantity{};
  int reservedQuantity{};
};

enum class PurchaseOrderState : std::uint8_t {
  Submitted,
  InTransit,
  Arrived,
  Unloading,
  Completed,
  RejectedNoCapacity,
  Cancelled
};

enum class OrderError : std::uint8_t {
  None,
  InvalidQuantity,
  UnknownDestination,
  InsufficientStorageCapacity,
  MissingReceiving
};

struct OrderResult {
  PurchaseOrderId id{};
  OrderError error{OrderError::None};
  [[nodiscard]] bool ok() const noexcept { return id != 0 && error == OrderError::None; }
};

struct PurchaseOrderView {
  PurchaseOrderId id{};
  std::string item;
  int quantity{};
  StorageNodeId destination{};
  PurchaseOrderState state{PurchaseOrderState::Submitted};
  int remainingSeconds{};
  BlockReason blockedReason{BlockReason::None};
};

struct StockMoveView {
  ServiceId id{};
  PurchaseOrderId orderId{};
  std::string item;
  int quantity{};
  StorageNodeId from{};
  StorageNodeId to{};
  int remainingSeconds{};
  bool completed{};
  BlockReason blockedReason{BlockReason::None};
};

struct LogisticsSnapshot {
  std::int64_t elapsedSeconds{};
  std::vector<StorageNodeView> storage;
  std::vector<InventoryStackView> inventory;
  std::vector<PurchaseOrderView> purchaseOrders;
  std::vector<StockMoveView> stockMoves;
  int wasteAtSources{};
  int wasteInBackOfHouse{};
  int wasteOverflowUnits{};
  bool wastePickupActive{};
};

class LogisticsSystem {
public:
  LogisticsSystem() = default;
  static LogisticsSystem standardHotel();

  StorageNodeId addStorage(const StorageNodeSpec &spec);
  [[nodiscard]] StorageNodeId firstStorage(StorageKind kind) const;
  [[nodiscard]] bool addInventory(StorageNodeId storage, std::string_view item,
                                  int quantity);
  [[nodiscard]] int inventoryAt(StorageNodeId storage,
                                std::string_view item) const;
  [[nodiscard]] int inventoryUsable(std::string_view item) const;
  [[nodiscard]] int totalInventory(std::string_view item) const;
  [[nodiscard]] bool consumeUsable(std::string_view item, int quantity);
  [[nodiscard]] bool consumeFromKind(StorageKind kind, std::string_view item,
                                     int quantity);
  [[nodiscard]] bool addToKind(StorageKind kind, std::string_view item,
                               int quantity);
  [[nodiscard]] bool canAddToKind(StorageKind kind, int quantity) const;

  OrderResult placePurchaseOrder(std::string_view item, int quantity,
                                 StorageNodeId destination,
                                 int leadSeconds = 3600);
  void produceWaste(int quantity);
  void requestWastePickup();
  void tickSecond();
  void tickSeconds(std::int64_t seconds);

  [[nodiscard]] LogisticsSnapshot snapshot() const;

private:
  friend class ServiceLogisticsRuntime;

  struct StorageNode {
    StorageNodeId id{};
    StorageKind kind{StorageKind::CentralStorage};
    int capacityUnits{};
    int reservedUnits{};
    bool operational{};
  };
  struct Stack {
    StorageNodeId storage{};
    std::string item;
    int quantity{};
    int reservedQuantity{};
  };
  struct PurchaseOrder {
    PurchaseOrderId id{};
    std::string item;
    int quantity{};
    StorageNodeId destination{};
    PurchaseOrderState state{PurchaseOrderState::Submitted};
    int remainingSeconds{};
    BlockReason blockedReason{BlockReason::None};
  };
  struct StockMove {
    ServiceId id{};
    PurchaseOrderId orderId{};
    std::string item;
    int quantity{};
    StorageNodeId from{};
    StorageNodeId to{};
    int remainingSeconds{};
    bool completed{};
    BlockReason blockedReason{BlockReason::None};
  };

  [[nodiscard]] StorageNode *node(StorageNodeId id);
  [[nodiscard]] const StorageNode *node(StorageNodeId id) const;
  [[nodiscard]] int usedUnits(StorageNodeId id) const;
  [[nodiscard]] bool isUsable(StorageKind kind) const;
  [[nodiscard]] bool consumeAt(StorageNodeId storage, std::string_view item,
                               int quantity);

  ServiceId nextId_{1};
  std::int64_t elapsedSeconds_{};
  std::vector<StorageNode> storage_;
  std::vector<Stack> inventory_;
  std::vector<PurchaseOrder> orders_;
  std::vector<StockMove> moves_;
  int wasteAtSources_{};
  int wasteOverflowUnits_{};
  int wasteCollectionRemaining_{-1};
  int wastePickupRemaining_{-1};
};

} // namespace hh::game
