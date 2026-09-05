cmake_minimum_required(VERSION 3.25)
project(HotelHavenRenderer LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Hotel Haven renderer foundation C targets Windows 11 x64")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
include(cmake/CompilerWarnings.cmake)

add_library(hh_renderer_core STATIC
    src/Camera.cpp
    src/FloorVisibility.cpp
    src/Occlusion.cpp
    src/SceneComposer.cpp
)

target_include_directories(hh_renderer_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
hh_enable_strict_warnings(hh_renderer_core)

add_library(hh_renderer_d3d11 STATIC
    src/d3d11/D3D11Renderer.cpp
)
target_include_directories(hh_renderer_d3d11 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(hh_renderer_d3d11 PUBLIC
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)
target_link_libraries(hh_renderer_d3d11 PUBLIC
    hh_renderer_core
    d3d11
    dxgi
    d3dcompiler
)
hh_enable_strict_warnings(hh_renderer_d3d11)

add_library(hh_renderer_win32 STATIC
    src/win32/Win32Window.cpp
)
target_include_directories(hh_renderer_win32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(hh_renderer_win32 PUBLIC
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)
target_link_libraries(hh_renderer_win32 PUBLIC user32)
hh_enable_strict_warnings(hh_renderer_win32)

add_executable(hotel_haven_renderer_demo WIN32
    app/RendererDemo.cpp
)
target_compile_definitions(hotel_haven_renderer_demo PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)
target_link_libraries(hotel_haven_renderer_demo PRIVATE
    hh_renderer_core
    hh_renderer_d3d11
    hh_renderer_win32
)
hh_enable_strict_warnings(hotel_haven_renderer_demo)

add_custom_command(TARGET hotel_haven_renderer_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:hotel_haven_renderer_demo>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/InstancedBox.hlsl"
        "$<TARGET_FILE_DIR:hotel_haven_renderer_demo>/shaders/InstancedBox.hlsl"
)

add_executable(hh_renderer_tests
    tests/TestMain.cpp
    tests/CameraTests.cpp
    tests/FloorVisibilityTests.cpp
    tests/OcclusionTests.cpp
    tests/SceneComposerTests.cpp
    tests/D3D11SmokeTests.cpp
)

target_include_directories(hh_renderer_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_compile_definitions(hh_renderer_tests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(hh_renderer_tests PRIVATE hh_renderer_core d3d11 d3dcompiler)
hh_enable_strict_warnings(hh_renderer_tests)

add_test(NAME hh_renderer_tests COMMAND hh_renderer_tests)
