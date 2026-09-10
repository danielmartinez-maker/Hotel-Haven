#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class MarketSegment : std::uint8_t {
  CoupleLeisure = 0,
  Business = 1,
  ConferenceGroup = 2,
  LuxuryLeisure = 3,
  BudgetLeisure = 4,
  ExecutiveBusiness = 5,
  FamilyLeisure = 6,
  AirportTransit = 7,
  Wellness = 8,

  // Source compatibility aliases retained for FINAL-06 callers/saves created
  // before the HMG-030 nine-segment reconciliation.
  Leisure = CoupleLeisure,
  Group = ConferenceGroup,
  Luxury = LuxuryLeisure,
  Budget = BudgetLeisure
};

struct SegmentDemandProfile {
  MarketSegment segment{MarketSegment::CoupleLeisure};
  double baseDailyDemand{};
  std::array<int, 7> weekdayMultiplierBasisPoints{
      10000, 10000, 10000, 10000, 10000, 10000, 10000};
  int medianLeadTimeDays{};
  int medianStayNights{1};
  std::int64_t baseBudgetCents{};
  bool operator==(const SegmentDemandProfile &) const = default;
};

struct MarketDemandModifiers {
  int seasonMultiplierBasisPoints{10000};
  int economicMultiplierBasisPoints{10000};
  int eventMultiplierBasisPoints{10000};
  int scenarioMultiplierBasisPoints{10000};
  bool operator==(const MarketDemandModifiers &) const = default;
};

struct BookingRequest {
  std::uint64_t id{};
  MarketSegment segment{MarketSegment::CoupleLeisure};
  int arrivalDay{};
  int departureDay{};
  std::int64_t budgetCents{};
  int partySize{1};
  int amenityPreference{};
  int locationPreference{};
  int brandPreference{};
  // Appended to preserve aggregate initialization used by earlier FINAL-06 callers.
  int bookingDay{};
  bool operator==(const BookingRequest &) const = default;
};

struct MarketHotelOffer {
  std::uint64_t hotelId{};
  std::int64_t nightlyRateCents{};
  int reputation{};
  int stars{};
  int amenityScore{};
  int locationScore{};
  int brandScore{};
  bool sellable{};
  bool operator==(const MarketHotelOffer &) const = default;
};

struct CompetitorOffer {
  std::uint64_t hotelId{};
  std::string name;
  std::int64_t nightlyRateCents{};
  int reputation{};
  int stars{};
  int amenityScore{};
  int locationScore{};
  int brandScore{};
  bool operator==(const CompetitorOffer &) const = default;
};

struct MarketChoice {
  std::uint64_t requestId{};
  std::uint64_t hotelId{};
  bool playerWon{};
  bool operator==(const MarketChoice &) const = default;
};

struct MarketSnapshot {
  std::uint64_t generatedRequests{};
  std::uint64_t playerWins{};
  std::uint64_t competitorWins{};
  std::uint64_t unallocatedRequests{};
  std::int64_t comparableMedianRateCents{};
  int physicalCompetitorGuests{};
  std::vector<BookingRequest> requests;
  std::vector<CompetitorOffer> competitors;
  std::vector<MarketChoice> choices;
  bool operator==(const MarketSnapshot &) const = default;
};

class MarketDemandSystem {
public:
  explicit MarketDemandSystem(std::uint64_t seed = 1);

  void setPlayerOffer(const MarketHotelOffer &offer);
  void setCompetitors(std::vector<CompetitorOffer> competitors);
  void setSegmentDemandProfile(const SegmentDemandProfile &profile);
  [[nodiscard]] double potentialDemand(MarketSegment segment, int stayDay,
                                       const MarketDemandModifiers &modifiers) const;
  [[nodiscard]] int generatePotentialRequests(
      MarketSegment segment, int stayDay, const MarketDemandModifiers &modifiers);
  void generateRequests(int firstArrivalDay, int lastArrivalDay, int count);

  [[nodiscard]] bool isEligible(const BookingRequest &request,
                                const MarketHotelOffer &hotel) const;
  [[nodiscard]] double priceUtility(const BookingRequest &request,
                                    const MarketHotelOffer &hotel) const;
  [[nodiscard]] double playerChoiceWeight(const BookingRequest &request,
                                          const MarketHotelOffer &hotel) const;
  [[nodiscard]] MarketSnapshot snapshot() const;
  [[nodiscard]] std::string save() const;
  static MarketDemandSystem load(std::string_view data);

private:
  std::uint64_t seed_{};
  std::uint64_t rngState_{};
  std::uint64_t nextRequestId_{1};
  MarketHotelOffer player_{};
  std::vector<CompetitorOffer> competitors_;
  std::map<MarketSegment, SegmentDemandProfile> demandProfiles_;
  MarketSnapshot snapshot_{};

  [[nodiscard]] std::uint64_t nextRandom();
  [[nodiscard]] double unitRandom();
  [[nodiscard]] const SegmentDemandProfile *profileFor(MarketSegment segment) const;
  void allocate(const BookingRequest &request);
  void refreshComparableMedian();
};

} // namespace hh::game
