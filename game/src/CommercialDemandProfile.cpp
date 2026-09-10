#include "hh/game/CommercialDemand.h"

#include <stdexcept>

namespace hh::game {
namespace {

bool validCategoryScore(int value) {
  return value == -1 || (value >= 0 && value <= 100);
}

int categoryBasisPoints(int score, int overallBasisPoints) {
  return score < 0 ? overallBasisPoints : score * 100;
}

} // namespace

void CommercialDemand::setReputationProfile(
    int overallBasisPoints, const ReputationCategoryScores &categories) {
  if (overallBasisPoints < 0 || overallBasisPoints > 10000 ||
      !validCategoryScore(categories.service) ||
      !validCategoryScore(categories.room) ||
      !validCategoryScore(categories.cleanliness) ||
      !validCategoryScore(categories.quiet) ||
      !validCategoryScore(categories.business) ||
      !validCategoryScore(categories.food))
    throw std::invalid_argument("invalid reputation profile");

  snapshot_.overallReputationBasisPoints = overallBasisPoints;
  snapshot_.serviceReputationBasisPoints =
      categoryBasisPoints(categories.service, overallBasisPoints);
  snapshot_.roomReputationBasisPoints =
      categoryBasisPoints(categories.room, overallBasisPoints);
  snapshot_.cleanlinessReputationBasisPoints =
      categoryBasisPoints(categories.cleanliness, overallBasisPoints);
  snapshot_.quietReputationBasisPoints =
      categoryBasisPoints(categories.quiet, overallBasisPoints);
  snapshot_.businessReputationBasisPoints =
      categoryBasisPoints(categories.business, overallBasisPoints);
  snapshot_.foodReputationBasisPoints =
      categoryBasisPoints(categories.food, overallBasisPoints);
  snapshot_.valueReputationBasisPoints = overallBasisPoints;
  snapshot_.reviewCount = 0;
}

} // namespace hh::game
