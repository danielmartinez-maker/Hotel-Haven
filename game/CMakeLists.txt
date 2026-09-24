cmake_minimum_required(VERSION 3.20)
project(HotelHavenSimulation LANGUAGES CXX)

add_library(hh_game
  # Authoritative physical simulation + FINAL-04/05/06 integration.
  src/Simulation.cpp
  src/SimulationIntegrated.cpp
  src/SimulationEconomyBridge.cpp
  src/Housekeeping.cpp
  src/Laundry.cpp
  src/Logistics.cpp
  src/Engineering.cpp
  src/RoomService.cpp
  src/ServiceLogistics.cpp
  src/FoodService.cpp
  src/Events.cpp
  src/Amenities.cpp
  src/MarketDemand.cpp
  src/MarketDemandPersistence.cpp
  src/RevenueInventory.cpp
  src/RevenueManagement.cpp
  src/HotelEconomics.cpp
  src/Financing.cpp
  src/Overbooking.cpp
  src/CommercialDemand.cpp
  src/CommercialDemandProfile.cpp
  src/EconomyRuntime.cpp
  src/EconomyRuntimeBridge.cpp
  src/EconomyDiagnostics.cpp

  # FINAL-01 construction/building systems.
  src/Construction.cpp
  src/BuildJobs.cpp
  src/BuildingSystems.cpp
  src/ElevatorDispatch.cpp

  # FINAL-02 guest AI/psychology domain systems.
  src/GuestPsychology.cpp
  src/GuestPsychologyRestore.cpp
  src/GuestPsychologyArchive.cpp
  src/GuestGoals.cpp
  src/GuestExpectations.cpp
  src/GuestLoyalty.cpp
  src/GuestReviews.cpp
  src/SimulationPsychology.cpp

  # FINAL-03 workforce/organization/optimizer.
  src/Workforce.cpp
  src/Departments.cpp
  src/StaffOptimization.cpp
)

# FINAL-06 definition loaders are branch-independent and optional in older saves.
foreach(optional_final06_source MarketDemandDefinitions EconomyRuntimeDefinitions)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${optional_final06_source}.cpp")
    target_sources(hh_game PRIVATE src/${optional_final06_source}.cpp)
  endif()
endforeach()

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

  add_executable(hh_game_tests tests/SimulationTests.cpp)
  target_link_libraries(hh_game_tests PRIVATE hh_game)
  add_test(NAME hh_game_tests COMMAND hh_game_tests)

  # FINAL-01.
  foreach(final01_test Construction BuildJob BuildingSystems ConstructionSave ElevatorDispatchSoak)
    add_executable(hh_${final01_test}_tests tests/${final01_test}Tests.cpp)
    target_link_libraries(hh_${final01_test}_tests PRIVATE hh_game)
    add_test(NAME ${final01_test} COMMAND hh_${final01_test}_tests)
  endforeach()

  # FINAL-02.
  foreach(final02_test GuestPsychology GuestGoal GuestReview GuestDeterminism GuestSatisfaction GuestExpectation)
    add_executable(hh_${final02_test}_tests tests/${final02_test}Tests.cpp)
    target_link_libraries(hh_${final02_test}_tests PRIVATE hh_game)
    add_test(NAME ${final02_test} COMMAND hh_${final02_test}_tests)
  endforeach()

  # FINAL-03.
  add_executable(hh_workforce_tests tests/WorkforceTests.cpp)
  target_link_libraries(hh_workforce_tests PRIVATE hh_game)
  add_test(NAME Workforce COMMAND hh_workforce_tests)

  add_executable(hh_department_tests tests/DepartmentTests.cpp)
  target_link_libraries(hh_department_tests PRIVATE hh_game)
  add_test(NAME Department COMMAND hh_department_tests)

  add_executable(hh_optimizer_tests tests/OptimizerIntegrationTests.cpp)
  target_include_directories(hh_optimizer_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(hh_optimizer_tests PRIVATE hh_game hh_optimization)
  add_test(NAME Optimizer COMMAND hh_optimizer_tests)

  # FINAL-04/05.
  foreach(service_test Housekeeping Laundry Logistics Engineering ServiceLogistics SimulationServiceLogistics FoodService Event Amenity)
    add_executable(hh_${service_test}_tests tests/${service_test}Tests.cpp)
    target_link_libraries(hh_${service_test}_tests PRIVATE hh_game)
    add_test(NAME ${service_test} COMMAND hh_${service_test}_tests)
  endforeach()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/ServiceLogisticsSoakTests.cpp")
    add_executable(hh_ServiceLogisticsSoak_tests tests/ServiceLogisticsSoakTests.cpp)
    target_link_libraries(hh_ServiceLogisticsSoak_tests PRIVATE hh_game)
    add_test(NAME ServiceLogisticsSoak COMMAND hh_ServiceLogisticsSoak_tests)
  endif()

  add_executable(hh_FoodRoomServiceIntegration_tests tests/FoodRoomServiceIntegrationTests.cpp)
  target_link_libraries(hh_FoodRoomServiceIntegration_tests PRIVATE hh_game)
  add_test(NAME FoodRoomServiceIntegration COMMAND hh_FoodRoomServiceIntegration_tests)

  add_executable(hh_SimulationFinal05_tests tests/SimulationFinal05Tests.cpp)
  target_link_libraries(hh_SimulationFinal05_tests PRIVATE hh_game)
  add_test(NAME SimulationFinal05 COMMAND hh_SimulationFinal05_tests)

  # FINAL-06 economy/market.
  foreach(economy_test MarketDemand MarketPotentialDemand MarketReputationCategory RevenueInventory ReservationPolicySpec RevenueInventoryBlocks RevenueInventoryIndex RevenueManagement HotelEconomics Financing FinancingCovenant Overbooking CommercialDemand CommercialReputationCategory MarketingAttributionIntegration ContractBookingIntegration EconomyInventoryBlockIntegration EconomyInitialReputationProfile EconomyIntegration EconomyPotentialDemandIntegration EconomySoak EconomySpecReconciliation RevenueCommercialSpec SimulationEconomyBridge SimulationEconomyCapacity EconomyDiagnostics EconomyDepartmentDebtDiagnostics)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${economy_test}Tests.cpp")
      add_executable(hh_${economy_test}_tests tests/${economy_test}Tests.cpp)
      target_link_libraries(hh_${economy_test}_tests PRIVATE hh_game)
      add_test(NAME ${economy_test} COMMAND hh_${economy_test}_tests)
    endif()
  endforeach()

  if(TARGET hh_simulation_benchmarks)
    add_test(NAME SimulationBenchmarks COMMAND hh_simulation_benchmarks)
    set_tests_properties(SimulationBenchmarks PROPERTIES
      LABELS "benchmark;simulation"
      TIMEOUT 120)
  endif()
endif()
