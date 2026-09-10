#include "hh/game/MarketDemand.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game {

std::string MarketDemandSystem::save() const {
  std::ostringstream out;
  out << "HHMARKET 1 " << seed_ << ' ' << rngState_ << ' ' << nextRequestId_ << ' '
      << player_.hotelId << ' ' << player_.nightlyRateCents << ' ' << player_.reputation << ' '
      << player_.stars << ' ' << player_.amenityScore << ' ' << player_.locationScore << ' '
      << player_.brandScore << ' ' << player_.sellable << ' ' << competitors_.size();
  for (const auto &c : competitors_)
    out << ' ' << c.hotelId << ' ' << std::quoted(c.name) << ' ' << c.nightlyRateCents << ' '
        << c.reputation << ' ' << c.stars << ' ' << c.amenityScore << ' ' << c.locationScore << ' '
        << c.brandScore;
  out << ' ' << snapshot_.generatedRequests << ' ' << snapshot_.playerWins << ' '
      << snapshot_.competitorWins << ' ' << snapshot_.unallocatedRequests << ' '
      << snapshot_.comparableMedianRateCents << ' ' << snapshot_.physicalCompetitorGuests << ' '
      << snapshot_.requests.size();
  for (const auto &r : snapshot_.requests)
    out << ' ' << r.id << ' ' << static_cast<int>(r.segment) << ' ' << r.arrivalDay << ' '
        << r.departureDay << ' ' << r.budgetCents << ' ' << r.partySize << ' '
        << r.amenityPreference << ' ' << r.locationPreference << ' ' << r.brandPreference;
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
  if (!in || magic != "HHMARKET" || version != 1 || seed == 0 || nextRequest == 0)
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
    if (!in || segment < static_cast<int>(MarketSegment::Leisure) ||
        segment > static_cast<int>(MarketSegment::Budget))
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
