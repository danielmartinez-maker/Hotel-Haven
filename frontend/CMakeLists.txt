cmake_minimum_required(VERSION 3.25)
project(HotelHavenFrontend LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Hotel Haven frontend targets Windows 11 x64")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
include(${CMAKE_CURRENT_SOURCE_DIR}/../renderer/cmake/CompilerWarnings.cmake)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../renderer ${CMAKE_CURRENT_BINARY_DIR}/renderer)

add_library(hh_frontend_core STATIC
    src/MainMenuModel.cpp
    src/MainMenuController.cpp
    src/MainMenuView.cpp
    src/MenuTransitionDirector.cpp
    src/MenuSceneController.cpp
)
target_include_directories(hh_frontend_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
hh_enable_strict_warnings(hh_frontend_core)

add_library(hh_frontend_d2d STATIC
    src/d2d/D2DUiRenderer.cpp
    src/d2d/FontManager.cpp
    src/d2d/UiPrimitiveRenderer.cpp
)
target_include_directories(hh_frontend_d2d PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(hh_frontend_d2d PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(hh_frontend_d2d PUBLIC
    hh_frontend_core
    d2d1
    dwrite
    d3d11
    dxgi
)
hh_enable_strict_warnings(hh_frontend_d2d)

add_executable(hotel_haven_menu_demo WIN32
    app/MainMenuDemo.cpp
    src/ShowcaseHotelScene.cpp
)
target_include_directories(hotel_haven_menu_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(hotel_haven_menu_demo PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)
target_link_libraries(hotel_haven_menu_demo PRIVATE
    hh_frontend_core
    hh_frontend_d2d
    hh_renderer_core
    hh_renderer_d3d11
    hh_renderer_win32
)
hh_enable_strict_warnings(hotel_haven_menu_demo)

add_custom_command(TARGET hotel_haven_menu_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:hotel_haven_menu_demo>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../renderer/shaders/InstancedBox.hlsl"
        "$<TARGET_FILE_DIR:hotel_haven_menu_demo>/shaders/InstancedBox.hlsl"
)

add_executable(hh_frontend_tests
    tests/TestMain.cpp
    tests/MainMenuModelTests.cpp
    tests/MainMenuNavigationTests.cpp
    tests/MenuHotelProviderTests.cpp
    tests/MenuTransitionTests.cpp
    tests/MenuSceneControllerTests.cpp
    tests/MainMenuStressTests.cpp
    tests/MainMenuAcceptanceTests.cpp
    tests/ShowcaseHotelSceneTests.cpp
    tests/D2DUiSmokeTests.cpp
    src/ShowcaseHotelScene.cpp
)

target_include_directories(hh_frontend_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(hh_frontend_tests PRIVATE hh_frontend_core hh_frontend_d2d hh_renderer_core)
hh_enable_strict_warnings(hh_frontend_tests)
add_test(NAME hh_frontend_tests COMMAND hh_frontend_tests)
