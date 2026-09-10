#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class BookingChannel : std::uint8_t {
  Direct,
  Ota,
  Gds,
  Corporate,
  Group,
  TravelAgent = Gds
};
enum class BookingState : std::uint8_t { Confirmed, Cancelled, NoShow, Completed };

struct BookingRequestInput {
  std::uint64_t bookingId{};
  int arrivalDay{};
  int departureDay{};
  std::string roomCategory;
  std::int64_t rateCents{};
  BookingChannel channel{BookingChannel::Direct};
  std::uint64_t sourceContractId{};
  int paymentDelayDays{};
};

struct InventoryCommandResult {
  bool ok{};
  std::string reason;
  explicit operator bool() const noexcept { return ok; }
};

struct BookingView {
  std::uint64_t bookingId{};
  int arrivalDay{};
  int departureDay{};
  std::string roomCategory;
  std::int64_t rateCents{};
  BookingChannel channel{BookingChannel::Direct};
  BookingState state{BookingState::Confirmed};
  int commissionBasisPoints{};
  std::int64_t commissionCents{};
  std::uint64_t sourceContractId{};
  int paymentDay{};
  bool revenuePosted{};
  bool operator==(const BookingView &) const = default;
};

struct RevenueInventorySnapshot {
  std::vector<BookingView> bookings;
  int minimumAvailableUnits{};
  std::size_t hotPathBookingCount{};
  std::int64_t bookedRoomRevenueCents{};
  std::int64_t channelCommissionCents{};
  bool operator==(const RevenueInventorySnapshot &) const = default;
};

class RevenueInventory {
public:
  explicit RevenueInventory(std::uint64_t seed = 1);

  void setPhysicalCapacity(std::string category, int units);
  void setOverbookingAllowance(std::string category, int units);
  void setOverbookingAllowance(std::string category, int units,
                               int startDay, int endDay);
  void clearOverbookingAllowances(std::string_view category);
  void setCancellationBasisPoints(BookingChannel channel, int basisPoints);
  void setNoShowBasisPoints(BookingChannel channel, int basisPoints);

  [[nodiscard]] int sellableUnits(int day, std::string_view category) const;
  [[nodiscard]] int bookedUnits(int day, std::string_view category) const;
  [[nodiscard]] int availableUnits(int day, std::string_view category) const;
  [[nodiscard]] InventoryCommandResult book(const BookingRequestInput &request);
  [[nodiscard]] InventoryCommandResult cancel(std::uint64_t bookingId);
  void complete(std::uint64_t bookingId);
  void markRevenuePosted(std::uint64_t bookingId);
  void processDay(int day);

  [[nodiscard]] RevenueInventorySnapshot snapshot() const;
  [[nodiscard]] std::string save() const;
  static RevenueInventory load(std::string_view data);

private:
  struct AllowanceWindow {
    int startDay{};
    int endDay{};
    int units{};
    bool operator==(const AllowanceWindow &) const = default;
  };

  std::uint64_t seed_{1};
  std::map<std::string, int> physicalCapacity_;
  std::map<std::string, int> overbookingAllowance_;
  std::map<std::string, std::vector<AllowanceWindow>> overbookingWindows_;
  std::map<BookingChannel, int> cancellationBasisPoints_;
  std::map<BookingChannel, int> noShowBasisPoints_;
  std::vector<BookingView> bookings_;

  [[nodiscard]] static int defaultCommissionBasisPoints(BookingChannel channel);
  [[nodiscard]] std::uint32_t deterministicRoll(std::uint64_t bookingId, int day,
                                                std::uint64_t salt) const;
  [[nodiscard]] BookingView *find(std::uint64_t bookingId);
};

} // namespace hh::game
