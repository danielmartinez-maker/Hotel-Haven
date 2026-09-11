cmake_minimum_required(VERSION 3.20)
project(HotelHavenSimulation LANGUAGES CXX)
add_library(hh_game
  src/Simulation.cpp
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
  src/MarketDemandDefinitions.cpp
  src/MarketDemandPersistence.cpp
  src/RevenueInventory.cpp
  src/RevenueManagement.cpp
  src/HotelEconomics.cpp
  src/Financing.cpp
  src/Overbooking.cpp
  src/CommercialDemand.cpp
  src/CommercialDemandProfile.cpp
  src/EconomyRuntime.cpp
  src/EconomyRuntimeDefinitions.cpp
  src/EconomyRuntimeBridge.cpp
  src/EconomyDiagnostics.cpp
)
# FINAL-06 can add definition loaders independently of FINAL-07. Keep this
# branch buildable before that lineage is present while ensuring PR merge builds
# compile and link the authoritative implementations when the files exist.
foreach(optional_final06_source MarketDemandDefinitions EconomyRuntimeDefinitions)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${optional_final06_source}.cpp")
    target_sources(hh_game PRIVATE src/${optional_final06_source}.cpp)
  endif()
endforeach()
target_include_directories(hh_game PUBLIC include)
target_link_libraries(hh_game PRIVATE hh_assets)
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

  foreach(service_test Housekeeping Laundry Logistics Engineering ServiceLogistics SimulationServiceLogistics FoodService Event Amenity)
    add_executable(hh_${service_test}_tests tests/${service_test}Tests.cpp)
    target_link_libraries(hh_${service_test}_tests PRIVATE hh_game)
    add_test(NAME ${service_test} COMMAND hh_${service_test}_tests)
  endforeach()

  add_executable(hh_FoodRoomServiceIntegration_tests tests/FoodRoomServiceIntegrationTests.cpp)
  target_link_libraries(hh_FoodRoomServiceIntegration_tests PRIVATE hh_game)
  add_test(NAME FoodRoomServiceIntegration COMMAND hh_FoodRoomServiceIntegration_tests)

  add_executable(hh_SimulationFinal05_tests tests/SimulationFinal05Tests.cpp)
  target_link_libraries(hh_SimulationFinal05_tests PRIVATE hh_game)
  add_test(NAME SimulationFinal05 COMMAND hh_SimulationFinal05_tests)

  foreach(economy_test MarketDemand MarketPotentialDemand MarketReputationCategory MarketCompetitorCategoryAllocation MarketCompetitorCalendar MarketDefinitions RevenueInventory ReservationPolicySpec RevenueInventoryBlocks RevenueInventoryIndex RevenueManagement HotelEconomics Financing FinancingCovenant Overbooking OverbookingPercentage CommercialDemand CommercialReputationCategory MarketingAttributionIntegration MarketingDailyCostIntegration ContractBookingIntegration EconomyInventoryBlockIntegration EconomyInitialReputationProfile EconomyIntegration EconomyPotentialDemandIntegration EconomySoak EconomySpecReconciliation RevenueCommercialSpec SimulationEconomyBridge SimulationEconomyCapacity EconomyDiagnostics EconomyDepartmentDebtDiagnostics)
    add_executable(hh_${economy_test}_tests tests/${economy_test}Tests.cpp)
    target_link_libraries(hh_${economy_test}_tests PRIVATE hh_game)
    add_test(NAME ${economy_test} COMMAND hh_${economy_test}_tests)
  endforeach()
endif()
