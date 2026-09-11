#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  EconomyRuntime runtime(1618);
  MarketHotelOffer offer{1, 18'000, 80, 4, 75, 80, 70, true};
  offer.reputationCategories = {90, 72, 84, 65, 95, 58};
  runtime.setPlayerHotelOffer(offer);

  const auto commercial = runtime.commercialSnapshot();
  require(commercial.overallReputationBasisPoints == 8000,
          "initial overall reputation changed");
  require(commercial.serviceReputationBasisPoints == 9000 &&
              commercial.roomReputationBasisPoints == 7200 &&
              commercial.cleanlinessReputationBasisPoints == 8400 &&
              commercial.quietReputationBasisPoints == 6500 &&
              commercial.businessReputationBasisPoints == 9500 &&
              commercial.foodReputationBasisPoints == 5800,
          "initial category reputation profile was flattened");

  const auto saved = runtime.save();
  const auto restored = EconomyRuntime::load(saved);
  const auto restoredCommercial = restored.commercialSnapshot();
  require(restoredCommercial.serviceReputationBasisPoints == 9000 &&
              restoredCommercial.businessReputationBasisPoints == 9500,
          "initial category reputation profile did not survive save/load");
}
