cmake_minimum_required(VERSION 3.25)
project(HotelHavenRenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(hh_renderer_tests
    tests/TestMain.cpp
    tests/CameraTests.cpp
    tests/FloorVisibilityTests.cpp
    tests/OcclusionTests.cpp
    tests/SceneComposerTests.cpp
)

target_include_directories(hh_renderer_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

if(MSVC)
    target_compile_options(hh_renderer_tests PRIVATE /W4 /WX /permissive- /EHsc)
endif()

add_test(NAME hh_renderer_tests COMMAND hh_renderer_tests)
