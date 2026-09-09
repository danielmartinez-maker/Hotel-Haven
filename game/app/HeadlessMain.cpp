#include "hh/game/Simulation.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

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
} // namespace
int main(int argc, char **argv) {
  try {
    int days = 7;
    std::uint64_t seed = 20260907;
    std::filesystem::path load, save, definitions;
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
      else if (arg == "--restock")
        restock = true;
      else if (arg == "--help") {
        std::cout << "Hotel Haven campaign runner\n--days N (0..365) --seed N "
                     "--load FILE --save FILE --data balance.json "
                     "--restock\nWrites one CSV row for each campaign day.\n";
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
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Hotel Haven: " << e.what() << '\n';
    return 1;
  }
}
