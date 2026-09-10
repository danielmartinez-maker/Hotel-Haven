cmake_minimum_required(VERSION 3.25)
project(HotelHaven VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
enable_testing()
add_subdirectory(Tools/ContentPipeline)
add_subdirectory(game)
add_executable(hotel_haven_headless game/app/HeadlessMain.cpp)
target_link_libraries(hotel_haven_headless PRIVATE hh_game)
add_test(NAME hh_campaign COMMAND hotel_haven_headless --days 14 --restock)
install(TARGETS hotel_haven_headless RUNTIME DESTINATION .)
add_library(hh_world_view STATIC game/app/WorldView.cpp game/app/WorldAssetBindings.cpp)
target_include_directories(hh_world_view PUBLIC game/app renderer/include)
target_link_libraries(hh_world_view PUBLIC hh_game)
add_executable(hh_world_view_tests game/app/WorldViewTests.cpp)
target_link_libraries(hh_world_view_tests PRIVATE hh_world_view)
add_test(NAME hh_world_view_tests COMMAND hh_world_view_tests)
if(WIN32)
    add_subdirectory(renderer)
    add_subdirectory(frontend)

    add_library(hh_runtime_world_assets STATIC game/app/RuntimeWorldAssets.cpp)
    target_include_directories(hh_runtime_world_assets PUBLIC game/app renderer/include)
    target_link_libraries(hh_runtime_world_assets PUBLIC hh_world_view hh_renderer_core)
    add_executable(hh_runtime_world_assets_tests game/app/RuntimeWorldAssetsTests.cpp)
    target_include_directories(hh_runtime_world_assets_tests PRIVATE game/app renderer/include)
    target_link_libraries(hh_runtime_world_assets_tests PRIVATE hh_runtime_world_assets hh_asset_format)
    add_test(NAME hh_runtime_world_assets_tests COMMAND hh_runtime_world_assets_tests)

    add_library(hh_frame_pipeline STATIC game/app/FramePipeline.cpp)
    target_include_directories(hh_frame_pipeline PUBLIC game/app renderer/include)
    target_link_libraries(hh_frame_pipeline PUBLIC hh_renderer_core)
    add_executable(hh_frame_pipeline_tests game/app/FramePipelineTests.cpp)
    target_include_directories(hh_frame_pipeline_tests PRIVATE game/app renderer/include)
    target_link_libraries(hh_frame_pipeline_tests PRIVATE hh_frame_pipeline)
    add_test(NAME hh_frame_pipeline_tests COMMAND hh_frame_pipeline_tests)

    add_library(hh_game_ui_bridge STATIC game/app/GameUiBridge.cpp)
    target_include_directories(hh_game_ui_bridge PUBLIC game/app)
    target_link_libraries(hh_game_ui_bridge PUBLIC hh_game hh_frontend_core)
    add_executable(hh_game_ui_bridge_tests game/app/GameUiBridgeTests.cpp)
    target_link_libraries(hh_game_ui_bridge_tests PRIVATE hh_game_ui_bridge)
    add_test(NAME hh_game_ui_bridge_tests COMMAND hh_game_ui_bridge_tests)

    add_executable(hotel_haven WIN32 game/app/GameMain.cpp game/app/Panels.cpp)
    target_compile_definitions(hotel_haven PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    target_link_libraries(hotel_haven PRIVATE
        hh_game
        hh_world_view
        hh_runtime_world_assets
        hh_frame_pipeline
        hh_game_ui_bridge
        hh_frontend_core
        hh_renderer_core
        hh_renderer_d3d11
        user32
        gdi32
        shell32)
    target_include_directories(hotel_haven PRIVATE game/app)
    if(MSVC)
        target_compile_options(hotel_haven PRIVATE /W4 /permissive- /utf-8)
    endif()
    add_custom_command(TARGET hotel_haven POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:hotel_haven>/shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/InstancedBox.hlsl" "$<TARGET_FILE_DIR:hotel_haven>/shaders/InstancedBox.hlsl"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/renderer/shaders/Mesh.hlsl" "$<TARGET_FILE_DIR:hotel_haven>/shaders/Mesh.hlsl"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/game/data" "$<TARGET_FILE_DIR:hotel_haven>/data")

    add_custom_target(hotel_haven_stage_runtime_assets
        COMMAND ${CMAKE_COMMAND}
            "-DHH_COOKED_ASSET_ROOT=${CMAKE_CURRENT_SOURCE_DIR}/Build/CookedAssets"
            "-DHH_RUNTIME_ASSET_ROOT=$<TARGET_FILE_DIR:hotel_haven>/data/assets"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageRuntimeAssets.cmake"
        DEPENDS hotel_haven
        COMMENT "Staging 500 renderer-safe cooked Hotel Haven assets")

    install(TARGETS hotel_haven RUNTIME DESTINATION .)
    install(FILES renderer/shaders/InstancedBox.hlsl renderer/shaders/Mesh.hlsl DESTINATION shaders)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Build/CookedAssets/"
        DESTINATION data/assets
        FILES_MATCHING PATTERN "*.hasset")
endif()
install(DIRECTORY game/data/ DESTINATION data)
install(FILES README.md docs/IMPLEMENTATION_STATUS.md DESTINATION .)
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME "Hotel-Haven-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
include(CPack)
