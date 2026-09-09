#include "hh/game/Simulation.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
std::string read(const std::filesystem::path &p) {
  std::ifstream f(p, std::ios::binary);
  if (!f)
    throw std::runtime_error("Cannot read " + p.string());
  return {std::istreambuf_iterator<char>(f), {}};
}
void write(const std::filesystem::path &p, const std::string &s) {
  std::ofstream f(p, std::ios::binary | std::ios::trunc);
  f << s;
  if (!f)
    throw std::runtime_error("Cannot write " + p.string());
}
std::string jsonEscape(std::string_view value) {
  std::ostringstream out;
  for (const unsigned char ch : value) {
    switch (ch) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (ch < 0x20) {
        out << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(ch) << std::dec << std::setfill(' ');
      } else {
        out << static_cast<char>(ch);
      }
    }
  }
  return out.str();
}
double guestSatisfaction(const hh::game::SimulationView &view) {
  // Reservation satisfaction is the authoritative 0..100 gameplay value and
  // includes completed-stay history in SimulationView. Review scores use the
  // separate 1.0..10.0 presentation scale and must not feed this objective.
  if (!view.reservations.empty()) {
    double total = 0.0;
    for (const auto &reservation : view.reservations)
      total += reservation.satisfaction;
    return total / static_cast<double>(view.reservations.size());
  }

  double total = 0.0;
  std::size_t count = 0;
  for (const auto &person : view.people)
    if (person.kind == hh::game::PersonKind::Guest &&
        person.state != hh::game::PersonState::CheckedOut) {
      total += person.satisfaction;
      ++count;
    }
  return count ? total / static_cast<double>(count) : view.economy.reputation;
}
double staffFatigueLoad(const hh::game::SimulationView &view) {
  double total = 0.0;
  std::size_t count = 0;
  for (const auto &person : view.people)
    if (person.kind != hh::game::PersonKind::Guest) {
      total += person.fatigue;
      ++count;
    }
  return count ? total / static_cast<double>(count) : 0.0;
}
double excessWaitMinutes(const hh::game::SimulationView &view) {
  // Keep the Balance Lab service-wait objective aligned with the simulation's
  // authoritative guest satisfaction penalty threshold.
  constexpr int acceptableWaitSeconds = 8 * 60;
  double totalSeconds = 0.0;
  for (const auto &person : view.people)
    if (person.kind == hh::game::PersonKind::Guest)
      totalSeconds += std::max(0, person.queueWaitSeconds - acceptableWaitSeconds);
  return totalSeconds / 60.0;
}
void writeBalanceSummary(const std::filesystem::path &path,
                         std::string_view runId, std::uint64_t seed, int days,
                         const hh::game::SimulationView &view) {
  if (runId.empty())
    throw std::invalid_argument("--run-id is required with --balance-summary");
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());

  const std::int64_t operatingCost =
      view.economy.payrollCents + view.economy.supplyCostCents +
      view.economy.utilityCostCents;
  const std::int64_t gop = view.economy.revenueCents - operatingCost;

  std::ostringstream json;
  json << std::fixed << std::setprecision(6)
       << "{\n"
       << "  \"schema\": 1,\n"
       << "  \"run_id\": \"" << jsonEscape(runId) << "\",\n"
       << "  \"seed\": " << seed << ",\n"
       << "  \"days\": " << days << ",\n"
       << "  \"simulation_second\": " << view.elapsedSeconds << ",\n"
       << "  \"operating_cost_minor\": " << operatingCost << ",\n"
       << "  \"guest_satisfaction\": " << guestSatisfaction(view) << ",\n"
       << "  \"fatigue_load\": " << staffFatigueLoad(view) << ",\n"
       << "  \"excess_wait_minutes\": " << excessWaitMinutes(view) << ",\n"
       << "  \"gop_minor\": " << gop << ",\n"
       << "  \"completed_stays\": " << view.economy.completedStays << ",\n"
       << "  \"reputation\": " << view.economy.reputation << "\n"
       << "}\n";
  write(path, json.str());
}
} // namespace
int main(int argc, char **argv) {
  try {
    int days = 7;
    std::uint64_t seed = 20260907;
    std::filesystem::path load, save, definitions, balanceSummary;
    std::string runId;
    bool restock = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      auto value = [&]() {
        if (++i >= argc)
          throw std::invalid_argument("Missing value for " + arg);
        return std::string(argv[i]);
      };
      if (arg == "--days")
        days = std::stoi(value());
      else if (arg == "--seed")
        seed = std::stoull(value());
      else if (arg == "--load")
        load = value();
      else if (arg == "--save")
        save = value();
      else if (arg == "--data")
        definitions = value();
      else if (arg == "--run-id")
        runId = value();
      else if (arg == "--balance-summary")
        balanceSummary = value();
      else if (arg == "--restock")
        restock = true;
      else if (arg == "--help") {
        std::cout << "Hotel Haven campaign runner\n--days N (0..365) --seed N "
                     "--load FILE --save FILE --data balance.json "
                     "--run-id ID --balance-summary FILE --restock\n"
                     "Writes one CSV row for each campaign day and optionally "
                     "a Balance Lab JSON run summary.\n";
        return 0;
      } else
        throw std::invalid_argument("Unknown argument: " + arg);
    }
    if (days < 0 || days > 365)
      throw std::invalid_argument("days must be 0..365");
    auto simulation = load.empty() ? hh::game::Simulation::tutorial(seed)
                                   : hh::game::Simulation::load(read(load));
    if (!definitions.empty()) {
      const auto r = simulation.loadDefinitions(read(definitions));
      if (!r)
        throw std::invalid_argument(r.message);
    }
    std::cout << "day,cash_cents,revenue_cents,payroll_cents,occupancy,"
                 "reputation,completed_stays,linen,blocked_tasks\n";
    for (int day = 0; day < days; ++day) {
      if (restock && simulation.view().inventory.linen < 12) {
        const auto r = simulation.orderSupplies({20, 40, 20, 20, 4});
        if (!r)
          std::cerr << "Reorder: " << r.message << '\n';
      }
      simulation.step(86400);
      const auto v = simulation.view();
      int blocked = 0;
      for (const auto &t : v.tasks)
        blocked += t.status == hh::game::TaskStatus::Blocked;
      std::cout << v.day << ',' << v.economy.cashCents << ','
                << v.economy.revenueCents << ',' << v.economy.payrollCents
                << ',' << v.economy.occupancy << ',' << v.economy.reputation
                << ',' << v.economy.completedStays << ',' << v.inventory.linen
                << ',' << blocked << '\n';
    }
    if (!save.empty())
      write(save, simulation.save());
    if (!balanceSummary.empty())
      writeBalanceSummary(balanceSummary, runId, seed, days, simulation.view());
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Hotel Haven: " << e.what() << '\n';
    return 1;
  }
}
