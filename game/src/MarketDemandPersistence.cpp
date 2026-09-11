#include "hh/game/MarketDemand.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

void writeReputationCategories(std::ostringstream &out,
                               const ReputationCategoryScores &scores) {
  out << ' ' << scores.service << ' ' << scores.room << ' '
      << scores.cleanliness << ' ' << scores.quiet << ' ' << scores.business
      << ' ' << scores.food;
}

void readReputationCategories(std::istringstream &in,
                              ReputationCategoryScores &scores) {
  in >> scores.service >> scores.room >> scores.cleanliness >> scores.quiet >>
      scores.business >> scores.food;
  const auto valid = [](int value) {
    return value == -1 || (value >= 0 && value <= 100);
  };
  if (!in || !valid(scores.service) || !valid(scores.room) ||
      !valid(scores.cleanliness) || !valid(scores.quiet) ||
      !valid(scores.business) || !valid(scores.food))
    throw std::invalid_argument("invalid saved reputation categories");
}

void writeChoiceWeights(std::ostringstream &out,
                        const SegmentChoiceWeights &weights) {
  out << ' ' << weights.priceBasisPoints << ' ' << weights.reputationBasisPoints
      << ' ' << weights.amenityBasisPoints << ' ' << weights.locationBasisPoints
      << ' ' << weights.starBasisPoints << ' ' << weights.roomBasisPoints << ' '
      << weights.brandBasisPoints;
}

void readChoiceWeights(std::istringstream &in, SegmentChoiceWeights &weights) {
  in >> weights.priceBasisPoints >> weights.reputationBasisPoints >>
      weights.amenityBasisPoints >> weights.locationBasisPoints >>
      weights.starBasisPoints >> weights.roomBasisPoints >> weights.brandBasisPoints;
  if (!in || weights.priceBasisPoints < 0 || weights.reputationBasisPoints < 0 ||
      weights.amenityBasisPoints < 0 || weights.locationBasisPoints < 0 ||
      weights.starBasisPoints < 0 || weights.roomBasisPoints < 0 ||
      weights.brandBasisPoints < 0 || weights.totalBasisPoints() != 10000)
    throw std::invalid_argument("invalid saved market choice weights");
}

} // namespace

std::string MarketDemandSystem::save() const {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "HHMARKET 6 " << seed_ << ' ' << rngState_ << ' ' << nextRequestId_ << ' '
      << player_.hotelId << ' ' << player_.nightlyRateCents << ' ' << player_.reputation << ' '
      << player_.stars << ' ' << player_.amenityScore << ' ' << player_.locationScore << ' '
      << player_.brandScore << ' ' << player_.sellable;
  writeReputationCategories(out, player_.reputationCategories);
  out << ' ' << competitors_.size();
  for (const auto &c : competitors_) {
    out << ' ' << c.hotelId << ' ' << std::quoted(c.name) << ' ' << c.nightlyRateCents << ' '
        << c.reputation << ' ' << c.stars << ' ' << c.amenityScore << ' '
        << c.locationScore << ' ' << c.brandScore;
    writeReputationCategories(out, c.reputationCategories);
    out << ' ' << c.roomCount << ' ' << c.roomCategories.size();
    for (const auto &category : c.roomCategories)
      out << ' ' << std::quoted(category);
    out << ' ' << c.inventoryWindows.size();
    for (const auto &window : c.inventoryWindows)
      out << ' ' << std::quoted(window.roomCategory) << ' ' << window.startDay << ' '
          << window.endDay << ' ' << window.nightlyRateCents << ' '
          << window.availableRooms;
  }

  out << ' ' << demandProfiles_.size();
  for (const auto &[segment, profile] : demandProfiles_) {
    out << ' ' << static_cast<int>(segment) << ' ' << profile.baseDailyDemand;
    for (const int multiplier : profile.weekdayMultiplierBasisPoints)
      out << ' ' << multiplier;
    out << ' ' << profile.medianLeadTimeDays << ' ' << profile.medianStayNights << ' '
        << profile.baseBudgetCents << ' ' << profile.priceElasticityBasisPoints << ' '
        << profile.amenitySensitivityBasisPoints << ' '
        << profile.cancellationBasisPoints << ' ' << profile.noShowBasisPoints;
    writeChoiceWeights(out, profile.choiceWeights);
  }

  out << ' ' << playerConsiderationBasisPoints_.size();
  for (const auto &[segment, basisPoints] : playerConsiderationBasisPoints_)
    out << ' ' << static_cast<int>(segment) << ' ' << basisPoints;
  out << ' ' << choiceTemperatureBasisPoints_;

  out << ' ' << snapshot_.generatedRequests << ' ' << snapshot_.playerWins << ' '
      << snapshot_.competitorWins << ' ' << snapshot_.unallocatedRequests << ' '
      << snapshot_.comparableMedianRateCents << ' ' << snapshot_.physicalCompetitorGuests << ' '
      << snapshot_.requests.size();
  for (const auto &r : snapshot_.requests)
    out << ' ' << r.id << ' ' << static_cast<int>(r.segment) << ' ' << r.arrivalDay << ' '
        << r.departureDay << ' ' << r.budgetCents << ' ' << r.partySize << ' '
        << r.amenityPreference << ' ' << r.locationPreference << ' ' << r.brandPreference << ' '
        << r.bookingDay << ' ' << std::quoted(r.roomCategory);
  out << ' ' << snapshot_.choices.size();
  for (const auto &c : snapshot_.choices)
    out << ' ' << c.requestId << ' ' << c.hotelId << ' ' << c.playerWon;
  return out.str();
}

MarketDemandSystem MarketDemandSystem::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::uint64_t seed{}, rngState{}, nextRequest{};
  in >> magic >> version >> seed >> rngState >> nextRequest;
  if (!in || magic != "HHMARKET" ||
      (version != 1 && version != 2 && version != 3 && version != 4 &&
       version != 5 && version != 6) ||
      seed == 0 || nextRequest == 0)
    throw std::invalid_argument("invalid market save");
  MarketDemandSystem result(seed);
  result.rngState_ = rngState;
  result.nextRequestId_ = nextRequest;
  in >> result.player_.hotelId >> result.player_.nightlyRateCents >> result.player_.reputation >>
      result.player_.stars >> result.player_.amenityScore >> result.player_.locationScore >>
      result.player_.brandScore >> result.player_.sellable;
  if (version >= 4)
    readReputationCategories(in, result.player_.reputationCategories);

  std::size_t count{};
  in >> count;
  if (!in || count > 10000)
    throw std::invalid_argument("invalid competitor count");
  std::vector<CompetitorOffer> loadedCompetitors;
  loadedCompetitors.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    CompetitorOffer c;
    in >> c.hotelId >> std::quoted(c.name) >> c.nightlyRateCents >> c.reputation >> c.stars >>
        c.amenityScore >> c.locationScore >> c.brandScore;
    if (version >= 4)
      readReputationCategories(in, c.reputationCategories);
    if (version >= 5) {
      std::size_t categoryCount{}, windowCount{};
      in >> c.roomCount >> categoryCount;
      if (!in || c.roomCount < 0 || categoryCount > 128)
        throw std::invalid_argument("invalid saved competitor categories");
      for (std::size_t j = 0; j < categoryCount; ++j) {
        std::string category;
        in >> std::quoted(category);
        if (!in || category.empty())
          throw std::invalid_argument("invalid saved competitor category");
        c.roomCategories.push_back(std::move(category));
      }
      in >> windowCount;
      if (!in || windowCount > 100000)
        throw std::invalid_argument("invalid saved competitor calendar count");
      for (std::size_t j = 0; j < windowCount; ++j) {
        CompetitorInventoryWindow window;
        in >> std::quoted(window.roomCategory) >> window.startDay >> window.endDay >>
            window.nightlyRateCents >> window.availableRooms;
        if (!in)
          throw std::invalid_argument("invalid saved competitor calendar");
        c.inventoryWindows.push_back(std::move(window));
      }
    }
    if (!in || c.hotelId == 0 || c.nightlyRateCents <= 0)
      throw std::invalid_argument("invalid saved competitor");
    loadedCompetitors.push_back(std::move(c));
  }
  result.setCompetitors(std::move(loadedCompetitors));

  if (version >= 2) {
    in >> count;
    if (!in || count > 9)
      throw std::invalid_argument("invalid demand profile count");
    for (std::size_t i = 0; i < count; ++i) {
      int segmentValue{};
      double baseDailyDemand{};
      in >> segmentValue >> baseDailyDemand;
      if (!in || segmentValue < static_cast<int>(MarketSegment::CoupleLeisure) ||
          segmentValue > static_cast<int>(MarketSegment::Wellness))
        throw std::invalid_argument("invalid saved demand profile segment");
      const auto segment = static_cast<MarketSegment>(segmentValue);
      auto profile = result.segmentDemandProfile(segment);
      profile.baseDailyDemand = baseDailyDemand;
      for (int &multiplier : profile.weekdayMultiplierBasisPoints)
        in >> multiplier;
      in >> profile.medianLeadTimeDays >> profile.medianStayNights >> profile.baseBudgetCents;
      if (version >= 6) {
        in >> profile.priceElasticityBasisPoints >>
            profile.amenitySensitivityBasisPoints >>
            profile.cancellationBasisPoints >> profile.noShowBasisPoints;
        readChoiceWeights(in, profile.choiceWeights);
      }
      if (!in)
        throw std::invalid_argument("invalid saved demand profile");
      result.setSegmentDemandProfile(profile);
    }
  }

  if (version >= 3) {
    in >> count;
    if (!in || count > 9)
      throw std::invalid_argument("invalid consideration policy count");
    for (std::size_t i = 0; i < count; ++i) {
      int segment{}, basisPoints{};
      in >> segment >> basisPoints;
      if (!in || segment < static_cast<int>(MarketSegment::CoupleLeisure) ||
          segment > static_cast<int>(MarketSegment::Wellness))
        throw std::invalid_argument("invalid saved consideration segment");
      result.setPlayerConsiderationBasisPoints(static_cast<MarketSegment>(segment),
                                               basisPoints);
    }
  }
  if (version >= 6) {
    in >> result.choiceTemperatureBasisPoints_;
    if (!in || result.choiceTemperatureBasisPoints_ <= 0 ||
        result.choiceTemperatureBasisPoints_ > 100000)
      throw std::invalid_argument("invalid saved choice temperature");
  }

  in >> result.snapshot_.generatedRequests >> result.snapshot_.playerWins >>
      result.snapshot_.competitorWins >> result.snapshot_.unallocatedRequests >>
      result.snapshot_.comparableMedianRateCents >> result.snapshot_.physicalCompetitorGuests >> count;
  if (!in || count > 2'000'000 || result.snapshot_.physicalCompetitorGuests != 0)
    throw std::invalid_argument("invalid market snapshot");
  result.snapshot_.requests.clear();
  for (std::size_t i = 0; i < count; ++i) {
    BookingRequest r;
    int segment{};
    in >> r.id >> segment >> r.arrivalDay >> r.departureDay >> r.budgetCents >> r.partySize >>
        r.amenityPreference >> r.locationPreference >> r.brandPreference;
    if (version >= 2)
      in >> r.bookingDay;
    else
      r.bookingDay = r.arrivalDay;
    if (version >= 5)
      in >> std::quoted(r.roomCategory);
    if (!in || segment < static_cast<int>(MarketSegment::CoupleLeisure) ||
        segment > static_cast<int>(MarketSegment::Wellness) || r.roomCategory.empty())
      throw std::invalid_argument("invalid saved booking request");
    r.segment = static_cast<MarketSegment>(segment);
    result.snapshot_.requests.push_back(std::move(r));
  }
  in >> count;
  if (!in || count > 2'000'000)
    throw std::invalid_argument("invalid market choice count");
  result.snapshot_.choices.clear();
  for (std::size_t i = 0; i < count; ++i) {
    MarketChoice choice;
    in >> choice.requestId >> choice.hotelId >> choice.playerWon;
    if (!in)
      throw std::invalid_argument("invalid saved market choice");
    result.snapshot_.choices.push_back(choice);
  }
  result.snapshot_.competitors = result.competitors_;
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected market trailing data");
  return result;
}

} // namespace hh::game
