#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace hh::stress {

enum class Scale { Pr, Extended, Exhaustive };
struct Config { Scale scale{Scale::Pr}; std::uint64_t seed{}; std::string scenario; };
inline std::uint64_t parseSeed(std::string_view text) {
  if (text.empty() || text.front() == '-') throw std::invalid_argument("HH_STRESS_SEED must be an unsigned integer");
  std::size_t consumed = 0;
  const auto value = std::stoull(std::string(text), &consumed, 0);
  if (consumed != text.size()) throw std::invalid_argument("HH_STRESS_SEED contains trailing characters");
  return static_cast<std::uint64_t>(value);
}
inline Config configFromEnvironment(std::uint64_t fallbackSeed) {
  Config config; config.seed = fallbackSeed;
  if (const char *scale = std::getenv("HH_STRESS_SCALE"); scale && *scale) {
    const std::string_view value{scale};
    if (value == "pr") config.scale = Scale::Pr;
    else if (value == "extended") config.scale = Scale::Extended;
    else if (value == "exhaustive") config.scale = Scale::Exhaustive;
    else throw std::invalid_argument("HH_STRESS_SCALE must be pr, extended, or exhaustive");
  }
  if (const char *seed = std::getenv("HH_STRESS_SEED"); seed && *seed) config.seed = parseSeed(seed);
  if (const char *scenario = std::getenv("HH_STRESS_SCENARIO"); scenario && *scenario) config.scenario = scenario;
  return config;
}
class Rng {
public:
  explicit Rng(std::uint64_t seed) : state_(seed) {}
  std::uint64_t next() { state_ += 0x9E3779B97F4A7C15ULL; auto z = state_; z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL; z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL; return z ^ (z >> 31U); }
  std::uint64_t bounded(std::uint64_t upper) { if (upper == 0) throw std::invalid_argument("bounded upper bound must be non-zero"); const auto threshold = static_cast<std::uint64_t>(-upper) % upper; for (;;) { const auto value = next(); if (value >= threshold) return value % upper; } }
  bool chance(std::uint32_t numerator, std::uint32_t denominator) { if (denominator == 0 || numerator > denominator) throw std::invalid_argument("invalid chance fraction"); return bounded(denominator) < numerator; }
private: std::uint64_t state_{};
};
class Trace {
public:
  explicit Trace(std::size_t capacity = 64) : capacity_(capacity) {}
  void push(std::string action) { if (capacity_ == 0) return; if (entries_.size() == capacity_) entries_.pop_front(); entries_.push_back(std::move(action)); }
  std::size_t size() const { return entries_.size(); }
  std::string dump() const { std::ostringstream out; bool first = true; for (const auto &entry : entries_) { if (!first) out << " | "; first = false; out << entry; } return out.str(); }
private: std::size_t capacity_{}; std::deque<std::string> entries_;
};
struct RunContext {
  std::string suite; Config config; Trace trace{64}; std::string phase;
  [[noreturn]] void fail(std::string_view invariant, std::uint64_t tickOrDay, std::string_view stateHash = {}) const {
    std::ostringstream out; out << "stress failure suite=" << suite << " seed=0x" << std::hex << config.seed << std::dec << " scenario=" << (config.scenario.empty() ? "all" : config.scenario) << " phase=" << (phase.empty() ? "unspecified" : phase) << " tick_or_day=" << tickOrDay << " invariant=" << invariant; if (!stateHash.empty()) out << " hash=" << stateHash; const auto recent = trace.dump(); if (!recent.empty()) out << " trace=[" << recent << ']'; std::cerr << out.str() << '\n'; throw std::runtime_error(out.str());
  }
};
} // namespace hh::stress
