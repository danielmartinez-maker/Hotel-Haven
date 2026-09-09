#include "hh/game/ServiceLogistics.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace hh::game {
namespace {
template <class E> int enumValue(E value) { return static_cast<int>(value); }

bool validBlockReason(int value) {
  return value >= enumValue(BlockReason::None) &&
         value <= enumValue(BlockReason::AwaitingProduction);
}
} // namespace

struct ServiceLogisticsRuntime::Impl {
  std::uint64_t seed{1};
  std::int64_t elapsedSeconds{};
  LogisticsSystem logistics;
  HousekeepingSystem housekeeping;
  LaundrySystem laundry;
  EngineeringSystem engineering;
  RoomServiceSystem roomService;

  Impl(std::uint64_t value, LaundryStations stations)
      : seed(value), logistics(LogisticsSystem::standardHotel()),
        housekeeping(logistics), laundry(logistics, stations),
        engineering(logistics, value) {}
};

ServiceLogisticsRuntime::ServiceLogisticsRuntime(std::uint64_t seed,
                                                 LaundryStations stations)
    : impl_(std::make_unique<Impl>(seed, stations)) {}
ServiceLogisticsRuntime::~ServiceLogisticsRuntime() = default;
ServiceLogisticsRuntime::ServiceLogisticsRuntime(ServiceLogisticsRuntime &&) noexcept = default;
ServiceLogisticsRuntime &ServiceLogisticsRuntime::operator=(ServiceLogisticsRuntime &&) noexcept = default;
ServiceLogisticsRuntime::ServiceLogisticsRuntime(const ServiceLogisticsRuntime &other)
    : ServiceLogisticsRuntime(load(other.save())) {}
ServiceLogisticsRuntime &ServiceLogisticsRuntime::operator=(const ServiceLogisticsRuntime &other) {
  if (this != &other)
    *this = load(other.save());
  return *this;
}

LogisticsSystem &ServiceLogisticsRuntime::logistics() { return impl_->logistics; }
const LogisticsSystem &ServiceLogisticsRuntime::logistics() const { return impl_->logistics; }
HousekeepingSystem &ServiceLogisticsRuntime::housekeeping() { return impl_->housekeeping; }
const HousekeepingSystem &ServiceLogisticsRuntime::housekeeping() const { return impl_->housekeeping; }
LaundrySystem &ServiceLogisticsRuntime::laundry() { return impl_->laundry; }
const LaundrySystem &ServiceLogisticsRuntime::laundry() const { return impl_->laundry; }
EngineeringSystem &ServiceLogisticsRuntime::engineering() { return impl_->engineering; }
const EngineeringSystem &ServiceLogisticsRuntime::engineering() const { return impl_->engineering; }
RoomServiceSystem &ServiceLogisticsRuntime::roomService() { return impl_->roomService; }
const RoomServiceSystem &ServiceLogisticsRuntime::roomService() const { return impl_->roomService; }

void ServiceLogisticsRuntime::registerRoom(RoomId room, ServiceRoomStatus status) {
  impl_->housekeeping.registerRoom(room, status);
}
void ServiceLogisticsRuntime::registerAsset(AssetId asset, int condition) {
  impl_->engineering.registerAsset(asset, condition);
}
LogisticsSnapshot ServiceLogisticsRuntime::logisticsSnapshot() const {
  return impl_->logistics.snapshot();
}
TaskId ServiceLogisticsRuntime::requestRoomTurn(RoomId room) {
  return impl_->housekeeping.requestRoomTurn(room);
}
LaundryBatchId ServiceLogisticsRuntime::requestLaundryBatch(int quantity) {
  return impl_->laundry.requestBatch(quantity);
}
WorkOrderId ServiceLogisticsRuntime::createWorkOrder(AssetId asset,
                                                     WorkOrderType type) {
  return impl_->engineering.createWorkOrder(asset, type);
}
RoomServiceOrderId ServiceLogisticsRuntime::placeRoomServiceOrder(
    GuestId guest, const RoomServiceOrder &order) {
  return impl_->roomService.placeRoomServiceOrder(guest, order);
}
bool ServiceLogisticsRuntime::markRoomServiceProductionReady(
    RoomServiceOrderId order) {
  return impl_->roomService.markProductionReady(order);
}
bool ServiceLogisticsRuntime::requestRoomServiceTrayPickup(
    RoomServiceOrderId order) {
  return impl_->roomService.requestTrayPickup(order);
}

void ServiceLogisticsRuntime::tickSecond() {
  ++impl_->elapsedSeconds;
  impl_->housekeeping.tickSecond();
  impl_->laundry.tickSecond();
  impl_->engineering.tickSecond();
  impl_->roomService.tickSecond();
  impl_->logistics.tickSecond();
}
void ServiceLogisticsRuntime::tickSeconds(std::int64_t seconds) {
  if (seconds < 0 || seconds > 1'000'000'000LL)
    throw std::invalid_argument("service tick seconds must be bounded and nonnegative");
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}
std::int64_t ServiceLogisticsRuntime::elapsedSeconds() const {
  return impl_->elapsedSeconds;
}

std::string ServiceLogisticsRuntime::save() const {
  std::ostringstream out;
  out << "HHSL 1 " << impl_->seed << ' ' << impl_->elapsedSeconds << '\n';

  const auto &l = impl_->logistics;
  out << "L " << l.nextId_ << ' ' << l.elapsedSeconds_ << ' '
      << l.wasteAtSources_ << ' ' << l.wasteOverflowUnits_ << ' '
      << l.wasteCollectionRemaining_ << ' ' << l.wastePickupRemaining_ << '\n';
  out << l.storage_.size() << '\n';
  for (const auto &node : l.storage_)
    out << node.id << ' ' << enumValue(node.kind) << ' ' << node.capacityUnits
        << ' ' << node.reservedUnits << ' ' << node.operational << '\n';
  out << l.inventory_.size() << '\n';
  for (const auto &stack : l.inventory_)
    out << stack.storage << ' ' << std::quoted(stack.item) << ' '
        << stack.quantity << ' ' << stack.reservedQuantity << '\n';
  out << l.orders_.size() << '\n';
  for (const auto &order : l.orders_)
    out << order.id << ' ' << std::quoted(order.item) << ' ' << order.quantity
        << ' ' << order.destination << ' ' << enumValue(order.state) << ' '
        << order.remainingSeconds << ' ' << enumValue(order.blockedReason) << '\n';
  out << l.moves_.size() << '\n';
  for (const auto &move : l.moves_)
    out << move.id << ' ' << move.orderId << ' ' << std::quoted(move.item) << ' '
        << move.quantity << ' ' << move.from << ' ' << move.to << ' '
        << move.remainingSeconds << ' ' << move.completed << ' '
        << enumValue(move.blockedReason) << '\n';

  const auto &h = impl_->housekeeping;
  out << "H " << h.nextId_ << ' ' << h.elapsedSeconds_ << '\n';
  out << h.rooms_.size() << '\n';
  for (const auto &room : h.rooms_)
    out << room.id << ' ' << enumValue(room.status) << '\n';
  out << h.jobs_.size() << '\n';
  for (const auto &job : h.jobs_)
    out << job.id << ' ' << job.roomId << ' ' << enumValue(job.stage) << ' '
        << job.remainingSeconds << ' ' << enumValue(job.blockedReason) << ' '
        << job.stageStarted << '\n';

  const auto &laundry = impl_->laundry;
  out << "A " << laundry.nextId_ << ' ' << laundry.elapsedSeconds_ << ' '
      << laundry.stations_.washers << ' ' << laundry.stations_.dryers << ' '
      << laundry.stations_.foldingStations << '\n';
  out << laundry.batches_.size() << '\n';
  for (const auto &batch : laundry.batches_)
    out << batch.id << ' ' << batch.quantity << ' ' << enumValue(batch.stage)
        << ' ' << batch.remainingSeconds << ' ' << enumValue(batch.blockedReason)
        << '\n';

  const auto &engineering = impl_->engineering;
  out << "E " << engineering.nextId_ << ' ' << engineering.elapsedSeconds_ << ' '
      << engineering.failures_ << '\n';
  out << engineering.rng_ << '\n';
  out << engineering.assets_.size() << '\n';
  for (const auto &asset : engineering.assets_)
    out << asset.id << ' ' << asset.condition << ' ' << asset.failurePressure
        << ' ' << asset.failed << '\n';
  out << engineering.workOrders_.size() << '\n';
  for (const auto &order : engineering.workOrders_)
    out << order.id << ' ' << order.assetId << ' ' << enumValue(order.type) << ' '
        << enumValue(order.stage) << ' ' << order.remainingSeconds << ' '
        << enumValue(order.blockedReason) << ' ' << order.partClaimed << '\n';

  const auto &service = impl_->roomService;
  out << "R " << service.nextId_ << ' ' << service.elapsedSeconds_ << '\n';
  out << service.orders_.size() << '\n';
  for (const auto &order : service.orders_) {
    out << order.id << ' ' << order.guestId << ' ' << enumValue(order.stage)
        << ' ' << order.remainingSeconds << ' ' << order.ageSeconds << ' '
        << order.promisedSeconds << ' ' << enumValue(order.blockedReason) << ' '
        << order.stageHistory.size();
    for (const auto stage : order.stageHistory)
      out << ' ' << enumValue(stage);
    out << '\n';
  }
  return out.str();
}

ServiceLogisticsRuntime ServiceLogisticsRuntime::load(std::string_view data) {
  if (data.size() > 16 * 1024 * 1024)
    throw std::invalid_argument("service save too large");
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::uint64_t seed{};
  std::int64_t elapsed{};
  in >> magic >> version >> seed >> elapsed;
  if (!in || magic != "HHSL" || version != 1 || elapsed < 0)
    throw std::invalid_argument("unsupported or corrupt service save");

  ServiceLogisticsRuntime result(seed);
  result.impl_->elapsedSeconds = elapsed;
  auto count = [&]() -> std::size_t {
    std::size_t n{};
    in >> n;
    if (!in || n > 100000)
      throw std::invalid_argument("invalid service save count");
    return n;
  };
  auto readTag = [&](const char *expected) {
    std::string tag;
    in >> tag;
    if (!in || tag != expected)
      throw std::invalid_argument("invalid service save section");
  };

  auto &l = result.impl_->logistics;
  readTag("L");
  in >> l.nextId_ >> l.elapsedSeconds_ >> l.wasteAtSources_ >>
      l.wasteOverflowUnits_ >> l.wasteCollectionRemaining_ >>
      l.wastePickupRemaining_;
  if (!in || l.nextId_ == 0 || l.elapsedSeconds_ < 0 || l.wasteAtSources_ < 0 ||
      l.wasteOverflowUnits_ < 0 || l.wasteCollectionRemaining_ < -1 ||
      l.wastePickupRemaining_ < -1)
    throw std::invalid_argument("invalid logistics state");
  l.storage_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    LogisticsSystem::StorageNode node;
    int kind{};
    in >> node.id >> kind >> node.capacityUnits >> node.reservedUnits >>
        node.operational;
    if (!in || node.id == 0 || kind < enumValue(StorageKind::Receiving) ||
        kind > enumValue(StorageKind::Waste) || node.capacityUnits <= 0 ||
        node.reservedUnits < 0 || node.reservedUnits > node.capacityUnits)
      throw std::invalid_argument("invalid storage node");
    node.kind = static_cast<StorageKind>(kind);
    l.storage_.push_back(node);
  }
  l.inventory_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    LogisticsSystem::Stack stack;
    in >> stack.storage >> std::quoted(stack.item) >> stack.quantity >>
        stack.reservedQuantity;
    if (!in || stack.storage == 0 || stack.item.empty() || stack.quantity < 0 ||
        stack.reservedQuantity < 0 || stack.reservedQuantity > stack.quantity)
      throw std::invalid_argument("invalid inventory stack");
    l.inventory_.push_back(std::move(stack));
  }
  l.orders_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    LogisticsSystem::PurchaseOrder order;
    int state{}, block{};
    in >> order.id >> std::quoted(order.item) >> order.quantity >>
        order.destination >> state >> order.remainingSeconds >> block;
    if (!in || order.id == 0 || order.item.empty() || order.quantity <= 0 ||
        order.destination == 0 || state < enumValue(PurchaseOrderState::Submitted) ||
        state > enumValue(PurchaseOrderState::Cancelled) ||
        order.remainingSeconds < 0 || !validBlockReason(block))
      throw std::invalid_argument("invalid purchase order");
    order.state = static_cast<PurchaseOrderState>(state);
    order.blockedReason = static_cast<BlockReason>(block);
    l.orders_.push_back(std::move(order));
  }
  l.moves_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    LogisticsSystem::StockMove move;
    int block{};
    in >> move.id >> move.orderId >> std::quoted(move.item) >> move.quantity >>
        move.from >> move.to >> move.remainingSeconds >> move.completed >> block;
    if (!in || move.id == 0 || move.orderId == 0 || move.item.empty() ||
        move.quantity <= 0 || move.from == 0 || move.to == 0 ||
        move.remainingSeconds < 0 || !validBlockReason(block))
      throw std::invalid_argument("invalid stock move");
    move.blockedReason = static_cast<BlockReason>(block);
    l.moves_.push_back(std::move(move));
  }

  auto &h = result.impl_->housekeeping;
  readTag("H");
  in >> h.nextId_ >> h.elapsedSeconds_;
  if (!in || h.nextId_ == 0 || h.elapsedSeconds_ < 0)
    throw std::invalid_argument("invalid housekeeping state");
  h.rooms_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    HousekeepingSystem::RoomState room;
    int status{};
    in >> room.id >> status;
    if (!in || room.id == 0 || status < enumValue(ServiceRoomStatus::Ready) ||
        status > enumValue(ServiceRoomStatus::Blocked))
      throw std::invalid_argument("invalid service room");
    room.status = static_cast<ServiceRoomStatus>(status);
    h.rooms_.push_back(room);
  }
  h.jobs_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    HousekeepingSystem::Job job;
    int stage{}, block{};
    in >> job.id >> job.roomId >> stage >> job.remainingSeconds >> block >>
        job.stageStarted;
    if (!in || job.id == 0 || job.roomId == 0 ||
        stage < enumValue(HousekeepingStage::StripLinen) ||
        stage > enumValue(HousekeepingStage::Completed) ||
        job.remainingSeconds < 0 || !validBlockReason(block))
      throw std::invalid_argument("invalid housekeeping job");
    job.stage = static_cast<HousekeepingStage>(stage);
    job.blockedReason = static_cast<BlockReason>(block);
    h.jobs_.push_back(job);
  }

  auto &laundry = result.impl_->laundry;
  readTag("A");
  in >> laundry.nextId_ >> laundry.elapsedSeconds_ >> laundry.stations_.washers >>
      laundry.stations_.dryers >> laundry.stations_.foldingStations;
  if (!in || laundry.nextId_ == 0 || laundry.elapsedSeconds_ < 0 ||
      laundry.stations_.washers < 0 || laundry.stations_.dryers < 0 ||
      laundry.stations_.foldingStations < 0)
    throw std::invalid_argument("invalid laundry state");
  laundry.batches_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    LaundrySystem::Batch batch;
    int stage{}, block{};
    in >> batch.id >> batch.quantity >> stage >> batch.remainingSeconds >> block;
    if (!in || batch.id == 0 || batch.quantity <= 0 || batch.quantity > 30 ||
        stage < enumValue(LaundryStage::AwaitingWasher) ||
        stage > enumValue(LaundryStage::Completed) || batch.remainingSeconds < 0 ||
        !validBlockReason(block))
      throw std::invalid_argument("invalid laundry batch");
    batch.stage = static_cast<LaundryStage>(stage);
    batch.blockedReason = static_cast<BlockReason>(block);
    laundry.batches_.push_back(batch);
  }

  auto &engineering = result.impl_->engineering;
  readTag("E");
  in >> engineering.nextId_ >> engineering.elapsedSeconds_ >> engineering.failures_;
  in >> engineering.rng_;
  if (!in || engineering.nextId_ == 0 || engineering.elapsedSeconds_ < 0 ||
      engineering.failures_ < 0)
    throw std::invalid_argument("invalid engineering state");
  engineering.assets_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    EngineeringSystem::Asset asset;
    in >> asset.id >> asset.condition >> asset.failurePressure >> asset.failed;
    if (!in || asset.id == 0 || asset.condition < 0 || asset.condition > 10000 ||
        asset.failurePressure < 0 || asset.failurePressure > 9500)
      throw std::invalid_argument("invalid engineering asset");
    engineering.assets_.push_back(asset);
  }
  engineering.workOrders_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    EngineeringSystem::WorkOrder order;
    int type{}, stage{}, block{};
    in >> order.id >> order.assetId >> type >> stage >> order.remainingSeconds >>
        block >> order.partClaimed;
    if (!in || order.id == 0 || order.assetId == 0 ||
        type < enumValue(WorkOrderType::Preventive) ||
        type > enumValue(WorkOrderType::Corrective) ||
        stage < enumValue(WorkOrderStage::Queued) ||
        stage > enumValue(WorkOrderStage::Completed) || order.remainingSeconds < 0 ||
        !validBlockReason(block))
      throw std::invalid_argument("invalid engineering work order");
    order.type = static_cast<WorkOrderType>(type);
    order.stage = static_cast<WorkOrderStage>(stage);
    order.blockedReason = static_cast<BlockReason>(block);
    engineering.workOrders_.push_back(order);
  }

  auto &service = result.impl_->roomService;
  readTag("R");
  in >> service.nextId_ >> service.elapsedSeconds_;
  if (!in || service.nextId_ == 0 || service.elapsedSeconds_ < 0)
    throw std::invalid_argument("invalid room service state");
  service.orders_.clear();
  for (std::size_t i = 0, n = count(); i < n; ++i) {
    RoomServiceSystem::ActiveOrder order;
    int stage{}, block{};
    std::size_t historyCount{};
    in >> order.id >> order.guestId >> stage >> order.remainingSeconds >>
        order.ageSeconds >> order.promisedSeconds >> block >> historyCount;
    if (!in || order.id == 0 || order.guestId == 0 ||
        stage < enumValue(RoomServiceStage::AwaitingProduction) ||
        stage > enumValue(RoomServiceStage::Completed) || order.remainingSeconds < 0 ||
        order.ageSeconds < 0 || order.promisedSeconds <= 0 ||
        !validBlockReason(block) || historyCount == 0 || historyCount > 32)
      throw std::invalid_argument("invalid room service order");
    order.stage = static_cast<RoomServiceStage>(stage);
    order.blockedReason = static_cast<BlockReason>(block);
    for (std::size_t j = 0; j < historyCount; ++j) {
      int historyStage{};
      in >> historyStage;
      if (!in || historyStage < enumValue(RoomServiceStage::AwaitingProduction) ||
          historyStage > enumValue(RoomServiceStage::Completed))
        throw std::invalid_argument("invalid room service history");
      order.stageHistory.push_back(static_cast<RoomServiceStage>(historyStage));
    }
    if (order.stageHistory.back() != order.stage)
      throw std::invalid_argument("room service history/state mismatch");
    service.orders_.push_back(std::move(order));
  }

  std::unordered_set<StorageNodeId> storageIds;
  for (const auto &node : l.storage_) {
    if (!storageIds.insert(node.id).second || l.usedUnits(node.id) > node.capacityUnits)
      throw std::invalid_argument("invalid storage references or capacity");
  }
  for (const auto &stack : l.inventory_)
    if (!storageIds.contains(stack.storage))
      throw std::invalid_argument("inventory references missing storage");
  for (const auto &order : l.orders_)
    if (!storageIds.contains(order.destination))
      throw std::invalid_argument("order references missing storage");
  for (const auto &move : l.moves_)
    if (!storageIds.contains(move.from) || !storageIds.contains(move.to))
      throw std::invalid_argument("move references missing storage");

  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected trailing service save data");
  return result;
}

} // namespace hh::game
