#include "hh/game/MarketDemand.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game {

std::string MarketDemandSystem::save() const {
  std::ostringstream out;
  out << std::setprecision(17);
  out << "HHMARKET 3 " << seed_ << ' ' << rngState_ << ' ' << nextRequestId_ << ' '
      << player_.hotelId << ' ' << player_.nightlyRateCents << ' ' << player_.reputation << ' '
      << player_.stars << ' ' << player_.amenityScore << ' ' << player_.locationScore << ' '
      << player_.brandScore << ' ' << player_.sellable << ' ' << competitors_.size();
  for (const auto &c : competitors_)
    out << ' ' << c.hotelId << ' ' << std::quoted(c.name) << ' ' << c.nightlyRateCents << ' '
        << c.reputation << ' ' << c.stars << ' ' << c.amenityScore << ' ' << c.locationScore << ' '
        << c.brandScore;

  out << ' ' << demandProfiles_.size();
  for (const auto &[segment, profile] : demandProfiles_) {
    out << ' ' << static_cast<int>(segment) << ' ' << profile.baseDailyDemand;
    for (const int multiplier : profile.weekdayMultiplierBasisPoints)
      out << ' ' << multiplier;
    out << ' ' << profile.medianLeadTimeDays << ' ' << profile.medianStayNights << ' '
        << profile.baseBudgetCents;
  }

  out << ' ' << playerConsiderationBasisPoints_.size();
  for (const auto &[segment, basisPoints] : playerConsiderationBasisPoints_)
    out << ' ' << static_cast<int>(segment) << ' ' << basisPoints;

  out << ' ' << snapshot_.generatedRequests << ' ' << snapshot_.playerWins << ' '
      << snapshot_.competitorWins << ' ' << snapshot_.unallocatedRequests << ' '
      << snapshot_.comparableMedianRateCents << ' ' << snapshot_.physicalCompetitorGuests << ' '
      << snapshot_.requests.size();
  for (const auto &r : snapshot_.requests)
    out << ' ' << r.id << ' ' << static_cast<int>(r.segment) << ' ' << r.arrivalDay << ' '
        << r.departureDay << ' ' << r.budgetCents << ' ' << r.partySize << ' '
        << r.amenityPreference << ' ' << r.locationPreference << ' ' << r.brandPreference << ' '
        << r.bookingDay;
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
      (version != 1 && version != 2 && version != 3) || seed == 0 || nextRequest == 0)
    throw std::invalid_argument("invalid market save");
  MarketDemandSystem result(seed);
  result.rngState_ = rngState;
  result.nextRequestId_ = nextRequest;
  in >> result.player_.hotelId >> result.player_.nightlyRateCents >> result.player_.reputation >>
      result.player_.stars >> result.player_.amenityScore >> result.player_.locationScore >>
      result.player_.brandScore >> result.player_.sellable;
  std::size_t count{};
  in >> count;
  if (!in || count > 10000)
    throw std::invalid_argument("invalid competitor count");
  result.competitors_.clear();
  for (std::size_t i = 0; i < count; ++i) {
    CompetitorOffer c;
    in >> c.hotelId >> std::quoted(c.name) >> c.nightlyRateCents >> c.reputation >> c.stars >>
        c.amenityScore >> c.locationScore >> c.brandScore;
    if (!in || c.hotelId == 0 || c.nightlyRateCents <= 0)
      throw std::invalid_argument("invalid saved competitor");
    result.competitors_.push_back(std::move(c));
  }

  if (version >= 2) {
    in >> count;
    if (!in || count > 9)
      throw std::invalid_argument("invalid demand profile count");
    for (std::size_t i = 0; i < count; ++i) {
      SegmentDemandProfile profile;
      int segment{};
      in >> segment >> profile.baseDailyDemand;
      if (!in || segment < static_cast<int>(MarketSegment::CoupleLeisure) ||
          segment > static_cast<int>(MarketSegment::Wellness))
        throw std::invalid_argument("invalid saved demand profile segment");
      profile.segment = static_cast<MarketSegment>(segment);
      for (int &multiplier : profile.weekdayMultiplierBasisPoints)
        in >> multiplier;
      in >> profile.medianLeadTimeDays >> profile.medianStayNights >> profile.baseBudgetCents;
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
    if (!in || segment < static_cast<int>(MarketSegment::CoupleLeisure) ||
        segment > static_cast<int>(MarketSegment::Wellness))
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
