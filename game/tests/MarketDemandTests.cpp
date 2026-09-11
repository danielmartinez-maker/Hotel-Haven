#include "hh/game/MarketDemand.h"
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  MarketDemandSystem a(42), b(42);
  MarketHotelOffer player{1, 16000, 80, 4, 80, 80, 80, true};
  CompetitorOffer c1{2, "Rival A", 15500, 72, 4, 70, 75, 70};
  CompetitorOffer c2{3, "Rival B", 17000, 76, 4, 78, 70, 72};
  a.setPlayerOffer(player); b.setPlayerOffer(player);
  a.setCompetitors({c1, c2}); b.setCompetitors({c1, c2});
  a.generateRequests(10, 12, 16); b.generateRequests(10, 12, 16);
  require(a.snapshot() == b.snapshot(), "same seed/state did not reproduce market stream");

  BookingRequest overBudget{9001, MarketSegment::Leisure, 10, 12, 10000, 2, 50, 50, 50};
  require(!a.isEligible(overBudget, MarketHotelOffer{1, 15001, 80, 4, 80, 80, 80, true}),
          "hotel above 1.5x request budget remained eligible");

  BookingRequest request{9002, MarketSegment::Business, 10, 12, 20000, 2, 60, 60, 60};
  const auto low = a.playerChoiceWeight(request, MarketHotelOffer{1, 16000, 55, 4, 80, 80, 80, true});
  const auto high = a.playerChoiceWeight(request, MarketHotelOffer{1, 16000, 90, 4, 80, 80, 80, true});
  require(high >= low, "higher reputation reduced player choice weight");

  const auto snap = a.snapshot();
  require(snap.competitors.size() == 2 && snap.physicalCompetitorGuests == 0,
          "aggregate competitors created physical map guests");
}
