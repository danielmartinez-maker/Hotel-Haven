cmake_minimum_required(VERSION 3.25)
project(HotelHaven VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
enable_testing()
find_package(Python3 COMPONENTS Interpreter QUIET)
add_subdirectory(Tools/ContentPipeline)
add_subdirectory(optimization)
add_subdirectory(game)

add_library(hh_soundtrack_catalog STATIC game/audio/SoundtrackCatalog.cpp)
target_include_directories(hh_soundtrack_catalog PUBLIC game/audio)
target_compile_features(hh_soundtrack_catalog PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(hh_soundtrack_catalog PRIVATE /W4 /permissive-)
else()
    target_compile_options(hh_soundtrack_catalog PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(hh_soundtrack_catalog_tests game/audio/SoundtrackCatalogTests.cpp)
target_link_libraries(hh_soundtrack_catalog_tests PRIVATE hh_soundtrack_catalog)
add_test(NAME hh_soundtrack_catalog_tests COMMAND hh_soundtrack_catalog_tests)
add_executable(hh_soundtrack_stress_tests game/audio/SoundtrackStressTests.cpp)
target_link_libraries(hh_soundtrack_stress_tests PRIVATE hh_soundtrack_catalog)
add_test(NAME StressSoundtrackCatalog COMMAND hh_soundtrack_stress_tests)
set(HH_SOUNDTRACK_ASSET_NAMES
    Morning_in_the_Atrium.mp3
    Sunlight_on_Marble.mp3
    The_Concierge_Desk.mp3
    First_Light_on_Marble.mp3)
set(HH_SOUNDTRACK_ASSETS_PRESENT TRUE)
foreach(HH_SOUNDTRACK_ASSET IN LISTS HH_SOUNDTRACK_ASSET_NAMES)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/game/data/audio/${HH_SOUNDTRACK_ASSET}")
        set(HH_SOUNDTRACK_ASSETS_PRESENT FALSE)
    endif()
endforeach()
if(HH_SOUNDTRACK_ASSETS_PRESENT)
    add_test(
        NAME hh_soundtrack_assets
        COMMAND ${CMAKE_COMMAND}
                -DHH_SOUNDTRACK_DIR=${CMAKE_CURRENT_SOURCE_DIR}/game/data/audio
                -P ${CMAKE_CURRENT_SOURCE_DIR}/game/audio/VerifySoundtrackAssets.cmake)
else()
    message(WARNING "Hotel Haven soundtrack MP3 files are absent; playback will remain disabled until they are placed in game/data/audio")
endif()

add_executable(hotel_haven_headless game/app/HeadlessMain.cpp)
target_link_libraries(hotel_haven_headless PRIVATE hh_game)
add_test(NAME hh_campaign COMMAND hotel_haven_headless --days 14 --restock)
add_test(
    NAME hh_balance_summary_cli
    COMMAND hotel_haven_headless --days 14 --seed 20260909
            --run-id ci-balance
            --balance-summary ${CMAKE_BINARY_DIR}/balance-runs/ci-balance.json)
add_test(
    NAME hh_balance_output_alias_cli
    COMMAND hotel_haven_headless --days 0 --seed 20260909
            --run-id ci-balance-alias
            --balance-output ${CMAKE_BINARY_DIR}/balance-runs/ci-balance-alias.json)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME hh_balance_summary_semantics
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/Tools/BalanceLab/validate_headless_summary.py
                ${CMAKE_BINARY_DIR}/balance-runs/ci-balance.json
                --expected-run-id ci-balance
                --min-guest-satisfaction 20)
    set_tests_properties(hh_balance_summary_semantics PROPERTIES DEPENDS hh_balance_summary_cli)
    add_test(
        NAME hh_balance_summary_frontier
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/Tools/BalanceLab/balance_lab.py
                frontier
                --input-dir ${CMAKE_BINARY_DIR}/balance-runs
                --output-dir ${CMAKE_BINARY_DIR}/balance-output)
    set_tests_properties(hh_balance_summary_frontier PROPERTIES DEPENDS hh_balance_summary_semantics)
endif()
install(TARGETS hotel_haven_headless RUNTIME DESTINATION .)
add_library(hh_world_view STATIC game/app/WorldView.cpp)
target_include_directories(hh_world_view PUBLIC game/app renderer/include)
target_link_libraries(hh_world_view PUBLIC hh_game)
add_executable(hh_world_view_tests game/app/WorldViewTests.cpp)
target_link_libraries(hh_world_view_tests PRIVATE hh_world_view)
add_test(NAME hh_world_view_tests COMMAND hh_world_view_tests)
if(WIN32)
    add_subdirectory(renderer)
    add_library(hh_soundtrack_player STATIC game/audio/SoundtrackPlayer.cpp)
    target_include_directories(hh_soundtrack_player PUBLIC game/audio)
    target_link_libraries(hh_soundtrack_player PUBLIC hh_soundtrack_catalog winmm)
    target_compile_features(hh_soundtrack_player PUBLIC cxx_std_20)
    if(MSVC)
        target_compile_options(hh_soundtrack_player PRIVATE /W4 /permissive- /utf-8)
    endif()
    add_executable(hotel_haven WIN32 game/app/GameMain.cpp game/app/Panels.cpp)
    target_compile_definitions(hotel_haven PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    target_link_libraries(hotel_haven PRIVATE hh_game hh_world_view hh_renderer_core hh_renderer_d3d11 hh_soundtrack_player user32 gdi32 shell32)
    target_include_directories(hotel_haven PRIVATE game/app game/audio)
    if(MSVC)
        target_compile_options(hotel_haven PRIVATE /W4 /permissive- /utf-8)
    endif()
    add_custom_command(TARGET hotel_haven POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:hotel_haven>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/InstancedBox.hlsl" "$<TARGET_FILE_DIR:hotel_haven>/shaders/InstancedBox.hlsl"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/game/data" "$<TARGET_FILE_DIR:hotel_haven>/data")
    install(TARGETS hotel_haven RUNTIME DESTINATION .)
    install(FILES renderer/shaders/InstancedBox.hlsl DESTINATION shaders)
endif()
install(DIRECTORY game/data/ DESTINATION data)
install(FILES README.md docs/IMPLEMENTATION_STATUS.md DESTINATION .)
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME "Hotel-Haven-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
include(CPack)
