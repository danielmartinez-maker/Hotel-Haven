#include "hh/game/EconomyRuntime.h"

namespace hh::game {

MarketDefinitionLoadResult EconomyRuntime::loadMarketDefinitions(
    std::string_view jsonText) {
  return market_.loadDefinitions(jsonText);
}

int EconomyRuntime::marketChoiceTemperatureBasisPoints() const noexcept {
  return market_.choiceTemperatureBasisPoints();
}

} // namespace hh::game
