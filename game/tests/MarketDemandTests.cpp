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

  MarketDemandSystem capture(73);
  capture.setPlayerOffer({1, 16'000, 80, 4, 80, 80, 80, true});
  require(capture.competitiveCaptureBasisPoints(MarketSegment::Business) == 10000,
          "competitor-free market diluted physical demand");
  capture.setCompetitors({
      {2, "Weak Rival", 21'000, 65, 3, 55, 55, 50},
  });
  const int weakRivalCapture =
      capture.competitiveCaptureBasisPoints(MarketSegment::Business);
  capture.setCompetitors({
      {2, "Strong Rival", 13'000, 92, 5, 95, 92, 90},
  });
  const int strongRivalCapture =
      capture.competitiveCaptureBasisPoints(MarketSegment::Business);
  require(weakRivalCapture > 0 && weakRivalCapture < 10000,
          "eligible competitor did not dilute player capture");
  require(strongRivalCapture < weakRivalCapture,
          "stronger competitor did not reduce player segment capture");

  MarketDemandSystem unconfigured(73);
  require(unconfigured.competitiveCaptureBasisPoints(MarketSegment::Business) == 10000,
          "unconfigured market should be neutral to physical bookings");
}
