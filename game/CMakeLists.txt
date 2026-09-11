cmake_minimum_required(VERSION 3.20)
project(HotelHavenSimulation LANGUAGES CXX)
add_library(hh_game src/Simulation.cpp src/Workforce.cpp src/Departments.cpp src/StaffOptimization.cpp)
target_include_directories(hh_game PUBLIC include)
target_link_libraries(hh_game PRIVATE hh_assets)
target_compile_features(hh_game PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(hh_game PRIVATE /W4 /permissive-)
else()
  target_compile_options(hh_game PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(hh_simulation_benchmarks benchmarks/SimulationBenchmarks.cpp)
target_link_libraries(hh_simulation_benchmarks PRIVATE hh_game)
target_compile_features(hh_simulation_benchmarks PRIVATE cxx_std_20)
target_compile_definitions(hh_simulation_benchmarks PRIVATE
  HH_BENCHMARK_BUILD_CONFIGURATION="$<CONFIG>")
if(MSVC)
  target_compile_options(hh_simulation_benchmarks PRIVATE /W4 /permissive-)
else()
  target_compile_options(hh_simulation_benchmarks PRIVATE
    -Wall -Wextra -Wpedantic)
endif()

option(HH_GAME_BUILD_TESTS "Build simulation tests" ON)
if(HH_GAME_BUILD_TESTS)
  enable_testing()
  add_executable(hh_game_tests tests/SimulationTests.cpp)
  target_link_libraries(hh_game_tests PRIVATE hh_game)
  add_test(NAME hh_game_tests COMMAND hh_game_tests)

  add_executable(hh_workforce_tests tests/WorkforceTests.cpp)
  target_link_libraries(hh_workforce_tests PRIVATE hh_game)
  add_test(NAME Workforce COMMAND hh_workforce_tests)

  add_executable(hh_department_tests tests/DepartmentTests.cpp)
  target_link_libraries(hh_department_tests PRIVATE hh_game)
  add_test(NAME Department COMMAND hh_department_tests)

  add_executable(hh_optimizer_tests tests/OptimizerIntegrationTests.cpp)
  target_link_libraries(hh_optimizer_tests PRIVATE hh_game)
  add_test(NAME Optimizer COMMAND hh_optimizer_tests)

  add_test(NAME SimulationBenchmarks COMMAND hh_simulation_benchmarks)
  set_tests_properties(SimulationBenchmarks PROPERTIES
    LABELS "benchmark;simulation"
    TIMEOUT 120)
endif()
