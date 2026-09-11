#include "GameUiBridge.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace hh::client {
namespace {

std::string cents(std::int64_t value) {
  const bool negative = value < 0;
  const std::uint64_t absolute =
      negative ? static_cast<std::uint64_t>(-(value + 1)) + 1U
               : static_cast<std::uint64_t>(value);
  std::string dollars = std::to_string(absolute / 100);
  for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(dollars.size()) - 3;
       i > 0; i -= 3)
    dollars.insert(static_cast<std::size_t>(i), ",");
  std::ostringstream out;
  if (negative)
    out << '-';
  out << '$' << dollars << '.' << std::setw(2) << std::setfill('0')
      << absolute % 100;
  return out.str();
}

std::string channelName(hh::game::BookingChannel channel) {
  switch (channel) {
  case hh::game::BookingChannel::Direct: return "Direct";
  case hh::game::BookingChannel::Ota: return "OTA";
  case hh::game::BookingChannel::Gds: return "GDS / travel agent";
  case hh::game::BookingChannel::Corporate: return "Corporate";
  case hh::game::BookingChannel::Group: return "Group";
  case hh::game::BookingChannel::WalkIn: return "Walk-in";
  }
  return "Unknown channel";
}

std::string segmentName(hh::game::MarketSegment segment) {
  switch (segment) {
  case hh::game::MarketSegment::CoupleLeisure: return "Couple leisure";
  case hh::game::MarketSegment::Business: return "Business";
  case hh::game::MarketSegment::ConferenceGroup: return "Conference group";
  case hh::game::MarketSegment::LuxuryLeisure: return "Luxury leisure";
  case hh::game::MarketSegment::BudgetLeisure: return "Budget leisure";
  case hh::game::MarketSegment::ExecutiveBusiness: return "Executive business";
  case hh::game::MarketSegment::FamilyLeisure: return "Family leisure";
  case hh::game::MarketSegment::AirportTransit: return "Airport transit";
  case hh::game::MarketSegment::Wellness: return "Wellness";
  }
  return "Unknown segment";
}

std::string departmentName(hh::game::EconomicDepartment department) {
  switch (department) {
  case hh::game::EconomicDepartment::Unassigned: return "Unassigned";
  case hh::game::EconomicDepartment::Rooms: return "Rooms";
  case hh::game::EconomicDepartment::FoodBeverage: return "Food & beverage";
  case hh::game::EconomicDepartment::Events: return "Events";
  case hh::game::EconomicDepartment::Spa: return "Spa";
  case hh::game::EconomicDepartment::Parking: return "Parking";
  case hh::game::EconomicDepartment::Amenities: return "Amenities";
  case hh::game::EconomicDepartment::Undistributed: return "Undistributed";
  case hh::game::EconomicDepartment::NonOperating: return "Non-operating";
  }
  return "Unknown department";
}

void mixRevision(std::uint64_t &hash, std::uint64_t value) noexcept {
  constexpr std::uint64_t Prime = 1099511628211ULL;
  for (unsigned shift = 0; shift < 64; shift += 8) {
    hash ^= (value >> shift) & 0xffULL;
    hash *= Prime;
  }
}

void mixText(std::uint64_t &hash, const std::string &text) noexcept {
  constexpr std::uint64_t Prime = 1099511628211ULL;
  for (const unsigned char value : text) {
    hash ^= value;
    hash *= Prime;
  }
  mixRevision(hash, text.size());
}

void mixFieldList(std::uint64_t &hash,
                  const std::vector<hh::frontend::FieldSnapshot> &fields) noexcept {
  mixRevision(hash, fields.size());
  for (const auto &field : fields) {
    mixText(hash, field.label);
    mixText(hash, field.value);
  }
}

void mixEconomyRevision(std::uint64_t &hash,
                        const hh::frontend::EconomySnapshot &economy) noexcept {
  const auto &kpi = economy.kpis;
  mixRevision(hash, static_cast<std::uint64_t>(kpi.todayOccupancyPermille));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.sevenDayOccupancyPermille));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.thirtyDayOccupancyPermille));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.adrCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.revParCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.trevParCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.gopCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.roomRevenueCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.totalRevenueCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.laborCostCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.utilitiesCostCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.foodCostCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.cashCents));
  mixRevision(hash, static_cast<std::uint64_t>(kpi.cashRunwayDays));

  mixFieldList(hash, economy.departmentContribution);
  mixFieldList(hash, economy.bookingPace);
  mixFieldList(hash, economy.cancellationAndNoShow);
  mixFieldList(hash, economy.channelMix);
  mixFieldList(hash, economy.competitors);
  mixFieldList(hash, economy.demandBySegment);
  mixFieldList(hash, economy.futureRateCalendar);
  mixFieldList(hash, economy.campaigns);
  mixFieldList(hash, economy.contracts);
  mixFieldList(hash, economy.debtSchedule);

  mixRevision(hash, economy.financingDiagnostics.size());
  for (const auto &diagnostic : economy.financingDiagnostics) {
    mixText(hash, diagnostic.code);
    mixText(hash, diagnostic.message);
    mixRevision(hash, diagnostic.sourceEntityId);
    mixRevision(hash, diagnostic.causalParentId);
  }

  mixRevision(hash, economy.pricingRules.size());
  for (const auto &rule : economy.pricingRules) {
    mixRevision(hash, rule.ruleId);
    mixRevision(hash, static_cast<std::uint64_t>(rule.startDay));
    mixRevision(hash, static_cast<std::uint64_t>(rule.endDay));
    mixText(hash, rule.roomCategory);
    mixRevision(hash, static_cast<std::uint64_t>(rule.rateCents));
  }

  mixRevision(hash, economy.overbookingPolicies.size());
  for (const auto &policy : economy.overbookingPolicies) {
    mixText(hash, policy.roomCategory);
    mixRevision(hash, static_cast<std::uint64_t>(policy.allowance));
    mixRevision(hash,
                static_cast<std::uint64_t>(policy.relocationCompensationCents));
    mixRevision(hash, static_cast<std::uint64_t>(policy.startDay));
    mixRevision(hash, static_cast<std::uint64_t>(policy.endDay));
  }
}

void mapDiagnostics(const hh::game::EconomyDiagnostics &diagnostics,
                    hh::frontend::EconomySnapshot &economy) {
  economy.kpis.sevenDayOccupancyPermille =
      diagnostics.occupancy7DayBasisPoints / 10;
  economy.kpis.thirtyDayOccupancyPermille =
      diagnostics.occupancy30DayBasisPoints / 10;
  economy.kpis.adrCents = diagnostics.adrCents;
  economy.kpis.revParCents = diagnostics.revParCents;
  economy.kpis.roomRevenueCents = diagnostics.roomRevenueCents;
  economy.kpis.totalRevenueCents = diagnostics.totalRevenueCents;
  economy.kpis.laborCostCents = diagnostics.laborCostCents;
  economy.kpis.utilitiesCostCents = diagnostics.utilityCostCents;
  economy.kpis.foodCostCents = diagnostics.foodBeverageCostCents;
  economy.kpis.cashRunwayDays =
      static_cast<std::int64_t>(std::llround(diagnostics.cashRunwayDays));

  for (const auto &[arrivalDay, bookings] : diagnostics.bookingPaceByArrivalDay) {
    economy.bookingPace.push_back(
        {"Arrival day " + std::to_string(arrivalDay), std::to_string(bookings)});
  }

  economy.cancellationAndNoShow = {
      {"Cancellations", std::to_string(diagnostics.cancellations)},
      {"No-shows", std::to_string(diagnostics.noShows)},
  };

  economy.channelMix.clear();
  for (const auto &[channel, bookings] : diagnostics.channelBookings)
    economy.channelMix.push_back({channelName(channel), std::to_string(bookings)});

  economy.demandBySegment.clear();
  for (const auto &[segment, requests] : diagnostics.demandBySegment)
    economy.demandBySegment.push_back({segmentName(segment), std::to_string(requests)});

  economy.departmentContribution.clear();
  for (const auto &[department, contribution] : diagnostics.departmentContribution) {
    economy.departmentContribution.push_back(
        {departmentName(department),
         "Revenue " + cents(contribution.revenueCents) + " · expense " +
             cents(contribution.expenseCents) + " · contribution " +
             cents(contribution.contributionCents)});
  }

  economy.debtSchedule.clear();
  for (const auto &payment : diagnostics.debtPaymentSchedule) {
    economy.debtSchedule.push_back(
        {"Loan " + std::to_string(payment.loanId) + " · payment " +
             std::to_string(payment.paymentNumber) + " · day " +
             std::to_string(payment.paymentDay),
         cents(payment.amountCents) + " · principal " +
             cents(payment.principalCents) + " · interest " +
             cents(payment.interestCents)});
  }
}

void mapControlState(const hh::game::SimulationEconomyBridge &simulation,
                     hh::frontend::EconomySnapshot &economy) {
  economy.pricingRules.clear();
  for (const auto &rule : simulation.revenueManagementSnapshot().rules) {
    economy.pricingRules.push_back({rule.ruleId, rule.startDay, rule.endDay,
                                    rule.roomCategory, rule.rateCents});
  }

  economy.overbookingPolicies.clear();
  for (const auto &[category, policy] : simulation.overbookingPolicies()) {
    economy.overbookingPolicies.push_back(
        {category, policy.allowance, policy.relocationCompensationCents,
         policy.startDay, policy.endDay});
  }
}

} // namespace

hh::frontend::SimulationSnapshot
makeGameUiSnapshotSource(const hh::game::SimulationEconomyBridge &simulation,
                         const GameUiBridgeContext &context) {
  auto bridgedContext = context;
  bridgedContext.economy = &simulation.economyRuntime();
  auto snapshot =
      makeGameUiSnapshotSource(simulation.physicalSimulation(), bridgedContext);
  mapDiagnostics(simulation.economyDiagnostics(), snapshot.economy);
  mapControlState(simulation, snapshot.economy);
  mixEconomyRevision(snapshot.revision, snapshot.economy);
  return snapshot;
}

} // namespace hh::client
