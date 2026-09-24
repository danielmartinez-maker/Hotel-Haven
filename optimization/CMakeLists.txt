cmake_minimum_required(VERSION 3.25)
project(HotelHavenOptimization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(hh_optimization STATIC
    src/Optimizer.cpp
    src/PlanValidator.cpp
    src/MilpRequest.cpp
)
target_include_directories(hh_optimization PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(hh_optimization PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(hh_optimization PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(hh_optimization_tests tests/OptimizerTests.cpp)
target_link_libraries(hh_optimization_tests PRIVATE hh_optimization)
if(MSVC)
    target_compile_options(hh_optimization_tests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(hh_optimization_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME hh_optimization_tests COMMAND hh_optimization_tests)
