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

  Leisure = CoupleLeisure,
  Group = ConferenceGroup,
  Luxury = LuxuryLeisure,
  Budget = BudgetLeisure
};

struct SegmentChoiceWeights {
  int priceBasisPoints{3000};
  int reputationBasisPoints{2000};
  int amenityBasisPoints{1500};
  int locationBasisPoints{1000};
  int starBasisPoints{1000};
  int roomBasisPoints{500};
  int brandBasisPoints{1000};
  [[nodiscard]] int totalBasisPoints() const noexcept {
    return priceBasisPoints + reputationBasisPoints + amenityBasisPoints +
           locationBasisPoints + starBasisPoints + roomBasisPoints +
           brandBasisPoints;
  }
  bool operator==(const SegmentChoiceWeights &) const = default;
};

struct SegmentDemandProfile {
  MarketSegment segment{MarketSegment::CoupleLeisure};
  double baseDailyDemand{};
  std::array<int, 7> weekdayMultiplierBasisPoints{
      10000, 10000, 10000, 10000, 10000, 10000, 10000};
  int medianLeadTimeDays{};
  int medianStayNights{1};
  std::int64_t baseBudgetCents{};
  int priceElasticityBasisPoints{10000};
  int amenitySensitivityBasisPoints{10000};
  int cancellationBasisPoints{};
  int noShowBasisPoints{};
  SegmentChoiceWeights choiceWeights{};
  bool operator==(const SegmentDemandProfile &) const = default;
};

struct MarketDemandModifiers {
  int seasonMultiplierBasisPoints{10000};
  int economicMultiplierBasisPoints{10000};
  int eventMultiplierBasisPoints{10000};
  int scenarioMultiplierBasisPoints{10000};
  bool operator==(const MarketDemandModifiers &) const = default;
};

struct ReputationCategoryScores {
  int service{-1};
  int room{-1};
  int cleanliness{-1};
  int quiet{-1};
  int business{-1};
  int food{-1};
  bool operator==(const ReputationCategoryScores &) const = default;
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
  int bookingDay{};
  std::string roomCategory{"standard"};
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
  ReputationCategoryScores reputationCategories{};
  bool operator==(const MarketHotelOffer &) const = default;
};

struct CompetitorInventoryWindow {
  std::string roomCategory{"standard"};
  int startDay{};
  int endDay{};
  std::int64_t nightlyRateCents{};
  int availableRooms{};
  bool operator==(const CompetitorInventoryWindow &) const = default;
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
  ReputationCategoryScores reputationCategories{};
  int roomCount{};
  std::vector<std::string> roomCategories;
  std::vector<CompetitorInventoryWindow> inventoryWindows;
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

struct MarketDefinitionLoadResult {
  bool ok{};
  std::string reason;
  explicit operator bool() const noexcept { return ok; }
};

class MarketDemandSystem {
public:
  explicit MarketDemandSystem(std::uint64_t seed = 1);

  void setPlayerOffer(const MarketHotelOffer &offer);
  void setCompetitors(std::vector<CompetitorOffer> competitors);
  void setSegmentDemandProfile(const SegmentDemandProfile &profile);
  [[nodiscard]] const SegmentDemandProfile &segmentDemandProfile(MarketSegment segment) const;
  void setPlayerConsiderationBasisPoints(MarketSegment segment, int basisPoints);
  [[nodiscard]] MarketDefinitionLoadResult loadDefinitions(std::string_view jsonText);
  [[nodiscard]] int choiceTemperatureBasisPoints() const noexcept;
  [[nodiscard]] double potentialDemand(MarketSegment segment, int stayDay,
                                       const MarketDemandModifiers &modifiers) const;
  [[nodiscard]] int generatePotentialRequests(
      MarketSegment segment, int stayDay, const MarketDemandModifiers &modifiers);
  void generateRequests(int firstArrivalDay, int lastArrivalDay, int count);

  [[nodiscard]] bool isEligible(const BookingRequest &request,
                                const MarketHotelOffer &hotel) const;
  [[nodiscard]] double priceUtility(const BookingRequest &request,
                                    const MarketHotelOffer &hotel) const;
  [[nodiscard]] double reputationUtility(const BookingRequest &request,
                                         const MarketHotelOffer &hotel) const;
  [[nodiscard]] double playerChoiceWeight(const BookingRequest &request,
                                          const MarketHotelOffer &hotel) const;
  [[nodiscard]] MarketHotelOffer effectiveCompetitorOffer(
      std::uint64_t hotelId, const BookingRequest &request) const;
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
  std::map<MarketSegment, int> playerConsiderationBasisPoints_;
  int choiceTemperatureBasisPoints_{3500};
  MarketSnapshot snapshot_{};

  [[nodiscard]] std::uint64_t nextRandom();
  [[nodiscard]] double unitRandom();
  [[nodiscard]] const SegmentDemandProfile *profileFor(MarketSegment segment) const;
  [[nodiscard]] bool playerConsidered(const BookingRequest &request) const;
  void allocate(const BookingRequest &request);
  void refreshComparableMedian();
};

} // namespace hh::game
