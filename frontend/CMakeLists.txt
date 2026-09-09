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

add_executable(hh_frontend_tests
    tests/TestMain.cpp
    tests/MainMenuModelTests.cpp
    tests/MainMenuNavigationTests.cpp
    tests/MenuHotelProviderTests.cpp
    tests/MenuTransitionTests.cpp
    tests/MenuSceneControllerTests.cpp
    tests/MainMenuStressTests.cpp
)

target_include_directories(hh_frontend_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)
target_link_libraries(hh_frontend_tests PRIVATE hh_frontend_core)
hh_enable_strict_warnings(hh_frontend_tests)
add_test(NAME hh_frontend_tests COMMAND hh_frontend_tests)
