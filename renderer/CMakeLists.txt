cmake_minimum_required(VERSION 3.25)
project(HotelHavenRenderer LANGUAGES CXX)

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
target_link_libraries(hh_renderer_tests PRIVATE hh_renderer_core d3d11 d3dcompiler)
hh_enable_strict_warnings(hh_renderer_tests)

add_test(NAME hh_renderer_tests COMMAND hh_renderer_tests)
