#include "hh/game/RevenueManagement.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  RevenueManagement rm;
  require(rm.setRule({1, 10, 20, 0x7f, "standard", 0, 10000, 12000, 1, 10000, 30000}).ok,
          "base pricing rule rejected");
  require(rm.setRule({2, 10, 20, 0x7f, "standard", 8000, 10000, 22000, 10, 10000, 30000}).ok,
          "high-occupancy pricing rule rejected");
  require(rm.effectiveRateCents(12, 2, "standard", 8500, 15000) == 22000,
          "explicit pricing priority or occupancy threshold failed");
  require(rm.effectiveRateCents(12, 2, "standard", 5000, 15000) == 12000,
          "lower-priority base rule did not apply");

  require(rm.setRule({3, 10, 20, 0x7f, "suite", 0, 10000, 50000, 5, 18000, 32000}).ok,
          "clamped rule rejected");
  require(rm.effectiveRateCents(12, 2, "suite", 5000, 25000) == 32000,
          "maximum rate clamp was not enforced");

  rm.setComparableMedianCents("standard", 16000);
  require(rm.snapshot().pricePositions.at("standard") == PricePosition::BelowMarket,
          "comparable-rate price position incorrect");
}
