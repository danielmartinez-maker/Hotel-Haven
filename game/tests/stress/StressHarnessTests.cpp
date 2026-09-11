#include "StressHarness.h"

#include <cassert>
#include <cstdlib>
#include <string>

int main() {
  hh::stress::Rng a{0x123456789abcdef0ULL};
  hh::stress::Rng b{0x123456789abcdef0ULL};
  for (int i = 0; i < 1000; ++i) {
    const auto av = a.bounded(37);
    const auto bv = b.bounded(37);
    assert(av == bv);
    assert(av < 37);
  }
  hh::stress::Trace trace{64};
  for (int i = 0; i < 100; ++i)
    trace.push("op=" + std::to_string(i));
  assert(trace.size() == 64);
  const auto dumped = trace.dump();
  assert(dumped.find("op=36") != std::string::npos);
  assert(dumped.find("op=99") != std::string::npos);
  assert(dumped.find("op=35") == std::string::npos);

#if defined(_WIN32)
  _putenv_s("HH_STRESS_SCALE", "");
  _putenv_s("HH_STRESS_SEED", "");
  _putenv_s("HH_STRESS_SCENARIO", "");
#else
  unsetenv("HH_STRESS_SCALE");
  unsetenv("HH_STRESS_SEED");
  unsetenv("HH_STRESS_SCENARIO");
#endif
  const auto defaults = hh::stress::configFromEnvironment(1234);
  assert(defaults.scale == hh::stress::Scale::Pr);
  assert(defaults.seed == 1234);
  assert(defaults.scenario.empty());

#if defined(_WIN32)
  _putenv_s("HH_STRESS_SEED", "0x2a");
#else
  setenv("HH_STRESS_SEED", "0x2a", 1);
#endif
  assert(hh::stress::configFromEnvironment(1).seed == 42);
  return 0;
}
