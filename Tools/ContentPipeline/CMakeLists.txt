cmake_minimum_required(VERSION 3.25)
project(HotelHavenContentPipeline LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HH_CONTENT_PIPELINE_FORMAT_ONLY
    "Build only the runtime-safe hh_asset_format target"
    OFF)

include(cmake/CompilerWarnings.cmake)

add_library(hh_asset_format STATIC
    src/Types.cpp
    src/Json.cpp
    src/Hasset.cpp
)
target_include_directories(hh_asset_format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
hh_enable_strict_warnings(hh_asset_format)

if(HH_CONTENT_PIPELINE_FORMAT_ONLY)
    return()
endif()

enable_testing()

add_library(hh_assets STATIC
    src/Metadata.cpp
    src/Catalog.cpp
    src/DependencyGraph.cpp
    src/Hash.cpp
    src/Fingerprint.cpp
    src/Cooker.cpp
    src/Exporter.cpp
    src/Cli.cpp
)
target_include_directories(hh_assets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hh_assets PUBLIC hh_asset_format)
hh_enable_strict_warnings(hh_assets)

add_executable(hh_asset_tests
    tests/TestMain.cpp
    tests/MetadataTests.cpp
    tests/DependencyGraphTests.cpp
    tests/FingerprintTests.cpp
    tests/HassetTests.cpp
    tests/CookerTests.cpp
    tests/CliTests.cpp
)
target_include_directories(hh_asset_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(hh_asset_tests PRIVATE hh_assets)
hh_enable_strict_warnings(hh_asset_tests)

add_executable(asset app/AssetMain.cpp)
target_link_libraries(asset PRIVATE hh_assets)
hh_enable_strict_warnings(asset)

add_test(NAME hh_asset_tests COMMAND hh_asset_tests)
