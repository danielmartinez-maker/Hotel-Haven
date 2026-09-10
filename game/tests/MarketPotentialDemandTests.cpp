#include "hh/game/MarketDemand.h"
#include <cmath>
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }
static void requireNear(double a, double b, double eps, const char *m) {
  if (std::abs(a - b) > eps) throw std::runtime_error(m);
}

int main() {
  MarketDemandSystem market(3030);
  SegmentDemandProfile profile;
  profile.segment = MarketSegment::Business;
  profile.baseDailyDemand = 100.0;
  profile.weekdayMultiplierBasisPoints.fill(10000);
  profile.weekdayMultiplierBasisPoints[2] = 12000;
  profile.medianLeadTimeDays = 10;
  profile.medianStayNights = 2;
  profile.baseBudgetCents = 20'000;
  market.setSegmentDemandProfile(profile);

  MarketDemandModifiers modifiers;
  modifiers.seasonMultiplierBasisPoints = 15000;
  modifiers.economicMultiplierBasisPoints = 8000;
  modifiers.eventMultiplierBasisPoints = 12500;
  modifiers.scenarioMultiplierBasisPoints = 9000;
  requireNear(market.potentialDemand(MarketSegment::Business, 30, modifiers),
              162.0, 1e-9,
              "HMG-030 potential demand product changed");

  const int generated = market.generatePotentialRequests(
      MarketSegment::Business, 30, modifiers);
  require(generated == 162,
          "integer potential demand did not generate exact request count");
  const auto snap = market.snapshot();
  require(snap.requests.size() == 162,
          "potential demand request count diverged from snapshot");
  for (const auto &request : snap.requests) {
    require(request.segment == MarketSegment::Business,
            "segment-specific demand generated wrong segment");
    require(request.bookingDay == 20 && request.arrivalDay == 30 &&
                request.departureDay == 32,
            "lead-time/stay baseline was not applied to generated request");
    require(request.budgetCents == 20'000,
            "segment budget baseline was not retained");
  }

  const auto saved = market.save();
  const auto restored = MarketDemandSystem::load(saved);
  require(restored.save() == saved,
          "segment profiles/potential-demand state did not round-trip");
  requireNear(restored.potentialDemand(MarketSegment::Business, 30, modifiers),
              162.0, 1e-9,
              "restored segment profile changed potential demand");
}
