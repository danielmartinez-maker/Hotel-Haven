#include "hh/game/RevenueInventory.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace hh::game {

RevenueInventory::RevenueInventory(std::uint64_t seed) : seed_(seed ? seed : 1) {
  for (const auto channel : {BookingChannel::Direct, BookingChannel::Ota,
                             BookingChannel::Gds, BookingChannel::Corporate,
                             BookingChannel::Group}) {
    cancellationBasisPoints_[channel] = 0;
    noShowBasisPoints_[channel] = 0;
  }
}

void RevenueInventory::setPhysicalCapacity(std::string category, int units) {
  if (category.empty() || units < 0)
    throw std::invalid_argument("invalid physical inventory capacity");
  physicalCapacity_[std::move(category)] = units;
}

void RevenueInventory::setOverbookingAllowance(std::string category, int units) {
  if (category.empty() || units < 0)
    throw std::invalid_argument("invalid overbooking allowance");
  overbookingAllowance_[std::move(category)] = units;
}

void RevenueInventory::setOverbookingAllowance(std::string category, int units,
                                               int startDay, int endDay) {
  if (category.empty() || units < 0 || startDay < 0 || endDay < startDay)
    throw std::invalid_argument("invalid dated overbooking allowance");
  auto &windows = overbookingWindows_[category];
  auto it = std::find_if(windows.begin(), windows.end(), [&](const auto &window) {
    return window.startDay == startDay && window.endDay == endDay;
  });
  if (it == windows.end())
    windows.push_back({startDay, endDay, units});
  else
    it->units = units;
  std::sort(windows.begin(), windows.end(), [](const auto &a, const auto &b) {
    if (a.startDay != b.startDay)
      return a.startDay < b.startDay;
    return a.endDay < b.endDay;
  });
}

void RevenueInventory::clearOverbookingAllowances(std::string_view category) {
  const std::string key(category);
  overbookingAllowance_.erase(key);
  overbookingWindows_.erase(key);
}

void RevenueInventory::setCancellationBasisPoints(BookingChannel channel, int basisPoints) {
  if (basisPoints < 0 || basisPoints > 10000)
    throw std::invalid_argument("invalid cancellation probability");
  cancellationBasisPoints_[channel] = basisPoints;
}

void RevenueInventory::setNoShowBasisPoints(BookingChannel channel, int basisPoints) {
  if (basisPoints < 0 || basisPoints > 10000)
    throw std::invalid_argument("invalid no-show probability");
  noShowBasisPoints_[channel] = basisPoints;
}

int RevenueInventory::sellableUnits(int day, std::string_view category) const {
  const std::string key(category);
  const auto physical = physicalCapacity_.find(key);
  if (physical == physicalCapacity_.end())
    return 0;
  int allowance = 0;
  if (const auto broad = overbookingAllowance_.find(key);
      broad != overbookingAllowance_.end())
    allowance = broad->second;
  if (const auto dated = overbookingWindows_.find(key);
      dated != overbookingWindows_.end())
    for (const auto &window : dated->second)
      if (day >= window.startDay && day <= window.endDay)
        allowance = window.units;
  return physical->second + allowance;
}

int RevenueInventory::bookedUnits(int day, std::string_view category) const {
  int units = 0;
  for (const auto &booking : bookings_)
    if (booking.state == BookingState::Confirmed && booking.roomCategory == category &&
        booking.arrivalDay <= day && day < booking.departureDay)
      ++units;
  return units;
}

int RevenueInventory::availableUnits(int day, std::string_view category) const {
  return std::max(0, sellableUnits(day, category) - bookedUnits(day, category));
}

int RevenueInventory::defaultCommissionBasisPoints(BookingChannel channel) {
  switch (channel) {
  case BookingChannel::Direct: return 0;
  case BookingChannel::Ota: return 1500;
  case BookingChannel::Gds: return 1000;
  case BookingChannel::Corporate: return 500;
  case BookingChannel::Group: return 300;
  }
  return 0;
}

InventoryCommandResult RevenueInventory::book(const BookingRequestInput &request) {
  if (request.bookingId == 0 || request.arrivalDay < 0 ||
      request.departureDay <= request.arrivalDay || request.roomCategory.empty() ||
      request.rateCents <= 0 || request.paymentDelayDays < 0 ||
      request.departureDay > std::numeric_limits<int>::max() - request.paymentDelayDays)
    return {false, "INVALID_BOOKING"};
  if (find(request.bookingId))
    return {false, "DUPLICATE_BOOKING_ID"};
  if (!physicalCapacity_.contains(request.roomCategory))
    return {false, "UNKNOWN_ROOM_CATEGORY"};
  for (int day = request.arrivalDay; day < request.departureDay; ++day)
    if (bookedUnits(day, request.roomCategory) >= sellableUnits(day, request.roomCategory))
      return {false, "NO_SELLABLE_INVENTORY"};

  BookingView booking;
  booking.bookingId = request.bookingId;
  booking.arrivalDay = request.arrivalDay;
  booking.departureDay = request.departureDay;
  booking.roomCategory = request.roomCategory;
  booking.rateCents = request.rateCents;
  booking.channel = request.channel;
  booking.state = BookingState::Confirmed;
  booking.commissionBasisPoints = defaultCommissionBasisPoints(request.channel);
  const auto roomNights = static_cast<std::int64_t>(request.departureDay - request.arrivalDay);
  booking.commissionCents =
      (request.rateCents * roomNights * booking.commissionBasisPoints + 5000) / 10000;
  booking.sourceContractId = request.sourceContractId;
  booking.paymentDay = request.departureDay + request.paymentDelayDays;
  booking.revenuePosted = false;
  bookings_.push_back(std::move(booking));
  std::sort(bookings_.begin(), bookings_.end(), [](const auto &a, const auto &b) {
    return a.bookingId < b.bookingId;
  });
  return {true, "OK"};
}

BookingView *RevenueInventory::find(std::uint64_t bookingId) {
  for (auto &booking : bookings_)
    if (booking.bookingId == bookingId)
      return &booking;
  return nullptr;
}

InventoryCommandResult RevenueInventory::cancel(std::uint64_t bookingId) {
  auto *booking = find(bookingId);
  if (!booking)
    return {false, "BOOKING_NOT_FOUND"};
  if (booking->state != BookingState::Confirmed)
    return {false, "BOOKING_NOT_ACTIVE"};
  booking->state = BookingState::Cancelled;
  return {true, "OK"};
}

void RevenueInventory::complete(std::uint64_t bookingId) {
  if (auto *booking = find(bookingId);
      booking && booking->state == BookingState::Confirmed)
    booking->state = BookingState::Completed;
}

void RevenueInventory::markRevenuePosted(std::uint64_t bookingId) {
  auto *booking = find(bookingId);
  if (!booking)
    throw std::invalid_argument("booking not found for revenue posting");
  if (booking->revenuePosted)
    throw std::invalid_argument("booking revenue already posted");
  booking->revenuePosted = true;
}

std::uint32_t RevenueInventory::deterministicRoll(std::uint64_t bookingId, int day,
                                                   std::uint64_t salt) const {
  std::uint64_t z = seed_ ^ (bookingId * 0x9E3779B97F4A7C15ULL) ^
                    (static_cast<std::uint64_t>(day + 1) * 0xBF58476D1CE4E5B9ULL) ^ salt;
  z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
  z ^= z >> 31U;
  return static_cast<std::uint32_t>(z % 10000ULL);
}

void RevenueInventory::processDay(int day) {
  for (auto &booking : bookings_) {
    if (booking.state != BookingState::Confirmed)
      continue;
    if (day < booking.arrivalDay) {
      const auto it = cancellationBasisPoints_.find(booking.channel);
      const int threshold = it == cancellationBasisPoints_.end() ? 0 : it->second;
      if (day == booking.arrivalDay - 1 &&
          deterministicRoll(booking.bookingId, day, 0xCACE11A7ULL) <
              static_cast<std::uint32_t>(threshold))
        booking.state = BookingState::Cancelled;
    } else if (day == booking.arrivalDay) {
      const auto it = noShowBasisPoints_.find(booking.channel);
      const int threshold = it == noShowBasisPoints_.end() ? 0 : it->second;
      if (deterministicRoll(booking.bookingId, day, 0xA05A0ULL) <
          static_cast<std::uint32_t>(threshold))
        booking.state = BookingState::NoShow;
    } else if (day >= booking.departureDay) {
      booking.state = BookingState::Completed;
    }
  }
}

RevenueInventorySnapshot RevenueInventory::snapshot() const {
  RevenueInventorySnapshot out;
  out.bookings = bookings_;
  out.minimumAvailableUnits = std::numeric_limits<int>::max();
  std::size_t active = 0;
  for (const auto &booking : bookings_) {
    if (booking.state != BookingState::Confirmed)
      continue;
    ++active;
    const auto nights = static_cast<std::int64_t>(booking.departureDay - booking.arrivalDay);
    out.bookedRoomRevenueCents += booking.rateCents * nights;
    out.channelCommissionCents += booking.commissionCents;
    for (int day = booking.arrivalDay; day < booking.departureDay; ++day)
      out.minimumAvailableUnits = std::min(
          out.minimumAvailableUnits, availableUnits(day, booking.roomCategory));
  }
  out.hotPathBookingCount = active;
  if (out.minimumAvailableUnits == std::numeric_limits<int>::max())
    out.minimumAvailableUnits = 0;
  return out;
}

std::string RevenueInventory::save() const {
  std::ostringstream out;
  out << "HHRINV 3 " << seed_ << ' ' << physicalCapacity_.size();
  for (const auto &[category, units] : physicalCapacity_)
    out << ' ' << std::quoted(category) << ' ' << units << ' '
        << overbookingAllowance_.contains(category) << ' '
        << (overbookingAllowance_.contains(category) ? overbookingAllowance_.at(category) : 0);

  std::size_t windowCount = 0;
  for (const auto &[category, windows] : overbookingWindows_) {
    (void)category;
    windowCount += windows.size();
  }
  out << ' ' << windowCount;
  for (const auto &[category, windows] : overbookingWindows_)
    for (const auto &window : windows)
      out << ' ' << std::quoted(category) << ' ' << window.startDay << ' '
          << window.endDay << ' ' << window.units;

  out << ' ' << cancellationBasisPoints_.size();
  for (const auto &[channel, bp] : cancellationBasisPoints_)
    out << ' ' << static_cast<int>(channel) << ' ' << bp << ' '
        << noShowBasisPoints_.at(channel);
  out << ' ' << bookings_.size();
  for (const auto &b : bookings_)
    out << ' ' << b.bookingId << ' ' << b.arrivalDay << ' ' << b.departureDay << ' '
        << std::quoted(b.roomCategory) << ' ' << b.rateCents << ' '
        << static_cast<int>(b.channel) << ' ' << static_cast<int>(b.state) << ' '
        << b.commissionBasisPoints << ' ' << b.commissionCents << ' '
        << b.sourceContractId << ' ' << b.paymentDay << ' ' << b.revenuePosted;
  return out.str();
}

RevenueInventory RevenueInventory::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::uint64_t seed{};
  std::size_t count{};
  in >> magic >> version >> seed >> count;
  if (!in || magic != "HHRINV" ||
      (version != 1 && version != 2 && version != 3) || count > 10000)
    throw std::invalid_argument("invalid revenue inventory save");
  RevenueInventory result(seed);
  for (std::size_t i = 0; i < count; ++i) {
    std::string category;
    int units{}, hasAllowance{}, allowance{};
    in >> std::quoted(category) >> units >> hasAllowance >> allowance;
    result.setPhysicalCapacity(category, units);
    if (hasAllowance)
      result.setOverbookingAllowance(category, allowance);
  }

  if (version >= 2) {
    in >> count;
    if (!in || count > 100000)
      throw std::invalid_argument("invalid dated allowance count");
    for (std::size_t i = 0; i < count; ++i) {
      std::string category;
      int startDay{}, endDay{}, units{};
      in >> std::quoted(category) >> startDay >> endDay >> units;
      if (!in)
        throw std::invalid_argument("invalid saved dated allowance");
      result.setOverbookingAllowance(category, units, startDay, endDay);
    }
  }

  in >> count;
  if (!in || count > 16)
    throw std::invalid_argument("invalid channel policy count");
  for (std::size_t i = 0; i < count; ++i) {
    int channel{}, cancelBp{}, noShowBp{};
    in >> channel >> cancelBp >> noShowBp;
    if (channel < 0 || channel > static_cast<int>(BookingChannel::Group))
      throw std::invalid_argument("invalid booking channel");
    const auto c = static_cast<BookingChannel>(channel);
    result.setCancellationBasisPoints(c, cancelBp);
    result.setNoShowBasisPoints(c, noShowBp);
  }
  in >> count;
  if (!in || count > 1000000)
    throw std::invalid_argument("invalid booking count");
  result.bookings_.clear();
  for (std::size_t i = 0; i < count; ++i) {
    BookingView b;
    int channel{}, state{};
    in >> b.bookingId >> b.arrivalDay >> b.departureDay >>
        std::quoted(b.roomCategory) >> b.rateCents >> channel >> state >>
        b.commissionBasisPoints >> b.commissionCents;
    if (version >= 3)
      in >> b.sourceContractId >> b.paymentDay >> b.revenuePosted;
    else {
      b.sourceContractId = 0;
      b.paymentDay = b.departureDay;
      b.revenuePosted = state == static_cast<int>(BookingState::Completed);
    }
    if (!in || channel < 0 || channel > static_cast<int>(BookingChannel::Group) ||
        state < 0 || state > static_cast<int>(BookingState::Completed) ||
        b.paymentDay < b.departureDay)
      throw std::invalid_argument("invalid saved booking");
    b.channel = static_cast<BookingChannel>(channel);
    b.state = static_cast<BookingState>(state);
    result.bookings_.push_back(std::move(b));
  }
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected revenue inventory trailing data");
  return result;
}

} // namespace hh::game
