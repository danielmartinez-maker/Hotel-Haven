cmake_minimum_required(VERSION 3.20)
project(HotelHavenSimulation LANGUAGES CXX)
add_library(hh_game
  src/Simulation.cpp
  src/StaffOptimization.cpp
  src/Construction.cpp
  src/BuildJobs.cpp
  src/BuildingSystems.cpp
  src/GuestPsychology.cpp)
target_include_directories(hh_game PUBLIC include)
target_link_libraries(hh_game PRIVATE hh_assets hh_optimization)
target_compile_features(hh_game PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(hh_game PRIVATE /W4 /permissive-)
else()
  target_compile_options(hh_game PRIVATE -Wall -Wextra -Wpedantic)
endif()
option(HH_GAME_BUILD_TESTS "Build simulation tests" ON)
if(HH_GAME_BUILD_TESTS)
  enable_testing()
  add_executable(hh_game_tests tests/SimulationTestsV11.cpp)
  target_link_libraries(hh_game_tests PRIVATE hh_game)
  add_test(NAME hh_game_tests COMMAND hh_game_tests)

  add_executable(hh_construction_tests tests/ConstructionTests.cpp)
  target_link_libraries(hh_construction_tests PRIVATE hh_game)
  add_test(NAME hh_construction_tests COMMAND hh_construction_tests)

  add_executable(hh_build_job_tests tests/BuildJobTests.cpp)
  target_link_libraries(hh_build_job_tests PRIVATE hh_game)
  add_test(NAME hh_build_job_tests COMMAND hh_build_job_tests)

  add_executable(hh_building_systems_tests tests/BuildingSystemsTests.cpp)
  target_link_libraries(hh_building_systems_tests PRIVATE hh_game)
  add_test(NAME hh_building_systems_tests COMMAND hh_building_systems_tests)

  add_executable(hh_construction_save_tests tests/ConstructionSaveTests.cpp)
  target_link_libraries(hh_construction_save_tests PRIVATE hh_game)
  add_test(NAME hh_construction_save_tests COMMAND hh_construction_save_tests)

  add_executable(hh_guest_psychology_tests tests/GuestPsychologyTests.cpp)
  target_link_libraries(hh_guest_psychology_tests PRIVATE hh_game)
  add_test(NAME hh_guest_psychology_tests COMMAND hh_guest_psychology_tests)

  add_executable(hh_optimizer_integration_tests tests/OptimizerIntegrationTests.cpp)
  target_include_directories(hh_optimizer_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(hh_optimizer_integration_tests PRIVATE hh_game hh_optimization)
  add_test(NAME hh_optimizer_integration_tests COMMAND hh_optimizer_integration_tests)
endif()
