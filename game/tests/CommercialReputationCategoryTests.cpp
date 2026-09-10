#include "hh/game/CommercialDemand.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  CommercialDemand commercial;
  commercial.setReputation(7000);

  ReviewSignal review;
  review.sourceId = 1;
  review.overallBasisPoints = 7000;
  review.serviceBasisPoints = 10'000;
  review.cleanlinessBasisPoints = 8000;
  review.valueBasisPoints = 7000;
  review.roomBasisPoints = 9000;
  review.quietBasisPoints = 6000;
  review.businessBasisPoints = 9500;
  review.foodBasisPoints = 5000;
  commercial.applyReview(review);

  const auto snapshot = commercial.snapshot();
  require(snapshot.overallReputationBasisPoints == 7000,
          "category review unexpectedly changed neutral overall reputation");
  require(snapshot.serviceReputationBasisPoints > 7000 &&
              snapshot.roomReputationBasisPoints > 7000 &&
              snapshot.cleanlinessReputationBasisPoints > 7000 &&
              snapshot.businessReputationBasisPoints > 7000,
          "positive reputation categories did not improve independently");
  require(snapshot.quietReputationBasisPoints < 7000 &&
              snapshot.foodReputationBasisPoints < 7000,
          "negative reputation categories did not decline independently");

  const auto saved = commercial.save();
  const auto restored = CommercialDemand::load(saved);
  require(restored.save() == saved,
          "category reputation state did not round-trip");
  require(restored.snapshot().roomReputationBasisPoints ==
              snapshot.roomReputationBasisPoints &&
              restored.snapshot().businessReputationBasisPoints ==
                  snapshot.businessReputationBasisPoints,
          "category reputation values changed after load");

  CommercialDemand backwardCompatible;
  backwardCompatible.setReputation(7000);
  backwardCompatible.applyReview({2, 8000, 8000, 8000, 8000});
  const auto legacyStyle = backwardCompatible.snapshot();
  require(legacyStyle.roomReputationBasisPoints > 7000 &&
              legacyStyle.quietReputationBasisPoints > 7000 &&
              legacyStyle.businessReputationBasisPoints > 7000 &&
              legacyStyle.foodReputationBasisPoints > 7000,
          "legacy review signal did not fall back missing categories to overall score");
}
