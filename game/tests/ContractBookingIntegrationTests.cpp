#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  EconomyRuntime runtime(808);
  runtime.setPhysicalRoomCapacity("standard", 2);
  runtime.setPlayerHotelOffer({1, 17'000, 75, 4, 80, 80, 80, true});
  runtime.setCompetitors({{2, "Rival", 17'000, 75, 4, 80, 80, 80}});
  MarketDemandModifiers noTransientDemand;
  noTransientDemand.scenarioMultiplierBasisPoints = 0;
  runtime.setDemandModifiers(noTransientDemand);

  CommercialContract contract;
  contract.id = 88;
  contract.startDay = 10;
  contract.endDay = 12;
  contract.minimumRoomNights = 3;
  contract.maximumRoomNights = 4;
  contract.negotiatedRateCents = 9'000;
  contract.paymentDelayDays = 5;
  const auto accepted = runtime.acceptCommercialContract(
      contract, ContractFeasibility{6, 0, 0}, false);
  require(accepted.ok && !accepted.acceptedRisk,
          "feasible commercial contract was rejected");

  const auto inventory = runtime.revenueManagementSnapshot().inventory;
  int contractBookings = 0;
  for (const auto &booking : inventory.bookings)
    if (booking.sourceContractId == contract.id) {
      ++contractBookings;
      require(booking.rateCents == contract.negotiatedRateCents,
              "contract booking lost negotiated rate");
      require(booking.channel == BookingChannel::Group,
              "contract booking lost group-channel attribution");
      require(booking.departureDay == booking.arrivalDay + 1,
              "contract room night was not scheduled as one room-night");
      require(booking.paymentDay == booking.departureDay + contract.paymentDelayDays,
              "contract payment timing was not retained");
    }
  require(contractBookings == contract.minimumRoomNights,
          "contract did not reserve its minimum room-night commitment");

  const auto commercial = runtime.commercialSnapshot();
  require(commercial.contracts.size() == 1 &&
              commercial.contracts.front().reservedRoomNights == 3 &&
              commercial.contracts.front().unfulfilledRoomNights == 0,
          "contract commitment diagnostics did not reconcile to inventory");

  runtime.runDays(15);
  require(runtime.financialSnapshot().economics.roomRevenueCents == 0,
          "delayed contract payment posted before contractual payment day");
  runtime.runDays(4);
  require(runtime.financialSnapshot().economics.roomRevenueCents == 27'000,
          "contract room revenue did not post at delayed payment timing");

  EconomyRuntime risky(809);
  risky.setPhysicalRoomCapacity("standard", 1);
  risky.setPlayerHotelOffer({1, 17'000, 75, 4, 80, 80, 80, true});
  CommercialContract impossible = contract;
  impossible.id = 89;
  impossible.startDay = 1;
  impossible.endDay = 2;
  impossible.minimumRoomNights = 5;
  impossible.maximumRoomNights = 5;
  require(!risky.acceptCommercialContract(
               impossible, ContractFeasibility{5, 0, 0}, false)
               .ok,
          "runtime accepted a contract exceeding authoritative room inventory");
  const auto acceptedRisk = risky.acceptCommercialContract(
      impossible, ContractFeasibility{5, 0, 0}, true);
  require(acceptedRisk.ok && acceptedRisk.acceptedRisk,
          "explicit contract risk route was not preserved");
  const auto riskyContract = risky.commercialSnapshot().contracts.front();
  require(riskyContract.reservedRoomNights == 2 &&
              riskyContract.unfulfilledRoomNights == 3,
          "accepted-risk contract did not expose unfulfilled room nights");

  const auto saved = risky.save();
  require(EconomyRuntime::load(saved).save() == saved,
          "contract inventory/economic state did not round-trip");
}
