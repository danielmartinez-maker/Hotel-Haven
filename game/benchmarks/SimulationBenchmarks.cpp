#include "hh/game/Simulation.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using hh::game::CommandResult;
using hh::game::PersonKind;
using hh::game::Simulation;
using hh::game::SimulationView;

constexpr int kRepetitions = 3;
constexpr std::int64_t kScenarioSeconds = 24 * 60 * 60;

struct ScenarioSpec {
  std::string_view name;
  std::uint64_t seed{};
  int rooms{};
  int staff{};
};

struct StateSummary {
  int width{};
  int height{};
  int floors{};
  std::size_t tiles{};
  std::size_t rooms{};
  std::size_t staff{};
  std::size_t guests{};
  std::size_t reservations{};
  std::size_t tasks{};
  std::size_t reviews{};
  int completedStays{};
  std::size_t saveBytes{};

  bool operator==(const StateSummary &) const = default;
};

struct Sample {
  double setupMilliseconds{};
  double simulationMilliseconds{};
  std::uint64_t checksum{};
  StateSummary initial;
  StateSummary final;
};

struct ScenarioResult {
  ScenarioSpec spec;
  bool available{};
  std::string unavailableReason;
  bool deterministic{};
  std::uint64_t checksum{};
  std::vector<double> setupMilliseconds;
  std::vector<double> simulationMilliseconds;
  StateSummary initial;
  StateSummary final;
};

struct OptimizerResult {
  int employeeTaskScale{};
  bool deterministic{};
  std::size_t assignments{};
  std::uint64_t planChecksum{};
  std::vector<double> timingMilliseconds;
};

[[nodiscard]] double milliseconds(Clock::duration duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

void requireCommand(const CommandResult &result, std::string_view operation) {
  if (!result.ok)
    throw std::runtime_error(std::string(operation) + ": " + result.message);
}

void addBenchmarkRooms(Simulation &simulation, int requestedRoomCount) {
  constexpr int kTutorialRooms = 6;
  constexpr std::array<int, 8> kRoomColumns{1, 5, 9, 13, 17, 21, 25, 29};
  if (requestedRoomCount < kTutorialRooms || requestedRoomCount > 18)
    throw std::runtime_error(
        "public-API benchmark fixture supports 6..18 cash-funded rooms");

  const int additionalRooms = requestedRoomCount - kTutorialRooms;
  if (additionalRooms == 0)
    return;

  // The tutorial already exposes floor 2 through the stair at (12, 8).
  // Extend only through public construction commands so benchmark state follows
  // the same cash, topology, and reachability rules as player-built state.
  for (int x = 1; x < 32; ++x)
    requireCommand(simulation.buildTile(
                       {2, x, 8},
                       x == 12 ? hh::game::TileKind::Stairs
                               : hh::game::TileKind::Floor),
                   "extend floor-2 corridor");

  if (additionalRooms > static_cast<int>(kRoomColumns.size())) {
    for (int y = 9; y <= 12; ++y)
      requireCommand(simulation.buildTile({2, 28, y},
                                          hh::game::TileKind::Floor),
                     "connect second benchmark corridor");
    for (int x = 1; x < 32; ++x)
      requireCommand(simulation.buildTile({2, x, 12},
                                          hh::game::TileKind::Floor),
                     "build second benchmark corridor");
  }

  for (int index = 0; index < additionalRooms; ++index) {
    const int row = index / static_cast<int>(kRoomColumns.size());
    const int column = index % static_cast<int>(kRoomColumns.size());
    const int x = kRoomColumns[static_cast<std::size_t>(column)];
    const int y = row == 0 ? 9 : 13;
    requireCommand(
        simulation.buildFurnishedRoom(
            {"Benchmark " + std::to_string(index + 1),
             2,
             x,
             y,
             3,
             3,
             {2, x, y},
             1,
             1,
             140.0 + static_cast<double>(index % 4) * 5.0}),
        "build benchmark room");
  }
}

void addBenchmarkStaff(Simulation &simulation, int requestedStaffCount) {
  constexpr int kTutorialStaff = 3;
  if (requestedStaffCount < kTutorialStaff)
    throw std::runtime_error("tutorial fixture already contains three staff");

  constexpr std::array<PersonKind, 3> kRoles{
      PersonKind::Receptionist, PersonKind::Housekeeper,
      PersonKind::Maintenance};
  for (int index = kTutorialStaff; index < requestedStaffCount; ++index) {
    const auto role = kRoles[static_cast<std::size_t>(index) % kRoles.size()];
    requireCommand(simulation.hireStaff({"Benchmark Staff " +
                                             std::to_string(index + 1),
                                         role,
                                         0,
                                         0,
                                         20.0}),
                   "hire benchmark staff");
  }
}

[[nodiscard]] Simulation makeScenario(const ScenarioSpec &spec) {
  auto simulation = Simulation::tutorial(spec.seed);
  addBenchmarkRooms(simulation, spec.rooms);
  addBenchmarkStaff(simulation, spec.staff);
  requireCommand(
      simulation.loadDefinitions(
          R"({"baseDemand":100,"initialLinen":100000,"initialTowels":100000,"initialAmenities":100000,"initialChemicals":100000,"initialParts":100000,"roomConditionLossPerDay":0})"),
      "load benchmark definitions");
  return simulation;
}

[[nodiscard]] StateSummary summarize(const SimulationView &view,
                                     std::size_t saveBytes = 0) {
  StateSummary summary;
  summary.width = view.width;
  summary.height = view.height;
  summary.floors = view.floors;
  summary.tiles = view.tiles.size();
  summary.rooms = view.rooms.size();
  summary.reservations = view.reservations.size();
  summary.tasks = view.tasks.size();
  summary.reviews = view.reviews.size();
  summary.completedStays = view.economy.completedStays;
  summary.saveBytes = saveBytes;
  for (const auto &person : view.people) {
    if (person.kind == PersonKind::Guest)
      ++summary.guests;
    else
      ++summary.staff;
  }
  return summary;
}

[[nodiscard]] std::uint64_t fnv1a64(std::string_view bytes) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const unsigned char byte : bytes) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

void hashAssignment(std::uint64_t &hash,
                    const hh::game::Assignment &assignment) {
  for (const std::uint64_t value : {assignment.taskId, assignment.employeeId,
                                    static_cast<std::uint64_t>(
                                        assignment.startSecond),
                                    static_cast<std::uint64_t>(
                                        assignment.endSecond)}) {
    for (unsigned shift = 0; shift < 64; shift += 8) {
      hash ^= (value >> shift) & 0xffU;
      hash *= 1099511628211ULL;
    }
  }
}

[[nodiscard]] std::uint64_t checksumPlan(const hh::game::AssignmentPlan &plan) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto &assignment : plan.assignments)
    hashAssignment(hash, assignment);
  return hash;
}

[[nodiscard]] Sample runSample(const ScenarioSpec &spec) {
  const auto setupStart = Clock::now();
  auto simulation = makeScenario(spec);
  const auto setupEnd = Clock::now();
  const auto initial = summarize(simulation.view());

  const auto simulationStart = Clock::now();
  simulation.step(static_cast<double>(kScenarioSeconds));
  const auto simulationEnd = Clock::now();

  const auto saved = simulation.save();
  return {milliseconds(setupEnd - setupStart),
          milliseconds(simulationEnd - simulationStart),
          fnv1a64(saved),
          initial,
          summarize(simulation.view(), saved.size())};
}

[[nodiscard]] ScenarioResult runScenario(const ScenarioSpec &spec) {
  ScenarioResult result;
  result.spec = spec;
  try {
    for (int repetition = 0; repetition < kRepetitions; ++repetition) {
      const auto sample = runSample(spec);
      if (repetition == 0) {
        result.checksum = sample.checksum;
        result.initial = sample.initial;
        result.final = sample.final;
      } else if (sample.initial != result.initial ||
                 sample.final != result.final) {
        result.unavailableReason =
            "fixed-seed repetitions produced different state summaries";
      }
      result.setupMilliseconds.push_back(sample.setupMilliseconds);
      result.simulationMilliseconds.push_back(sample.simulationMilliseconds);
      result.deterministic = repetition == 0 ||
                             (result.deterministic &&
                              sample.checksum == result.checksum);
    }
    result.available = true;
    if (!result.unavailableReason.empty())
      result.deterministic = false;
  } catch (const std::exception &error) {
    result.available = false;
    result.deterministic = false;
    result.unavailableReason = error.what();
  }
  return result;
}

[[nodiscard]] hh::game::OptimizerSnapshot
makeOptimizerSnapshot(int employeeTaskScale) {
  hh::game::OptimizerSnapshot snapshot;
  snapshot.capturedSecond = 0;
  snapshot.horizonEndSecond = 7 * 86400;
  snapshot.employees.reserve(static_cast<std::size_t>(employeeTaskScale));
  snapshot.tasks.reserve(static_cast<std::size_t>(employeeTaskScale));
  for (int index = 0; index < employeeTaskScale; ++index) {
    hh::game::OptimizerEmployee employee;
    employee.id = static_cast<std::uint64_t>(index + 1);
    employee.role = hh::game::StaffRole::Housekeeper;
    employee.availableNow = true;
    employee.shiftWindows.push_back({0, snapshot.horizonEndSecond});
    if (index % 4 == 0)
      employee.unavailableWindows.push_back({index * 60, index * 60 + 30});
    snapshot.employees.push_back(std::move(employee));

    snapshot.tasks.push_back(
        {static_cast<std::uint64_t>(index + 1),
         hh::game::StaffRole::Housekeeper, index % 10 == 0, index * 60, 300});
  }
  return snapshot;
}

[[nodiscard]] OptimizerResult
runOptimizerScenario(int employeeTaskScale) {
  OptimizerResult result;
  result.employeeTaskScale = employeeTaskScale;
  hh::game::AssignmentPlan reference;
  for (int repetition = 0; repetition < kRepetitions; ++repetition) {
    const auto snapshot = makeOptimizerSnapshot(employeeTaskScale);
    const auto start = Clock::now();
    const auto plan = hh::game::buildDeterministicFallbackPlan(snapshot);
    const auto end = Clock::now();
    if (repetition == 0) {
      reference = plan;
      result.assignments = plan.assignments.size();
      result.planChecksum = checksumPlan(plan);
      result.deterministic = true;
    } else if (plan != reference) {
      result.deterministic = false;
    }
    result.timingMilliseconds.push_back(milliseconds(end - start));
  }
  return result;
}

[[nodiscard]] double median(std::vector<double> values) {
  if (values.empty())
    return 0.0;
  std::sort(values.begin(), values.end());
  const auto middle = values.size() / 2;
  if ((values.size() % 2U) != 0U)
    return values[middle];
  return (values[middle - 1] + values[middle]) / 2.0;
}

[[nodiscard]] double minimum(const std::vector<double> &values) {
  return values.empty() ? 0.0
                        : *std::min_element(values.begin(), values.end());
}

[[nodiscard]] double maximum(const std::vector<double> &values) {
  return values.empty() ? 0.0
                        : *std::max_element(values.begin(), values.end());
}

void writeJsonString(std::ostream &output, std::string_view value) {
  static constexpr char kHex[] = "0123456789abcdef";
  output << '"';
  for (const unsigned char character : value) {
    switch (character) {
    case '"':
      output << "\\\"";
      break;
    case '\\':
      output << "\\\\";
      break;
    case '\b':
      output << "\\b";
      break;
    case '\f':
      output << "\\f";
      break;
    case '\n':
      output << "\\n";
      break;
    case '\r':
      output << "\\r";
      break;
    case '\t':
      output << "\\t";
      break;
    default:
      if (character < 0x20U) {
        output << "\\u00" << kHex[(character >> 4U) & 0x0fU]
               << kHex[character & 0x0fU];
      } else {
        output << static_cast<char>(character);
      }
    }
  }
  output << '"';
}

void writeDoubleArray(std::ostream &output,
                      const std::vector<double> &values) {
  output << '[';
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0)
      output << ',';
    output << values[index];
  }
  output << ']';
}

void writeState(std::ostream &output, const StateSummary &state,
                std::string_view indent) {
  output << "{\n" << indent << "  \"map\": {\"width\": " << state.width
         << ", \"height\": " << state.height << ", \"floors\": "
         << state.floors << "},\n"
         << indent << "  \"tiles\": " << state.tiles << ",\n"
         << indent << "  \"rooms\": " << state.rooms << ",\n"
         << indent << "  \"staff\": " << state.staff << ",\n"
         << indent << "  \"guests\": " << state.guests << ",\n"
         << indent << "  \"reservations\": " << state.reservations << ",\n"
         << indent << "  \"tasks\": " << state.tasks << ",\n"
         << indent << "  \"reviews\": " << state.reviews << ",\n"
         << indent << "  \"completed_stays\": " << state.completedStays
         << ",\n"
         << indent << "  \"save_bytes\": " << state.saveBytes << '\n'
         << indent << '}';
}

[[nodiscard]] std::string checksumText(std::uint64_t checksum) {
  std::ostringstream text;
  text.imbue(std::locale::classic());
  text << "0x" << std::hex << std::setfill('0') << std::setw(16) << checksum;
  return text.str();
}

[[nodiscard]] std::string_view buildConfiguration() {
#ifdef HH_BENCHMARK_BUILD_CONFIGURATION
  return HH_BENCHMARK_BUILD_CONFIGURATION;
#elif defined(NDEBUG)
  return "Release-like";
#else
  return "Debug-like";
#endif
}

[[nodiscard]] std::string_view platformName() {
#if defined(_WIN32)
  return "Windows";
#elif defined(__APPLE__)
  return "macOS";
#elif defined(__linux__)
  return "Linux";
#else
  return "Unknown";
#endif
}

void writeScenario(std::ostream &output, const ScenarioResult &result,
                   bool trailingComma) {
  output << "    {\n      \"name\": ";
  writeJsonString(output, result.spec.name);
  output << ",\n      \"available\": "
         << (result.available ? "true" : "false") << ",\n"
         << "      \"seed\": " << result.spec.seed << ",\n"
         << "      \"requested\": {\"rooms\": " << result.spec.rooms
         << ", \"staff\": " << result.spec.staff
         << ", \"simulated_seconds\": " << kScenarioSeconds << "}";

  if (!result.available) {
    output << ",\n      \"unavailable_reason\": ";
    writeJsonString(output, result.unavailableReason);
    output << "\n    }";
    if (trailingComma)
      output << ',';
    output << '\n';
    return;
  }

  const double simulationMedian = median(result.simulationMilliseconds);
  const double safeSeconds = std::max(simulationMedian / 1000.0, 1.0e-9);
  output << ",\n      \"repetitions\": " << kRepetitions << ",\n"
         << "      \"deterministic\": "
         << (result.deterministic ? "true" : "false") << ",\n"
         << "      \"checksum\": ";
  writeJsonString(output, checksumText(result.checksum));
  output << ",\n      \"timing_ms\": {\n"
         << "        \"setup_samples\": ";
  writeDoubleArray(output, result.setupMilliseconds);
  output << ",\n        \"setup_median\": "
         << median(result.setupMilliseconds) << ",\n"
         << "        \"simulation_samples\": ";
  writeDoubleArray(output, result.simulationMilliseconds);
  output << ",\n        \"simulation_min\": "
         << minimum(result.simulationMilliseconds) << ",\n"
         << "        \"simulation_median\": " << simulationMedian << ",\n"
         << "        \"simulation_max\": "
         << maximum(result.simulationMilliseconds) << "\n      },\n"
         << "      \"throughput\": {\n"
         << "        \"simulated_seconds_per_wall_second\": "
         << static_cast<double>(kScenarioSeconds) / safeSeconds << ",\n"
         << "        \"authoritative_ticks_per_wall_second\": "
         << static_cast<double>(kScenarioSeconds) / safeSeconds << "\n"
         << "      },\n      \"initial_state\": ";
  writeState(output, result.initial, "      ");
  output << ",\n      \"final_state\": ";
  writeState(output, result.final, "      ");
  if (!result.unavailableReason.empty()) {
    output << ",\n      \"diagnostic\": ";
    writeJsonString(output, result.unavailableReason);
  }
  output << "\n    }";
  if (trailingComma)
    output << ',';
  output << '\n';
}

void writeOptimizerScenario(std::ostream &output,
                            const OptimizerResult &result,
                            bool trailingComma) {
  output << "    {\n      \"employee_task_scale\": "
         << result.employeeTaskScale << ",\n      \"repetitions\": "
         << kRepetitions << ",\n      \"deterministic\": "
         << (result.deterministic ? "true" : "false")
         << ",\n      \"assignments\": " << result.assignments
         << ",\n      \"plan_checksum\": ";
  writeJsonString(output, checksumText(result.planChecksum));
  output
         << ",\n      \"timing_ms\": {\n        \"samples\": ";
  writeDoubleArray(output, result.timingMilliseconds);
  output << ",\n        \"median\": "
         << median(result.timingMilliseconds) << "\n      }\n    }";
  if (trailingComma)
    output << ',';
  output << '\n';
}

void writeResults(std::ostream &output,
                  const std::vector<ScenarioResult> &results,
                  const std::vector<OptimizerResult> &optimizerResults) {
  output.imbue(std::locale::classic());
  output << std::fixed << std::setprecision(3);
  output << "{\n"
         << "  \"schema_version\": 1,\n"
         << "  \"benchmark\": \"Hotel Haven portable simulation and staff scheduler\",\n"
         << "  \"build\": {\n"
         << "    \"configuration\": ";
  writeJsonString(output, buildConfiguration());
  output << ",\n    \"compiler\": ";
  writeJsonString(output, __VERSION__);
  output << ",\n    \"cpp_standard\": " << __cplusplus
         << ",\n    \"platform\": ";
  writeJsonString(output, platformName());
  output << "\n  },\n"
         << "  \"methodology\": {\n"
         << "    \"clock\": \"std::chrono::steady_clock\",\n"
         << "    \"timed_regions\": {\"simulation\": \"Simulation::step only\", \"optimizer\": \"buildDeterministicFallbackPlan only\"},\n"
         << "    \"fixed_seeds\": true,\n"
         << "    \"checksum\": \"FNV-1a 64-bit over Simulation::save()\",\n"
         << "    \"scenario_names_are_current_api_tiers\": true\n"
         << "  },\n"
         << "  \"capability_gaps\": [\n"
         << "    {\"name\": \"500_rooms_1000_guests\", \"available\": false, "
            "\"reason\": \"The public API has no initial-cash or direct guest-population injection; the benchmark does not alter private or serialized state.\"},\n"
         << "    {\"name\": \"gpu_and_renderer_metrics\", \"available\": false, "
            "\"reason\": \"This portable target links only the authoritative headless simulation.\"},\n"
         << "    {\"name\": \"portable_process_memory_and_allocations\", \"available\": false, "
            "\"reason\": \"C++20 and the public simulation API expose no portable allocator or process-memory counters.\"}\n"
         << "  ],\n"
         << "  \"scenarios\": [\n";
  for (std::size_t index = 0; index < results.size(); ++index)
    writeScenario(output, results[index], index + 1 != results.size());
  output << "  ],\n  \"optimizer_scaling\": [\n";
  for (std::size_t index = 0; index < optimizerResults.size(); ++index)
    writeOptimizerScenario(output, optimizerResults[index],
                           index + 1 != optimizerResults.size());
  output << "  ]\n}\n";
}

} // namespace

int main() {
  constexpr std::array<ScenarioSpec, 4> kScenarios{{
      {"small", 2026091001ULL, 6, 3},
      {"normal", 2026091002ULL, 12, 12},
      {"large", 2026091003ULL, 18, 100},
      {"stress", 2026091004ULL, 18, 300},
  }};

  std::vector<ScenarioResult> results;
  results.reserve(kScenarios.size());
  constexpr std::array<int, 3> kOptimizerScales{64, 256, 512};
  std::vector<OptimizerResult> optimizerResults;
  optimizerResults.reserve(kOptimizerScales.size());
  bool verified = true;
  for (const auto &scenario : kScenarios) {
    auto result = runScenario(scenario);
    verified = verified && result.available && result.deterministic;
    results.push_back(std::move(result));
  }
  for (const int scale : kOptimizerScales) {
    auto result = runOptimizerScenario(scale);
    verified = verified && result.deterministic;
    optimizerResults.push_back(std::move(result));
  }

  writeResults(std::cout, results, optimizerResults);
  if (!verified) {
    std::cerr << "Simulation benchmark verification failed\n";
    return 1;
  }
  return 0;
}
