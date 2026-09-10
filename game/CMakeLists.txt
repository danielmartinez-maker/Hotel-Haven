cmake_minimum_required(VERSION 3.20)
project(HotelHavenSimulation LANGUAGES CXX)
add_library(hh_game
  src/Simulation.cpp
  src/Housekeeping.cpp
  src/Laundry.cpp
  src/Logistics.cpp
  src/Engineering.cpp
  src/RoomService.cpp
  src/ServiceLogistics.cpp
  src/Workforce.cpp
  src/Departments.cpp
  src/StaffOptimization.cpp
)
target_include_directories(hh_game PUBLIC include)
target_link_libraries(hh_game PRIVATE hh_assets)
target_compile_features(hh_game PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(hh_game PRIVATE /W4 /permissive-)
else()
  target_compile_options(hh_game PRIVATE -Wall -Wextra -Wpedantic)
  add_executable(hh_workforce_tests tests/WorkforceTests.cpp)
  target_link_libraries(hh_workforce_tests PRIVATE hh_game)
  add_test(NAME Workforce COMMAND hh_workforce_tests)

  add_executable(hh_department_tests tests/DepartmentTests.cpp)
  target_link_libraries(hh_department_tests PRIVATE hh_game)
  add_test(NAME Department COMMAND hh_department_tests)

  add_executable(hh_optimizer_tests tests/OptimizerIntegrationTests.cpp)
  target_link_libraries(hh_optimizer_tests PRIVATE hh_game)
  add_test(NAME Optimizer COMMAND hh_optimizer_tests)
endif()
option(HH_GAME_BUILD_TESTS "Build simulation tests" ON)
if(HH_GAME_BUILD_TESTS)
  enable_testing()
  add_executable(hh_game_tests tests/SimulationTests.cpp)
  target_link_libraries(hh_game_tests PRIVATE hh_game)
  add_test(NAME hh_game_tests COMMAND hh_game_tests)

  foreach(service_test Housekeeping Laundry Logistics Engineering ServiceLogistics ServiceLogisticsSoak SimulationServiceLogistics)
    add_executable(hh_${service_test}_tests tests/${service_test}Tests.cpp)
    target_link_libraries(hh_${service_test}_tests PRIVATE hh_game)
    add_test(NAME ${service_test} COMMAND hh_${service_test}_tests)
  endforeach()

  add_executable(hh_stress_harness_tests tests/stress/StressHarnessTests.cpp)
  target_include_directories(hh_stress_harness_tests PRIVATE tests/stress)
  add_test(NAME StressHarness COMMAND hh_stress_harness_tests)
endif()
